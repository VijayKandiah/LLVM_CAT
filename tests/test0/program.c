#include <stdio.h>
#include <stdlib.h>
#include "CAT.h"

CATData p (void){
  return CAT_new(1);
}

int main (int argc, char *argv[]){
  CATData r = p();
  
  CATData t = p();

  printf("%ld %ld\n", CAT_get(r), CAT_get(t));

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
