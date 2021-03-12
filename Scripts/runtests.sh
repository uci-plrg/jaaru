#!/bin/bash
PROJECTDIR=/scratch/nvm/pmcheck
BIN=$PROJECTDIR/bin
ulimit -c unlimited
cd $PROJECTDIR
#make clean
make
make test
export PMCheck="-t -p 3 -v 3 -x"
for test in $BIN/testrace* ;
do 
    bash $BIN/run.sh $test
done