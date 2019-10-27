#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

void my_f (CATData d){
  printf("Value: %ld\n", CAT_get(d));
  
  return ;
}

int main (int argc, char *argv[]){
  CATData d = CAT_new(5);

  my_f(d);

  printf("Value at the end: %ld\n", CAT_get(d));

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
