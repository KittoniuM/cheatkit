#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include "memf.h"

static const char usage[] =
	"Usage: memf [options] <intermediate blob>\n"
	"Options:\n"
	"  -h, --help                  Show this message.\n"
	"  -V, --verbose               Talk more.\n"
	"  -p, --pid PID               Process ID to work at, in base 10.\n"
	"  -r, --range FROM..TO        Range to consider pages from.  Must be in\n"
	"                              base 16 with NO 0x prefix.  Default is\n"
	"                              0..7fffffffffffffff .\n"
	"  -m, --mask ....             Flags mask for pages.  Default is r?-p ,\n"
	"                              which will match any readable, non-executable\n"
	"                              private page.\n"
	"  -f, --func EXPR             Defines what to look for.  Must be written as\n"
	"                              FUNC or FUNC,TYPE,VALUE .\n"
	"                              Valid FUNCs are: = != < > <= >= .\n"
	"                              Valid TYPEs are: i8, i16, i32, i64, f32, f64\n"
	"                              VALUE may be written in base 10 or base 16.\n"
	"\n"
	"If intermediate blob is provided, --range and --mask will have no effect,\n"
	"and --func is only required to provide FUNC.  TYPE and VALUE are provided\n"
	"by the intermediate blob itself and are ignored in --func.\n";



static enum memf_type to_type(const char *str)
{
	struct typealias { const char *a; enum memf_type t; } aliases[] = {
		{"ill", TYPE_ILL},
		{"i8",  TYPE_I8},
		{"i16", TYPE_I16},
		{"i32", TYPE_I32},
		{"i64", TYPE_I64},
		{"f32", TYPE_F32},
		{"f64", TYPE_F64},
	};

	for (size_t i = 0; i < sizeof(aliases) / sizeof(*aliases); i++) {
		if (strcmp(aliases[i].a, str) == 0)
			return aliases[i].t;
	}
	return TYPE_ILL;
}

static enum memf_func to_func(const char *str)
{
	struct funcalias { const char *a; enum memf_func f; } aliases[] = {
		{"=",  FUNC_EQ},
		{"!=", FUNC_NE},
		{"<",  FUNC_LT},
		{">",  FUNC_GT},
		{"<=", FUNC_LE},
		{">=", FUNC_GE},
	};
	for (size_t i = 0; i < sizeof(aliases) / sizeof(*aliases); i++) {
		if (strcmp(aliases[i].a, str) == 0)
			return aliases[i].f;
	}
	return FUNC_ILL;
}

static union memf_value to_value(enum memf_type type, const char *str)
{
	switch (type) {
	case TYPE_I8:
	case TYPE_I16:
	case TYPE_I32:
	case TYPE_I64:
		return (union memf_value) {
			.i = str[0] == '0' && (str[1] == 'x' || str[1] == 'X')
				? strtoll(&str[2], NULL, 16)
				: strtoll(str,     NULL, 10)};
	case TYPE_F32:
	case TYPE_F64:
		return (union memf_value) {.f = strtod(str, NULL)};
	default:
		assert(0);
	}
}

static void from_f(struct memf_args *args, const char *str)
{
	char	type_str[32], func_str[32], value_str[32];
	int	c;

	if ((c = sscanf(str, "%32[^,],%32[^,],%32[^-]",
			func_str, type_str, value_str)) >= 1) {
		args->func = to_func(func_str);
		if (c == 1)
			return;
		if (type_str[0] == '^') {
			args->noalign = true;
			args->type = to_type(&type_str[1]);
		} else {
			args->noalign = false;
			args->type = to_type(type_str);
		}
		if (args->type == TYPE_ILL || args->func == FUNC_ILL) {
			args->type = TYPE_ILL;
			args->func = FUNC_ILL;
		} else {
			args->value = to_value(args->type, value_str);
		}
	} else {
		args->type = TYPE_ILL;
		args->func = FUNC_ILL;
	}
}

int main(int argc, char **argv)
{
	const struct option long_options[] = {
		{"help",    no_argument,       NULL, 'h'},
		{"verbose", no_argument,       NULL, 'V'},
		{"pid",     required_argument, NULL, 'p'},
		{"range",   required_argument, NULL, 'r'},
		{"mask",    required_argument, NULL, 'm'},
		{"func",    required_argument, NULL, 'f'},
		{0}
	};
	struct memf_args args = {
		.verbose    = false,
		.pid	    = 0,
		.from	    = 0,
		.to	    = 0x7fffffffffffffff,
		.mask	    = "r?-p",
		.noalign    = false,
		.type	    = TYPE_ILL,
		.func	    = FUNC_ILL,
		.value	    = {0},
		.num_stores = 0,
		.stores	    = NULL,
	};
	FILE *in, *out;
	size_t num_stores;
	struct memf_store *stores;
	int c, option_index;

	while ((c = getopt_long(argc, argv, "hVp:r:m:f:",
				long_options, &option_index)) != -1) {
		switch (c) {
		case 'h':
			printf(usage);
			return EXIT_SUCCESS;
		case 'V':
			args.verbose = true;
			break;
		case 'p':
			sscanf(optarg, "%lu", &args.pid);
			break;
		case 'r':
			sscanf(optarg, "%llx-%llx", &args.from, &args.to);
			break;
		case 'm':
			sscanf(optarg, "%4s", args.mask);
			break;
		case 'f':
			from_f(&args, optarg);
			break;
		case '?':
			goto fail;
		default:
			assert(0);
		}
	}

	if (args.pid == 0) {
		fprintf(stderr, "memf: error: pid is not set\n");
		goto fail;
	}
	if (optind != (argc - 1)) {
		fprintf(stderr, "memf: error: where to output?\n");
		goto fail;
	}
	if ((in = fopen(argv[argc-1], "rb")) != NULL) {
		fread(&args.type,       sizeof(args.type),       1, in);
		fread(&args.num_stores, sizeof(args.num_stores), 1, in);
		if (args.num_stores == 0) {
			fprintf(stderr, "memf: error: corrupted blob\n");
			fclose(in);
			goto fail;
		}
		if ((args.stores = malloc(args.num_stores
					  * sizeof(*args.stores))) == NULL) {
			fprintf(stderr, "memf: error: no memory to load blob\n");
			fclose(in);
			goto fail;
		}
		if (fread(args.stores, sizeof(*args.stores),
			  args.num_stores, in) != args.num_stores) {
			if (args.verbose)
				printf("memf: blob could be corrupt\n");
		}
		fclose(in);
	}
	if (args.func == FUNC_ILL) {
		fprintf(stderr, "memf: error: illegal function\n");
		goto fail;
	}
	if (args.type == TYPE_ILL) {
		fprintf(stderr, "memf: error: illegal type\n");
		goto fail;
	}

	switch (memf(&args, &stores, &num_stores)) {
	case MEMF_OK:
		break;
	case MEMF_ERR_PROC:
		fprintf(stderr, "memf: error: pid does not exist or permission denied\n");
		goto fail;
	case MEMF_ERR_IO:
	case MEMF_ERR_OOM:
	default:
		/* these should never happen */
		assert(0);
	}

	if ((out = fopen(argv[argc-1], "wb")) != NULL) {
		fwrite(&args.type, sizeof(args.type), 1, out);
		fwrite(&num_stores, sizeof(num_stores), 1, out);
		fwrite(stores, sizeof(*stores), num_stores, out);
		fclose(out);
	} else {
		fprintf(stderr, "memf: error: failed to write %s\n", argv[argc-1]);
		goto fail_post_memf;
	}
fail_post_memf:
	if (num_stores > 0)
		free(stores);
fail:
	if (args.num_stores > 0)
		free(args.stores);
	return 0;
}
