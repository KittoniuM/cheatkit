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
	TOK_ILL,
};

enum synv_status {
	SYNV_OK,
	SYNV_BRACKET,
	SYNV_ILLTOKEN,
};

typedef struct {
	size_t		col;
	enum token_type type;
	union token_value {
		int64_t	 sint;
		double	 flt;
		char	*str;
	} value;
} lisp_token_t;

typedef struct {
	lisp_token_t	*tokens;
	size_t		 num_tokens;
} lisp_program_t;

void			lisp_ldprog(lisp_program_t *prog, const char *src);
enum synv_status	lisp_synv(lisp_program_t *prog, size_t *col);
void			lisp_free(lisp_program_t *prog);

#endif /* __LISP_H__ */
