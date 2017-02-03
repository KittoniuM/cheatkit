#define _GNU_SOURCE

#include "memf.h"

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct map_e {
	unsigned long long	from;
	unsigned long long	to;
	char			perms[4];
	unsigned long long	off;
	int			devmaj;
	int			devmin;
	unsigned long long	ino;
	bool			has_file;
	char			file[256];
};

static size_t type_align(enum memf_type type)
{
	switch (type) {
	case TYPE_I8:
		return 1;
	case TYPE_I16:
		return 2;
	case TYPE_I32:
	case TYPE_F32:
		return 4;
	case TYPE_I64:
	case TYPE_F64:
		return 8;
	default:
		assert(0);
	}
}

static bool memf_test(const struct memf_args *args, void *ptr)
{
	bool	is_int;
	bool	is_flt;
	int64_t i;
	double	f;

	switch (args->type) {
	case TYPE_I8:
		is_int = true;
		i = *(int8_t *)ptr  - (int8_t) args->value.i;
		break;
	case TYPE_I16:
		is_int = true;
		i = *(int16_t *)ptr - (int16_t) args->value.i;
		break;
	case TYPE_I32:
		is_int = true;
		i = *(int32_t *)ptr - (int32_t) args->value.i;
		break;
	case TYPE_I64:
		is_int = true;
		i = *(int64_t *)ptr - (int64_t) args->value.i;
		break;
	case TYPE_F32:
		is_flt = true;
		f = *(float *)ptr   - (float) args->value.f;
		break;
	case TYPE_F64:
		is_flt = true;
		f = *(double *)ptr  - (double) args->value.f;
		break;
	default:
		assert(0);
	}
	switch (args->func) {
	case FUNC_EQ:
		return is_int ? i == 0
			: args->type == TYPE_F32 ? f < (double) DBL_EPSILON
			: args->type == TYPE_F64 ? f < DBL_EPSILON
			: false;
	case FUNC_NE:
		return is_int ? i != 0
			: args->type == TYPE_F32 ? f >= (double) DBL_EPSILON
			: args->type == TYPE_F64 ? f >= DBL_EPSILON
			: false;
	case FUNC_LT: return is_int ? i < 0 : is_flt ? f < 0.0 : false;
	case FUNC_GT: return is_int ? i > 0 : is_flt ? f > 0.0 : false;
	case FUNC_LE: return is_int ? i <= 0 : is_flt ? f <= 0.0 : false;
	case FUNC_GE: return is_int ? i >= 0 : is_flt ? f >= 0.0 : false;
	default:
		assert(0);
	}
}

static bool memf_maskmap(const struct map_e *map, const char mask[4])
{
	for (size_t i = 0; i < 4; i++) {
		if (mask[i] != '?' && map->perms[i] != mask[i])
			return false;
	}
	return true;
}

static enum memf_status memf_maps(const struct memf_args *args,
				  struct map_e	**out_maps,
				  size_t	 *out_mapsn)
{
	char		 smaps[32];
	FILE		*mapsf;
	size_t		 len = 0;
	char		*line = NULL;
	size_t		 cur;
	struct map_e	*maps;

	/* snprintf should not run out of buffer space */
	assert(snprintf(smaps, sizeof(smaps), "/proc/%lu/maps", args->pid)
	       < (int) sizeof(smaps));
	/* this is an expected error -- we might lack permissions */
	if ((mapsf = fopen(smaps, "r")) == NULL)
		return MEMF_ERR_PROC;
	cur = 0;
	maps = NULL;
	while (getline(&line, &len, mapsf) != -1) {
		struct map_e	*map;
		int		 c;

		maps = realloc(maps, (cur + 1) * sizeof(*maps));
		assert(maps);
		map = &maps[cur];
		c = sscanf(line, "%llx-%llx %4s %llx %x:%x %llu %256s",
			   &map->from, &map->to, map->perms, &map->off,
			   &map->devmaj, &map->devmin, &map->ino, map->file);
		/* sure we read maps the right way? (assuming procfs is ok) */
		assert(c >= 7);
		assert(map->from < map->to);
		/* an optional file field */
		map->has_file = c >= 8;
		cur++;
	}
	free(line);
	fclose(mapsf);
	*out_mapsn = cur;
	*out_maps = maps;
	return MEMF_OK;
}

enum memf_status memf_lookmap(const struct memf_args	 *args,
			      const struct map_e	 *map,
			      struct memf_store		**out_stores,
			      size_t			 *out_storesn)
{
	char			 smem[32];
	void			*buf;
	FILE			*memf;
	enum memf_status	 rc;

	/* snprintf should not run out of buffer space */
	assert(snprintf(smem, sizeof(smem), "/proc/%lu/mem", args->pid)
	       < (int) sizeof(smem));
	/* just return immediately! */
	if ((map->to - map->from) > SIZE_MAX)
		return MEMF_ERR_OOM;
	if ((memf = fopen(smem, "rb")) == NULL) {
		rc = MEMF_ERR_PROC;
		goto fail_fopen;
	}
	if ((buf = malloc((size_t) (map->to - map->from))) == NULL) {
		rc = MEMF_ERR_OOM;
		goto fail_malloc;
	}
	/* by now map information could become outdated so we must handle it */
	if (fseeko(memf, (off_t) map->from, SEEK_SET) != 0) {
		rc = MEMF_ERR_IO;
		goto fail_io;
	}
	if (fread(buf, (size_t) (map->to - map->from), 1, memf) != 1) {
		rc = MEMF_ERR_IO;
		goto fail_io;
	}

	const size_t s = args->noalign ? 1 : type_align(args->type);
	for (size_t o = 0; o < (size_t) (map->to - map->from); o += s) {
		if (memf_test(args, (void *) ((uintptr_t) buf + o))) {
			printf("%llx\n", map->from + (unsigned long long) o);
		}
	}
	rc = MEMF_OK;
fail_io:
	free(buf);
fail_malloc:
	fclose(memf);
fail_fopen:
	return rc;
}

enum memf_status memf(const struct memf_args *args)
{
	size_t			 mapsn;
	struct map_e		*maps;
	enum memf_status	 rc;

	if ((rc = memf_maps(args, &maps, &mapsn)) != MEMF_OK)
		return rc;
	for (size_t i = 0; i < mapsn; i++) {
		struct map_e *map = &maps[i];

		if (!memf_maskmap(map, args->mask))
			continue;
		if (memf_lookmap(args, map, NULL, NULL) == MEMF_OK)
			if (args->verbose) {
				printf("memf: failed map: "
				       "%zu %s %016llx-%016llx %s\n",
				       i,
				       maps[i].perms,
				       maps[i].from,
				       maps[i].to,
				       maps[i].has_file ? maps[i].file : "");
			}
	}
	free(maps);
	return MEMF_OK;
}
