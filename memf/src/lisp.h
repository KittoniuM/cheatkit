#ifndef __LISP_H__
#define __LISP_H__

#include <stdbool.h>
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
	SYNV_EMPTY,
	SYNV_SCOPE,
	SYNV_ILLTOK,
};

enum lisp_value_type {
	TYPE_BOOL,
	TYPE_INT,
	TYPE_FLT,
	TYPE_STR,
	TYPE_ILL,
};

typedef struct {
	enum lisp_value_type type;
	union lisp_value {
		bool	 sbool;
		int64_t	 sint;
		double	 flt;
		char	*str;
	} value;
} lisp_result_t;

typedef struct {
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
enum synv_status	lisp_synv(lisp_program_t *prog);
void			lisp_free(lisp_program_t *prog);

#endif /* __LISP_H__ */
