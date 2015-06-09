#!/bin/bash
# Script that tests every image in the entire dataset
trap 'j=$(jobs -p); [ "x$j" = "x" ] || kill $j >/dev/null 2>&1' EXIT

set -e

SCRIPT=$(readlink -f "$0")
EXEPATH=$(dirname "$SCRIPT")

usage() {
	echo "Model test script"
	echo
	echo "Tests entire dataset. Takes a while"
	echo
	echo -e "\tUsage: $0 DIRECTORY"
	echo
	echo -e "\tExample: $0 lfw"
	echo
	exit 1
}

[ $# = 1 ] || usage

dir=$1

if ! [ -d $dir ]; then
	echo "$dir: No such directory"
	exit 1
fi

files=$(find $dir -type f -iname "*.jpg" | sort)

for f in $files; do
	l=$($EXEPATH/test-parallel8.sh $f $dir 2>/dev/null | tail -n +2 | sort -g | tail -n1)
	max=$(echo $l | cut -f3 -d",")
	max_corr=$(echo $l | cut -f1 -d",")
	if ! [ "x$f" = "x$max" ]; then
		f_corr=$($EXEPATH/test-fftw $f $f 2>/dev/null | cut -f1 -d ",")
		echo "FAIL: $f,$max,$f_corr,$max_corr"
	else
		echo "OK: $f"
	fi
done

