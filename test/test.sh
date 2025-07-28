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
    content=$(cat $actualfile)
    declare -i exit_status=0

    if [[ "$base" == *"_exec_"* ]]; then
        if [[ -a "$asmfile" ]]; then 
            objfile=/tmp/$base.o
            as -o $objfile $asmfile
            execfile=/tmp/$base
            ld -o $execfile $objfile ../libc/libc.a ../libecc/libecc.a
            $($execfile &> $actualfile)
            exit_status=$?
            rm -rf $execfile
            rm -rf $objfile
        fi
    fi

    diff=$(diff $actualfile $expectedfile)

    if [[ "$diff" == "" ]] && [[ $exit_status -lt 128 ]]; then
        printf " - %s: pass\n" $filename
        passed=$(($passed + 1))
    else
        if [[ "$content" != "" ]]; then
            printf " - %s: FAIL, compilation error:\n%s\n" $filename "$content"
        elif [[ $exit_status -ge 128 ]]; then
            printf " - %s: FAIL, output program interrupted by signal: %d\n" $filename $(($exit_status - 128))
        else
            printf " - %s: FAIL\n" $filename
        fi
    fi
    count=$(($count + 1))

    echo $diff > $difffile

done

ratio=$(echo "scale=2; $passed / $count * 100" | bc)

[[ $count -eq 1 ]] && c="" || c="s"
printf "passed %d/%d test%s (%.1f%%)\n" "$passed" "$count" "$c" "$ratio"
