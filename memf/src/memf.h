#ifndef __MEMF_H__
#define __MEMF_H__

#include <stdint.h>

enum memf_status {
	MEMF_OK,
	MEMF_ERR_IO,
	MEMF_ERR_MAP,
};

enum memf_type {
	TYPE_INVALID,
	TYPE_I8,
	TYPE_I16,
	TYPE_I32,
	TYPE_I64,
	TYPE_U8,
	TYPE_U16,
	TYPE_U32,
	TYPE_U64,
	TYPE_F32,
	TYPE_F64,
};

enum memf_func {
	FUNC_INVALID,
	FUNC_EQ,
	FUNC_NE,
	FUNC_LT,
	FUNC_LE,
	FUNC_GT,
	FUNC_GE,
};

struct memf_args {
	uint64_t	pid;
	enum memf_type	type;
	uint64_t	align;
	uint64_t	from;
	uint64_t	to;
	enum memf_func	func;
	uint64_t	val;
};

struct memf_store {
	uint64_t address;
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
	} value;
};

enum memf_status memf(const struct memf_args *args);

#endif /* __MEMF_H__ */
