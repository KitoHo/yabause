#ifndef DEBUG_H
#define DEBUG_H

#include "core.h"
#include <stdio.h>

typedef enum { DEBUG_STRING, DEBUG_STREAM , DEBUG_STDOUT, DEBUG_STDERR } DebugOutType;

typedef struct {
	DebugOutType output_type;
	union {
		FILE * stream;
		char * string;
	} output;
	char * name;
} Debug;

Debug * DebugNew(const char *, DebugOutType, char *);
void DebugDelete(Debug *);

void DebugChangeOutput(Debug *, DebugOutType, char *);

void DebugPrintf(Debug *, const char *, u32, const char *, ...);

#ifdef DEBUG
#define LOG(d, f, r...) DebugPrintf(d, __FILE__, __LINE__, f, ## r)
#else
#define LOG(d, f, r...)
#endif

#endif
