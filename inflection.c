
#include "inflection.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* 
  using 
  http://github.com/visionmedia/ext.js/blob/master/lib/ext/core_ext/string/inflections.js
  as a source
  
  Yes it's ugly, but it's fast, get over it.
*/

int num_uncountables = 24;
char * uncountables[] = {
    "advice", "energy", "excretion", "digestion", "cooperation", "health", "justice", "labour", 
    "machinery", "equipment", "information", "pollution", "sewage", "paprer", "money", 
    "species", "series", "rain", "rice", "fish", "sheep", "moose", "deer", "news" };

int eq(char a, char b)
{
  return a == b || a == toupper(b);
}

char samecase(char a, char b)
{
  return a == toupper(a) ? toupper(b) : b;
}

int is_plural(char *s)
{
  int l = strlen(s);
  char *s_plural = malloc(l+10);
  pluralize(s, s_plural);
  int same = strcmp(s,s_plural)==0;
  free(s_plural);
  return same;
}

int is_singular(char *s)
{
  int l = strlen(s);
  char *s_plural = malloc(l+10);
  pluralize(s, s_plural);
  int same = strcmp(s, s_plural)==0;
  if (same == 0)
  {
    char *s_plus_s = malloc(l+10);
    sprintf(s_plus_s, "%ss", s);
    same = strcmp(s_plus_s, s_plural)==0;
    free(s_plus_s);
  }
  free(s_plural);
  return same;
}

char *pluralize(char *s, char *c) // puts the plural form of s into c and returns c (c must be big enough for it)
{
  if (s != c) strcpy(c, s);
  
  int l = 0;
  for (l = 0 ; l < num_uncountables ; l++)
    if (strcasecmp(uncountables[l], c)==0) return c;
  
  l = strlen(c);
  if (l >= 3 && 
      eq(c[l-3], 'm') && 
      eq(c[l-2], 'a') && 
      eq(c[l-1], 'n')) /* [/(m)an$/gi, '$1en'], */
  {
    c[l-2] = samecase(c[l-2], 'e');
  }
  else if (l >= 6 && 
           eq(c[l-6], 'p') && 
           eq(c[l-5], 'e') && 
           eq(c[l-4], 'r') && 
           eq(c[l-3], 's') && 
           eq(c[l-2], 'o') && 
           eq(c[l-1], 'n')) /* [/(pe)rson$/gi, '$1ople'], */
  {
    c[l-4] = samecase(c[l-4], 'o');
    c[l-3] = samecase(c[l-3], 'p');
    c[l-2] = samecase(c[l-2], 'l');
    c[l-1] = samecase(c[l-1], 'e');
  }
  else if (l >= 5 &&  
           eq(c[l-5], 'c') && 
           eq(c[l-4], 'h') && 
           eq(c[l-3], 'i') && 
           eq(c[l-2], 'l') && 
           eq(c[l-1], 'd')) /* [/(child)$/gi, '$1ren'], */
  {
    c[l] = samecase(c[l-1], 'r');
    c[l+1] = samecase(c[l-1], 'e');
    c[l+2] = samecase(c[l-1], 'n');
    c[l+3] = 0;
  }
  else if (l == 2 && strcasecmp(c, "ox")==0) /* [/^(ox)$/gi, '$1en'], */
  {
    c[2] = samecase(c[1], 'e');
    c[3] = samecase(c[l-1], 'n');
    c[4] = 0;
  }
  else if ((l >= 4 && strcasecmp(&c[l-4], "axis")==0) || (l >= 6 && strcasecmp(&c[l-6], "testis")==0)) /* [/(ax|test)is$/gi, '$1es'], */
  {
    c[l-2] = samecase(c[l-1], 'e');
    c[l-1] = samecase(c[l-1], 's');
    c[l] = 0;
  }
  else if ((l >= 5 && strcasecmp(&c[l-5], "alias")==0) || (l >= 6 && strcasecmp(&c[l-6], "status")==0) || /* [/(alias|status)$/gi, '$1es'], */
            (l >= 7 && strcasecmp(&c[l-7], "octopus")==0) || (l >= 5 && strcasecmp(&c[l-5], "virus")==0) || /* [/(octopus|virus)$/gi, '$1es'], */
            (l >= 3 && strcasecmp(&c[l-3], "bus")==0) || /* [/(bu)s$/gi, '$1ses'], */
            (l >= 7 && strcasecmp(&c[l-7], "buffalo")==0) || (l >= 6 && strcasecmp(&c[l-6], "tomato")==0) || (l >= 6 && strcasecmp(&c[l-6], "potato")==0)) /* [/(buffal|tomat|potat)o$/gi, '$1oes'], */
  {
    c[l] = samecase(c[l-1], 'e');
    c[l+1] = samecase(c[l-1], 's');
    c[l+2] = 0;
  }
  else if (l >= 3 && (eq(c[l-3], 't') || eq(c[l-3],'i')) && strcasecmp(&c[l-2], "um")==0) /* [/([ti])um$/gi, '$1a'], */
  {
    c[l-2] = samecase(c[l-1], 'a');
    c[l-1] = 0;
  }
  else if (l >= 3 && strcasecmp(&c[l-3], "sis")==0) /* [/sis$/gi, 'ses'], */
  {
    c[l-2] = samecase(c[l-2], 'e');
  }
  else if ((l >= 3 && !eq(c[l-3], 'f') && eq(c[l-2], 'f') && eq(c[l-1], 'e')) || /* [/([^f])fe$/gi, '$1ves'], */
            (l >= 2 && (eq(c[l-2], 'l') || eq(c[l-2], 'r')) && eq(c[l-1],'f'))) /* [/([lr])f$/gi, '$1ves'], */
  {
    if (eq(c[l-1], 'f'))
    {
      c[l-1] = samecase(c[l-1], 'v');
      c[l] = samecase(c[l-1], 'e');
      c[l+1] = samecase(c[l-1], 's');
      c[l+2] = 0;
    }
    else if (eq(c[l-1], 'e'))
    {
      c[l-2] = samecase(c[l-2], 'v');
      c[l-1] = samecase(c[l-2], 'e');
      c[l] = samecase(c[l-2], 's');
      c[l+1] = 0;
    }
  }
  else if (l >= 4 && strcasecmp(&c[l-4], "hive")==0) /* [/(hive)$/gi, '$1s'], */
  {
    c[l] = samecase(c[l-1], 's');
    c[l+1] = 0;
  }
  else if (l >= 2 && eq(c[l-1], 'y') && !(eq(c[l-2], 'a') || eq(c[l-2], 'e') || eq(c[l-2], 'i') || eq(c[l-2], 'o') || eq(c[l-2], 'u') || eq(c[l-2], 'y')))
  { /* [/([^aeiouy])y$/gi, '$1ies'], */
    c[l-1] = samecase(c[l-1], 'i');
    c[l] = samecase(c[l-1], 'e');
    c[l+1] = samecase(c[l-1], 's');
    c[l+2] = 0;
  }
  else if (l >= 3 && strcasecmp(&c[l-3], "quy")==0) /* [/(qu)y$/gi, '$1ies'], */
  {
    c[l-1] = samecase(c[l-1], 'i');
    c[l] = samecase(c[l-1], 'e');
    c[l+1] = samecase(c[l-1], 's');
    c[l+2] = 0;
  }
  else if ((l >= 6 && strcasecmp(&c[l-6], "matrix")==0 || strcasecmp(&c[l-6], "vertex")==0) || (l >= 5 && strcasecmp(&c[l-5], "index")==0)) /* [/(matr|vert|ind)ix|ex$/gi, '$1ices'], */
  {
    c[l-2] = samecase(c[l-2], 'i');
    c[l-1] = samecase(c[l-2], 'c');
    c[l] = samecase(c[l-2], 'e');
    c[l+1] = samecase(c[l-2], 's');
    c[l+2] = 0;
  }
  else if (l >= 2 && (c[l-1] == 'x' || strcasecmp(&c[l-2], "ch")==0 || strcasecmp(&c[l-2], "ss")==0 || strcasecmp(&c[l-2], "sh")==0)) /* [/(x|ch|ss|sh)$/gi, '$1es'], */
  {
    c[l] = samecase(c[l-1], 'e');
    c[l+1] = samecase(c[l-1], 's');
    c[l+2] = 0;
  }
  else if (l >= 5 && (strcasecmp(&c[l-5], "mouse")==0 || strcasecmp(&c[l-5], "louse")==0)) /* [/([m|l])ouse$/gi, '$1ice'], */
  {
    c[l-4] = samecase(c[l-1], 'i');
    c[l-3] = samecase(c[l-1], 'c');
    c[l-2] = samecase(c[l-1], 'e');
    c[l-1] = 0;
  }
  else if (l >= 4 && strcasecmp(&c[l-4], "quiz")==0) /* [/(quiz)$/gi, '$1zes'], */
  {
    c[l] = samecase(c[l-1], 'z');
    c[l+1] = samecase(c[l-1], 'e');
    c[l+2] = samecase(c[l-1], 's');
    c[l+3] = 0;
  }
  else if (l >= 1 && !eq(c[l-1],'s'))
  {
    c[l] = samecase(c[l-1], 's');
    c[l+1] = 0;
  }
  
  return c;
  
  /*
  [/(m)an$/gi, '$1en'],
  [/(pe)rson$/gi, '$1ople'],
  [/(child)$/gi, '$1ren'],
  [/^(ox)$/gi, '$1en'],
  [/(ax|test)is$/gi, '$1es'],
  [/(octop|vir)us$/gi, '$1i'], // neither are valid - via wikipedia
  [/(alias|status)$/gi, '$1es'],
  [/(bu)s$/gi, '$1ses'],
  [/(buffal|tomat|potat)o$/gi, '$1oes'],
  [/([ti])um$/gi, '$1a'],
  [/sis$/gi, 'ses'],
  [/(?:([^f])fe|([lr])f)$/gi, '$1$2ves'],
  [/(hive)$/gi, '$1s'],
  [/([^aeiouy]|qu)y$/gi, '$1ies'],
  [/(x|ch|ss|sh)$/gi, '$1es'], 
  [/(matr|vert|ind)ix|ex$/gi, '$1ices'], // this will never trigger because of the previous regex
  [/([m|l])ouse$/gi, '$1ice'],
  [/(quiz)$/gi, '$1zes'],
  [/s$/gi, 's'],
  [/$/gi, 's']
  */
}

char *singularize(char *s, char *c)
{
  if (c != s) strcpy(c, s);
  
  int l = 0;
  for (l = 0 ; l < num_uncountables ; l++)
    if (strcasecmp(uncountables[l], c)==0) return c;
  
  l = strlen(c);
  if (l >= 3 && strcasecmp(&c[l-3], "men")==0) /* [/(m)en$/gi, '$1an'], */
  {
    c[l-2] = samecase(c[l-2], 'a');
  }
  else if (l >= 6 && strcasecmp(&c[l-6], "people")==0) /* [/(pe)ople$/gi, '$1rson'], */
  {
    c[l-4] = samecase(c[l-4], 'r');
    c[l-3] = samecase(c[l-3], 's');
    c[l-2] = samecase(c[l-2], 'o');
    c[l-1] = samecase(c[l-1], 'n');
  }
  else if (l >= 8 && strcasecmp(&c[l-8], "children")==0) /* [/(child)ren$/gi, '$1'], */
  {
    c[l-3] = 0;
    c[l-2] = 0;
    c[l-1] = 0;
  }
  else if ((l >= 8 && strcasecmp(&c[l-8], "analyses")==0) || 
           (l >= 5 && strcasecmp(&c[l-5], "bases")==0) || 
           (l >= 9 && strcasecmp(&c[l-9], "diagnoses")==0) || 
           (l >= 9 && strcasecmp(&c[l-9], "prognoses")==0) || 
           (l >= 8 && strcasecmp(&c[l-8], "synopses")==0) || 
           (l >= 6 && strcasecmp(&c[l-6], "theses")==0)) /* [/((analy|ba|diagno|parenthe|progno|synop|the)ses$/gi, '$1sis'], */
  {
    c[l-3] = samecase(c[l-3], 's');
    c[l-2] = samecase(c[l-2], 'i');
    c[l-1] = samecase(c[l-1], 's');
  }
  else if ((l >= 5 && strcasecmp(&c[l-5], "hives")==0) || 
           (l >= 5 && strcasecmp(&c[l-5], "tives")==0) || 
           (l >= 6 && strcasecmp(&c[l-6], "curves")==0)) /* [/(hive)s$/gi, '$1'], [/(tive)s$/gi, '$1'], [/(curve)s$/gi, '$1'], */
  {
    c[l-1] = 0;
  }
  else if (l >= 6 && strcasecmp(&c[l-6], "movies")==0)
  { /* [/(m)ovies$/gi, '$1ovie'], */
    c[l-1] = 0;
  }
  else if (l >= 5 && strcasecmp(&c[l-5], "shoes")==0)
  { /* [/(shoe)s$/gi, '$1'], */
    c[l-1] = 0;
  }
  else if (l >= 3 && (eq(c[l-2], 't') || eq(c[l-2], 'i')) && eq(c[l-1], 'a')) /* [/([ti])a$/gi, '$1um'], */
  {
    c[l-1] = samecase(c[l-1], 'u');
    c[l] = samecase(c[l-1], 'm');
    c[l+1] = 0;
  }
  else if ((l >= 4 && strcasecmp(&c[l-4], "rves")==0) || 
           (l >= 4 && strcasecmp(&c[l-4], "lves")==0)) /* [/([lr])ves$/gi, '$1f'], */
  {
    c[l-3] = samecase(c[l-3], 'f');
    c[l-2] = 0;
  }
  else if (l >= 5 && strcasecmp(&c[l-5], "quies")==0)
  { /* [/(qu)ies$/gi, '$1y'], */
    c[l-3] = samecase(c[l-3], 'y');
    c[l-2] = 0;
  }
  else if (l >= 4 && (strcasecmp(&c[l-4], "mice")==0 || strcasecmp(&c[l-4], "lice")==0))
  { /* [/([m|l])ice$/gi, '$1ouse'], */
    c[l-3] = samecase(c[l-3], 'o');
    c[l-2] = samecase(c[l-3], 'u');
    c[l-1] = samecase(c[l-3], 's');
    c[l] = samecase(c[l-3], 'e');
    c[l+1] = 0;
  }
  else if ((l >= 6 && strcasecmp(&c[l-6], "crises")==0) || (l >= 6 && strcasecmp(&c[l-6], "testes")==0) || (l >= 4 && strcasecmp(&c[l-4], "axes")==0))
  { /* [/(cris|ax|test)es$/gi, '$1is'], */
    c[l-2] = samecase(c[l-2], 'i');
    c[l-1] = samecase(c[l-2], 's');
    c[l] = 0;
  }
  else if ((l >= 7 && strcasecmp(&c[l-7], "aliases")==0) || (l >= 8 && strcasecmp(&c[l-8], "statuses")==0) ||
           (l >= 7 && strcasecmp(&c[l-7], "viruses")==0) || (l >= 9 && strcasecmp(&c[l-9], "octopuses")==0))
  { /* [/(alias|status|virus|octopus)es$/gi, '$1'], */
    c[l-2] = 0;
  }
  else if ((l >= 4 && strcasecmp(&c[l-3], "ves")==0) && !(eq(c[l-4],'f') || eq(c[l-4],'o'))) /* [/([^fo])ves$/gi, '$1fe'], */
  {
    c[l-3] = samecase(c[l-3], 'f');
    c[l-2] = samecase(c[l-3], 'e');
    c[l-1] = 0;
  }
  else if (l >= 4 && strcasecmp(&c[l-3], "ies")==0 && !(eq(c[l-4], 'a') || eq(c[l-4], 'e') || eq(c[l-4], 'i') || eq(c[l-4], 'o') || eq(c[l-4], 'u') || eq(c[l-4], 'y')))
  { /* [/([^aeiouy])ies$/gi, '$1y'], */
    c[l-3] = samecase(c[l-3], 'y');
    c[l-2] = 0;
  }
  else if ((l >= 4 && (strcasecmp(&c[l-4], "ches")==0 || strcasecmp(&c[l-4], "sses")==0 || strcasecmp(&c[l-4], "shes")==0)) ||
           (l >= 3 && strcasecmp(&c[l-3], "xes")==0) || (l >= 5 && strcasecmp(&c[l-5], "buses")==0) || (l >= 3 && strcasecmp(&c[l-5], "oes")==0))
  { /* [/(x|ch|ss|sh)es$/gi, '$1'], [/(bus)es$/gi, '$1'], [/(o)es$/gi, '$1'], */
    c[l-2] = 0;
  }
  else if ((l >= 8 && strcasecmp(&c[l-8], "vertices")==0) || (l >= 7 && strcasecmp(&c[l-7], "indices")==0))
  { /* [/(vert|ind)ices$/gi, '$1ex'], */
    c[l-4] = samecase(c[l-2], 'e');
    c[l-3] = samecase(c[l-2], 'x');
    c[l-2] = 0;
  }
  else if ((l >= 8 && strcasecmp(&c[l-8], "matrices")==0))
  { /* [/(matr)ices$/gi, '$1ix'], */
    c[l-4] = samecase(c[l-2], 'i');
    c[l-3] = samecase(c[l-2], 'x');
    c[l-2] = 0;
  }
  else if ((l >= 7 && strcasecmp(&c[l-7], "quizzes")==0))
  { /* [/(quiz)zes$/gi, '$1'], */
    c[l-3] = 0;
  }
  else if ((l == 4 && strcasecmp(c, "oxen")==0))
  { /* [/(quiz)zes$/gi, '$1'], */
    c[l-2] = 0;
  }
  else if (l >= 1 && eq(c[l-1], 's'))
  { /* [/s$/gi, ''], */
    c[l-1] = 0;
  }
  
  return c;
  /*
  [/(m)en$/gi, '$1an'],
  [/(pe)ople$/gi, '$1rson'],
  [/(child)ren$/gi, '$1'],
  [/([ti])a$/gi, '$1um'],
  [/((a)naly|(b)a|(d)iagno|(p)arenthe|(p)rogno|(s)ynop|(t)he)ses$/gi, '$1$2sis'], // theses covers parentheses
  [/(hive)s$/gi, '$1'],
  [/(tive)s$/gi, '$1'],
  [/(curve)s$/gi, '$1'],
  [/([lr])ves$/gi, '$1f'],
  [/([^fo])ves$/gi, '$1fe'],
  [/([^aeiouy]|qu)ies$/gi, '$1y'],
  [/(s)eries$/gi, '$1eries'], // what the fuck (uncountable)
  [/(m)ovies$/gi, '$1ovie'], // what the fuck (2 lines above hits it first)
  [/(x|ch|ss|sh)es$/gi, '$1'],
  [/([m|l])ice$/gi, '$1ouse'],
  [/(bus)es$/gi, '$1'],
  [/(o)es$/gi, '$1'],
  [/(shoe)s$/gi, '$1'], // what the heck (previous line wins)
  [/(cris|ax|test)es$/gi, '$1is'],  // what the heck, (ax)es is picked up by the row 5 lines up
  [/(octop|vir)i$/gi, '$1us'], // neither are valid - via wikipedia
  [/(alias|status)es$/gi, '$1'],
  
  [/^(ox)en/gi, '$1'],
  [/(vert|ind)ices$/gi, '$1ex'],
  [/(matr)ices$/gi, '$1ix'],
  [/(quiz)zes$/gi, '$1'],
  [/s$/gi, '']
  */
}

int main()
{
  char *word = malloc(50);
  char *temp = malloc(50);
  
  clock_t start = clock();
  
  char *words [] = {
    "man", "person", "child", "analysis", "hive", "parenthesis", "synopsis", "creative", "curve", "life", "glove", "movie", "crisis",
    "shoe", "axis", "virus", "octopus", "ox", "vertex", "index", "matrix", "quiz", "ham"
  };
  
  int j=1;
  int i=1;
  //for (j = 0 ; j < 100000 ; j++)
  for (i = 0 ; i < sizeof(words)/8 ; i++)
  {
    sprintf(word, "%s", words[i]);
    printf("  (%d/%d) %-15s -> ", is_singular(word), is_plural(word), words[i]);
    pluralize(words[i], temp);
    printf("(%d/%d) %-15s -> ", is_singular(temp), is_plural(temp), temp);
    singularize(temp, temp);
    printf("(%d/%d) %-15s\n", is_singular(temp), is_plural(temp), temp);
  }
  
  printf("%d pluralize('%s') in %f seconds\n", i, word, (clock()-(float)start)/CLOCKS_PER_SEC);
}













