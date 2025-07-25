#!/bin/bash

declare -i passed=0
declare -i count=0

printf "*** TESTING RESULTS ***\n"

for filepath in "$@"
do

    filename=$(basename $filepath)
    base=${filename%.*}
    actualfile=actual/$base.txt
    expectedfile=expected/$base.txt
    asmfile=asm/$base.s
    difffile=diff/$base.diff

    touch $expectedfile

    ../ecc -S -o $asmfile $filepath &> $actualfile

    if [[ "$base" == *"_exec_"* ]]; then
        objfile=/tmp/$base.o
        as -o $objfile $asmfile
        execfile=/tmp/$base
        ld -o $execfile $objfile ../libc/libc.a ../libecc/libecc.a
        $($execfile &> $actualfile)
        rm -rf $execfile
        rm -rf $objfile
    fi

    diff=$(diff $actualfile $expectedfile)

    if [[ "$diff" == "" ]]; then
        printf " - %s: pass\n" $filename
        passed=$(($passed + 1))
    else
        printf " - %s: FAIL\n" $filename
    fi
    count=$(($count + 1))

    echo $diff > $difffile

done

ratio=$(echo "scale=2; $passed / $count * 100" | bc)

printf "passed %d/%d test(s) (%.1f%%)\n" "$passed" "$count" "$ratio"
