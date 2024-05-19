#!/usr/bin/env bash
# Usage: ./run_and_collect.sh [args for litmus]
#
# Runs kvm_litmus with the provided args, in quiet mode
#      with a default run and batch size
#      directing the output to hw-refs/device_name/logs/YYYY-MM-DD.log
#
# device_name is the HWREF_NAME env var, or simply the machine hostname if
#             that does not exist.
#
# Example: ./run_and_collect.sh MP+pos --run-forever
# Example: HWREF_NAME=myResults ./run_and_collect.sh MP+pos --run-forever

# ANSI escapes for nice coloured text
ESC='\033'
CSI='['
RESET="${ESC}${CSI}0m"
YELLOW="${ESC}${CSI}0;33m"
RED="${ESC}${CSI}0;31m"

HERE=$(dirname "$0")
HARNESS_ROOT=$(dirname ${HERE})
KVM_LITMUS=${HARNESS_ROOT}/kvm_litmus
TS=$(date -I)
RESULTS_DIR=${HERE}/hw-refs/${HWREF_NAME:-$(hostname)}

# write h/w identification
if ! ${KVM_LITMUS} --id > ${RESULTS_DIR}/id.txt; then
  printf "${RED}[run_and_collect] Failed to generate id.txt${RESET}\n"
  exit 1
fi

mkdir -p ${RESULTS_DIR}/logs
LOG=${RESULTS_DIR}/logs/${TS}.log

printf "${YELLOW}[run_and_collect] Storing results to ${LOG}${RESET}\n"
nohup ${KVM_LITMUS} --no-color -q -n500k -b2k $@ 2>&1 >> ${LOG} </dev/null &