#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

int main (int argc, char *argv[]){
  CATData x, y;
  void **mem;

  mem = malloc(sizeof(void *));

  x	= CAT_new(8);
  (*mem) = x;

  y = (*mem);

	printf("H1: 	Y = %ld\n", CAT_get(y));
	printf("H1: 	X = %ld\n", CAT_get(x));

  CAT_add(*mem, *mem, *mem);
	printf("H1: 	Y = %ld\n", CAT_get(y));
	printf("H1: 	X = %ld\n", CAT_get(x));

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
