#include <stdio.h>
#include <stdlib.h>
#include "CAT.h"

CATData p (CATData v){
  CATData m;

  if (CAT_get(v) < 10){
    m = CAT_new(1);
  } else {
    m = CAT_new(2);
  }
  return m;
}

int main (int argc, char *argv[]){
  CATData x = CAT_new(7);
  CATData r = p(x);
  
  CATData y = CAT_new(80);
  CATData t = p(y);

  printf("%ld %ld\n", CAT_get(r), CAT_get(t));

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
