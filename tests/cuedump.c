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

// Prints the track table cueparser builds for a cuesheet. The data files it references are described
// by a sidecar next to it, same name with .sizes instead of .cue: one "<size in bytes> <filename>"
// per line. A FILE missing from the sidecar doesn't exist.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"

struct Sizes {
    char* text;
};

static int lookup(void* user, const char* name, uint64_t* size) {
    struct Sizes* sizes = user;
    if (!sizes->text) return 0;
    for (char* line = sizes->text; *line;) {
        char* eol = strchr(line, '\n');
        size_t len = eol ? (size_t)(eol - line) : strlen(line);
        char* end;
        unsigned long long value = strtoull(line, &end, 10);
        if ((end != line) && (*end == ' ')) {
            end++;
            size_t nameLen = len - (end - line);
            if ((strlen(name) == nameLen) && !memcmp(name, end, nameLen)) {
                *size = value;
                return 1;
            }
        }
        line += len;
        if (*line) line++;
    }
    return 0;
}

static char* slurp(const char* path, size_t* size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* data = malloc(len + 1);
    if (fread(data, 1, len, f) != (size_t)len) {
        fclose(f);
        free(data);
        return NULL;
    }
    fclose(f);
    data[len] = 0;
    if (size) *size = len;
    return data;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s file.cue\n", argv[0]);
        return 3;
    }
    size_t size;
    char* cue = slurp(argv[1], &size);
    if (!cue) {
        perror(argv[1]);
        return 3;
    }
    size_t pathLen = strlen(argv[1]);
    char* sizesPath = malloc(pathLen + 7);
    memcpy(sizesPath, argv[1], pathLen + 1);
    char* dot = strrchr(sizesPath, '.');
    strcpy(dot ? dot : sizesPath + pathLen, ".sizes");
    struct Sizes sizes = {slurp(sizesPath, NULL)};
    int ret = harness_parse((const uint8_t*)cue, size, lookup, &sizes, stdout);
    // LeakSanitizer exits without flushing stdio.
    fflush(stdout);
    free(sizes.text);
    free(sizesPath);
    free(cue);
    return ret == 2 ? 2 : 0;
}
