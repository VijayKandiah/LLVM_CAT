#ifndef CAT_H
#define CAT_H

#include <stdint.h>

typedef void * CATData;

CATData CAT_new (int64_t value);

const int64_t CAT_get (const CATData v) ;

void CAT_set (CATData v, int64_t value);

void CAT_add (CATData result, const CATData v1, const CATData v2);

void CAT_sub (CATData result, const CATData v1, const CATData v2);

const int64_t CAT_invocations (void);

#endif
