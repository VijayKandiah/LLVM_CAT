#!/bin/bash

if test $# -lt 4 ; then
  echo "USAGE: `basename $0` AA INPUT_FILE OUTPUT_FILE MAX_INVOCATIONS" ;
  exit 1;
fi

AA="$1" ;
inputFile="$2" ;
outputFile="$3" ;
maxIterations=$4 ;

let iter=0 ;
fixedPointReached="0" ;
while [ "$fixedPointReached" == "0" ] ; do

  # Run the compiler
  echo "#### Running the CAT compiler" ;
  opt -load ~/CAT/lib/CAT.so ${inputFile} ${AA} -CAT -o ${outputFile}

  # Check if the compiler has generated the bitcode
  if ! test -f ${outputFile} ; then
    echo "####  ERROR = the compiler did not generate the bitcode file." ;
    exit 1 ;
  fi

  # Check if we reached the fixed point
  llvm-dis ${inputFile} -o I.ll ;
  llvm-dis ${outputFile} -o O.ll ;
  differentLines=`diff I.ll O.ll | wc -l` ;
  if test $differentLines != "4" ; then
    echo "####  The compiler modified the input bitcode: the fixed point isn't reached." ;
    inputFile="output_code_iter_${iter}.bc" ;
    cp "${outputFile}" "${inputFile}" ;

  else
    echo "####  The compiler did not modify the input bitcode: the fixed point has been reached." ;
    fixedPointReached="1" ;
  fi

  let iter=${iter}+1 ;
  if [ $maxIterations -ne "0" -a $iter -eq $maxIterations ] ; then
    break ;
  fi
done
