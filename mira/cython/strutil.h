#ifndef STRUTIL_H
#define STRUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

char *strstrsep(char **stringp, const char *delim);
char *strip(char *s);
char **split(char *s, const char *delim, int *pn);

#ifdef __cplusplus
}
#endif

#endif
