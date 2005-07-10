#ifndef CORE_H
#define CORE_H

#include <stdio.h>

#define FASTCALL __attribute__((regparm(3)))

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

inline int StateWriteHeader(FILE *, const char *, int);
inline int StateFinishHeader(FILE *, int);
inline int StateCheckRetrieveHeader(FILE *, const char *, int *, int *);

#endif
