#include <stdio.h>
#include "CAT.h"

int main (int argc, char *argv[]){
	CATData	ref;
  auto ref1 = CAT_new(1);
  auto ref2 = CAT_new(1);
  auto ref3 = CAT_new(1);
  auto ref4 = CAT_new(1);

  if (argc > 100){
    if (strcmp(argv[80], "ciao") == 0){
      ref	= CAT_new(CAT_get(ref1));
    } else {
      ref	= CAT_new(CAT_get(ref2));
    }

  } else {
    if (argc > 10){
      ref	= CAT_new(CAT_get(ref3));
    } else {
      ref	= CAT_new(CAT_get(ref4));
    }
  }

  printf("%ld\n", CAT_get(ref));

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
