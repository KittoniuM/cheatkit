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

struct mapd {
	uint64_t	from;
	uint64_t	to;
	uint64_t	off;
	int		prot;
	int		flags;
	char		path[256];
};

static enum memf_status memf_maps(const struct memf_args *args,
				  struct map **out_maps, size_t *out_maps_count)
{
	char		 smaps[32];
	FILE		*mapsf;
	size_t		 maps_count;
	struct mapd	*maps;

	assert(args != NULL);
	assert(out_maps != NULL);
	assert(out_maps_count != NULL);

	/* snprintf should not run out of buffer space */
	assert(snprintf(smaps, sizeof(smaps), "/proc/%llu/maps",
			(unsigned long long) args->pid)
	       < (int) sizeof(smaps));
	if ((mapsf = fopen(smaps, "r")) == NULL)
		return MEMF_ERR_IO;
	maps_count = 0;
	maps = NULL;
	while (!feof(mapsf)) {
		char		line[256];
		long long	from, to, off;
		char		perms[5], path[256];
		int		maj, min;
		long		ino;

		if (fgets(line, (int) sizeof(line), mapsf) == NULL)
			break;
		assert(sscanf(line, "%llx-%llx %4s %llx %d:%d %ld %s",
			      &from, &to, perms, &off,
			      &maj, &min, &ino, path) > 0);
		maps_count++;
		assert((maps = realloc(maps, sizeof(*maps) * maps_count))
		       != NULL);
		struct mapd *cur = &maps[maps_count - 1];
		cur->from = (uint64_t) from;
		cur->to   = (uint64_t) to;
		cur->off  = (uint64_t) off;
		cur->prot = (perms[0] == 'r' ? PROT_READ : 0)
			| (perms[1] == 'w' ? PROT_WRITE : 0)
			| (perms[2] == 'x' ? PROT_EXEC : 0);
		cur->flags = (perms[3] == 'p' ? MAP_PRIVATE :
			      perms[3] == 's' ? MAP_SHARED : 0);
		strncpy(cur->path, path, sizeof(cur->path));
	}
	assert(fclose(mapsf) == 0);
	*out_maps_count = maps_count;
	*out_maps = maps;
	return MEMF_OK;
}

static enum memf_status memf_look(const struct memf_args *args,
				  const struct mapd *map)
{
	/* long	pagesize = sysconf(_SC_PAGESIZE); */
	char	smem[32];
	int	memfd;

	assert(args != NULL);
	assert(map != NULL);

	assert(snprintf(smem, sizeof(smem), "/proc/%llu/mem",
			(unsigned long long) args->pid)
	       < (int) sizeof(smem));
	memfd = open(smem, O_RDONLY);
	if (memfd == -1)
		return MEMF_ERR_IO;
	for (size_t i = 0; i < maps_count; i++) {
		size_t len = (size_t) (map->to - map->from);
		char *mapmap = mmap(NULL, len, PROT_READ, MAP_PRIVATE,
				    memfd, (off_t) map->from);
		assert(mapmap != NULL);
		/* ... */
		assert(munmap(mapmap, len) == 0);
	}
	close(memfd);
	return MEMF_OK;
}

enum memf_status memf(const struct memf_args *args)
{
	size_t			 maps_count;
	struct mapd		*maps;
	enum memf_status	 rc;

	assert(args != NULL);

	if ((rc = memf_maps(args, &maps, &maps_count)) != MEMF_OK)
		return rc;
	free(maps);
	return MEMF_OK;
}
