#!/bin/sh
# Runs cuedump over every cuesheet in corpus/ and diffs the output against its .gold file.
# With --update, rewrites the .gold files instead. The exit status is part of the output, and a leak
# exits with 23, so a sheet that leaks says so in its .gold file.

dump=$1
update=$2
dir=$(dirname "$0")/corpus
failed=0

run() {
    LSAN_OPTIONS=exitcode=23 "$dump" "$1"
    echo "exit $?"
}

count=0
for cue in "$dir"/*.cue; do
    gold=${cue%.cue}.gold
    count=$((count + 1))
    if [ "$update" = --update ]; then
        run "$cue" > "$gold"
    elif ! run "$cue" | diff -u "$gold" -; then
        echo "FAIL $cue"
        failed=1
    fi
done
if [ $count -eq 0 ]; then
    echo "no cuesheets in $dir"
    exit 1
fi
if [ $failed -ne 0 ]; then
    echo "$count cuesheets, some FAILED"
    exit 1
fi
echo "$count cuesheets OK"
