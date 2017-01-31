#include "memf.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

struct maps_e {
	unsigned long long	from;
	unsigned long long	to;
	unsigned long long	off;
	int			prot;
	int			flags;
	char			path[256];
};

static enum memf_status memf_maps(const struct memf_args *args,
				  struct map **out_maps, size_t *out_maps_count)
{
	char		 smaps[32];
	FILE		*mapsfd;
	size_t		 maps_count;
	struct maps_e	*maps;

	assert(args != NULL);
	assert(out_maps != NULL);
	assert(out_maps_count != NULL);

	/* snprintf should not run out of buffer space */
	assert(snprintf(smaps, sizeof(smaps), "/proc/%llu/maps",
			(unsigned long long) args->pid)
	       < (int) sizeof(smaps));
	if ((mapsfd = fopen(smaps, "r")) == NULL)
		return MEMF_ERR_IO;
	maps_count = 0;
	maps = malloc(1 * sizeof(*maps));
	do {
		char		line[256];
		long long	from, to, off;
		char		perms[5], path[256];
		int		maj, min;
		long		ino;

		/* TODO: work on this */
		assert(fgets(line, (int) sizeof(line), mapsfd) == NULL);
		assert(sscanf(line, "%llx-%llx %4s %llx %d:%d %ld %s",
			      &from, &to, perms, &off,
			      &maj, &min, &ino, path) > 0);
		maps_count++;
		assert((maps = realloc(maps, sizeof(*maps) * maps_count))
		       != NULL);

		struct maps_e *cur = &maps[maps_count - 1];
		cur->from = (uint64_t) from;
		cur->to   = (uint64_t) to;
		cur->off  = (uint64_t) off;
		cur->prot = (perms[0] == 'r' ? PROT_READ  : 0)
			  | (perms[1] == 'w' ? PROT_WRITE : 0)
			  | (perms[2] == 'x' ? PROT_EXEC  : 0);
		cur->flags = (perms[3] == 'p' ? MAP_PRIVATE :
			      perms[3] == 's' ? MAP_SHARED : 0);
		strncpy(cur->path, path, sizeof(cur->path));
	} while (!feof(mapsfd));
	assert(fclose(mapsfd) == 0);
	*out_maps_count = maps_count;
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
	size_t			 maps_count;
	struct maps_e		*maps;
	enum memf_status	 rc;

	assert(args != NULL);

	if ((rc = memf_maps(args, &maps, &maps_count)) != MEMF_OK)
		return rc;
	free(maps);
	return MEMF_OK;
}
