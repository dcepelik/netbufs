#ifndef COMMON_H
#define COMMON_H

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

typedef unsigned char	nb_byte_t;

#endif
