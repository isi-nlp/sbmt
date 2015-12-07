/* Copyright (c) 2001 by David Chiang. All rights reserved.*/

#ifndef GENERIC_H
#define GENERIC_H

#include <stdio.h>

#define _ENDO(f) void * (*f) (void *)
#define ENDO void * (*) (void *)

#define _DELETE(f) void (*f) (void *)
#define DELETE void (*) (void *)

#define _COMPARE(f) int (*f) (const void *, const void *)
#define COMPARE int (*) (const void *, const void *)

#define _HASH(f) unsigned int (*f) (const void *)
#define HASH unsigned int (*) (const void *)

#define _FROM_STRING(f) void * (*f) (const char *)
#define FROM_STRING void * (*) (const char *)

#define _TO_STRING(f) int (*f) (const void *, char *, int)
#define TO_STRING int (*) (const void *, char *, int)

#define _READ(f) void * (*f) (FILE *fp)
#define READ void * (*) (FILE *fp)

#define _WRITE(f) void (*f) (FILE *fp, void *)
#define WRITE void (*) (FILE *fp, void *)

#endif
