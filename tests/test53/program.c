#include <stdio.h>
#include <stdlib.h>
#include "CAT.h"

int my_f (CATData x, CATData y, CATData z, int argc){

  int i = 0;
  do {
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

    for (int k=0; k < (i + 1); k++){
      printf("Z3: %ld\n", CAT_get(z));

      CAT_add(z, x, y);
      printf("Z3: %ld\n", CAT_get(z));

      CAT_add(x, x, x);
      CAT_add(y, y, y);

      z = CAT_new(1);

      for (int w=0; w < (k * 3); w++){
        printf("Z21: %ld\n", CAT_get(z));

        CAT_add(z, x, y);
        printf("Z21: %ld\n", CAT_get(z));

        CAT_add(x, x, x);
        CAT_add(y, y, y);

        z = CAT_new(1);
        if (CAT_get(x) > 1000000){
          z = CAT_new(my_f(x, y, z, argc));
        }

        for (int w2=0; w2 < (w * 2); w2++){
          printf("Z211: %ld\n", CAT_get(z));

          CAT_add(z, x, y);
          printf("Z211: %ld\n", CAT_get(z));

          CAT_add(x, x, x);
          CAT_add(y, y, y);

          z = CAT_new(1);
        }
      }
    }

    i++;
  } while (i < (argc + 2));

  return CAT_get(z);
}

int main (int argc, char *argv[]){
  CATData x = CAT_new(40);
  CATData y = CAT_new(2);
  CATData z = CAT_new(0);

  int res = my_f(x, y, z, argc);
  printf("Result = %d\n", res);

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
