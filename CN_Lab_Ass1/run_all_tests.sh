#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
#  run_all_tests.sh — Automated test harness
#
#  Runs sender + receiver in loopback for ALL 5 detection schemes against
#  the message.txt file, producing the output data needed for the report.
#
#  Usage:  bash run_all_tests.sh
# ─────────────────────────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

INPUT="${1:-message.txt}"
PORT_BASE=9100

echo "----------------------------------------------------------"
echo "           AUTOMATED TEST HARNESS - ALL 5 SCHEMES         "
echo "----------------------------------------------------------"
echo ""
echo "Input file: $INPUT"
echo ""

for SCHEME in 1 2 3 4 5; do
    PORT=$((PORT_BASE + SCHEME))

    # Kill any leftover receiver on this port
    pkill -f "./receiver $PORT" 2>/dev/null
    sleep 0.3

    echo "--------------------------------------------------------"
    echo "  TEST: Scheme $SCHEME   Port $PORT   File: $INPUT"
    echo "--------------------------------------------------------"

    # Start receiver in background, redirect output to a temp file
    ./receiver $PORT > /tmp/rx_output_$SCHEME.txt 2>&1 &
    RX_PID=$!
    sleep 0.5

    # Run sender with piped input (scheme choice + no more packages)
    printf "$SCHEME\n" | ./sender 127.0.0.1 $PORT "$INPUT" 2>&1

    # Wait for receiver
    wait $RX_PID 2>/dev/null

    echo ""
    echo "──── RECEIVER OUTPUT (Scheme $SCHEME) ────"
    cat /tmp/rx_output_$SCHEME.txt
    echo ""
done

echo ""
echo "----------------------------------------------------------"
echo "                 ALL TESTS COMPLETE                       "
echo "----------------------------------------------------------"
