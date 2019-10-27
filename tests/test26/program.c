#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

int main (int argc, char *argv[]){
  CATData x, y, z;
  void **mem;

  mem = malloc(sizeof(void *));

  z	= CAT_new(2);
  x	= CAT_new(8);
  (*mem) = x;

  y = (*mem);

	printf("H1: 	Y = %ld\n", CAT_get(y));
	printf("H1: 	X = %ld\n", CAT_get(x));
	printf("H1: 	Z = %ld\n", CAT_get(z));

  CAT_add(*mem, *mem, *mem);
	printf("H1: 	Y = %ld\n", CAT_get(y));
	printf("H1: 	X = %ld\n", CAT_get(x));
	printf("H1: 	Z = %ld\n", CAT_get(z));

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
