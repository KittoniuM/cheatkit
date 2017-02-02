#include "memf_lisp.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "memf.h"
#include "lisp.h"
#include "memf_lisp_env.h"

/* invokes a function from env */
static lisp_result_t exec(const char *func,
			  size_t num_args, const lisp_result_t *args)
{
	for (const memf_lisp_env_t *e = memf_lisp_fenv; e->name != NULL; e++) {
		if (strcmp(e->name, func) == 0)
			return e->func(e->name, num_args, args);
	}
	return (lisp_result_t) {.type = TYPE_ILL};
}

/* resolves variable's value */
static lisp_result_t var(const char *var)
{
	lisp_result_t res;
	res.type = TYPE_INT;
	res.value.sint = 1337;
	return res;
}

/* interpreter itself */
static lisp_result_t eval(const lisp_program_t *prog, size_t *x)
{
	const size_t	 max_args = 8;
	size_t		 argn;
	lisp_result_t	 args[max_args];
	lisp_token_t	*tok, *func;
	/* opening bracket */
	if (prog->tokens[(*x)++].type != TOK_BEG)
		goto fail;
	/* function symbol */
	func = &prog->tokens[(*x)++];
	if (func->type != TOK_SYM)
		goto fail;
	/* collect arguments */
	argn = 0;
	while (*x < prog->num_tokens) {
		if (argn >= (max_args - 1))
			goto fail;
		switch ((tok = &prog->tokens[(*x)++])->type) {
		case TOK_BEG:
			args[argn++] = eval(prog, x);
			break;
		case TOK_SYM:
			args[argn++] = var(tok->value.str);
			break;
		case TOK_INT:
			args[argn].type = TYPE_INT;
			args[argn].value.sint = tok->value.sint;
			argn++;
			break;
		case TOK_FLT:
			args[argn].type = TYPE_FLT;
			args[argn].value.flt = tok->value.flt;
			argn++;
			break;
		case TOK_STR:
			args[argn].type = TYPE_STR;
			args[argn].value.str = tok->value.str;
			argn++;
			break;
		case TOK_END:
			/* invoke function on closing bracket */
			return exec(func->value.str, argn, args);
		default:
			goto fail;
		}
	}
	assert(0);
fail:
	return (lisp_result_t) {.type = TYPE_ILL};
}

lisp_result_t memf_lisp_eval(const lisp_program_t *prog)
{
	size_t x = 0;
	return eval(prog, &x);
}
