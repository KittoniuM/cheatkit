#include "lisp.h"

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	const char	*src;
	size_t		 cur;
} lexer_t;

static char cur(const lexer_t *lex)
{
	return lex->src[lex->cur];
}

static void step(lexer_t *lex)
{
	lex->cur++;
}

static void token(program_t *prog, const token_t *tok)
{
	prog->tokens = realloc(prog->tokens,
		sizeof(*prog->tokens) * (prog->num_tokens + 1));
	prog->tokens[prog->num_tokens] = *tok;
	prog->num_tokens++;
}

static void token_free(token_t *tok)
{
	if (tok->type == TOK_SYM || tok->type == TOK_STR)
		free(tok->value.str);
}

static void tok_scope(lexer_t *lex, token_t *tok)
{
	switch (cur(lex)) {
	case '(':
		tok->type = TOK_BEG;
		step(lex);
		break;
	case ')':
		tok->type = TOK_END;
		step(lex);
		break;
	default:
		assert(0);
	}
}

static void tok_str(lexer_t *lex, token_t *tok)
{
	size_t	 len;
	char	*str;
	char	 c;

	assert(cur(lex) == '"');
	step(lex);

	len = 0;
	str = malloc(1);
	while (1) {
		str = realloc(str, len + 1);
		switch (c = cur(lex)) {
		case '\0':
		case '"':
			/*
			 * Once we hit end, skip over the quotemark,
			 * put null term and leave.
			 */
			str[len] = '\0';
			step(lex);
			tok->type = TOK_STR;
			tok->value.str = str;
			return;
		default:
			str[len++] = c;
			step(lex);
			break;
		}
	}
	assert(0);
}

static void tok_sym(lexer_t *lex, token_t *tok)
{
	size_t	 len;
	char	*str;
	char	 c;

	len = 0;
	str = malloc(1);
	while (1) {
		/* allocate amount of chars needed + null terminator */
		str = realloc(str, len + 1);
		switch (c = cur(lex)) {
		case '\0':
		case ' ': case '\t': case '\n': case '\r':
		case '(': case ')':
			/* once we hit end, put null term and leave */
			str[len] = '\0';
			tok->type = TOK_SYM;
			tok->value.str = str;
			return;
		default:
			str[len++] = c;
			step(lex);
			break;
		}
	}
	assert(0);
}

static void tok_num(lexer_t *lex, token_t *tok)
{
	size_t	 len;
	char	*str;
	char	 c;
	int	 base, sign;

	len = 0;
	str = malloc(1);
	base = 10;
	sign = 1;
	while (1) {
		str = realloc(str, len + 1);
		switch (c = cur(lex)) {
		case '\0':
		case ' ': case '\t': case '\n': case '\r':
		case '(': case ')':
			str[len] = '\0';
			tok->type = TOK_SYM;
			tok->value.str = str;
			return;
		case '-':
			sign = -1;
			step(lex);
			break;
		default:
			str[len++] = c;
			step(lex);
			break;
		}
	}
	assert(0);
}

void lisp_ldprog(program_t *prog, const char *src)
{
	lexer_t lex;
	token_t tok;

	assert(prog != NULL);

	prog->tokens = NULL;
	prog->num_tokens = 0;

	lex.src = src;
	lex.cur = 0;

	while (1) {
		switch (cur(&lex)) {
		case '\0':
			return;
		case '(': case ')':
			tok_scope(&lex, &tok);
			token(prog, &tok);
			break;
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9': case '-': case '+':
			/*
			 * This should work well with numbers written as 13,
			 * 0x0, 0xdEadBeEf, -0x01.1, +0666.6 etc.
			 */
			tok_num(&lex, &tok);
			token(prog, &tok);
			break;
		case '"':
			tok_str(&lex, &tok);
			token(prog, &tok);
			break;
		case ' ': case '\t': case '\n': case '\r':
			step(&lex);
			break;
		default:
			tok_sym(&lex, &tok);
			token(prog, &tok);
			break;
		}
	}
}

void lisp_free(program_t *prog)
{
	for (size_t i = 0; i < prog->num_tokens; i++)
		token_free(&prog->tokens[i]);
	free(prog->tokens);
}
