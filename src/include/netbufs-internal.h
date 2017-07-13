#ifndef NETBUFS_INTERNAL_H
#define NETBUFS_INTERNAL_H

#include "error.h"
#include "netbufs.h"

nb_err_t nb_error(struct nb *nb, nb_err_t err, char *msg, ...);

#endif
