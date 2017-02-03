#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>

#include "memf.h"
#include "lisp.h"

static const char usage[] =
	"Usage: memf [options] <filter file>\n"
	"Options:\n"
	"  -h, --help                  Show this message.\n"
	"  -p, --pid PID               Process ID to work at, in base 10.\n"
	"  -r, --range RANGE           Range to scan on.  Defined as from..to .  Default\n"
	"                              is 0..7fffffffffffffff .  Input must be in base\n"
	"                              16 and with NO 0x prefix.\n"
	"  -f, --func                  Test function.  Must be written in a LISP-like\n"
	"                              language.  Symbols are presented in a table\n"
	"                              below.\n"
	"LISP symbols:\n"
	"  nan,inf+,inf-               IEEE 754 special floating point non-numeric\n"
	"                              values.\n"
	"  (+,*,-,/ a b ...)           Arithmetics.\n"
	"\n"
	/*
	 * "  (trunc a)                   Strip fraction part.\n"
	 * "  (floor a)                   Round down.\n"
	 * "  (round a)                   Round to nearest.\n"
	 * "  (ceil a)                    Round up.\n"
	 * "NOTE: These will fail with non-integers:\n"
	 * "  (~ a)                       Bitwise not.\n"
	 * "  (&,|,^ a b)                 Bitwise and, or and xor of two integers.\n"
	 * "\n"
	 */
	"  (=,!= a b)                  Test two numbers for equality.\n"
	/*
	 * "  (f=,f!= a b)                Test at least two numbers for equality, with\n"
	 * "                              tolerance for inaccuracies in floating point\n"
	 * "                              numbers.\n"
	 * "  (<,>,<=,>= a b)             Compare two numbers.\n"
	 * "  (! a)                       Logical not.\n"
	 * "  (&&,|| a b)                 Logical and and or.\n"
	 * "\n"
	 * "  (sig signature [mask])      Check if address points to matching region\n"
	 * "\n"
	 */
	/* "  previous                    Value stored from previous scan.\n" */
	/* "  [i,u][16,32,64][le,be]      Word from current address, endian-independent\n" */
	"  i[8,16,32,64]               Word from current address, in native platform\n"
	"                              endianness.\n"
	/*
	 * "  f[32,64][le,be, ]           Single or double precision floating point\n"
	 * "                              number from current address.\n"
	 */
	;

int main(int argc, char **argv)
{
	const struct option long_options[] = {
		{"help",  no_argument,       NULL, 'h'},
		{"pid",   required_argument, NULL, 'p'},
		{"range", required_argument, NULL, 'r'},
		{"func",  required_argument, NULL, 'f'},
		/* add -s that converts everything to human-readable table */
		{0}
	};
	struct memf_args args = {
		.pid  = 0,
		.from = 0,
		.to   = (unsigned long) -1,
		.prog = {0}
	};
	enum memf_status rc;
	int c, option_index;

	while ((c = getopt_long(argc, argv, "hp:r:f:",
				long_options, &option_index)) != -1) {
		switch (c) {
		case 'h':
			printf(usage);
			if (lisp_synv(&args.prog) != SYNV_EMPTY)
				lisp_free(&args.prog);
			return EXIT_SUCCESS;
		case 'p':
			sscanf(optarg, "%lu", &args.pid);
			break;
		case 'r':
			sscanf(optarg, "%lx-%lx", &args.from, &args.to);
			break;
		case 'f':
			if (lisp_synv(&args.prog) != SYNV_EMPTY)
				lisp_free(&args.prog);
			lisp_ldprog(&args.prog, optarg);
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
	if (args.from >= args.to) {
		fprintf(stderr, "memf: error: bad range\n");
		goto fail;
	}

	switch (lisp_synv(&args.prog)) {
	case SYNV_OK:
		break;
	case SYNV_EMPTY:
		fprintf(stderr, "memf: error: program is empty\n");
		goto fail;
	case SYNV_SCOPE:
		fprintf(stderr, "memf: error: missed a bracket\n");
		goto fail;
	case SYNV_ILLTOK:
		fprintf(stderr, "memf: error: illegal token\n");
		goto fail;
	default:
		assert(0);
	}

	switch (rc = memf(&args)) {
	case MEMF_OK:
		break;
	case MEMF_ERR_PROC:
		fprintf(stderr, "memf: error: procfs error\n");
		goto fail;
	case MEMF_ERR_IO:
		fprintf(stderr, "memf: error: nigger error\n");
		goto fail;
	default:
		assert(0);
	}
	lisp_free(&args.prog);
	return EXIT_SUCCESS;
fail:
	if (lisp_synv(&args.prog) != SYNV_EMPTY)
		lisp_free(&args.prog);
	return EXIT_FAILURE;
}
