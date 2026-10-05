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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Parses the cuesheet in cue[0..size). lookup() is asked for every FILE the sheet references and
// returns 0 if it doesn't exist, or 1 with its size filled in. Writes the parsed track table, or the
// parser's error, to out if it isn't NULL. Returns 0 on success, 1 on a parse error, 2 if the
// completion callback didn't fire exactly once.
int harness_parse(const uint8_t* cue, size_t size, int (*lookup)(void* user, const char* name, uint64_t* size),
                  void* user, FILE* out);
