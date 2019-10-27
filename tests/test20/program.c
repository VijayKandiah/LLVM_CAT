#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

void function_that_complicates_everything (CATData *par1){
  CAT_add(*par1, *par1, *par1);
  return ;
}

int main (int argc, char *argv[]){
	CATData d1	= CAT_new(5);
  
  function_that_complicates_everything(&d1);

  int64_t value = CAT_get(d1);

  printf("Values: %ld\n", value);

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
