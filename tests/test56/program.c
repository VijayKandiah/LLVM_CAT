#include <stdio.h>
#include "CAT.h"

void CAT_execution (int userInput){
	CATData	d1;

	d1	= CAT_new(5);
	printf("Before loop: 	Value = %ld\n", CAT_get(d1));

  double r = 52340.43F;
  for (int i=0; i < userInput; i++){
    r = sqrt(r);
  }
	printf("After loop:	Value = %ld, %f\n", CAT_get(d1), r);

	return ;
}

int main (int argc, char *argv[]){
	CAT_execution(argc + 10);

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
