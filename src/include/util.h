#ifndef UTIL_H
#define UTIL_H

#define MIN(a, b)	((a) <= (b) ? (a) : (b))
#define MAX(a, b)	((a) >= (b) ? (a) : (b))
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))

void die(char *fmt, ...);

#endif
