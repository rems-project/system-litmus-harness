# litmus_cron
# Runs ./litmus.exe in a background loop
# example usage:  `nohup ./litmus_cron.sh &``
#
# While litmus_cron.sh is running:
#   every night at 23:00 local it will 
#       UNTIL 6am Mon-Fri
#           run ./litmus.exe
#           append output to overnights/YYYY-overnight-ddd.txt
#           append time it took
#
# This script should be POSIX-compatible and run anywhere.
# The script's own stdout is written to at a rate of 2kb/day and so
# if the output is piped to a file, that file should be routinely flushed.

mkdir -p overnights/
while true; do
    echo "... $(date)" ;
    if [ "$(date +%H)" = 23 ]; then
        echo 'Running Litmus ...'
        bash -c '
        day="$(date +%d)" ;
        while [ "$(date +%d)" = $day -o "$(date +%H)" -lt 7 -o "$(date +%w)" -gt 4 ]; do
            time ./litmus.exe @all -n500000 --pgtable
        done;' > overnights/$(date +%Y)-overnight-$(date +%j).txt 2>&1 ;
        echo '... done and continuing' ;
        continue ;
    fi;
    sleep 1800;
done
