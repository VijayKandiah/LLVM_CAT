#include <stdio.h>
#include <CAT.h>

void a_generic_C_function (CATData d1, int argc){
L1:
  if (argc < 100) return;
  if (CAT_get(d1) > 10){
    d1	= CAT_new(CAT_get(d1) - 1);
  } else {
    argc--;
  }

  if (argc > 10){
    d1	= CAT_new(CAT_get(d1) - 1);
  }
  goto L1;
  
  return ;
}


int main (int argc, char *argv[]){
  a_generic_C_function(CAT_new(5), argc);
  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
