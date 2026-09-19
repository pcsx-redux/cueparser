# cueparser

A CUE sheet parser in C, with pluggable file I/O and no blocking calls.

```c
struct CueDisc;                         /* disc.h   - tracks[MAXTRACK], audio/data, indices */
void CueParser_construct(struct CueParser*, struct CueDisc*);
void CueParser_close(struct CueParser*, struct CueScheduler*, ...);
```

You supply a `CueFile` (open/read/close) and a `CueScheduler`, and the parser drives them through
callbacks rather than calling into the filesystem itself, so it drops into a host program or an
async event loop without bringing an I/O model with it. Handles `REM`, cuesheets with no trailing
newline, and FLAC/OPUS/OGG track types alongside the usual BINARY/WAVE.

C, no dependencies, no build system. Drop the `.c` files into your own build.

## Where this came from

Written by Nicolas "Pixel" Noble and vendored in
[pcsx-redux](https://github.com/grumpycoders/pcsx-redux) as `third_party/cueparser`, where
`src/cdrom/cdriso-cue.cc` uses it to open CUE-described discs. Extracted here with
`git subtree split`, so the ten commits and their authorship are the original ones; only this
README is new.

Related: [iec-60908b](https://github.com/rixnobis/iec-60908b) for the EDC/ECC inside the sectors a
cuesheet describes, and [iec-60908](https://github.com/rixnobis/iec-60908) for the physical layer
underneath both.

MIT, see `LICENSE`.
