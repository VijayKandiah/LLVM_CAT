#!/bin/bash

allInputs=( $@ ) ;

oracleFile=${allInputs[0]} ;
outputFile=${allInputs[1]} ;

echo "Checking $oracleFile against $outputFile" ;
if ! test -f $oracleFile ; then
  echo "  Test failed" ; 
  echo "  File \"$oracleFile\" is missing" ;
  exit 1;
fi
if ! test -f $outputFile ; then
  echo "  Test failed" ; 
  echo "  File \"$outputFile\" is missing" ;
  exit 1;
fi

# Copy the output and oracle files to temporary files
outFileToCheck="`mktemp`" ;
oracleFileToCheck="`mktemp`" ;
cp $oracleFile $oracleFileToCheck ;
cp $outputFile $outFileToCheck ;

# Strip out the last lines to both files
../misc/remove_last_line.sh $oracleFileToCheck ;
../misc/remove_last_line.sh $outFileToCheck ;

# Check the output
diffOut=`diff $oracleFileToCheck $outFileToCheck` ;
if test "$diffOut" != "" ; then 
  echo "  Test failed because output $fileName isn't correct" ; 
  echo "  Output differences can be found in \"diff\"" ;
  mkdir -p diff ;
  echo "$diffOut" > diff/${i}_diff_output ;
  exit 1;
fi

# Check the optimization
maxInvocations=`tail -n 1 $oracleFile | awk '{print $4}'` ;
currentInvocations=`tail -n 1 $outputFile | awk '{print $4}'` ;
enoughOpt=`echo "$currentInvocations <= $maxInvocations" | bc` ; 
if test $enoughOpt == "0" ; then
  echo "  Test failed because there are too many CAT invocations left in the generated bitcode" ;
  echo "  The maximum number of CAT invocations are $maxInvocations and the generated bitcode has $currentInvocations" ;
  exit 1;
fi

echo "Test passed!" ;
