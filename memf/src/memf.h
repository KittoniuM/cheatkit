#ifndef __MEMF_H__
#define __MEMF_H__

#include <stdint.h>

#include "lisp.h"

enum memf_status {
	MEMF_OK,
	MEMF_ERR_IO,
};

union memf_value {
	int8_t		i8;
	int16_t		i16;
	int32_t		i32;
	int64_t		i64;
	uint8_t		u8;
	uint16_t	u16;
	uint32_t	u32;
	uint64_t	u64;
	float		f32;
	double		f64;
};

struct memf_args {
	uint64_t	pid;
	uint64_t	from;
	uint64_t	to;
	lisp_program_t	prog;
};

/*
 * struct memf_store {
 * 	uint64_t		address;
 * 	union memf_value	value;
 * };
 */

enum memf_status memf(const struct memf_args *args);

#endif /* __MEMF_H__ */
