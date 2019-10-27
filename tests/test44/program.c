#include <stdio.h>
#include <stdlib.h>
#include "CAT.h"

int main (int argc, char *argv[]){
  CATData x = CAT_new(40);
  CATData y = CAT_new(2);

  for (int i=1; i <= 5; i++){
    printf("X: %ld\n", CAT_get(x));
    printf("Y: %ld\n", CAT_get(y));

    CATData z = CAT_new(0);
    CAT_add(z, x, y);
    printf("Z: %ld\n", CAT_get(z));

    CAT_add(x, x, x);
  }

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
