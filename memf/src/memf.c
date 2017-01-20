#include "memf.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * struct map {
 * 	long long	from;
 * 	long long	to;
 * 	char		perms[4];
 * 	long		drv;
 * 	long		dev;
 * 	long		ino;
 * 	char		pathname[256];
 * };
 *
 * static enum memf_status memf_maps(const struct memf_args *args,
 * 				  struct map **maps, size_t *count)
 * {
 * 	char		 smaps[32];
 * 	char		 line[256];
 * 	FILE		*mapsf;
 * 	size_t		 lo_count;
 * 	struct map	*lo_maps;
 *
 * 	assert(args != NULL);
 * 	assert(maps != NULL);
 * 	assert(count != NULL);
 *
 * 	/\* snprintf should not run out of buffer space *\/
 * 	assert(snprintf(smaps, sizeof(smaps), "/proc/%llu/maps",
 * 			(unsigned long long) args->pid)
 * 	       < (int) sizeof(smaps));
 * 	if ((mapsf = fopen(smaps, "r")) == NULL)
 * 		return MEMF_ERR_IO;
 * 	lo_count = 0;
 * 	lo_maps  = NULL;
 * 	while (!feof(mapsf)) {
 * 		lo_count++;
 * 		lo_maps = realloc(lo_maps, lo_count);
 * 		fgets(line, (int) sizeof(line), mapsf);
 * 		sscanf(line, "%llx-%llx %4s %lx %5s %ld %s", );
 * 	}
 * 	fclose(mapsf);
 * }
 */

enum memf_status memf(const struct memf_args *args)
{
	long	 pagesize = sysconf(_SC_PAGESIZE);
	char	 smem[32];
	int	 memfd;
	char	*map;

	assert(args != NULL);

	/* snprintf should not run out of buffer space */
	assert(snprintf(smem, sizeof(smem), "/proc/%llu/mem",
			(unsigned long long) args->pid)
	       < (int) sizeof(smem));
	/* TODO: Ignore any memory outside of ranges in maps */
	if ((memfd = open(smem, O_RDONLY)) == -1)
		return MEMF_ERR_IO;
	for (uint64_t page = args->from & (uint64_t) (~pagesize + 1);
	     page < args->to;
	     page += (uint64_t) pagesize) {
		/* printf("%p\n", page); */
		if ((map = mmap(NULL, (size_t) pagesize, PROT_READ, MAP_SHARED,
				memfd, (off_t) page)) == MAP_FAILED) {
			continue;
		}
		printf("%p\n", map);
		assert(munmap(map, (size_t) page) == 0);
	}
	assert(close(memfd) == 0);
	return MEMF_OK;
}
