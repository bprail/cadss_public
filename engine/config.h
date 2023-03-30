#ifndef CONFIG_H
#define CONFIG_H

//
// Config
//
//   Reads and parses a configuration file into a set of argument vectors
// that can be passed to other components.  This allows each component to
// have its own set of arguments without requiring a specific convention.
// The tool will also skip any // or /* portions of text, handle "" text,
// but otherwise split on whitespace.
//
//   Component names start with __ such as:
//  __processor
//
//   As whitespace is ignored, the configuration file can be mixed tabs, spaces,
// newlines, and other inelegant melanges of horrors.
//

int openSettings(char*);
char** getSettings(char*, int*);
void freeSettings();

#endif
