/*

MIT License

Copyright (c) 2026 Rix

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cueparser/cueparser.h"
#include "cueparser/disc.h"
#include "cueparser/fileabstract.h"
#include "cueparser/scheduler.h"

// The data files a cuesheet references are never read by the parser, only sized, so a file here is
// a name plus a size. The cuesheet itself is an in-memory buffer.
struct HarnessFile {
    char* name;
    uint64_t size;
    const uint8_t* data;
    struct CueFile* file;
    struct HarnessFile* next;
};

struct Context {
    struct HarnessFile* files;
    int (*lookup)(void* user, const char* name, uint64_t* size);
    void* user;
    const char* error;
    int done;
};

static void file_destroy(struct CueFile* file) {}

static void file_close(struct CueFile* file, struct CueScheduler* scheduler,
                       void (*cb)(struct CueFile*, struct CueScheduler*)) {
    File_schedule_close(file, scheduler, cb);
}

static void file_size(struct CueFile* file, struct CueScheduler* scheduler, int compressed,
                      void (*cb)(struct CueFile*, struct CueScheduler*, uint64_t)) {
    struct HarnessFile* hf = file->opaque;
    File_schedule_size(file, scheduler, hf->size, cb);
}

static void file_read(struct CueFile* file, struct CueScheduler* scheduler, uint32_t amount, uint64_t cursor,
                      uint8_t* buffer,
                      void (*cb)(struct CueFile*, struct CueScheduler*, int error, uint32_t amount, uint8_t* buffer)) {
    struct HarnessFile* hf = file->opaque;
    if (!hf->data || (cursor >= hf->size)) {
        File_schedule_read(file, scheduler, 0, 0, NULL, cb);
        return;
    }
    uint64_t left = hf->size - cursor;
    if (amount > left) amount = left;
    memcpy(buffer, hf->data + cursor, amount);
    File_schedule_read(file, scheduler, 0, amount, buffer, cb);
}

static void file_write(struct CueFile* file, struct CueScheduler* scheduler, uint32_t amount, uint64_t cursor,
                       const uint8_t* buffer,
                       void (*cb)(struct CueFile*, struct CueScheduler*, int error, uint32_t amount)) {
    File_schedule_write(file, scheduler, 1, 0, cb);
}

static struct HarnessFile* setup_file(struct Context* context, struct CueFile* file, const char* name) {
    struct HarnessFile* hf = calloc(1, sizeof(struct HarnessFile));
    hf->name = strdup(name);
    hf->file = file;
    hf->next = context->files;
    context->files = hf;
    file->destroy = file_destroy;
    file->close = file_close;
    file->size = file_size;
    file->read = file_read;
    file->write = file_write;
    file->references = 1;
    file->cfilename = hf->name;
    file->filename = NULL;
    file->opaque = hf;
    return hf;
}

static struct CueFile* open_cb(struct CueFile* file, struct CueScheduler* scheduler, const char* name) {
    struct Context* context = scheduler->opaque;
    uint64_t size;
    file->destroy = file_destroy;
    if (!context->lookup(context->user, name, &size)) return NULL;
    struct HarnessFile* hf = setup_file(context, file, name);
    hf->size = size;
    return file;
}

static void parse_cb(struct CueParser* parser, struct CueScheduler* scheduler, const char* error) {
    struct Context* context = scheduler->opaque;
    context->error = error;
    context->done++;
}

static void close_cb(struct CueParser* parser, struct CueScheduler* scheduler, const char* error) {}

static void print_msf(FILE* out, uint32_t lba) {
    fprintf(out, "%02u:%02u:%02u", lba / 75 / 60, lba / 75 % 60, lba % 75);
}

static const char* track_type(enum CueTrackType type) {
    switch (type) {
        case TRACK_TYPE_UNKNOWN:
            return "UNKNOWN";
        case TRACK_TYPE_AUDIO:
            return "AUDIO";
        case TRACK_TYPE_DATA:
            return "DATA";
    }
    return "INVALID";
}

static void dump_disc(FILE* out, struct CueDisc* disc) {
    fprintf(out, "tracks %d\n", disc->trackCount);
    if (disc->catalog[0]) fprintf(out, "catalog %s\n", disc->catalog);
    if (disc->isrc[0]) fprintf(out, "isrc %s\n", disc->isrc);
    for (int i = 1; i <= disc->trackCount; i++) {
        struct CueTrack* track = &disc->tracks[i];
        struct HarnessFile* hf = track->file ? track->file->opaque : NULL;
        fprintf(out, "track %02d %s%s file \"%s\" refs %d\n", i, track_type(track->trackType),
                track->compressed ? " compressed" : "", hf ? hf->name : "(null)",
                track->file ? track->file->references : 0);
        if (track->digitalCopyPermitted || track->fourChannelAudio || track->preEmphasis ||
            track->serialCopyManagementSystem) {
            fprintf(out, "  flags%s%s%s%s\n", track->digitalCopyPermitted ? " DCP" : "",
                    track->fourChannelAudio ? " 4CH" : "", track->preEmphasis ? " PRE" : "",
                    track->serialCopyManagementSystem ? " SCMS" : "");
        }
        fprintf(out, "  fileOffset %u size %u postgap %u indexCount %d\n", track->fileOffset, track->size,
                track->postgap, track->indexCount);
        for (int j = 0; j <= track->indexCount && j < MAXINDEX; j++) {
            fprintf(out, "  index %02d %u ", j, track->indices[j]);
            print_msf(out, track->indices[j]);
            fprintf(out, "\n");
        }
        // What src/cdrom/cdriso-cue.cc in pcsx-redux derives from the table.
        if (track->indexCount >= 1) {
            fprintf(out, "  redux start ");
            print_msf(out, track->indices[1] + 150);
            fprintf(out, " pregap %u byteOffset %lld length %u\n", track->indices[1] - track->indices[0],
                    ((long long)track->indices[1] - (long long)track->fileOffset) * 2352, track->size);
        }
    }
}

int harness_parse(const uint8_t* cue, size_t size, int (*lookup)(void*, const char*, uint64_t*), void* user,
                  FILE* out) {
    struct Context context;
    memset(&context, 0, sizeof(context));
    context.lookup = lookup;
    context.user = user;

    struct CueScheduler scheduler;
    Scheduler_construct(&scheduler);
    scheduler.opaque = &context;

    struct CueFile cueFile;
    memset(&cueFile, 0, sizeof(cueFile));
    struct HarnessFile* cueHf = setup_file(&context, &cueFile, "(cuesheet)");
    cueHf->data = cue;
    cueHf->size = size;

    // Poisoned rather than zeroed, so anything the parser reports without having written it shows up
    // in the dump instead of reading as a plausible zero.
    struct CueDisc* disc = malloc(sizeof(struct CueDisc));
    memset(disc, 0xa5, sizeof(struct CueDisc));
    struct CueParser parser;
    CueParser_construct(&parser, disc);
    CueParser_parse(&parser, &cueFile, &scheduler, open_cb, parse_cb);
    Scheduler_run(&scheduler);

    int ret = 0;
    if (context.done != 1) {
        if (out) fprintf(out, "callback fired %d times\n", context.done);
        ret = 2;
    } else if (context.error) {
        if (out) fprintf(out, "error %s\n", context.error);
        ret = 1;
    } else if (out) {
        dump_disc(out, disc);
    }

    CueParser_close(&parser, &scheduler, close_cb);
    Scheduler_run(&scheduler);
    CueParser_destroy(&parser);

    // The parser allocated every data file's CueFile and hands them over; the cuesheet's is ours.
    while (context.files) {
        struct HarnessFile* hf = context.files;
        context.files = hf->next;
        if (hf->file != &cueFile) free(hf->file);
        free(hf->name);
        free(hf);
    }
    free(disc);
    return ret;
}
