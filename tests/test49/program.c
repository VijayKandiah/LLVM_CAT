#include <stdio.h>
#include <stdlib.h>
#include "CAT.h"

void myF (int *p){
  if (*p > 500){
    (*p)--;
  }

  return ;
}

int main (int argc, char *argv[]){
  CATData x = CAT_new(40);
  CATData y = CAT_new(2);
  CATData z = CAT_new(0);

  for (int i=0; i < ((argc + 1) * 2); i++){
    myF(&argc);

    printf("Z: %ld\n", CAT_get(z));

    CAT_add(z, x, y);
    printf("Z: %ld\n", CAT_get(z));

    CAT_add(x, x, x);
    CAT_add(y, y, y);

    z = CAT_new(1);

  }

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
