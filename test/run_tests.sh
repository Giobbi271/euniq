#!/bin/sh
set -eu

BIN="./euniq"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

# 1. Non-adjacent deduplication via stdin/stdout
printf "apple\nbanana\napple\ncherry\nbanana\n" | "$BIN" >"$TMPDIR/out1.txt"
printf "apple\nbanana\ncherry\n" >"$TMPDIR/exp1.txt"
diff -u "$TMPDIR/exp1.txt" "$TMPDIR/out1.txt"

# 2. Input and output file arguments
"$BIN" "$TMPDIR/out1.txt" "$TMPDIR/out2.txt"
diff -u "$TMPDIR/exp1.txt" "$TMPDIR/out2.txt"

# 3. Long line (>16 KB) to verify dynamic getline() buffering
python3 -c 'print("A" * 20000 + "\n" + "A" * 20000)' | "$BIN" >"$TMPDIR/long.txt"
[ "$(wc -l <"$TMPDIR/long.txt")" -eq 1 ]

# 4. SIGINT clean shutdown (expects exit code 130)
set +e
sleep 5 | "$BIN" >/dev/null &
PID=$!
sleep 0.1
kill -INT "$PID"
wait "$PID"
STATUS=$?
set -e
[ "$STATUS" -eq 130 ]

echo "All tests passed."
