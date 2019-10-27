#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

int my_f (const CATData const d, int var) {
  if (d == NULL){
    return var+1;
  }
  
  return 0;
}

int main (int argc, char *argv[]){
  CATData d = CAT_new(5);

  auto v = my_f(d, argc);

  printf("Value at the end: %ld\n", CAT_get(d) + v);

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
