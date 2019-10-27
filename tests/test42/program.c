#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

CATData CAT_create_aaaaaa_aaaaa (int v){
  return CAT_new(v + 1);
}

int main (int argc, char *argv[]){
  auto d1	= CAT_create_aaaaaa_aaaaa(5);
  printf("Values: %ld\n", CAT_get(d1));

  printf("CAT invocations = %ld\n", CAT_invocations());
  return 0;
}
