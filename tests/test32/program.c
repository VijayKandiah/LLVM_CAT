#include <stdio.h>
#include <CAT.h>

int main (int argc, char *argv[]){
  CATData x;
  x	= CAT_new(8);
  x	= CAT_new(9);
	printf("H1: 	X = %ld\n", CAT_get(x));
  CAT_add(x, x, x);
	printf("H1: 	X = %ld\n", CAT_get(x));

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
