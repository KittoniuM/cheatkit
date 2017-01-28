#ifndef __LISP_H__
#define __LISP_H__

#include <stddef.h>
#include <stdint.h>

enum token_type {
	TOK_BEG,
	TOK_END,
	TOK_SYM,
	TOK_STR,
	TOK_INT,
	TOK_FLT,
	TOK_BAD_NAN,
};

typedef struct {
	enum token_type type;
	union token_value {
		int64_t		 sint;
		uint64_t	 uint;
		double		 flt;
		char		*str;
	} value;
} token_t;

typedef struct {
	token_t	*tokens;
	size_t	 num_tokens;
} program_t;

void lisp_ldprog(program_t *prog, const char *src);
void lisp_free(program_t *prog);

#endif /* __LISP_H__ */
