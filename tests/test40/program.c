#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

int another_function (int a, float b, double c){
  printf("HELLO %f\n", ((double)a) + ((double)b) + c);

  return a + 100;
}
      
void yet_another_function (CATData *p, CATData d1){
  if (p != NULL){
    CAT_add(d1, d1, d1);
  } else {
    printf("WOW\n");
  }
}

CATData * you_cannot_take_decisions_based_on_function_names (CATData par1){
  static CATData internalD1 = NULL;

  if (par1 == NULL){
    return NULL;
  }
  if (internalD1 == NULL){
    internalD1 = par1 ;
  }

  return &internalD1;
}

int main (int argc, char *argv[]){
  auto d1	= CAT_new(5);
  CATData *p_d1 = NULL;

  if (argc > 5){
    printf("This block doesn't matter\n");

  } else {
    p_d1 = you_cannot_take_decisions_based_on_function_names(d1);
    printf("Hello\n");
  }

  printf("Hello 2\n");

  if (another_function(argc, (float)(argc + 1), 42.42) > 10){
    printf("Invoking a function\n");
    yet_another_function(*p_d1, d1);

  } else {
    printf("This block doesn't matter\n");
  }

  printf("Values: %ld\n", CAT_get(d1));

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
