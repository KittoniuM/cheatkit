#ifndef __MEMF_H__
#define __MEMF_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum memf_status {
	MEMF_OK,
	/*
	 * Failure to open a file on procfs.  This usually indicates
	 * that the process either non-existent or we don't have
	 * privileges to touch it.
	 */
	MEMF_ERR_PROC,
	/*
	 * Internal IO error, usually caused by outdated maps.  Less
	 * likely, this could mean that something's wrong in our own
	 * code.
	 */
	MEMF_ERR_IO,
	/* No more memory for malloc to give. */
	MEMF_ERR_OOM,
};

enum memf_type {
	TYPE_ILL = 0,
	TYPE_I8,
	TYPE_I16,
	TYPE_I32,
	TYPE_I64,
	TYPE_F32,
	TYPE_F64,
};

enum memf_func {
	FUNC_ILL = 0,
	FUNC_EQ,
	FUNC_NE,
	FUNC_LT,
	FUNC_GT,
	FUNC_LE,
	FUNC_GE,
};

union memf_value {
	int64_t	i;
	double	f;
};

struct memf_store {
	unsigned long long	addr;
	union memf_value	value;
};

struct memf_args {
	bool			 verbose;
	unsigned long		 pid;
	unsigned long long	 from;
	unsigned long long	 to;
	char			 mask[4];
	bool			 noalign;
	enum memf_type		 type;
	enum memf_func		 func;
	union memf_value	 value;
	size_t			 num_stores;
	struct memf_store	*stores;
};

enum memf_status memf(const struct memf_args	 *args,
		      struct memf_store		**out_stores,
		      size_t			 *out_num_stores);

#endif /* __MEMF_H__ */
