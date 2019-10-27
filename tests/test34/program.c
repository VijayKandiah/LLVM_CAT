#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

typedef struct {
  CATData d;
} my_struct_t ;

int64_t my_f (my_struct_t *d, int32_t important_condition){
  CATData local_d = CAT_new(6);

  if (important_condition){
    local_d = d->d;
  }

  return CAT_get(local_d);
}

int main (int argc, char *argv[]){
  my_struct_t s;
  s.d = CAT_new(5);

  auto value = my_f(&s, 1);

  printf("Values: %ld\n", value);

  printf("CAT invocations = %ld\n", CAT_invocations());
	return 0;
}
