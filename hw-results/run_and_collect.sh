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

HERE=$(dirname "$0")
HARNESS_ROOT=$(dirname ${HERE})
KVM_LITMUS=${HARNESS_ROOT}/kvm_litmus
TS=$(date -I)
RESULTS_DIR=${HERE}/hw-refs/${HWREF_NAME:-$(hostname)}
mkdir -p ${RESULTS_DIR}/logs
${KVM_LITMUS} --id > ${RESULTS_DIR}/id.txt
nohup ${KVM_LITMUS} -q -n500k -b2k $@ >> ${RESULTS_DIR}/logs/${TS}.log &