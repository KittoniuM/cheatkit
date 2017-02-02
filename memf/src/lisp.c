#include "lisp.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_CHARS 0x100

typedef struct {
	const char	*src;
	size_t		 cur;
} lexer_t;

typedef struct {
	char	str[MAX_CHARS];
	int	sign;
	int	base;
	bool	ill;
	bool	flt;
} lexer_num_t;

static char cur(const lexer_t *lex)
{
	return lex->src[lex->cur];
}

static char peek(const lexer_t *lex)
{
	return lex->src[lex->cur+1];
}

static void step(lexer_t *lex)
{
	lex->cur++;
}

static void token(lisp_program_t *prog, const lisp_token_t *tok)
{
	prog->tokens = realloc(prog->tokens,
			       sizeof(*prog->tokens) * (prog->num_tokens + 1));
	prog->tokens[prog->num_tokens] = *tok;
	prog->num_tokens++;
}

static void token_free(lisp_token_t *tok)
{
	if (tok->type == TOK_SYM || tok->type == TOK_STR)
		free(tok->value.str);
}

static void tok_scope(lexer_t *lex, lisp_token_t *tok)
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

static void tok_str(lexer_t *lex, lisp_token_t *tok)
{
	size_t	 pos;
	char	*str;
	char	 c;

	assert(cur(lex) == '"');
	step(lex);

	pos = 0;
	str = malloc(MAX_CHARS);
	while (1) {
		switch (c = cur(lex)) {
		case '\0':
		case '"':
			/*
			 * Once we hit end, skip over the quotemark,
			 * put null term and leave.
			 */
			str[pos] = '\0';
			step(lex);
			tok->type = TOK_STR;
			tok->value.str = str;
			return;
		default:
			str[pos++] = c;
			step(lex);
			break;
		}
	}
	assert(0);
}

static void tok_sym(lexer_t *lex, lisp_token_t *tok)
{
	size_t	 pos;
	char	*str;
	char	 c;

	pos = 0;
	str = malloc(MAX_CHARS);
	while (1) {
		switch (c = cur(lex)) {
		case '\0':
		case ' ': case '\t': case '\n': case '\r':
		case '(': case ')':
			/* once we hit end, put null term and leave */
			str[pos] = '\0';
			tok->type = TOK_SYM;
			tok->value.str = str;
			return;
		default:
			str[pos++] = c;
			step(lex);
			break;
		}
	}
	assert(0);
}

static int tok_num_sign(lexer_t *lex)
{
	switch (cur(lex)) {
	case '-':
		step(lex);
		return -1;
	case '+':
		step(lex);
		/* fallthrough */
	default:
		return 1;
	}
}

static int tok_num_base(lexer_t *lex)
{
	switch (cur(lex)) {
	case '0':
		switch (peek(lex)) {
		case 'b': case 'B':
			step(lex);
			step(lex);
			return 2;
		case 'x': case 'X':
			step(lex);
			step(lex);
			return 16;
		case '.':
			/* a decimal fraction */
			return 10;
		default:
			return 8;
		}
	default:
		return 10;
	}
}

static bool tok_num_inrange(int base, char c)
{
	switch (base) {
	case 2:
		return c >= '0' && c <= '1';
	case 8:
		return c >= '0' && c <= '7';
	case 10:
		return c >= '0' && c <= '9';
	case 16:
		return ((c >= '0' && c <= '9') ||
			(c >= 'a' && c <= 'f') ||
			(c >= 'A' && c <= 'F'));
	default:
		assert(0);
	}
}

static void tok_fromnum(const lexer_num_t *num, lisp_token_t *tok)
{
	if (num->ill) {
		tok->type = TOK_ILL;
	} else {
		if (num->flt) {
			tok->type = TOK_FLT;
			tok->value.flt = (double) num->sign
				* strtod(num->str, NULL);
		} else {
			tok->type = TOK_INT;
			tok->value.sint = num->sign
				* (int64_t) strtoll(num->str, NULL, num->base);
		}
	}
}

static void tok_num(lexer_t *lex, lisp_token_t *tok)
{
	size_t		pos = 0;
	char		c;
	lexer_num_t	num = {0};

	num.sign = tok_num_sign(lex);
	num.base = tok_num_base(lex);
	while (1) {
		switch (c = cur(lex)) {
		case '\0':
		case ' ': case '\t': case '\n': case '\r':
		case '(': case ')':
			assert(pos > 0);
			num.str[pos] = '\0';
			tok_fromnum(&num, tok);
			return;
		case '.':
			/* numbers can only have one dot in them */
			if (num.flt)
				num.ill = true;
			else
				num.flt = true;
			/* no support for non base 10 floats */
			if (num.base != 10)
				num.ill = true;
			num.str[pos++] = c;
			step(lex);
			break;
		default:
			/* make sure character is in range */
			if (!tok_num_inrange(num.base, c))
				num.ill = true;
			num.str[pos++] = c;
			step(lex);
			break;
		}
	}
	assert(0);
}

void lisp_ldprog(lisp_program_t *prog, const char *src)
{
	lexer_t		lex;
	lisp_token_t	tok;

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
		case '-': case '+':
			if (peek(&lex) < '0' || peek(&lex) > '9')
				goto common_symbol;
			/* fallthrough */
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9':
			/*
			 * This should work well with numbers written as 13.3,
			 * 0x0, 0xdEadBeEf, -0x01, +0666 etc.
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
		default: common_symbol:
			tok_sym(&lex, &tok);
			token(prog, &tok);
			break;
		}
	}
	assert(0);
}

enum synv_status lisp_synv(lisp_program_t *prog)
{
	lisp_token_t	*tok;
	int		 bal;

	if (prog->num_tokens == 0)
		return SYNV_EMPTY;
	bal = 0;
	for (size_t i = 0; i < prog->num_tokens; i++) {
		switch ((tok = &prog->tokens[i])->type) {
		case TOK_BEG:
			bal++;
			break;
		case TOK_END:
			bal--;
			/*
			 * At this point, there are more opening brackets
			 * than closing one which means bad source.
			 */
			if (bal < 0)
				return SYNV_SCOPE;
			break;
		case TOK_ILL:
			return SYNV_ILLTOK;
		default:
			break;
		}
	}
	if (bal != 0)
		return SYNV_SCOPE;
	return SYNV_OK;
}

void lisp_free(lisp_program_t *prog)
{
	for (size_t i = 0; i < prog->num_tokens; i++)
		token_free(&prog->tokens[i]);
	free(prog->tokens);
}
