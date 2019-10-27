#!/bin/bash

for i in `ls | grep test` ; do
  if ! test -d $i ; then
    continue ;
  fi
  pushd ./ &> /dev/null ;
  cd $i ;
  make clean &> /dev/null ;
  popd &> /dev/null ;
done
