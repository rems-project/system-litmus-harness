test_files=0
find unittests/ -name '*.c'  | while read f; do
    #grep $f -e "^UNIT_TEST([a-Z0-9\_]*)$"
    tests=$(grep $f -e "^UNIT_TEST([a-Z0-9\_]*)$" | sed 's/UNIT_TEST(\([a-Z0-9\_]*\))/\1/')
    NO_TESTS=$(echo $tests | wc -w)
    if [ ! -z "$tests" ]
    then
        printf "(test_file_t){.name = \"$f\"\n,.no_tests=$NO_TESTS\n,.fns = (unit_test_t[]) {";
        for t in $tests
        do
            printf "(unit_test_t){.name=\"$t\",.fn=$t,.result=0},\n"
        done;
        printf "}},";
    fi
done > unittests/tests.cstruct

grep -r unittests/tests -h -e "^UNIT_TEST([a-Z0-9\_]*)$" | sed 's/UNIT_TEST(\([a-Z0-9\_]*\))/extern unit_test_fn \1;/' > unittests/tests.lst

grep -l -e "^UNIT_TEST([a-Z0-9\_]*)$" -r unittests/tests -h | wc -l