#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

int main (int argc, char *argv[]){
  CATData x;
  void **ref1;
  void **ref2;

  ref1 = malloc(sizeof(void *));
  ref2 = malloc(sizeof(void *));

  x	= CAT_new(8);

  (*ref1) = x;
  (*ref2) = *ref1;

  printf("H1: 	X    = %ld\n", CAT_get(x));

  CAT_add(*ref1, x, x);
  printf("H1: 	X    = %ld\n", CAT_get(x));

  free(ref1);
  free(ref2);

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
