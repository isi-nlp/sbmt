#include <string.h>
#include <stdlib.h>

/* Like strsep(3) except that the delimiter is a string, not a set of characters. 
*/
char *strstrsep(char **stringp, const char *delim) {
  char *match, *save;
  save = *stringp;
  if (*stringp == NULL)
    return NULL;
  match = strstr(*stringp, delim);
  if (match == NULL) {
    *stringp = NULL;
    return save;
  }
  *match = '\0';
  *stringp = match + strlen(delim);
  return save;
}
