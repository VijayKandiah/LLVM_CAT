#include <stdio.h>
#include <CAT.h>

int main(int argc, char **argv) {
  CATData x = CAT_new(5);
  CATData y = CAT_new(8);
  CATData *p;

  if (argc > 5) p = &x; else p = &y;

  *p = CAT_new(10);

  printf("%ld %ld\n", CAT_get(x), CAT_get(y));

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
