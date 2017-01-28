#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include <sys/mman.h>

#include "memf.h"
#include "lisp.h"

typedef struct {
	char		*text;
	enum memf_func	 func;
} func_alias_t;

typedef struct {
	char		*text;
	enum memf_type	 type;
} type_alias_t;

typedef struct {
	enum memf_type	type;
	uint64_t	align;
} type_align_t;

static const char usage[] =
	"Usage: memf [options] <filter file>\n"
	"Options:\n"
	"  -h, --help                  Show this message.\n"
	"  -p, --pid PID               Process ID to work at.\n"
	"  -t, --type TYPE             The type values should be treated as during\n"
	"                              memf.  Possible types are: i8, i16, i32, i64,\n"
	"                              u8, u16, u32, u64, f32 and f64.\n"
	"  -a, --align ALIGNMENT       Follow alignment when memfning.  Overrides\n"
	"                              alignment set by --type.\n"
	"  -r, --range RANGE           Memf range.  Defined as from..to.  Ranges have\n"
	"                              two aliases:\n"
	"                                32 for 0..ffffffff;\n"
	"                                64 for 0..7fffffffffffffff.  Default is 64.\n"
	"  -f, --func FUNCTION         Comparison function for filtering addresses.\n"
	"                              Such functions are available:\n"
	"                                eq(=), ne(!=), lt(<), le(<=), gt(>), ge(>=)\n"
	"                              When the value is not specified, values from\n"
	"                              filter file are used.\n"
	"  -v, --val VALUE             User defined value for comparison function to\n"
	"                              compare against.  If no value is set, filter\n"
	"                              file must be present.\n";

static enum memf_func to_func(const char *text)
{
	static const func_alias_t aliases[] = {
		{"eq", FUNC_EQ},
		{"ne", FUNC_NE},
		{"lt", FUNC_LT},
		{"le", FUNC_LE},
		{"gt", FUNC_GT},
		{"ge", FUNC_GE},
		{NULL, FUNC_INVALID},
	};

	for (const func_alias_t *alias = aliases;
	     alias->text != NULL;
	     alias++) {
		if (strcmp(text, alias->text) == 0)
			return alias->func;
	}
	return FUNC_INVALID;
}

static enum memf_type to_type(const char *text)
{
	static const type_alias_t aliases[] = {
		{"i8",  TYPE_I8},
		{"i16", TYPE_I16},
		{"i32", TYPE_I32},
		{"i64", TYPE_I64},
		{"u8",  TYPE_U8},
		{"u16", TYPE_U16},
		{"u32", TYPE_U32},
		{"u64", TYPE_U64},
		{"f32", TYPE_F32},
		{"f64", TYPE_F64},
		{NULL,  TYPE_INVALID},
	};

	for (const type_alias_t *alias = aliases;
	     alias->text != NULL;
	     alias++) {
		if (strcmp(text, alias->text) == 0)
			return alias->type;
	}
	return TYPE_INVALID;
}

static uint64_t type_align(enum memf_type type)
{
	switch (type) {
	case TYPE_I8:
	case TYPE_U8:
		return 0;
	case TYPE_I16:
	case TYPE_U16:
		return 2;
	case TYPE_I32:
	case TYPE_U32:
	case TYPE_F32:
		return 4;
	case TYPE_I64:
	case TYPE_U64:
	case TYPE_F64:
		return 8;
	default:
		return 0;
	}
}

int main(int argc, char **argv)
{
	static struct option long_options[] = {
		{"help",  no_argument,       NULL, 'h'},
		{"pid",   required_argument, NULL, 'p'},
		{"type",  required_argument, NULL, 't'},
		{"align", required_argument, NULL, 'a'},
		{"range", required_argument, NULL, 'r'},
		{"func",  required_argument, NULL, 'f'},
		{"val",   required_argument, NULL, 'v'},
		{0}
	};
	struct memf_args args = {
		.pid   = 0,
		.type  = TYPE_INVALID,
		.align = 0,
		.from  = 0x0,
		.to    = 0x7fffffffffffffff,
		.func  = FUNC_INVALID,
		.val   = 0,
	};

	int c, option_index;
	while ((c = getopt_long(argc, argv, "hp:t:a:r:f:v:",
				long_options, &option_index)) != -1) {
		unsigned long long from, to;

		switch (c) {
		case 'h':
			printf(usage);
			return EXIT_SUCCESS;

		case 'p':
			args.pid = (uint64_t) strtoull(optarg, NULL, 0);
			break;

		case 't':
			args.type  = to_type(optarg);
			args.align = type_align(args.type);
			break;

		case 'a':
			args.align = (uint64_t) strtoull(optarg, NULL, 0);
			break;

		case 'r':
			sscanf(optarg, "%llx-%llx", &from, &to);
			args.from = (uint64_t) from;
			args.to   = (uint64_t) to;
			break;

		case 'f':
			args.func = to_func(optarg);
			break;

		case 'v':
			args.val = (uint64_t) strtoull(optarg, NULL, 0);
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

	program_t prog;
	lisp_ldprog(&prog, "(= (u32) 0xdeadbeef)");
	for (size_t i = 0; i < prog.num_tokens; i++) {
		token_t *tok = &prog.tokens[i];
		switch (tok->type) {
		case TOK_BEG:
			printf("(\n");
			break;
		case TOK_END:
			printf(")\n");
			break;
		case TOK_SYM:
			printf("%s\n", tok->value.str);
			break;
		default:
			printf("whatever.\n");
		}
	}
	lisp_free(&prog);

	/* enum memf_status rc = memf(&args); */
	return EXIT_SUCCESS;
}
