#!/bin/sh

set -eu

target=${1:-./producer_consumer}

help_output=$(printf '4\n\n0\n' | "$target")
printf '%s' "$help_output" | grep -q "HELP"
printf '%s' "$help_output" | grep -q "Goodbye"

invalid_output=$(printf 'not-a-number\n0\n' | "$target")
printf '%s' "$invalid_output" | grep -q "Invalid input"

cancel_output=$(printf '1\nn\n0\n' | "$target")
printf '%s' "$cancel_output" | grep -q "Balanced"
printf '%s' "$cancel_output" | grep -q "Simulation cancelled"

run_output=$(printf '1\ny\n\n0\n' | "$target")
printf '%s' "$run_output" | grep -q "Correctness checks:   PASS"
printf '%s' "$run_output" | grep -q "Goodbye"

printf '[TEST] CLI flows\nAll CLI tests passed.\n'
