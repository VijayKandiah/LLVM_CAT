#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CAT.h"

#define VALID_STRING 		"p6pbbUlpLo0BL1bM2k8K"
#define VALID_STRING_SIZE	20

typedef struct {
	char	begin_validation_string[VALID_STRING_SIZE];
	int64_t	value;
	char	end_validation_string[VALID_STRING_SIZE];
} internal_data_t;

static int64_t invocations = 0;

static internal_data_t * internal_check_data (CATData v);

CATData CAT_new (int64_t value){
	internal_data_t		*d;

  invocations++;

	d			= (internal_data_t *) malloc(sizeof(internal_data_t));

	strncpy(d->begin_validation_string, VALID_STRING, VALID_STRING_SIZE);
	strncpy(d->end_validation_string, VALID_STRING, VALID_STRING_SIZE);
	d->value	= value;

	return (CATData) d;
}

const int64_t CAT_get (const CATData v) {
	internal_data_t	*d;
  invocations++;

	d	= internal_check_data(v);

	return d->value;
}

void CAT_set (CATData v, int64_t value){
	internal_data_t	*d;

  invocations++;
	d	= internal_check_data(v);

    d->value = value;

    return ;
}

void CAT_sub (CATData result, const CATData v1, const CATData v2){
	internal_data_t		*d1;
	internal_data_t		*d2;
	internal_data_t		*dresult;
  invocations++;

  if (v1 == NULL || v2 == NULL) return;

	d1				= internal_check_data(v1);
	d2				= internal_check_data(v2);
	dresult			= internal_check_data(result);

	dresult->value	= d1->value - d2->value;

	return ;
}

void CAT_add (CATData result, const CATData v1, const CATData v2){
	internal_data_t		*d1;
	internal_data_t		*d2;
	internal_data_t		*dresult;
  invocations++;

  if (v1 == NULL || v2 == NULL) return;
	d1				= internal_check_data(v1);
	d2				= internal_check_data(v2);
	dresult			= internal_check_data(result);

	dresult->value	= d1->value + d2->value;

	return ;
}

static internal_data_t * internal_check_data (CATData v){
	internal_data_t	*d;

	if (v == NULL){
		fprintf(stderr, "libCAT: ERROR = input is NULL\n");
		abort();
	}
	d	= (internal_data_t *) v;

	if ( (strncmp(d->begin_validation_string, VALID_STRING, VALID_STRING_SIZE) != 0)		||
			 (strncmp(d->end_validation_string, VALID_STRING, VALID_STRING_SIZE) != 0)		  ){
		fprintf(stderr, "libCAT: ERROR = data has been corrupted\n");
		abort();
	}

	return d;
}

const int64_t CAT_invocations (void){
  return invocations;
}
