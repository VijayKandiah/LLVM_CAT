#include <stdio.h>
#include <CAT.h>

int main (int argc, char *argv[]){
  CATData d1;
  
  if (argc > 10){
    d1	= CAT_new(8);
  } else {
    d1	= CAT_new(8);
  }

	printf("H5: 	Value of d1 = %ld\n", CAT_get(d1));

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
