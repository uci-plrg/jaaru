#!/bin/bash
set -e

input=$1
while IFS= read -r line
do
	if [[ $line = Dumping* ]]
	then
		echo "$line"
		continue;
	fi
	TRACE=$(echo "$line" | sed -e 's/^[ \t]*//')
	LIB="$( cut -d '(' -f 1 <<< "$TRACE" )"; 
	REST="$( cut -d '(' -f 2- <<< "$TRACE" )";
	FUNC="$( cut -d '+' -f 1 <<< "$REST" )";
	REST="$( cut -d '+' -f 2- <<< "$REST" )";
	OFFSET="$( cut -d ')' -f 1 <<< "$REST" )";
	if test -z "$FUNC"
	then
		LOCATION=$(addr2line -a -f --exe=$LIB $OFFSET | sed -z 's/\n/:/g;s/:$/\n/')
	else
		funcaddr=$(nm $LIB | grep $FUNC | head -n1 | cut -d " " -f1) && funcoffset=$(python -c "print hex(0x${funcaddr}+$OFFSET)") && LOCATION=$(addr2line -e $LIB ${funcoffset} | sed -z 's/\n/:/g;s/:$/\n/')
	fi
	echo "	$LIB:$FUNC($LOCATION)"
done < "$input"
