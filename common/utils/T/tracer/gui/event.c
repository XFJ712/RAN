#include "gui.h"
#include "gui_defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

/*****************************************************************/
/*                       generic functions                       */
/*****************************************************************/

static void event_list_append(struct gui *g, struct event *e)
{
  struct event_list *new;

  new = calloc(1, sizeof(struct event_list));
  if (new == NULL) OOM;

  new->item = e;

  if (g->queued_events == NULL) {
    g->queued_events = new;
    new->last = new;
    return;
  }

  g->queued_events->last->next = new;
  g->queued_events->last = new;
}

static void free_event(struct event *e)
{
  switch (e->type) {
  case REPACK: /* nothing */ break;
  case DIRTY: /* nothing */ break;
  }
  free(e);
}

/*****************************************************************/
/*                         sending events                        */
/*****************************************************************/

static event *new_event_repack(int id)
{
  struct repack_event *ret;
  ret = calloc(1, sizeof(struct repack_event));
  if (ret == NULL) OOM;
  ret->id = id;
  return ret;
}

static event *new_event_dirty(int id)
{
  struct dirty_event *ret;
  ret = calloc(1, sizeof(struct dirty_event));
  if (ret == NULL) OOM;
  ret->id = id;
  return ret;
}

void send_event(gui *_gui, enum event_type type, ...)
{
printf("send_event %d\n", type);
  struct gui *g = _gui;
  int do_write = 0;
  va_list ap;
  struct event *e;

  if (g->queued_events == NULL) do_write = 1;

  va_start(ap, type);

  switch (type) {
  case REPACK: {
    int id;
    id = va_arg(ap, int);
    e = new_event_repack(id);
    break;
  }
  case DIRTY: {
    int id;
    id = va_arg(ap, int);
    e = new_event_dirty(id);
    break;
  }
  }

  va_end(ap);

  e->type = type;

  event_list_append(g, e);

  if (do_write) {
    char c = 1;
    if (write(g->event_pipe[1], &c, 1) != 1)
      ERR("error writing to pipe: %s\n", strerror(errno));
  }
}

/*****************************************************************/
/*                      processing events                        */
/*****************************************************************/

static void repack_event(struct gui *g, int id)
{
  struct widget *w = find_widget(g, id);
  if (w == NULL) { WARN("widget id %d not found\n", id); return; }
  w->repack(g, w);
}

/* TODO: put that function somewhere else? */
static struct toplevel_window_widget *get_toplevel_window(struct widget *w)
{
  while (w != NULL) {
    if (w->type == TOPLEVEL_WINDOW)
      return (struct toplevel_window_widget *)w;
    w = w->parent;
  }
  return NULL;
}

static void dirty_event(struct gui *g, int id)
{
  struct widget *w = find_widget(g, id);
  struct toplevel_window_widget *win;
  if (w == NULL) { WARN("widget id %d not found\n", id); return; }
  win = get_toplevel_window(w);
  if (win == NULL)
    { WARN("widget id %d not contained in a window\n", id); return; }
  g->xwin = win->x;
  w->clear(g, w);
  w->paint(g, w);
  g->xwin = NULL;
  g->repainted = 1;
}

static void process_event(struct gui *g, struct event *e)
{
printf("processing event type %d\n", e->type);
  switch (e->type) {
  case REPACK: repack_event(g, ((struct repack_event *)e)->id); break;
  case DIRTY: dirty_event(g, ((struct dirty_event *)e)->id); break;
  }
}

/* TODO: events' compression */
void gui_events(gui *_gui)
{
  struct gui *g = _gui;

printf("gui_events START: head %p\n", g->queued_events);

  while (g->queued_events) {
    struct event_list *cur = g->queued_events;
    g->queued_events = cur->next;
    if (g->queued_events) g->queued_events->last = cur->last;
    process_event(g, cur->item);
    free_event(cur->item);
    free(cur);
  }
printf("gui_events DONE\n");
}
