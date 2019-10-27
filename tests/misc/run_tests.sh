#!/bin/bash

let successes=0 ;
let tests=0 ;
testFailed="";
for i in `ls | grep test` ; do
  if ! test -d $i ; then
    continue ;
  fi
  let tests=$tests+1 ;
  pushd ./ &> /dev/null ;
  cd $i ;
  make clean &> /dev/null ;
  make check &> tmpOut ;
  testSucceeded=`grep "Test passed!" tmpOut` ;
  if test "$testSucceeded" != ""; then
    let successes=$successes+1 ;
  else
    testFailed="  $i\n$testFailed" ;
  fi
  rm tmpOut ;
  popd &> /dev/null ;
done

echo "SUMMARY: $successes tests passed out of $tests" ;
if test "$testFailed" != "" ; then
  echo "Failed tests:";
  echo -e "$testFailed" ;
fi
