#include <stdio.h>

static struct sss{
  long f;
  double snd;
} sss;

#define _offsetof(st,f) ((char *)&((st *) 16)->f - (char *) 16)

int main (void) {
  printf ("+++Struct long-double:\n");
  printf ("size=%d,align=%d,offset-long=%d,offset-double=%d,\nalign-long=%d,align-double=%d\n",
          sizeof (sss), __alignof__ (sss),
          _offsetof (struct sss, f), _offsetof (struct sss, snd),
          __alignof__ (sss.f), __alignof__ (sss.snd));
  return 0;
}
