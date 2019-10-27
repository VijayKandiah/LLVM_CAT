#include <stdio.h>
#include <stdlib.h>
#include "CAT.h"

void p (uint64_t v, CATData d){
  CATData m;

  if (v == 0){
    return CAT_add(d, d, CAT_new(1));
  }

  CAT_add(d, d, CAT_new(v + 5));

  p(v - 1, d);
  return ;
}

int main (int argc, char *argv[]){
  CATData x = CAT_new(7);
  for (int i=1; i <= 5; i++){
    p(i, x);
    printf("%ld\n", CAT_get(x));
  }

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
