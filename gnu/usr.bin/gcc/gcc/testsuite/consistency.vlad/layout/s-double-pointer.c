#include <stdio.h>

static struct sss{
  double f;
  char * snd;
} sss;

#define _offsetof(st,f) ((char *)&((st *) 16)->f - (char *) 16)

int main (void) {
  printf ("+++Struct double-pointer:\n");
  printf ("size=%d,align=%d,offset-double=%d,offset-pointer=%d,\nalign-double=%d,align-pointer=%d\n",
          sizeof (sss), __alignof__ (sss),
          _offsetof (struct sss, f), _offsetof (struct sss, snd),
          __alignof__ (sss.f), __alignof__ (sss.snd));
  return 0;
}
