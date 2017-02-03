#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include "memf.h"

static const char usage[] =
	"Usage: memf [options] <filter file>\n"
	"Options:\n"
	"  -h, --help                  Show this message.\n"
	"  -p, --pid PID               Process ID to work at, in base 10.\n"
	"  -r, --range RANGE           Range to scan on.  Defined as from..to .  Default\n"
	"                              is 0-7fffffffffffffff .  Input must be in base\n"
	"                              16 and with NO 0x prefix.\n";

static enum memf_type to_type(const char *str)
{
	struct typealias { const char *a; enum memf_type t; } aliases[] = {
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
		{"><", FUNC_IN},
		{"<>", FUNC_EX},
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
	char	type_str[32], func_str[32], from[32], to[32];
	int	c;

	if ((c = sscanf(str, "%32[^,],%32[^,],%32[^-]-%32[^-]",
			type_str, func_str, from, to)) >= 3) {
		if (type_str[0] == '^') {
			args->noalign = true;
			args->type = to_type(&type_str[1]);
		} else {
			args->noalign = false;
			args->type = to_type(type_str);
		}
		args->func = to_func(func_str);
		if (args->type == TYPE_ILL || args->func == FUNC_ILL) {
			args->type = TYPE_ILL;
			args->func = FUNC_ILL;
		} else {
			if (c >= 4) {
				args->ranged = true;
				args->vfrom = to_value(args->type, from);
				args->vto = to_value(args->type, to);
			} else {
				args->ranged = false;
				args->value = to_value(args->type, from);
			}
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
		{"verbose", required_argument, NULL, 'v'},
		{"pid",     required_argument, NULL, 'p'},
		{"range",   required_argument, NULL, 'r'},
		{"func",    required_argument, NULL, 'f'},
		/* add -s that converts everything to human-readable table */
		{0}
	};
	struct memf_args args = {
		.verbose = false,
		.pid	 = 0,
		.from	 = 0,
		.to	 = 0x7fffffffffffffff,
		.mask	 = "rw-p",
		.noalign = false,
		.ranged	 = false,
		.type	 = TYPE_ILL,
		.func	 = FUNC_ILL,
		.value	 = {0},
		.vfrom	 = {0},
		.vto	 = {0},
	};
	enum memf_status rc;
	int c, option_index;

	while ((c = getopt_long(argc, argv, "hvp:r:f:",
				long_options, &option_index)) != -1) {
		switch (c) {
		case 'h':
			printf(usage);
			return EXIT_SUCCESS;
		case 'v':
			args.verbose = true;
			break;
		case 'p':
			sscanf(optarg, "%lu", &args.pid);
			break;
		case 'r':
			sscanf(optarg, "%llx-%llx", &args.from, &args.to);
			break;
		case 'f':
			from_f(&args, optarg);
			break;
		case '?':
			return EXIT_FAILURE;
		default:
			assert(0);
		}
	}

	if (args.pid == 0) {
		fprintf(stderr, "memf: error: pid is not set\n");
		return EXIT_FAILURE;
	}
	if (args.from >= args.to) {
		fprintf(stderr, "memf: error: bad range\n");
		return EXIT_FAILURE;
	}
	if (args.type == TYPE_ILL || args.func == FUNC_ILL) {
		fprintf(stderr, "memf: error: illegal type, function or value\n");
		return EXIT_FAILURE;
	}

	switch (rc = memf(&args)) {
	case MEMF_OK:
		break;
	default:
		assert(0);
	}
	return EXIT_SUCCESS;
}
