#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

int my_f (CATData d, int v){
  if (d == NULL){
    return v+1;
  }
  return v - 10;
}

int main (int argc, char *argv[]){
  CATData d = CAT_new(5);
  int v = 0;

  if (argc > 5){
    d = CAT_new(5);
    v = my_f(d, argc);
  } else {
    d = CAT_new(5);
  }

  printf("Value at the end: %ld\n", CAT_get(d) + v);

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
