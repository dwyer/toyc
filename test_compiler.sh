#!/bin/bash

tmpdir='./tmp.test'

padding_dots=$(printf '%0.1s' "."{1..60})
padlength=50

test_success () {
    echo "OK"
    ((success++))
}

test_failure () {
    echo "FAIL"
    ((fail++))
}

if [ "$1" == "" ]; then
    echo "USAGE: ./test_compiler.sh /path/to/compiler"
    exit 1
fi

cmp=$1

success_total=0
failure_total=0

num_stages=9

for i in `seq 1 $num_stages`; do
    success=0
    fail=0
    echo "===================================================="
    echo "STAGE $i"
    echo "===================Valid Programs==================="
    for prog in `ls stages/stage_$i/valid/{,**/}*.kc 2>/dev/null`; do
        outdir="${tmpdir}/$(dirname ${prog})"
        cout="${tmpdir}/${prog%.kc}.c.out"
        if [ ! -e ${cout} ]; then
            mkdir -p ${outdir}
            gcc -w -o ${cout} ${prog%.kc}.c
        fi
        expected_out=`${cout}`
        expected_exit_code=$?

        base="${tmpdir}/${prog%.*}" #name of executable (filename w/out extension)
        $cmp $prog $base >/dev/null
        actual_out=`./$base`
        actual_exit_code=$?
        test_name="${base##*valid/}"
        printf '%s' "$test_name"
        printf '%*.*s' 0 $((padlength - ${#test_name})) "$padding_dots"

        if [[ $test_name == "undefined"* ]]; then
            # return value is undefined
            # make sure it runs w/out segfaulting, but otherwise don't check exit code
            if [ "$actual_exit_code" -eq 139 ]; then
                #segfault!
                test_failure
            else
                test_success
            fi
        else
            # make sure exit code is correct
            if [ "$expected_exit_code" -ne "$actual_exit_code" ] || [ "$expected_out" != "$actual_out" ]
            then
                test_failure
                # cat $prog
                # echo expected $expected_exit_code, got $actual_exit_code
                exit 1
            else
                test_success
            fi
        fi
        rm $base
    done
    # echo "===================Invalid Programs================="
    # for prog in `ls stage_$i/invalid/{,**/}*.c 2>/dev/null`; do
    #     base="${prog%.*}" #name of executable (filename w/out extension)
    #     test_name="${base##*invalid/}"

    #     $cmp $prog >/dev/null 2>&1
    #     failed=$? #failed, as we expect, if exit code != 0

    #     printf '%s' "$test_name"
    #     printf '%.*s' $((padlength - ${#test_name})) "$padding_dots"

    #     if [[ -f $base || -f $base".s" ]] #make sure neither executable nor assembly was produced
    #     then
    #         test_failure
    #         rm $base 2>/dev/null
    #         rm $base".s" 2>/dev/null
    #     else
    #         test_success
    #     fi
    # done
    echo "===================Stage $i Summary================="
    printf "%d successes, %d failures\n" $success $fail
    ((success_total=success_total+success))
    ((failure_total=failure_total + fail))
done

echo "===================TOTAL SUMMARY===================="
printf "%d successes, %d failures\n" $success_total $failure_total
