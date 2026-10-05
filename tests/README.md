# Tests

`make -C tests check` parses every cuesheet in `corpus/` with `cuedump` and diffs the result
against its `.gold` file. `make -C tests gold` rewrites the `.gold` files from the current parser;
review the diff before committing it.

The parser only ever asks the data files for their size, so a cuesheet's data files are described by
a `.sizes` file next to it, one `<bytes> <filename>` per line. A FILE that isn't listed doesn't
exist. The `.gold` file holds the track table, or the parser's error, then the exit status: 23 means
LeakSanitizer found a leak.

`make -C tests fuzz` builds a libFuzzer target over the same harness, which needs clang. The
`Fuzz` workflow runs it daily and keeps its corpus in the Actions cache; pull requests get a short
run seeded from that corpus and `corpus/`.

Where the sheets come from:

- `nugget-cdrom-test`: the disc `create-test-iso.lua` in nugget's `tests/cdrom` writes.
- `redux-1137-castlevania`: the sheet from grumpycoders/pcsx-redux#1137. The bin size is a guess,
  the issue only says it is about 500 MB.
- `single-bin-three-tracks`: one bin cut into three tracks, which #1 broke and fixed.
- `index00-cut`: tracks whose first INDEX is past the start of their FILE (#1 and #3).
- The rest are written for the case they're named after; `bad-*` are malformed.
