#include <stdio.h>
#include "CAT.h"

void CAT_execution (int userInput){
	CATData	d1;

	d1	= CAT_new(5);
	printf("Before loop: 	Value = %ld\n", CAT_get(d1));

  for (int i=0; i < userInput; i++){
	  printf("Inside loop: Value = %ld\n", CAT_get(d1));
	  d1	= CAT_new(42);
	  printf("Inside loop 2: Value = %ld\n", CAT_get(d1));
  }
	printf("After loop:	Value = %ld\n", CAT_get(d1));

	return ;
}

int main (int argc, char *argv[]){
	CAT_execution(argc + 10);

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
