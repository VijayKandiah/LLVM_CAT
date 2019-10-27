#include <stdio.h>
#include "CAT.h"

int main (int argc, char *argv[]){
  CATData d = CAT_new(argc);
  printf("%ld\n", CAT_get(d));

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
