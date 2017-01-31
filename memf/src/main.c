#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include <sys/mman.h>

#include "memf.h"
#include "lisp.h"

static const char usage[] =
	"Usage: memf [options] <filter file>\n"
	"Options:\n"
	"  -h, --help                  Show this message.\n"
	"  -p, --pid PID               Process ID to work at.\n"
	"  -r, --range RANGE           Memf range.  Defined as from..to.  Ranges have\n"
	"                              two aliases:\n"
	"                                32 for 0..ffffffff;\n"
	"                                64 for 0..7fffffffffffffff.  Default is 64.\n"
	"  -c, --func                  Test function.  Must be written in a LISP-like\n"
	"                              language.  Symbols are presented in a table\n"
	"                              below.\n"
	"LISP symbols:\n"
	"  pi                          Pi constant.\n"
	"  nan,inf+,inf-               IEEE 754 special floating point non-numeric\n"
	"                              values.\n"
	"  (+,* ...)                   Sum, product.\n"
	"  (-,/ a b)                   Difference and division.\n"
	"\n"
	"  (trunc a)                   Strip fraction part.\n"
	"  (floor a)                   Round down.\n"
	"  (round a)                   Round to nearest.\n"
	"  (ceil a)                    Round up.\n"
	"NOTE: These will fail with non-integers:\n"
	"  (~ a)                       Bitwise not.\n"
	"  (&,|,^ a b)                 Bitwise and, or and xor of two integers.\n"
	"\n"
	"  (=,!= a ...)                Test at least two numbers for equality.\n"
	"  (f=,f!= a ...)              Test at least two numbers for equality, with\n"
	"                              floating inaccuracy point tolerance.\n"
	"  (<,>,<=,>= a b)             Compare two numbers.\n"
	"  (! a)                       Logical not.\n"
	"  (&&,|| a b)                 Logical and and or.\n"
	"\n"
	"  (sig signature [mask])      Check if address points to matching region\n"
	"\n"
	"  previous                    Value stored from previous scan.\n"
	"  i8, u8                      Single byte from current address\n"
	"  [i,u][16,32,64][le,be]      Word from current address, endian-independent\n"
	"  [i,u][16,32,64]             Word from current address, -le or -be depending\n"
	"                              on platform's endianness.\n"
	"  f[32,64][le,be, ]           Single or double precision floating point\n"
	"                              number from current address.\n"
	;

int main(int argc, char **argv)
{
	static struct option long_options[] = {
		{"help",  no_argument,       NULL, 'h'},
		{"pid",   required_argument, NULL, 'p'},
		{"range", required_argument, NULL, 'r'},
		{"func",  required_argument, NULL, 'c'},
		{0}
	};
	int c, option_index;
	while ((c = getopt_long(argc, argv, "hp:r:c:",
				long_options, &option_index)) != -1) {
		switch (c) {
		case 'h':
			printf(usage);
			return EXIT_SUCCESS;

		case 'p':
			break;

		case 'r':
			/* sscanf(optarg, "%llx-%llx", &from, &to); */
			break;

		case 'c':
			break;
		}
	}

	/*
	 * if (args.pid == 0) {
	 * 	fprintf(stderr, "memf: error: pid is not set\n");
	 * 	return EXIT_FAILURE;
	 * } else if (args.type == TYPE_INVALID) {
	 * 	fprintf(stderr, "memf: error: bad type\n");
	 * 	return EXIT_FAILURE;
	 * } else if (args.func == FUNC_INVALID) {
	 * 	fprintf(stderr, "memf: error: bad function\n");
	 * 	return EXIT_FAILURE;
	 * } else if (args.to <= args.from) {
	 * 	fprintf(stderr, "memf: error: bad range\n");
	 * 	return EXIT_FAILURE;
	 * }
	 */

	lisp_program_t prog;
	lisp_ldprog(&prog, "(= u32 ()");
	size_t col;
	switch (lisp_synv(&prog, &col)) {
	case SYNV_ILLTOKEN:
		printf("memf: error: bad token at %d\n", col);
		break;
	case SYNV_BRACKET:
		printf("memf: error: unmatched bracket at %d\n", col);
		break;
	}
	for (size_t i = 0; i < prog.num_tokens; i++) {
		lisp_token_t *tok = &prog.tokens[i];
		switch (tok->type) {
		case TOK_BEG:
			printf("(\n");
			break;
		case TOK_END:
			printf(")\n");
			break;
		case TOK_SYM:
			printf("sym %s\n", tok->value.str);
			break;
		case TOK_FLT:
			printf("flt %f\n", tok->value.flt);
			break;
		case TOK_INT:
			printf("int %d\n", (int) tok->value.sint);
			break;
		case TOK_STR:
			printf("str %s\n", tok->value.str);
			break;
		default:
			printf("??\n");
			break;
		}
	}
	lisp_free(&prog);

	/* enum memf_status rc = memf(&args); */
	return EXIT_SUCCESS;
}
