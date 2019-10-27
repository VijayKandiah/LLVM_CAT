#include <stdio.h>
#include <CAT.h>

void a_generic_C_function (CATData d1, int argc){
  goto L1 ;

L3:
	printf("H5: 	Value of d1 = %ld\n", CAT_get(d1));
  return ;

L2:
  if (argc > 10){
    d1	= CAT_new(8);
  }
  goto L3;

L1:   
  d1	= CAT_new(8);
  goto L2;
}


int main (int argc, char *argv[]){
  a_generic_C_function(CAT_new(5), argc);
  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
