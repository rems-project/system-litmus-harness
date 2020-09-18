filtertests=$*
test_files=0

testpairs=`find unittests -name '*.c'  | while read f; do
    tests=$(grep $f -e "^UNIT_TEST([a-zA-Z0-9\_]*)$" | sed 's/UNIT_TEST(\([a-zA-Z0-9\_]*\))/\1/')
    echo $f $tests
done`

echo "$testpairs" | while read line; do
    for t in `echo $line | awk '{$1=""; print $0}'`; do
        echo $t | sed 's/\([a-zA-Z0-9\_]*\)/extern unit_test_fn \1;/'
    done
done > unittests/tests.externs

filter=$(echo $filtertests | sed 's/ /\|/g')
filteredtestpairs=`find unittests -name '*.c'  | while read f; do
    tests="$(grep $f -e '^UNIT_TEST([a-zA-Z0-9\_]*)$' | grep -e "$filter" | sed 's/UNIT_TEST(\([a-zA-Z0-9\_]*\))/\1/')"
    if [ ! -z "$tests" ]; then
        echo $f $tests
    fi
done`

echo "$filteredtestpairs" | while read line; do
    echo "$(echo $line | awk '{print $1}'):"
    for t in `echo $line | awk '{$1=""; print $0}'`; do
        echo " $t"
    done
done > unittests/tests.list

filteredtestfiles=`echo "$filteredtestpairs" | while read line; do
    echo "$(echo $line | awk '{print $1}'):"
done`

echo "$filteredtestpairs" | while read line; do
    tests=$(echo $line | awk '{$1=""; print $0}')
    fname=$(echo $line | awk '{print $1}')
    no_tests=$(echo $tests | wc -w)
    printf "  (test_file_t){\n    .name = \"$fname\",\n    .no_tests=$no_tests,\n    .fns = (unit_test_t*[]){";
    for t in $tests
    do
        printf "      &(unit_test_t){\n        .name=\"$t\",\n        .fn=$t,\n        .result=0\n      },\n"
    done;
    printf "    }\n";
    printf "  },\n"
done > unittests/tests.cstruct

echo `echo $filteredtestpairs | wc -w` `echo $filteredtestfiles | wc -w`