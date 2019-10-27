#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

CATData * my_f (CATData d){
  static CATData internal = NULL;
  if (internal == NULL) internal = d;
  return &internal;
}

int main (int argc, char *argv[]){
  CATData d = CAT_new(5);

  CATData *d2 = my_f(d);
  CATData d3 = *d2;

  printf("Value at the end: %ld\n", CAT_get(d3));

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
