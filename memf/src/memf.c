#define _GNU_SOURCE

#include "memf.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "lisp.h"
#include "memf_lisp.h"
/* #include "memf_proc.h" */

struct map_e {
	unsigned long	from;
	unsigned long	to;
	char		perms[4];
	unsigned long	off;
	int		devmaj;
	int		devmin;
	unsigned long	ino;
	bool		has_file;
	char		file[256];
};

static enum memf_status memf_maps(const struct memf_args *args,
				  struct map_e **out_maps, size_t *out_maps_count)
{
	char		 smaps[32];
	FILE		*mapsfd;
	size_t		 line_len = 0;
	char		*line = NULL;
	size_t		 cur;
	struct map_e	*maps;

	/* snprintf should not run out of buffer space */
	assert(snprintf(smaps, sizeof(smaps), "/proc/%lu/maps", args->pid)
	       < (int) sizeof(smaps));
	if ((mapsfd = fopen(smaps, "r")) == NULL)
		return MEMF_ERR_PROC;
	cur = 0;
	maps = malloc(1 * sizeof(*maps));
	while (getline(&line, &line_len, mapsfd) != -1) {
		struct map_e *map;
		maps = realloc(maps, (cur + 1) * sizeof(*maps));
		map  = &maps[cur];
		assert(sscanf(line, "%lx-%lx %4s %lx %x:%x %lu",
			      &map->from, &map->to, map->perms, &map->off,
			      &map->devmaj, &map->devmin, &map->ino)
		       == 7);
		/* an optional file field */
		map->has_file = sscanf(line, "%256s", map->file) == 1;
		cur++;
	}
	free(line);
	fclose(mapsfd);
	*out_maps_count = cur + 1;
	*out_maps = maps;
	return MEMF_OK;
}

static enum memf_status memf_look(const struct memf_args *args)
{
	assert(0);
	return MEMF_OK;
}

enum memf_status memf(const struct memf_args *args)
{
	size_t			 maps_cnt;
	struct map_e		*maps;
	enum memf_status	 rc;

	lisp_result_t res = memf_lisp_eval(&args->prog);
	printf("%d\n", res.value.sint);
	printf("%f\n", res.value.flt);
	/*
	 * if ((rc = memf_maps(args, &maps, &maps_cnt)) != MEMF_OK)
	 * 	return rc;
	 * free(maps);
	 */
	return MEMF_OK;
}
