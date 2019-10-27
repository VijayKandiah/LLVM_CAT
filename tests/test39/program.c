#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

void you_cannot_take_decisions_based_on_function_names (CATData par1){
  CAT_add(par1, par1, par1);
  return ;
}

int main (int argc, char *argv[]){
	CATData d1	= CAT_new(5);
  
  you_cannot_take_decisions_based_on_function_names(d1);

  int64_t value = CAT_get(d1);

  printf("Values: %ld\n", value);

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
