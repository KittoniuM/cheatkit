#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>

#include "memf.h"

static const char usage[] =
	"Usage: memf [options] <filter file>\n"
	"Options:\n"
	"  -h, --help                  Show this message.\n"
	"  -p, --pid PID               Process ID to work at, in base 10.\n"
	"  -r, --range RANGE           Range to scan on.  Defined as from..to .  Default\n"
	"                              is 0..7fffffffffffffff .  Input must be in base\n"
	"                              16 and with NO 0x prefix.\n"
	;

int main(int argc, char **argv)
{
	const struct option long_options[] = {
		{"help",  no_argument,       NULL, 'h'},
		{"pid",   required_argument, NULL, 'p'},
		{"range", required_argument, NULL, 'r'},
		/* add -s that converts everything to human-readable table */
		{0}
	};
	struct memf_args args = {
		.pid  = 0,
		.from = 0,
		.to   = (unsigned long) -1,
	};
	enum memf_status rc;
	int c, option_index;

	while ((c = getopt_long(argc, argv, "hp:r:f:",
				long_options, &option_index)) != -1) {
		switch (c) {
		case 'h':
			printf(usage);
			return EXIT_SUCCESS;
		case 'p':
			sscanf(optarg, "%lu", &args.pid);
			break;
		case 'r':
			sscanf(optarg, "%lx-%lx", &args.from, &args.to);
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
	return EXIT_SUCCESS;
fail:
	return EXIT_FAILURE;
}
