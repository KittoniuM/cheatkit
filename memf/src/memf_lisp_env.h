#ifndef __MEMF_LISP_ENV__
#define __MEMF_LISP_ENV__

#include <stddef.h>

#include "memf.h"
#include "lisp.h"

typedef struct {
	const char	*name;
	lisp_result_t (*func)(const char *, size_t, const lisp_result_t *);
} memf_lisp_fenv_t;

typedef struct {
	const char	*name;
	lisp_result_t (*func)(const char *);
} memf_lisp_venv_t;

extern const memf_lisp_fenv_t memf_lisp_fenv[];
extern const memf_lisp_venv_t memf_lisp_venv[];

#endif /* __MEMF_LISP_ENV__ */
