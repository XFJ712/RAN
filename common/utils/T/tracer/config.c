#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *local;
static int local_size;
static char *remote;
static int remote_size;

static char *PUT(char *to, int tosize, char c)
{
  if ((tosize & 4095) == 0) {
    to = realloc(to, tosize + 4096); if (to == NULL) abort();
  }
  to[tosize] = c;
  return to;
}

void append_received_config_chunk(char *buf, int length)
{
  int buflen = *(int *)buf;
  if (buflen != length - sizeof(int)) {
    printf("ERROR: bad trace -1, should not happen...\n");
    abort();
  }
  buf += sizeof(int);
  while (buflen) {
    remote = PUT(remote, remote_size, *buf);
    remote_size++;
    buf++;
    buflen--;
  }
}

void load_config_file(char *filename)
{
  int c;
  FILE *f = fopen(filename, "r");
  if (f == NULL) { perror(filename); abort(); }
  while (1) {
    c = fgetc(f); if (c == EOF) break;
    local = PUT(local, local_size, c);
    local_size++;
  }
  fclose(f);
}

void verify_config(void)
{
  if (local_size != remote_size || memcmp(local, remote, local_size) != 0) {
    printf("ERROR: local and remote T_messages.txt not identical\n");
    abort();
  }
}
