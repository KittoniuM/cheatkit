#include "lisp.h"

#include <assert.h>

typedef struct {
	const char	*src;
	size_t		 src_len;
	size_t		 cur;
} lexer_t;

static char peek(lexst_t *lex)
{
	return lex.src[lex.cur + 1];
}

static void step(lexst_t *lex)
{
	lex.cur++;
}

static void token(program_t *prog, const token_t *tok)
{
	prog->tokens = realloc(prog->tokens,
		sizeof(*prog->tokens) * (prog->num_tokens + 1));
	prog->tokens[prog->num_tokens] = tok;
	prog->num_tokens++;
}

void lisp_ldprog(program_t *prog, const char *src, size_t src_len)
{
	lexer_t lex;
	token_t tok;

	assert(prog != NULL);

	prog->tokens = NULL;
	prog->num_tokens = 0;

	lex.src = src;
	lex.src_len = src_len;
	lex.cur = 0;

	while (lex.cur < lex.src_len) {
		switch (peek(&lex)) {
		case '(':
			tok.type = TOK_BEG;
			token(prog, &tok);
			step(&lex);
			break;
		case ')':
			tok.type = TOK_END;
			token(prog, &tok);
			step(&lex);
			break;
		}
	}
}
