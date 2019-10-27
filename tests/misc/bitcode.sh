#!/bin/bash

for i in `ls | grep test` ; do
  if ! test -d $i ; then
    continue ;
  fi
  pushd ./ &> /dev/null ;
  cd $i ;
  if ! test -f *.bc ; then
    echo $i ;
    make install ;
    echo "" ;
  fi
  popd &> /dev/null ;
done
