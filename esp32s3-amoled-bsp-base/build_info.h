#ifndef BUILD_INFO_H
#define BUILD_INFO_H

// Build number = the git commit count of the commit that contains this
// file. It is bumped on every push, so the number shown on the web page
// (and in the Serial "Heap:" line) can be compared with
//     git rev-list --count HEAD
// on the PC to confirm the board is running the latest pushed code.
// The compile date/time shown next to it comes from the compiler
// (__DATE__ / __TIME__), so it also proves the sketch was rebuilt.
#define FW_BUILD_NUMBER 245

#endif
