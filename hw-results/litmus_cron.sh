#!/bin/bash
# litmus_cron
# Runs ./litmus.exe in a background loop
# example usage:  `nohup ./litmus_cron.sh &``
#
# since not all systems support 'cron' this script can be used
# as a replacment, to run the litmus tests every night.
# While litmus_cron.sh is running:
#   every night at 23:00 local it will
#       UNTIL 6am Mon-Fri
#           run ./litmus.exe
#           append output to overnights/YYYY-overnight-ddd.log
#           append time it took
#
# This script requires bash.
# The script's own stdout is written to at a rate of 2kb/day and so
# if the output is piped to a file, that file should be routinely flushed.

START_HOUR=23
END_HOUR=8

run_litmus () {
    time -p ./litmus.$YYYY-$ddd.exe @all -n500000 --pgtable
}

mkdir -p overnights/
while true; do
    echo "... $(date)" ;
    if [ "$(date +%H)" = $START_HOUR ]; then
        echo 'Running Litmus ...'
        YYYY="$(date +%Y)" ;
        ddd="$(date +%j)" ;
        cp ./litmus.exe ./litmus.$YYYY-$ddd.exe ;

        fname="overnights/$YYYY-overnight-$ddd.log";
        run_litmus &>$fname ;

        # it's expected that a run of 500k could take an hour or two
        # want to make sure that it's finished before $END_HOUR the next morning
        # so use the time the first run took as an estimate for how long we expect the run to last
        estsec="$(cat $fname | grep real | awk '{print $2}')" ;
        echo "Took $estsec seconds -- using as average."  ;
        while [ $(date +%j) = $ddd -o $(date -d "+$estsec seconds" +%H) -lt $END_HOUR -o $(date +%w) -gt 4 ]; do
            run_litmus &>>$fname ;
            [ ${ec=$?} -gt 0 ] && rm -f ./litmus.$YYYY-$ddd.exe && exit ${ec} ;
        done;
        echo '... done and continuing' ;
        rm ./litmus.$YYYY-$ddd.exe ;
        continue ;
    fi;
    sleep 1800;
done
