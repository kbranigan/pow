
#ifndef POW_INFLECTION_H
#define POW_INFLECTION_H

#ifdef __cplusplus
extern "C" {
#endif

char *pluralize(char *s, char *c);
char *singularize(char *s, char *c);

int is_plural(char *s);
int is_singular(char *s);

#ifdef __cplusplus
}
#endif

#endif