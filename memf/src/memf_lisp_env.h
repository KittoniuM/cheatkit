#ifndef __MEMF_LISP_ENV__
#define __MEMF_LISP_ENV__

#include <stddef.h>

#include "memf.h"
#include "lisp.h"

typedef struct {
	const char	*name;
	lisp_result_t (*func)(const char *, size_t, const lisp_result_t *);
} memf_lisp_env_t;

extern const memf_lisp_env_t memf_lisp_fenv[];

#endif /* __MEMF_LISP_ENV__ */
