#!/bin/bash

for i in `ls | grep test` ; do
  if ! test -d $i ; then
    continue ;
  fi
  pushd ./ &> /dev/null ;
  cd $i ;
  if ! test -f output*/*_output ; then
    echo $i ;
    make oracle ;
    echo "" ;
  fi
  popd &> /dev/null ;
done
