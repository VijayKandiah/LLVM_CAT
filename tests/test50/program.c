#include <stdio.h>
#include <stdlib.h>
#include "CAT.h"

int main (int argc, char *argv[]){
  CATData x = CAT_new(40);
  CATData y = CAT_new(2);
  CATData z = CAT_new(0);

  for (int i=0; i < 10; i++){
    printf("Z: %ld\n", CAT_get(z));

    CAT_add(z, x, y);
    printf("Z: %ld\n", CAT_get(z));

    CAT_add(x, x, x);
    CAT_add(y, y, y);

    z = CAT_new(1);

    for (int j=0; j < 10; j++){
      printf("Z2: %ld\n", CAT_get(z));

      CAT_add(z, x, y);
      printf("Z2: %ld\n", CAT_get(z));

      CAT_add(x, x, x);
      CAT_add(y, y, y);

      z = CAT_new(1);
    }

    for (int k=0; k < 10; k++){
      printf("Z3: %ld\n", CAT_get(z));

      CAT_add(z, x, y);
      printf("Z3: %ld\n", CAT_get(z));

      CAT_add(x, x, x);
      CAT_add(y, y, y);

      z = CAT_new(1);

      for (int w=0; w < 10; w++){
        printf("Z21: %ld\n", CAT_get(z));

        CAT_add(z, x, y);
        printf("Z21: %ld\n", CAT_get(z));

        CAT_add(x, x, x);
        CAT_add(y, y, y);

        z = CAT_new(1);

        for (int w2=0; w2 < 10; w2++){
          printf("Z211: %ld\n", CAT_get(z));

          CAT_add(z, x, y);
          printf("Z211: %ld\n", CAT_get(z));

          CAT_add(x, x, x);
          CAT_add(y, y, y);

          z = CAT_new(1);
        }
      }
    }
  }

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
