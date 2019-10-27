#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

int main (int argc, char *argv[]){
  CATData x;
  void **ref;

  ref = malloc(sizeof(void *));

  x = CAT_new(8);
  (*ref) = x;

  printf("%ld", CAT_get(x));

  free(ref);
  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
