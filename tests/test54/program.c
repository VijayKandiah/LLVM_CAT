#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>
#include <math.h>

CATData myF (CATData p1){
  return p1;
}

void computeF (void){
  int counter1;
  int result;

  void **ref1 = malloc(sizeof(void *));

  CATData x	= CAT_new(4);
  CATData w	= CAT_new(42);

  (*ref1) = x;

  counter1 = 0;
  do {
    CATData y,z; 
    y = NULL; z = NULL;
    int x_value = CAT_get(x);
    x_value++;

    if (x_value > 10){
      y = CAT_new(CAT_get(x));
    } else {
      y = myF(CAT_new(5)); 
      printf("L1: %ld\n", CAT_get(y));
    }

    CAT_add(*ref1, x, y);
    result = CAT_get(w);

    do {
      int x_value;
      CATData y,z; y = NULL; z = NULL;
      x_value = CAT_get(x);
      x_value++;
      if (counter1 >= 0){
        if (x_value > 10){
          y = CAT_new(CAT_get(x));
        } else {
         y = myF(CAT_new(5)); 
         printf("L2: %ld\n", CAT_get(y));
        }
      }
      
     CAT_add(*ref1, x, y);
     result = CAT_get(w);

      if (counter1 == 0){
        int z_value;
        if (z != NULL){
          z_value = CAT_get(z);
        } else {
          z_value = 125;
        }
        z = CAT_new(z_value);
      }
      counter1++;
      } while (counter1 < 10);

    if (counter1 == 0){
      z = CAT_new(125);
    }
    counter1++;
  } while (counter1 < 10);

  free(ref1);

  printf("Result = %d\n\n\n\n", result);

  return ;
}

int main (int argc, char *argv[]){

  computeF();

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
