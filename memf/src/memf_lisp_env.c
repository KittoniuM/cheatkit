#include "memf_lisp_env.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memf.h"
#include "lisp.h"

static lisp_result_t env_arith(const char *name,
			       size_t argn, const lisp_result_t *args)
{
	lisp_result_t res = {.type = TYPE_ILL};
	if (argn == 0)
		goto fail;
	/* figure type and initialize to zero */
	for (size_t i = 0; i < argn; i++) {
		switch (args[i].type) {
		case TYPE_INT:
			/* integer, unless it's already a float */
			if (res.type != TYPE_INT && res.type != TYPE_FLT) {
				res.type = TYPE_INT;
				res.value.sint = 0;
			}
			break;
		case TYPE_FLT:
			if (res.type != TYPE_FLT) {
				res.type = TYPE_FLT;
				res.value.flt = 0.0;
			}
			break;
		default:
			goto fail;
		}
	}
	for (size_t i = 0; i < argn; i++) {
		int64_t sint;
		double	flt;
		if (res.type == TYPE_INT && args[i].type == TYPE_INT)
			sint = args[i].value.sint;
		else if (res.type == TYPE_FLT && args[i].type == TYPE_FLT)
			flt = args[i].value.flt;
		else if (res.type == TYPE_INT && args[i].type == TYPE_FLT)
			sint = (int64_t) args[i].value.flt;
		else if (res.type == TYPE_FLT && args[i].type == TYPE_INT)
			flt = (double) args[i].value.sint;
		/*
		 * Hack that pushes first argument into res.value
		 * without doing anything to it.
		 */
		if (i == 0) {
			if (res.type == TYPE_INT)
				res.value.sint = sint;
			else if (res.type == TYPE_FLT)
				res.value.flt = flt;
			else
				assert(0);
			continue;
		}
		switch (res.type) {
		case TYPE_INT:
			switch (name[0]) {
			case '+': res.value.sint += sint; break;
			case '-': res.value.sint -= sint; break;
			case '*': res.value.sint *= sint; break;
			case '/': res.value.sint /= sint; break;
			default:
				assert(0);
			}
			break;
		case TYPE_FLT:
			switch (name[0]) {
			case '+': res.value.flt += flt; break;
			case '-': res.value.flt -= flt; break;
			case '*': res.value.flt *= flt; break;
			case '/': res.value.flt /= flt; break;
			default:
				assert(0);
			}
			break;
		default:
			assert(0);
		}
	}
	return res;
fail:
	return (lisp_result_t) {.type = TYPE_ILL};
}

static lisp_result_t env_test(const char *name,
			      size_t argn, const lisp_result_t *args)
{
	lisp_result_t	res;
	bool		eq;
	if (argn != 2)
		goto fail;
	if (args[0].type != args[1].type) {
		res.type = TYPE_BOOL;
		res.value.sbool = false;
		return res;
	}
	switch (args[0].type) {
	case TYPE_BOOL:
		eq = args[0].value.sbool == args[1].value.sbool;
		break;
	case TYPE_INT:
		eq = args[0].value.sint == args[1].value.sint;
		break;
	case TYPE_FLT:
		eq = args[0].value.flt == args[1].value.flt;
		break;
	default:
		goto fail;
	}
	switch (name[0]) {
	case '=':
		res.type = TYPE_BOOL;
		res.value.sbool = eq;
		break;
	case '!':
		res.type = TYPE_BOOL;
		res.value.sbool = !eq;
		break;
	default:
		assert(0);
	}
	return res;
fail:
	return (lisp_result_t) {.type = TYPE_ILL};
}

static lisp_result_t env_vieee754_special(const char *name)
{
	lisp_result_t res;
	res.type = TYPE_FLT;
	res.value.flt =
		name[3] == '+' ? +1.0/0.0 :
		name[3] == '-' ? -1.0/0.0 :
		0.0/0.0;
	return res;
}

static lisp_result_t env_vreadptr_native(const char *name)
{
}

const memf_lisp_fenv_t memf_lisp_fenv[] = {
	{"+", env_arith}, {"-", env_arith}, {"*", env_arith}, {"/", env_arith},
	{"=",  env_test}, {"!=", env_test},
	{0},
};

const memf_lisp_venv_t memf_lisp_venv[] = {
	{"nan",  env_vieee754_special},
	{"inf+", env_vieee754_special},
	{"inf-", env_vieee754_special},
	{"i8",   env_vreadptr_native},
	{"i16",  env_vreadptr_native},
	{"i32",  env_vreadptr_native},
	{"i64",  env_vreadptr_native},
	{"f32",  env_vreadptr_native},
	{"f64",  env_vreadptr_native},
};
