#!/bin/bash

if test $# -lt 5 ; then
  echo "USAGE: `basename $0` INPUT OUTPUT NUMS AA MAX_ITERATIONS" ; 
  exit 1 ;
fi

inputFile=$1 ;
outputFile=$2 ;
nums=$3 ;
AA="-mem2reg $4" ;
maxIterations=$5 ;

rm -f compiler_output.txt ;
totalTime="0" ;
let count=0 ;
for i in `seq 1 $nums` ; do
  /usr/bin/time -f "%e"  ../misc/run_test.sh "$AA" "$inputFile" "$outputFile" $maxIterations &> tmpOut ;
  cat tmpOut >> compiler_output.txt ;
  if ! test -f $outputFile ; then
    echo "ERROR: the compiler didn't generate the bitcode" ;
    exit 1 ;
  fi
  llvm-dis $outputFile ;

  time=`tail -n 1 tmpOut` ;
  rm tmpOut ;

  totalTime=`echo "$totalTime + $time" | bc` ;
  let count=$count+1 ;
done
echo "" ;

averageTime=`echo "scale=3; $totalTime / $count" | bc` ;

make program_optimized ;
./program_optimized &> tmpOut ;
echo `grep "CAT invocations" tmpOut` ;
echo "Time: $averageTime seconds" ;

rm tmpOut ;
