#!/bin/bash
PROJECTDIR=/scratch/nvm/pmcheck
BIN=$PROJECTDIR/bin
ulimit -c unlimited
cd $PROJECTDIR
#make clean
make
make test
#export PMCheck="-t -p 3 -v 3 -x"
export PMCheck="-x"
for test in $BIN/testrace* ;
do 
    bash $BIN/run.sh $test
    read -p "Press any keys to continue " n1
done