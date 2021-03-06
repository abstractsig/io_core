/*
 *
 * io
 * ==
 *
 * io provides an event-driven operating system for micontroller,
 * microprocessor and application processors.
 *
 * VERSION HISTORY
 * ===============
 * 1.0	 (2020-01)
 *
 * LICENSE
 * =======
 * See end of file for license terms.
 *
 * USAGE
 * =====
 * Include this file in whatever places need to refer to it.
 * In one C source define IMPLEMENT_IO_CORE prior to the include
 * directive.
 *
 * COMMING SOON
 * ============
 * Add cycle detection to garbage collection
 * Persistent storage of io_values
 * Additional io language support for Javascript and Python
 * A ms windows io cpu
 * A linux io cpu
 *
 */
#ifndef io_core_H_
#define io_core_H_
#include <configure_io_build.h>
#include <limits.h>

#ifdef USE_LIBC
# include <string.h>
# define io_memset	memset

// utf8 for now, need locale for longer codes
# include <ctype.h>
#define character_is_digit		isdigit
#define character_is_alpha		isalpha
#define character_is_isalnum	isalnum

#define character_to_decimal_digit(c)	((c) - 0x30)

#else
# error "not yet"
#endif

//#include <io_math.h>

typedef union io_value_reference vref_t;
typedef struct io_encoding io_encoding_t;
typedef struct io_pipe io_pipe_t;
typedef struct io_socket io_socket_t;
typedef struct io io_t;
typedef struct io_event io_event_t;

//
// identity
//
#define SHA256_SIZE		32

typedef struct io_sha256_context {
    uint32_t total[2];          /*!< The number of Bytes processed.  */
    uint32_t state[8];          /*!< The intermediate digest state.  */
    unsigned char buffer[64];   /*!< The data block being processed. */
    int is224;                  /* not used */
} io_sha256_context_t;

/*
 *
 * Byte memory manager
 *
 * based on umm (see git hub)
 *
 */
typedef struct memory_info {
	uint32_t total_bytes;
	uint32_t used_bytes;
	uint32_t free_bytes;
} memory_info_t;

#define UMM_BEST_FIT
//#define UMM_FIRST_FIT
#undef  UMM_FIRST_FIT
#define UMM_CRITICAL_ENTRY(bm)	{\
												bool __h = enter_io_critical_section(bm->io);
#define UMM_CRITICAL_EXIT(bm)			exit_io_critical_section(bm->io,__h);\
											}

typedef struct PACK_STRUCTURE umm_ptr_t {
	unsigned short int next;
	unsigned short int prev;
} umm_ptr;

typedef struct PACK_STRUCTURE {
	union PACK_STRUCTURE {
		umm_ptr used;
	} header;
	union PACK_STRUCTURE {
		umm_ptr free;
		uint8_t data[4];
	} body;
} umm_block_t;

typedef enum {
	IO_MEMORY_FREE_OK = 0,
	IO_MEMORY_FREE_ERROR_ALREADY_FREE,
	IO_MEMORY_FREE_ERROR_NOT_IN_MEMORY,
} io_memory_status_t;

typedef struct {
	io_t *io;
	umm_block_t *heap;
	uint32_t number_of_blocks;
	uint32_t block_size_n;
} io_byte_memory_t;

#define io_byte_memory_io(this)						(this)->io
#define io_byte_memory_number_of_blocks(this)	(this)->number_of_blocks
#define io_byte_memory_last_block(this)			(io_byte_memory_number_of_blocks(this) - 1)
#define io_byte_memory_block_size_bits(this)		((this)->block_size_n)
#define io_byte_memory_block_size(this)			(1 << io_byte_memory_block_size_bits(this))

typedef struct io_value io_value_t;
typedef struct io_value_implementation io_value_implementation_t;
typedef struct io_value_memory io_value_memory_t;

void 	iterate_io_byte_memory_allocations(io_byte_memory_t*,bool (*cb) (void*,void*),void*);
void	incremental_iterate_io_byte_memory_allocations (io_byte_memory_t*,uint16_t*,bool (*) (io_value_t*,void*),void *);

io_byte_memory_t*	mk_io_byte_memory (io_t*,uint32_t,uint32_t);
io_byte_memory_t*	initialise_io_byte_memory (io_t*,io_byte_memory_t*,uint32_t);
void	free_io_byte_memory (io_byte_memory_t*);
void*	umm_malloc(io_byte_memory_t*,size_t);
void*	umm_calloc(io_byte_memory_t*,size_t,size_t);
void*	umm_realloc(io_byte_memory_t*,void *ptr, size_t size );
io_memory_status_t umm_free(io_byte_memory_t*,void *ptr );
void	io_byte_memory_get_info (io_byte_memory_t*,memory_info_t *info);

#define io_byte_memory_get_io(bm)	(bm)->io

#define io_byte_memory_allocate umm_malloc
#define io_byte_memory_free umm_free
#define io_byte_memory_reallocate umm_realloc

INLINE_FUNCTION void*
io_byte_memory_allocate_and_zero (io_byte_memory_t *bm,uint32_t size) {
	void *alloc = io_byte_memory_allocate (bm,size);
	memset (alloc,0,size);
	return alloc;
}

//
// block sizes
//

#define UMM_BLOCK_SIZE_1N		3UL	// 8, must be >= sizeof(umm_block_t)
#define UMM_BLOCK_SIZE_2N		4UL	// 16
#define UMM_BLOCK_SIZE_3N		5UL	// 32
#define UMM_BLOCK_SIZE_4N		6UL	// 64
#define UMM_BLOCK_SIZE_5N		7UL	// 128
#define UMM_BLOCK_SIZE_6N		8UL	// 256, up to 8Mbyte
#define UMM_BLOCK_SIZE_7N		10UL	// 1024, up to 32Mbyte
#define UMM_BLOCK_SIZE_8N		12UL	// 4096, up to 132Mbyte

//
// uid
//
#define IO_UID_BYTE_LENGTH				16
#define IO_UID_WORDS_LENGTH			(IO_UID_BYTE_LENGTH/sizeof(uint32_t))
#define IO_UID_RESERVED_LIMIT			0xffff

typedef union PACK_STRUCTURE {
	uint32_t words[IO_UID_BYTE_LENGTH/sizeof(uint32_t)];
	uint8_t bytes[IO_UID_BYTE_LENGTH];
} io_uid_t;


#define IO_AUTHENTICATION_KEY_BYTE_LENGTH			32	// bytes
#define IO_AUTHENTICATION_KEY_WORD_LENGTH			(IO_AUTHENTICATION_KEY_BYTE_LENGTH/sizeof(uint32_t))
#define IO_AUTHENTICATION_KEY_LONG_WORD_LENGTH	(IO_AUTHENTICATION_KEY_BYTE_LENGTH/sizeof(uint64_t))

typedef union PACK_STRUCTURE io_authentication_key {
	uint64_t long_words[IO_AUTHENTICATION_KEY_LONG_WORD_LENGTH];
	uint32_t words[IO_AUTHENTICATION_KEY_WORD_LENGTH];
	uint8_t bytes[IO_AUTHENTICATION_KEY_BYTE_LENGTH];
	struct PACK_STRUCTURE {
		uint8_t lower_half[IO_AUTHENTICATION_KEY_BYTE_LENGTH/2];
		uint8_t upper_half[IO_AUTHENTICATION_KEY_BYTE_LENGTH/2];
	} part;
} io_authentication_key_t;

#define IO_RETAINED_STATE_STRUCT_MEMBERS \
	uint32_t	first_run_flag;							\
	uint32_t	power_cycles;								\
	io_uid_t uid;											\
	io_authentication_key_t secret;					\
	io_authentication_key_t shared;					\
	/**/

typedef struct PACK_STRUCTURE {
	IO_RETAINED_STATE_STRUCT_MEMBERS
} io_persistant_state_t;

#define IO_FIRST_RUN_SET		0xaaaaaaaa
#define IO_FIRST_RUN_CLEAR		0xbbbbbbbb

#define io_config_u32_ptr(c)	(&(c).first_run_flag)
#define io_config_u32_size()	(sizeof(io_persistant_state_t)/sizeof(uint32_t))

void io_gererate_authentication_key_pair (io_t*,io_authentication_key_t*,io_authentication_key_t*);

INLINE_FUNCTION bool
io_authentication_key_test_equal (io_authentication_key_t const *a,io_authentication_key_t const *b) {
	return (
			(a->long_words[0] == b->long_words[0])
		&&	(a->long_words[1] == b->long_words[1])
		&&	(a->long_words[2] == b->long_words[2])
		&&	(a->long_words[3] == b->long_words[3])
	);
}

#include <io_curve25519.h>

//
// time
//

typedef union PACK_STRUCTURE io_time {
	int64_t nanoseconds;
	int64_t ns;
	uint64_t u64;
	uint8_t bytes[8];
	volatile uint16_t u16[4];
	volatile uint8_t u8[8];
	volatile struct {
		uint32_t low;
		uint32_t high;
	} part;
} io_time_t;

#define microsecond_time(m)			(io_time_t){(int64_t)(m) * 1000LL}
#define millisecond_time(m)			(io_time_t){(int64_t)(m) * 1000000LL}
#define second_time(m)					(io_time_t){(int64_t)(m) * 1000000000LL}
#define minute_time(m)					(io_time_t){(int64_t)(m) * 60000000000LL}
#define time_zero()						(io_time_t){0LL}

#define time_to_milliseconds(t)		((t)/1000000LL)
#define time_in_milliseconds(m)		((int64_t)(m) / 1000000LL)

//
// alarms
//
typedef struct io_alarm io_alarm_t;

struct PACK_STRUCTURE io_alarm {
	io_event_t *at;					// raised 'at' when
	io_event_t *error;				// raised if at cannot be raised as the correct time
	io_time_t when;
	io_alarm_t *next_alarm;
};

extern io_alarm_t s_null_io_alarm;

INLINE_FUNCTION io_alarm_t*
initialise_io_alarm (
	io_alarm_t *ev,io_event_t *at,io_event_t *err,io_time_t when
) {
	ev->when = when;
	ev->at = at;
	ev->error = err;
	ev->next_alarm = NULL;
	return ev;
}

INLINE_FUNCTION bool
is_io_alarm_active (io_alarm_t *alarm) {
	return alarm->next_alarm != NULL;
}

//
// Pipes
//
typedef struct io_pipe_implementation io_pipe_implementation_t;

#define IO_PIPE_IMPLEMENTATION_STRUCT_MEMBERS \
	io_pipe_implementation_t const *specialisation_of;\
	/**/
	
struct PACK_STRUCTURE io_pipe_implementation {
	IO_PIPE_IMPLEMENTATION_STRUCT_MEMBERS
};

#define IO_PIPE_STRUCT_MEMBERS \
	io_pipe_implementation_t const *implementation;\
	int16_t size_of_ring;\
	volatile int16_t write_index;\
	volatile int16_t read_index;\
	int16_t overrun;\
	/**/

struct PACK_STRUCTURE io_pipe {
	IO_PIPE_STRUCT_MEMBERS
};

INLINE_FUNCTION bool
io_pipe_is_readable (io_pipe_t const *this) {
	return  (this->read_index != this->write_index);
}

INLINE_FUNCTION int16_t
io_pipe_count_occupied_slots (io_pipe_t const *this) {
	int16_t s = this->write_index - this->read_index;
	if (s < 0) s += this->size_of_ring;
	return s;
}

INLINE_FUNCTION int16_t
io_pipe_count_free_slots (io_pipe_t const *this) {
	return this->size_of_ring - io_pipe_count_occupied_slots(this) - 1;
}

INLINE_FUNCTION bool
io_pipe_is_writeable (io_pipe_t const *this) {
	return io_pipe_count_free_slots(this) > 0;
}

//
// byte pipe
//
typedef struct PACK_STRUCTURE io_byte_pipe {
	IO_PIPE_STRUCT_MEMBERS
	uint8_t *byte_ring;
} io_byte_pipe_t;

io_byte_pipe_t* mk_io_byte_pipe (io_byte_memory_t*,uint16_t);
void free_io_byte_pipe (io_byte_pipe_t*,io_byte_memory_t*);
bool		io_byte_pipe_get_byte (io_byte_pipe_t*,uint8_t*);
bool		io_byte_pipe_put_byte (io_byte_pipe_t*,uint8_t);
uint32_t	io_byte_pipe_put_bytes (io_byte_pipe_t*,uint8_t const*,uint32_t);

bool	is_io_byte_pipe (io_pipe_t const*);
bool	is_io_encoding_pipe (io_pipe_t const*);

#define io_byte_pipe_count_free_slots(p) io_pipe_count_free_slots ((io_pipe_t const*) (p))
#define io_byte_pipe_is_readable(p) io_pipe_is_readable ((io_pipe_t const*) (p))
#define io_byte_pipe_is_writeable(p) io_pipe_is_writeable ((io_pipe_t const*) (p))

//
// encoding pipe
//
typedef struct PACK_STRUCTURE io_encoding_pipe_implementation {
	IO_PIPE_IMPLEMENTATION_STRUCT_MEMBERS
} io_encoding_pipe_implementation_t;

typedef struct PACK_STRUCTURE io_encoding_pipe {
	IO_PIPE_STRUCT_MEMBERS
	io_encoding_t **encoding_ring;
		
} io_encoding_pipe_t;

io_encoding_pipe_t* cast_to_io_encoding_pipe (io_pipe_t*);

io_encoding_pipe_t* mk_io_encoding_pipe (io_byte_memory_t*,uint16_t);
void free_io_encoding_pipe (io_encoding_pipe_t*,io_byte_memory_t*);
void reset_io_encoding_pipe (io_encoding_pipe_t*);
bool	io_encoding_pipe_pop_encoding (io_encoding_pipe_t*);
bool	io_encoding_pipe_put_encoding (io_encoding_pipe_t*,io_encoding_t*);
bool	io_encoding_pipe_peek (io_encoding_pipe_t*,io_encoding_t**);

#define io_encoding_pipe_count_occupied_slots(p) io_pipe_count_occupied_slots ((io_pipe_t const*) (p))
#define io_encoding_pipe_count_free_slots(p) io_pipe_count_free_slots ((io_pipe_t const*) (p))
#define io_encoding_pipe_is_readable(p) io_pipe_is_readable ((io_pipe_t const*) (p))
#define io_encoding_pipe_is_writeable(p) io_pipe_is_writeable ((io_pipe_t const*) (p))


/*
 *
 * References to values
 *
 */

typedef struct PACK_STRUCTURE io_value_reference_implementation {
	vref_t (*reference) (vref_t);
	void (*unreference) (vref_t);
	void const* (*cast_to_ro_pointer) (vref_t);
	void* (*cast_to_rw_pointer) (vref_t);
	int64_t	(*get_as_builtin_integer) (vref_t);
	io_value_memory_t* (*get_containing_memory) (vref_t);
} io_value_reference_implementation_t;

#if UINTPTR_MAX == 0xffffffff
# define IO_32_BIT_BUILD
#elif UINTPTR_MAX == 0xffffffffffffffff
# define IO_64_BIT_BUILD
#else
# error "what's happening here?"
#endif

union PACK_STRUCTURE io_value_reference {
	struct PACK_STRUCTURE {
		io_value_reference_implementation_t const *implementation;
		union PACK_STRUCTURE {
			intptr_t ptr;
			struct PACK_STRUCTURE {
				uint32_t memory:3;
				uint32_t address:29;
			} p32;
			struct PACK_STRUCTURE {
				uint32_t id:8;
				uint32_t block:24;
			} vm;
		} expando;
	} ref;
	uint64_t all;
};

#define vref_implementation(r)	(r).ref.implementation
#define vref_expando(r)			(r).ref.expando
#define vref_all(r)				(r).all

//
// value pipe
//
typedef struct PACK_STRUCTURE io_value_pipe {
	IO_PIPE_STRUCT_MEMBERS
	vref_t *value_ring;
} io_value_pipe_t;

io_value_pipe_t* mk_io_value_pipe (io_byte_memory_t*,uint16_t);
void free_io_value_pipe (io_value_pipe_t*,io_byte_memory_t*);
bool	io_value_pipe_get_value (io_value_pipe_t*,vref_t*);
bool	io_value_pipe_put_value (io_value_pipe_t*,vref_t);
bool	io_value_pipe_peek (io_value_pipe_t*,vref_t*);

#define io_value_pipe_count_occupied_slots(p) io_pipe_count_occupied_slots ((io_pipe_t const*) (p))
#define io_value_pipe_count_free_slots(p) io_pipe_count_free_slots ((io_pipe_t const*) (p))
#define io_value_pipe_is_readable(p) io_pipe_is_readable ((io_pipe_t const*) (p))
#define io_value_pipe_is_writeable(p) io_pipe_is_writeable ((io_pipe_t const*) (p))

#ifdef IO_32_BIT_BUILD
COMPILER_VERIFY(sizeof(vref_t) == 8);
#else
# error "only 32bit build supported"
#endif

#define def_ptr_vref_cast(T,P,...) __VA_ARGS__ {\
		.ref.implementation = T,						\
		.ref.expando.ptr = ((intptr_t) (P)),			\
	}

#define def_vref(T,P) (def_ptr_vref_cast(T,P,(vref_t)))

// sometines this version is required to create compile-time constant values
#define decl_vref(P) {\
		.ref.implementation = &reference_to_constant_value, \
		.ref.expando.ptr = ((intptr_t) (P)), \
	}

#define umm_vref(I,id,ptr) (vref_t) {\
	.ref.implementation = I,\
	.ref.expando.p32 = {\
		.memory = id,\
		.address = io_value_reference_p32_from_c_pointer(ptr),\
	},\
}

#define io_value_reference_p32_memory(r)	vref_expando(r).p32.memory
#define io_value_reference_p32_address(r)	vref_expando(r).p32.address

// value memory is 8-byte aligned
#define io_value_reference_p32_from_c_pointer(p) 	(((intptr_t)(p)) >> 2)
#define io_value_reference_p32_to_c_pointer(r) 		(io_value_reference_p32_address(r) << 2)

//
// inline io_value reference methods
//
INLINE_FUNCTION vref_t
reference_value (vref_t r_value) {
	return vref_implementation(r_value)->reference(r_value);
}

INLINE_FUNCTION vref_t
reference_io_value (vref_t r_value) {
	return vref_implementation(r_value)->reference(r_value);
}

INLINE_FUNCTION vref_t
unreference_value (vref_t r_value) {
	vref_implementation(r_value)->unreference(r_value);
	return r_value;
}

INLINE_FUNCTION vref_t
unreference_io_value (vref_t r_value) {
	vref_implementation(r_value)->unreference(r_value);
	return r_value;
}

INLINE_FUNCTION void*
vref_cast_to_rw_pointer (vref_t r_value) {
	return vref_implementation(r_value)->cast_to_rw_pointer(r_value);
}

INLINE_FUNCTION void const*
vref_cast_to_ro_pointer (vref_t r_value) {
	return vref_implementation(r_value)->cast_to_ro_pointer(r_value);
}

INLINE_FUNCTION io_value_memory_t*
vref_get_containing_memory (vref_t r_value) {
	return vref_implementation(r_value)->get_containing_memory(r_value);
}

INLINE_FUNCTION int64_t
vref_get_as_builtin_integer (vref_t r_value) {
	return vref_implementation(r_value)->get_as_builtin_integer(r_value);
}

extern EVENT_DATA io_value_reference_implementation_t reference_to_constant_value;
extern EVENT_DATA io_value_reference_implementation_t reference_to_data_section_value;

#define INVALID_VREF 				def_vref (NULL,NULL)
#define vref_is_valid(v)			(vref_implementation(v) != NULL)
#define vref_is_invalid(v)			(vref_implementation(v) == NULL)

INLINE_FUNCTION bool
vref_is_equal_to (vref_t r_a,vref_t r_b) {
	return (
			(vref_implementation(r_a) == vref_implementation(r_b))
		&&	(vref_expando(r_a).ptr == vref_expando(r_b).ptr)
	);
}

INLINE_FUNCTION bool
vref_not_equal_to (vref_t r_a,vref_t r_b) {
	return (
			(vref_implementation(r_a) != vref_implementation(r_b))
		||	(vref_expando(r_a).ptr != vref_expando(r_b).ptr)
	);
}

#define vref_is_nil(r)				vref_is_equal_to(r,cr_NIL)
#define vref_not_nil(r)				(!vref_is_equal_to(r,cr_NIL))

void store_vref (vref_t,vref_t*,vref_t);

//
// hash
//
typedef struct vref_hash_table vref_hash_table_t;

typedef struct vref_hash_table_implementation {
	void	(*free) (vref_hash_table_t*);
	bool	(*insert) (vref_hash_table_t*,vref_t);
	bool	(*contains) (vref_hash_table_t*,vref_t);
	bool	(*remove) (vref_hash_table_t*,vref_t);
} vref_hash_table_implementation_t;

#define VREF_HASH_TABLE_STRUCT_MEMBERS \
	vref_hash_table_implementation_t const *implementation;		\
	/**/

struct PACK_STRUCTURE vref_hash_table {
	VREF_HASH_TABLE_STRUCT_MEMBERS
};

//
// inline vref hash implementation
//
INLINE_FUNCTION void
free_vref_hash_table (vref_hash_table_t *h) {
	h->implementation->free(h);
}

INLINE_FUNCTION bool
vref_hash_table_insert (vref_hash_table_t *h,vref_t r_value) {
	return h->implementation->insert(h,r_value);
}

INLINE_FUNCTION bool
vref_hash_table_contains (vref_hash_table_t *h,vref_t r_value) {
	return h->implementation->contains(h,r_value);
}

INLINE_FUNCTION bool
vref_hash_table_remove (vref_hash_table_t *h,vref_t r_value) {
	return h->implementation->remove(h,r_value);
}

vref_hash_table_t*	mk_vref_bucket_hash_table (io_byte_memory_t*,uint32_t);

typedef union string_hash_table_mapping {
	uint32_t u32;
	int32_t i32;
	void *ptr;
	void const *ro_ptr;
} string_hash_table_mapping_t;

#define def_hash_mapping_i32(v)		(string_hash_table_mapping_t){.i32 = v}
#define def_hash_mapping_ptr(p)		(string_hash_table_mapping_t){.ptr = p}
#define def_hash_mapping_ro_ptr(p)	(string_hash_table_mapping_t){.ro_ptr = p}

typedef struct string_hash_table_entry {
	struct string_hash_table_entry *next_entry;
	char *bytes;
	uint32_t size;
	string_hash_table_mapping_t mapping;
} string_hash_table_entry_t;

typedef struct string_hash_table {
	string_hash_table_entry_t **table;	
	io_byte_memory_t *bm;
	uint32_t table_size;
	uint32_t table_grow;
} string_hash_table_t;

string_hash_table_t* mk_string_hash_table (io_byte_memory_t*,uint32_t);
void free_string_hash_table (string_hash_table_t*);
bool string_hash_table_insert (string_hash_table_t*,const char*,uint32_t,string_hash_table_mapping_t);
bool string_hash_table_remove (string_hash_table_t*,const char*,uint32_t);
bool string_hash_table_map (string_hash_table_t*,const char*,uint32_t,string_hash_table_mapping_t*);
void iterate_string_hash_table (string_hash_table_t*,bool (*) (string_hash_table_entry_t*,void*),void*);

//
// cht
//
typedef struct PACK_STRUCTURE io_constrained_hash_entry {
	vref_t key;
	vref_t value;
	int64_t age;
	struct PACK_STRUCTURE io_constrained_hash_entry_info {
		uint32_t access_count:29;
		uint32_t free:1;
		uint32_t user_flag1:1;
		uint32_t user_flag2:1;
	} info;
	struct io_constrained_hash_entry *successor;
	struct io_constrained_hash_entry *predecessor;
} io_constrained_hash_entry_t;

typedef struct io_constrained_hash_entry_info io_constrained_hash_entry_info_t;

#define cht_entry_access_count(e) ((e)->info.access_count)

typedef bool (*cht_purge_entry_helper_t) (vref_t,vref_t,void*);
typedef void (*cht_begin_purge_helper_t) (void*);

typedef struct PACK_STRUCTURE io_constrained_hash {
	io_t *io;
	uint32_t table_size;
	uint32_t entry_limit;
	uint32_t entry_count;
	uint32_t prune_count;		// number of entries to prune
	cht_purge_entry_helper_t purge_callback;
	cht_begin_purge_helper_t begin_purge;
	void *user_data;
	io_constrained_hash_entry_t *entries;
	io_constrained_hash_entry_t **ordered;
} io_constrained_hash_t;

io_constrained_hash_t*	mk_io_constrained_hash (io_byte_memory_t*,uint32_t,cht_begin_purge_helper_t,cht_purge_entry_helper_t,void*);
void free_io_constrained_hash (io_byte_memory_t*,io_constrained_hash_t*);

vref_t	cht_get_value (io_constrained_hash_t*,vref_t);
bool		cht_has_key (io_constrained_hash_t*,vref_t);
void		cht_sort (io_constrained_hash_t*);
void		cht_set_value (io_constrained_hash_t*,vref_t,vref_t);
bool		cht_unset (io_constrained_hash_t*,vref_t);
int		cht_compare_entries_by_key (const void*,const void*);

#define cht_entries_size(size) 					(sizeof(io_constrained_hash_entry_t) * size)
#define cht_ordered_size(size) 					(sizeof(io_constrained_hash_entry_t*) * size)
#define cht_index_of_entry(this,e) 				((uint32_t) (e - (this)->entries))
#define cht_count(this)								((this)->entry_count)
#define cht_entry_at_index(this,i)				((this)->entries[i])
#define cht_get_entry_limit(this)				((this)->entry_limit)
#define cht_get_table_size(this)					((this)->table_size)
#define cht_ordered_entry_at_index(this,i)	((this)->ordered[i])
#define cht_get_invalid_value(this)				((this)->invalid_value)
#define cht_get_prune_count(this)				((this)->prune_count)

/*
 *
 * Encoding
 *
 */
typedef struct io_encoding_implementation io_encoding_implementation_t;
typedef struct io_layer_implementation io_layer_implementation_t;
typedef struct io_layer io_layer_t;

typedef vref_t (*io_value_decoder_t) (io_encoding_t*,io_value_memory_t*);

typedef io_layer_t* (*io_make_layer_t) (io_byte_memory_t*,io_encoding_t*);

typedef struct io_encoding_layer_api {
	void* (*get_inner_layer) (io_encoding_t*,io_layer_t*);
	void* (*get_outer_layer) (io_encoding_t*,io_layer_t*);
	void* (*get_layer) (io_encoding_t*,io_layer_implementation_t const*);
	io_layer_t* (*push_layer) (io_encoding_t*,io_make_layer_t);
} io_encoding_layer_api_t;

#define IO_ENCODING_IMPLEMENTATION_STRUCT_MEMBERS \
	io_encoding_implementation_t const *specialisation_of;\
	io_encoding_t* (*make_encoding) (io_byte_memory_t*);\
	void (*free) (io_encoding_t*);\
	io_t* (*get_io) (io_encoding_t*);\
	void* (*get_byte_stream) (io_encoding_t*); \
	void (*get_content) (io_encoding_t*,uint8_t const**,uint8_t const**);\
	uint32_t (*increment_decode_offest) (io_encoding_t*,uint32_t); \
	vref_t (*decode_to_io_value) (io_encoding_t*,io_value_decoder_t,io_value_memory_t*);\
	size_t (*print) (io_encoding_t*,char const*,va_list);\
	size_t (*fill) (io_encoding_t*,uint8_t,size_t);\
	bool (*grow) (io_encoding_t*,uint32_t);\
	uint32_t (*grow_increment) (io_encoding_t*);\
	int32_t (*limit) (void);\
	size_t (*length) (io_encoding_t const*);\
	io_encoding_layer_api_t const *layer;\
	void (*reset) (io_encoding_t*);\
	bool (*append_byte) (io_encoding_t*,uint8_t);\
	bool (*append_bytes) (io_encoding_t*,uint8_t const*,size_t);\
	bool (*pop_last_byte) (io_encoding_t*,uint8_t*);\
	/**/

struct io_encoding_implementation {
	IO_ENCODING_IMPLEMENTATION_STRUCT_MEMBERS
};

#define IO_ENCODING_STRUCT_MEMBERS \
	io_encoding_implementation_t const *implementation;\
	union PACK_STRUCTURE {\
		uint32_t all;\
		struct PACK_STRUCTURE {\
			uint16_t reference_count;\
			struct PACK_STRUCTURE {\
				uint16_t requires_time:1;\
				uint16_t has_time:1;\
				uint16_t :14;\
			} flag;\
		} bit;\
	} tag;\
	/**/

struct PACK_STRUCTURE io_encoding {
	IO_ENCODING_STRUCT_MEMBERS
};

#define IO_ENCODING_REFERENCE_COUNT_LIMIT	0xffffUL

#define IO_ENCODING_IMPLEMENATAION(e)		((io_encoding_implementation_t const *) (e))
#define io_encoding_implementation(e)		(e)->implementation
#define io_encoding_reference_count(e)		(e)->tag.bit.reference_count

bool	io_encoding_has_implementation (io_encoding_t const*,io_encoding_implementation_t const*);
io_encoding_t*	reference_io_encoding (io_encoding_t*);
void	unreference_io_encoding (io_encoding_t*);
void* io_encoding_no_layer (io_encoding_t*,io_layer_implementation_t const*);

io_encoding_t* mk_null_encoding (io_byte_memory_t*);
void free_null_encoding (io_encoding_t*);
io_t* null_encoding_get_io (io_encoding_t*);
void* io_encoding_no_byte_stream (io_encoding_t*);
void io_encoding_no_content (io_encoding_t*,uint8_t const**,uint8_t const**);
vref_t io_value_encoding_decode_to_io_value (io_encoding_t*,io_value_decoder_t,io_value_memory_t*);
uint32_t io_encoding_no_decode_increment (io_encoding_t*,uint32_t);
size_t null_encoding_length (io_encoding_t const*);
int32_t null_encoding_limit (void);
size_t io_encoding_no_fill (io_encoding_t*,uint8_t,size_t);
bool io_encoding_no_grow (io_encoding_t*,uint32_t);
uint32_t null_encoding_grow_increment (io_encoding_t*);
void io_encoding_no_reset (io_encoding_t*);
size_t io_encoding_no_print (io_encoding_t*,char const*,va_list);
bool io_encoding_no_append_byte (io_encoding_t*,uint8_t);
bool io_encoding_no_append_bytes (io_encoding_t*,uint8_t const*,size_t);
bool io_encoding_no_pop_last_byte (io_encoding_t*,uint8_t*);

extern EVENT_DATA io_encoding_layer_api_t no_packet_layer_api;

#define SPECIALISE_IO_ENCODING_IMPLEMENTATION(S) \
	.specialisation_of = S, \
	.make_encoding = mk_null_encoding, \
	.free = free_null_encoding, \
	.get_io = null_encoding_get_io, \
	.get_byte_stream = io_encoding_no_byte_stream, \
	.get_content = io_encoding_no_content, \
	.decode_to_io_value = io_value_encoding_decode_to_io_value, \
	.increment_decode_offest = io_encoding_no_decode_increment, \
	.limit = null_encoding_limit, \
	.length = null_encoding_length, \
	.fill = io_encoding_no_fill, \
	.grow = io_encoding_no_grow, \
	.grow_increment = null_encoding_grow_increment, \
	.print = io_encoding_no_print, \
	.reset = io_encoding_no_reset, \
	.append_byte = io_encoding_no_append_byte,\
	.append_bytes = io_encoding_no_append_bytes, \
	.pop_last_byte = io_encoding_no_pop_last_byte, \
	.layer = &no_packet_layer_api,\
	/**/

//
// inline io_encoding methods
//
INLINE_FUNCTION io_encoding_t*
new_io_encoding (io_encoding_implementation_t const *I,io_byte_memory_t *bm) {
	return I->make_encoding (bm);
}

INLINE_FUNCTION io_t*
io_encoding_get_io (io_encoding_t *encoding) {
	return encoding->implementation->get_io (encoding);
}

INLINE_FUNCTION void*
io_encoding_get_byte_stream (io_encoding_t *encoding) {
	return encoding->implementation->get_byte_stream (encoding);
}

INLINE_FUNCTION void
io_encoding_get_content (io_encoding_t *encoding,uint8_t const **begin,uint8_t const **end) {
	encoding->implementation->get_content (encoding,begin,end);
}

INLINE_FUNCTION uint32_t
io_encoding_increment_decode_offest (io_encoding_t *encoding,uint32_t incr) {
	return encoding->implementation->increment_decode_offest (encoding,incr);
}

INLINE_FUNCTION vref_t
io_encoding_decode_to_io_value (io_encoding_t *encoding,io_value_decoder_t d,io_value_memory_t *vm) {
	return encoding->implementation->decode_to_io_value(encoding,d,vm);
}

INLINE_FUNCTION void
io_encoding_free (io_encoding_t *encoding) {
	encoding->implementation->free (encoding);
}

INLINE_FUNCTION void*
io_encoding_get_layer (io_encoding_t *encoding,io_layer_implementation_t const *L) {
	return encoding->implementation->layer->get_layer (encoding,L);
}

INLINE_FUNCTION void*
io_encoding_get_inner_layer (io_encoding_t *encoding,io_layer_t *L) {
	return encoding->implementation->layer->get_inner_layer (encoding,L);
}

INLINE_FUNCTION void*
io_encoding_get_innermost_layer (io_encoding_t *encoding) {
	return io_encoding_get_inner_layer (encoding,NULL);
}

INLINE_FUNCTION void*
io_encoding_get_outer_layer (io_encoding_t *encoding,io_layer_t *L) {
	return encoding->implementation->layer->get_outer_layer (encoding,L);
}

INLINE_FUNCTION void*
io_encoding_get_outermost_layer (io_encoding_t *encoding) {
	return io_encoding_get_layer (encoding,NULL);
}

INLINE_FUNCTION io_layer_t*
io_encoding_push_layer (io_encoding_t *encoding,io_make_layer_t make) {
	if (encoding != NULL) {
		return encoding->implementation->layer->push_layer (encoding,make);
	} else {
		return NULL;
	}
}

INLINE_FUNCTION size_t
io_encoding_limit (io_encoding_t *encoding) {
	return encoding->implementation->limit();
}

INLINE_FUNCTION uint32_t
io_encoding_get_grow_increment (io_encoding_t *encoding) {
	return encoding->implementation->grow_increment(encoding);
}

INLINE_FUNCTION bool
io_encoding_grow (io_encoding_t *encoding,uint32_t inc) {
	return encoding->implementation->grow(encoding,inc);
}

INLINE_FUNCTION size_t
io_encoding_length (io_encoding_t const *encoding) {
	return encoding->implementation->length(encoding);
}

INLINE_FUNCTION size_t
io_encoding_fill (io_encoding_t *encoding,uint8_t byte,size_t s) {
	return encoding->implementation->fill(encoding,byte,s);
}

INLINE_FUNCTION void
io_encoding_reset (io_encoding_t *encoding) {
	encoding->implementation->reset (encoding);
}

INLINE_FUNCTION size_t
io_encoding_print (io_encoding_t *encoding,char const *fmt,va_list va) {
	return encoding->implementation->print(encoding,fmt,va);
}


INLINE_FUNCTION bool
io_encoding_append_byte (io_encoding_t *encoding,uint8_t byte) {
	return encoding->implementation->append_byte (encoding,byte);
}

INLINE_FUNCTION bool
io_encoding_append_bytes (io_encoding_t *encoding,uint8_t const* bytes,size_t size) {
	return encoding->implementation->append_bytes (encoding,bytes,size);
}

INLINE_FUNCTION bool
io_encoding_append_string (io_encoding_t *encoding,char const* bytes,size_t size) {
		return encoding->implementation->append_bytes (encoding,(uint8_t const*)bytes,size);
}

INLINE_FUNCTION bool
io_encoding_pop_last_byte (io_encoding_t *encoding,uint8_t *byte) {
	return encoding->implementation->pop_last_byte (encoding,byte);
}

/*
 *
 * Binary Encoding
 *
 */

#define IO_BINARY_ENCODING_STRUCT_MEMBERS \
	IO_ENCODING_STRUCT_MEMBERS	\
	io_byte_memory_t *bm; \
	uint8_t *cursor; \
	uint8_t *byte_stream; \
	uint8_t *end; \
	/**/

typedef struct PACK_STRUCTURE {
	IO_BINARY_ENCODING_STRUCT_MEMBERS
} io_binary_encoding_t;

#define io_binary_encoding_byte_memory(this)			((this)->bm)
#define io_binary_encoding_data_size(this)			((this)->cursor - (this)->byte_stream)
#define io_binary_encoding_allocation_size(this)	((this)->end - (this)->byte_stream)

bool		is_io_binary_encoding (io_encoding_t const*);
size_t	io_encoding_printf (io_encoding_t*,const char*, ... );

void*		io_binary_encoding_initialise (io_binary_encoding_t*);
void		io_binary_encoding_free (io_encoding_t*);
void		io_binary_encoding_free_memory (io_binary_encoding_t*);
void		io_binary_encoding_reset (io_encoding_t*);
size_t	io_binary_encoding_fill_bytes (io_encoding_t*,uint8_t,size_t);
bool		io_binary_encoding_append_byte (io_encoding_t*,uint8_t);
bool		io_binary_encoding_append_bytes (io_encoding_t*,uint8_t const*,size_t);
size_t	io_binary_encoding_print (io_encoding_t*,char const*,va_list);
bool		io_binary_encoding_pop_last_byte (io_encoding_t*,uint8_t*);
vref_t	io_binary_encoding_decode_to_io_value (io_encoding_t*,io_value_decoder_t,io_value_memory_t*);
io_t*		io_binary_encoding_get_io (io_encoding_t*);
void*		io_binary_encoding_get_byte_stream (io_encoding_t*);
void		io_binary_encoding_get_content (io_encoding_t*,uint8_t const**,uint8_t const**);
bool		io_binary_encoding_grow (io_encoding_t*,uint32_t);
uint32_t	default_io_encoding_grow_increment (io_encoding_t*);
size_t	io_binary_encoding_length (io_encoding_t const*);
int32_t	io_binary_encoding_nolimit (void);

#define SPECIALISE_IO_BINARY_ENCODING_IMPLEMENTATION(S) \
	SPECIALISE_IO_ENCODING_IMPLEMENTATION (S)\
	.get_io = io_binary_encoding_get_io,\
	.length = io_binary_encoding_length,\
	.limit = io_binary_encoding_nolimit,\
	.grow = io_binary_encoding_grow,\
	.grow_increment = default_io_encoding_grow_increment,\
	.fill = io_binary_encoding_fill_bytes, \
	.append_byte = io_binary_encoding_append_byte, \
	.append_bytes = io_binary_encoding_append_bytes, \
	.pop_last_byte = io_binary_encoding_pop_last_byte, \
	.print = io_binary_encoding_print, \
	.reset = io_binary_encoding_reset, \
	.get_byte_stream = io_binary_encoding_get_byte_stream, \
	.get_content = io_binary_encoding_get_content, \
	/**/
	
extern EVENT_DATA io_encoding_implementation_t io_binary_encoding_implementation;

//
// text encoding: a human readable encoding of values
//
typedef int32_t io_character_t;

typedef bool (*io_character_iterator_t) (io_character_t,void*);

typedef struct PACK_STRUCTURE {
	IO_BINARY_ENCODING_STRUCT_MEMBERS
	vref_hash_table_t *visited;
} io_text_encoding_t;

extern EVENT_DATA io_encoding_implementation_t io_text_encoding_implementation;

bool	is_io_text_encoding (io_encoding_t const*);
bool	io_text_encoding_iterate_characters (io_encoding_t*,io_character_iterator_t,void*);
vref_hash_table_t* io_text_encoding_get_visited (io_text_encoding_t*);

INLINE_FUNCTION io_encoding_t*
mk_io_text_encoding (io_byte_memory_t *bm) {
	extern EVENT_DATA io_encoding_implementation_t io_text_encoding_implementation;
	return io_text_encoding_implementation.make_encoding(bm);
}

INLINE_FUNCTION io_encoding_t*
mk_io_x70_encoding (io_byte_memory_t *bm) {
	extern EVENT_DATA io_encoding_implementation_t io_x70_encoding_implementation;
	return io_x70_encoding_implementation.make_encoding(bm);
}

INLINE_FUNCTION bool
is_io_x70_encoding (io_encoding_t const *encoding) {
	extern EVENT_DATA io_encoding_implementation_t io_x70_encoding_implementation;
	return io_encoding_has_implementation (
		encoding,&io_x70_encoding_implementation
	);
}

int32_t io_x70_encoding_take_uint_value (const uint8_t*,const uint8_t*,uint32_t*);
vref_t io_x70_decoder (io_encoding_t*,io_value_memory_t*);
void io_x70_encoding_append_uint_value (io_encoding_t*,uint32_t);

#define X70_UINT_VALUE_BYTE	'U'

//
// int64 encoding
//
typedef struct PACK_STRUCTURE io_value_int64_encoding {
	IO_ENCODING_STRUCT_MEMBERS
	int64_t encoded_value;
} io_value_int64_encoding_t;

#define io_value_int64_encoding_encoded_value(e) (e)->encoded_value

bool encoding_is_io_value_int64 (io_encoding_t*);

extern EVENT_DATA io_encoding_implementation_t io_value_int64_encoding_implementation;
#define def_int64_encoding(VALUE) (io_value_int64_encoding_t) {\
		.implementation = &io_value_int64_encoding_implementation,	\
		.encoded_value = VALUE,		\
	}

typedef struct PACK_STRUCTURE io_value_float64_encoding {
	IO_ENCODING_STRUCT_MEMBERS
	float64_t encoded_value;
} io_value_float64_encoding_t;

bool encoding_is_io_value_float64 (io_encoding_t*);

#define io_value_float64_encoding_encoded_value(e) (e)->encoded_value

extern EVENT_DATA io_encoding_implementation_t io_value_float64_encoding_implementation;
#define def_float64_encoding(VALUE) (io_value_float64_encoding_t) {\
		.implementation = &io_value_float64_encoding_implementation,	\
		.encoded_value = VALUE,		\
	}

#define IO_MESSAGE_ENCODING_STRUCT_MEMBERS \
	IO_BINARY_ENCODING_STRUCT_MEMBERS \
	uint32_t *stack;\
	/**/

typedef struct PACK_STRUCTURE io_message_encoding {
	IO_MESSAGE_ENCODING_STRUCT_MEMBERS
} io_message_encoding_t;


extern EVENT_DATA io_value_reference_implementation_t reference_to_c_stack_value;

/*
 *
 * Value memory
 *
 */
typedef struct PACK_STRUCTURE io_value_memory_implementation {
	void (*free) (io_value_memory_t*);
	vref_t (*allocate_value) (io_value_memory_t*,io_value_implementation_t const*,size_t);
	vref_t (*new_value) (io_value_memory_t*,io_value_implementation_t const*,size_t,vref_t);

	void (*do_gc) (io_value_memory_t*,int32_t);
	void (*get_info) (io_value_memory_t*,memory_info_t*);
	io_t* (*get_io) (io_value_memory_t*);
	bool (*is_persistant) (io_value_memory_t*);

	void const* (*get_value_ro_pointer) (io_value_memory_t*,vref_t);
	void * (*get_value_rw_pointer) (io_value_memory_t*,vref_t);
} io_value_memory_implementation_t;

#define IO_VALUE_MEMORY_STRUCT_MEMBERS \
	io_value_memory_implementation_t const *implementation;\
	uint32_t id_;\
	/**/

struct PACK_STRUCTURE io_value_memory {
	IO_VALUE_MEMORY_STRUCT_MEMBERS
};

#define io_value_memory_id(vm)	(vm)->id_
bool	register_io_value_memory (io_value_memory_t*);

//
// inline value memory implementation
//
INLINE_FUNCTION void
free_io_value_memory (io_value_memory_t *mem) {
	return mem->implementation->free(mem);
}

INLINE_FUNCTION vref_t
io_value_memory_new_value (
	io_value_memory_t *mem,io_value_implementation_t const *I,size_t size,vref_t r_base
) {
	return mem->implementation->new_value(mem,I,size,r_base);
}

INLINE_FUNCTION void
io_value_memory_get_info (io_value_memory_t *mem,memory_info_t *info) {
	mem->implementation->get_info(mem,info);
}

INLINE_FUNCTION void
io_value_memory_do_gc (io_value_memory_t *vm,int32_t count) {
	vm->implementation->do_gc(vm,count);
}

INLINE_FUNCTION io_t*
io_value_memory_get_io (io_value_memory_t *vm) {
	return vm->implementation->get_io(vm);
}

io_value_memory_t*	io_get_value_memory_by_id (uint32_t);

typedef struct PACK_STRUCTURE umm_io_value_memory {
	IO_VALUE_MEMORY_STRUCT_MEMBERS
	io_byte_memory_t *bm;
	io_t *io;

	uint16_t		gc_cursor;
	uint16_t		gc_stack_size;

} umm_io_value_memory_t;

extern EVENT_DATA io_value_memory_implementation_t umm_value_memory_implementation;

#define INVALID_MEMORY_ID	0x10000
io_value_memory_t* mk_umm_io_value_memory (io_t*,uint32_t,uint32_t);
void umm_value_memory_free_memory (io_value_memory_t*);

/*
 *-----------------------------------------------------------------------------
 *
 * power and clocks
 *
 *-----------------------------------------------------------------------------
 */
//
// power domain
//
typedef struct io_cpu_power_domain io_cpu_power_domain_t;
typedef struct io_cpu_power_domain_pointer io_cpu_power_domain_pointer_t;

typedef struct io_cpu_power_domain_pointer_implementation {
	io_cpu_power_domain_t const* (*get_as_read_only) (io_cpu_power_domain_pointer_t);
	io_cpu_power_domain_t* (*get_as_read_write) (io_cpu_power_domain_pointer_t);
} io_cpu_power_domain_pointer_implementation_t;

struct io_cpu_power_domain_pointer {
	io_cpu_power_domain_pointer_implementation_t const *implementation;
	union {
		io_cpu_power_domain_t const *ro;
		io_cpu_power_domain_t *rw;
	} ptr;
};

//
// inline io power domain pointer implementation
//
INLINE_FUNCTION io_cpu_power_domain_t const*
io_cpu_power_domain_ro_pointer (io_cpu_power_domain_pointer_t p) {
	return p.implementation->get_as_read_only(p);
}

INLINE_FUNCTION io_cpu_power_domain_t*
io_cpu_power_domain_rw_pointer (io_cpu_power_domain_pointer_t p) {
	return p.implementation->get_as_read_write(p);
}

extern EVENT_DATA io_cpu_power_domain_pointer_implementation_t 
read_only_power_domain_implementation;

#define def_io_cpu_power_domain_pointer(C)	((io_cpu_power_domain_pointer_t) {\
																.implementation = &read_only_power_domain_implementation,\
																.ptr.ro = (io_cpu_power_domain_t const*) (C)}\
															)

#define io_cpu_power_domain_pointer_ro(p) 	(p).ptr.ro
#define io_cpu_power_domain_pointer_rw(p) 	(p).ptr.rw
#define io_cpu_power_domain_is_valid(p) 		((p).ptr.ro != NULL)

typedef struct PACK_STRUCTURE io_cpu_power_domain_implementation {
	struct io_cpu_power_domain_implementation const *specialisation_of;
	void (*turn_off) (io_t*,io_cpu_power_domain_pointer_t);
	void (*turn_on) (io_t*,io_cpu_power_domain_pointer_t);
} io_cpu_power_domain_implementation_t;

#define IO_CPU_POWER_DOMAIN_STRUCT_MEMBERS \
	io_cpu_power_domain_implementation_t const *implementation;\
	uint32_t reference_count;	\
	/**/

struct PACK_STRUCTURE io_cpu_power_domain {
    IO_CPU_POWER_DOMAIN_STRUCT_MEMBERS
};

void io_power_domain_no_operation (io_t *io,io_cpu_power_domain_pointer_t);
extern EVENT_DATA io_cpu_power_domain_t always_on_io_power_domain;

#define NULL_IO_POWER_DOMAIN						((io_cpu_power_domain_pointer_t){NULL})

//
// inline io power domain implementation
//
INLINE_FUNCTION void
turn_on_io_power_domain (io_t* io,io_cpu_power_domain_pointer_t pd) {
    return io_cpu_power_domain_ro_pointer(pd)->implementation->turn_on(io,pd);
}

INLINE_FUNCTION void
turn_off_io_power_domain (io_t* io,io_cpu_power_domain_pointer_t pd) {
    return io_cpu_power_domain_ro_pointer(pd)->implementation->turn_off(io,pd);
}

//
// clock
//
typedef struct io_cpu_clock io_cpu_clock_t;
typedef union io_cpu_clock_pointer io_cpu_clock_pointer_t;

typedef struct io_cpu_clock_pointer_implementation {
	io_cpu_clock_t const* (*get_as_read_only) (io_cpu_clock_pointer_t);
	io_cpu_clock_t* (*get_as_read_write) (io_cpu_clock_pointer_t);
} io_cpu_clock_pointer_implementation_t;

union io_cpu_clock_pointer {
	io_cpu_clock_t const *ro;
	io_cpu_clock_t *rw;
};

typedef bool (*io_cpu_clock_iterator_t) (io_cpu_clock_pointer_t,void*);

typedef struct io_cpu_clock_implementation io_cpu_clock_implementation_t;

#define IO_CPU_CLOCK_IMPLEMENTATION_STRUCT_MEMBERS \
	io_cpu_clock_implementation_t const *specialisation_of; \
	float64_t (*get_current_frequency) (io_cpu_clock_pointer_t); \
	float64_t (*get_expected_frequency) (io_cpu_clock_pointer_t); \
	io_cpu_clock_pointer_t (*get_input) (io_cpu_clock_pointer_t); \
	io_cpu_power_domain_pointer_t (*get_power_domain) (io_cpu_clock_pointer_t); \
	bool (*start) (io_t*,io_cpu_clock_pointer_t); \
	void (*stop) (io_t*,io_cpu_clock_pointer_t); \
	bool (*iterate_outputs) (io_cpu_clock_pointer_t,io_cpu_clock_iterator_t,void*);\
	/**/

struct PACK_STRUCTURE io_cpu_clock_implementation {
	IO_CPU_CLOCK_IMPLEMENTATION_STRUCT_MEMBERS
};

#define IO_CPU_CLOCK_STRUCT_MEMBERS \
	io_cpu_clock_implementation_t const *implementation; \
	/**/

struct PACK_STRUCTURE io_cpu_clock {
	IO_CPU_CLOCK_STRUCT_MEMBERS
};

#define NULL_IO_CLOCK						((io_cpu_clock_pointer_t){NULL})
#define IO_CPU_CLOCK(C)						((io_cpu_clock_pointer_t){(io_cpu_clock_t const*) (C)})
#define decl_io_cpu_clock_pointer(C)	{(io_cpu_clock_t const*) (C)}
#define def_io_cpu_clock_pointer(C)		((io_cpu_clock_pointer_t){(io_cpu_clock_t const*) (C)})

#define io_cpu_clock_ro_pointer(c) 		(c).ro
#define io_cpu_clock_is_null(c) 			(io_cpu_clock_ro_pointer(c) == NULL)

void	io_cpu_clock_stop (io_t*,io_cpu_clock_pointer_t);
bool	io_cpu_clock_has_implementation (io_cpu_clock_pointer_t,io_cpu_clock_implementation_t const*);
io_cpu_clock_pointer_t io_cpu_clock_get_input_nop (io_cpu_clock_pointer_t);
io_cpu_power_domain_pointer_t get_always_on_io_power_domain (io_cpu_clock_pointer_t);
bool	io_cpu_clock_is_derrived_from (io_cpu_clock_pointer_t,io_cpu_clock_implementation_t const*);

//
// inline clock implementation
//
INLINE_FUNCTION float64_t
io_cpu_clock_get_current_frequency (io_cpu_clock_pointer_t c) {
	return io_cpu_clock_ro_pointer(c)->implementation->get_current_frequency(c);
}

INLINE_FUNCTION float64_t
io_cpu_clock_get_expected_frequency (io_cpu_clock_pointer_t c) {
	return io_cpu_clock_ro_pointer(c)->implementation->get_expected_frequency(c);
}

INLINE_FUNCTION io_cpu_power_domain_pointer_t
io_cpu_clock_power_domain (io_cpu_clock_pointer_t c) {
	return io_cpu_clock_ro_pointer(c)->implementation->get_power_domain(c);
}

INLINE_FUNCTION bool
io_cpu_clock_start (io_t *io,io_cpu_clock_pointer_t c) {
	return io_cpu_clock_ro_pointer(c)->implementation->start(io,c);
}

INLINE_FUNCTION bool
io_cpu_clock_iterate_outputs (io_cpu_clock_pointer_t clock ,io_cpu_clock_iterator_t cb,void *uv) {
	return io_cpu_clock_ro_pointer(clock)->implementation->iterate_outputs(clock,cb,uv);
}

INLINE_FUNCTION io_cpu_clock_pointer_t
io_cpu_clock_get_input (io_cpu_clock_pointer_t clock) {
	return io_cpu_clock_ro_pointer(clock)->implementation->get_input(clock);
}

bool io_cpu_clock_always_on_start (io_t*,io_cpu_clock_pointer_t);
void io_cpu_clock_always_on_stop (io_t*,io_cpu_clock_pointer_t);
float64_t io_cpu_clock_no_frequency (io_cpu_clock_pointer_t);
io_cpu_clock_pointer_t io_cpu_clock_no_input (io_cpu_clock_pointer_t);
io_cpu_power_domain_pointer_t io_cpu_clock_no_power_domain (io_cpu_clock_pointer_t);
bool io_cpu_clock_iterate_no_outputs (io_cpu_clock_pointer_t,io_cpu_clock_iterator_t,void*);

#define SPECIALISE_IO_CPU_CLOCK_IMPLEMENTATION(S) \
	.specialisation_of = S, \
	.get_current_frequency = io_cpu_clock_no_frequency, \
	.get_expected_frequency = io_cpu_clock_no_frequency, \
	.get_input = io_cpu_clock_no_input, \
	.get_power_domain = get_always_on_io_power_domain, \
	.start = io_cpu_clock_always_on_start, \
	.stop = io_cpu_clock_always_on_stop, \
	.iterate_outputs = io_cpu_clock_iterate_no_outputs, \
	/**/

extern EVENT_DATA io_cpu_clock_implementation_t io_cpu_clock_implementation;

#define IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS \
	IO_CPU_CLOCK_STRUCT_MEMBERS					\
	io_cpu_clock_pointer_t const *outputs;		\
	/**/

typedef struct PACK_STRUCTURE io_cpu_clock_source {
	IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS
} io_cpu_clock_source_t;

#define SPECIALISE_IO_CPU_CLOCK_SOURCE_IMPLEMENTATION(S) \
	SPECIALISE_IO_CPU_CLOCK_IMPLEMENTATION(S) \
	/**/

extern EVENT_DATA io_cpu_clock_implementation_t io_cpu_clock_source_implementation;


#define IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS \
	IO_CPU_CLOCK_STRUCT_MEMBERS		\
	io_cpu_clock_pointer_t input;	\
	/**/

typedef struct PACK_STRUCTURE io_cpu_dependant_clock {
	IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
} io_cpu_dependant_clock_t;

bool	io_cpu_dependant_clock_start_input (io_t*,io_cpu_clock_pointer_t);
io_cpu_clock_pointer_t io_cpu_dependant_clock_get_input (io_cpu_clock_pointer_t);
bool io_dependant_cpu_clock_start (io_t*,io_cpu_clock_pointer_t);
float64_t io_dependant_cpu_clock_get_current_frequency (io_cpu_clock_pointer_t);
float64_t io_dependant_cpu_clock_get_expected_frequency (io_cpu_clock_pointer_t);

extern EVENT_DATA io_cpu_clock_implementation_t io_dependent_clock_implementation;

#define SPECIALISE_DEPENDANT_IO_CPU_CLOCK_IMPLEMENTATION(S) \
	SPECIALISE_IO_CPU_CLOCK_IMPLEMENTATION(S) \
	.get_current_frequency = io_dependant_cpu_clock_get_current_frequency, \
	.get_expected_frequency = io_dependant_cpu_clock_get_expected_frequency, \
	.get_input = io_cpu_dependant_clock_get_input, \
	.start = io_dependant_cpu_clock_start, \
	/**/

#define IO_CPU_CLOCK_FUNCTION_STRUCT_MEMBERS \
	IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS\
	io_cpu_clock_pointer_t const *outputs;\
	/**/

typedef struct PACK_STRUCTURE io_cpu_clock_function {
	IO_CPU_CLOCK_FUNCTION_STRUCT_MEMBERS
} io_cpu_clock_function_t;

bool	io_cpu_clock_function_iterate_outputs (io_cpu_clock_pointer_t,bool (*) (io_cpu_clock_pointer_t,void*),void*);
bool	io_cpu_clock_iterate_outputs_nop (io_cpu_clock_pointer_t,bool (*) (io_cpu_clock_pointer_t,void*),void*);

#define SPECIALISE_IO_CPU_CLOCK_FUNCTION_IMPLEMENTATION(S) \
	SPECIALISE_DEPENDANT_IO_CPU_CLOCK_IMPLEMENTATION(S) \
	/**/

extern EVENT_DATA io_cpu_clock_implementation_t io_cpu_clock_function_implementation;

//
// address
//
#include <io_address.h>

typedef struct PACK_STRUCTURE io_address {
	struct PACK_STRUCTURE {
		uint32_t size:31;
		uint32_t is_volatile:1;
	} tag;
	union PACK_STRUCTURE {
		uint32_t u32;
		uint16_t u16;
		uint8_t u8;
		uint8_t *rw_bytes;
		uint8_t const *ro_bytes;
		uint8_t m8[4];
	} value;
} io_address_t;

#define IO_ADDRESS_INVALID_SIZE	0

#define io_address_size(a)				(a).tag.size
#define io_address_is_volatile(a)	(a).tag.is_volatile
#define io_address_rw_bytes(a)		(a).value.rw_bytes
#define io_address_ro_bytes(a)		(a).value.ro_bytes
#define get_pointer_to_io_address_value(a)	((io_address_size(a) > 4) ? io_address_rw_bytes(a) : &io_u8_address_value(a))

#define io_invalid_address()			(io_address_t) {.tag.size = IO_ADDRESS_INVALID_SIZE,.tag.is_volatile = 0,}
#define is_invalid_io_address(a)		(io_address_size(a) == IO_ADDRESS_INVALID_SIZE)
#define io_address_is_invalid(a) 	(io_address_size(a) == IO_ADDRESS_INVALID_SIZE)
#define io_address_is_valid(a) 		(io_address_size(a) != IO_ADDRESS_INVALID_SIZE)

#define def_io_u8_address(a)			(io_address_t) {.tag.size = 1,.tag.is_volatile = 0,.value.u8 = a,}
#define def_io_u16_address(a)			(io_address_t) {.tag.size = 2,.tag.is_volatile = 0,.value.u16 = a,}
#define def_io_u32_address(a)			(io_address_t) {.tag.size = 4,.tag.is_volatile = 0,.value.u32 = a,}
#define def_io_const_address(s,ro)	(io_address_t) {.tag.size = s,.tag.is_volatile = 0,.value.ro_bytes = ro,}

#define io_u8_address_value(a)	(a).value.u8
#define io_u16_address_value(a)	(a).value.u16
#define io_u32_address_value(a)	(a).value.u32

io_address_t	mk_io_address(io_byte_memory_t*,uint32_t,uint8_t const*);
int32_t			compare_io_addresses (io_address_t,io_address_t);
io_address_t	duplicate_io_address (io_byte_memory_t*,io_address_t);
uint32_t			write_le_io_address (uint8_t*,uint32_t,io_address_t);
uint32_t			read_le_io_address (io_byte_memory_t*,uint8_t const*,uint32_t,io_address_t*);

INLINE_FUNCTION io_address_t
io_long_address (io_byte_memory_t *bm,uint32_t size,uint8_t const *bytes) {
	io_address_t a = {
		.tag.size = size,
		.tag.is_volatile = 1,
		.value.rw_bytes = io_byte_memory_allocate (bm,size)
	};
	memcpy (io_address_rw_bytes(a),bytes,size);
	return a;
}

INLINE_FUNCTION void
free_io_address (io_byte_memory_t *bm,io_address_t a) {
	if (io_address_size(a) > 4 && io_address_is_volatile(a)) {
		io_byte_memory_free (bm,io_address_rw_bytes(a));
	}
}

INLINE_FUNCTION void
assign_io_address (io_byte_memory_t *bm,io_address_t *location,io_address_t new_address) {
	free_io_address (bm,*location);
	*location = duplicate_io_address (bm,new_address);
}

//
// cpu io pins
//
typedef uint32_t io_pin_t;

#define IO_PIN_INACTIVE				0
#define IO_PIN_ACTIVE				1

//
// interrupts
//
typedef void (*io_interrupt_action_t) (void*);

typedef struct io_interrupt_handler io_interrupt_handler_t;

struct io_interrupt_handler {
	io_interrupt_action_t action;
	void *user_value;
};

INLINE_FUNCTION io_interrupt_handler_t*
initialise_io_interrupt_handler (
	io_interrupt_handler_t *ir,io_interrupt_action_t fn,void* user_value
) {
	ir->action = fn;
	ir->user_value = user_value;
	return ir;
}

//
// Io
//
// the model for io computation is a single cpu core per io_t instance
//
typedef struct PACK_STRUCTURE io_implementation {
	string_hash_table_t *value_implementation_map;
	//
	// core resources
	//
	io_byte_memory_t* (*get_byte_memory) (io_t*);
	io_value_memory_t* (*get_short_term_value_memory) (io_t*);
	io_value_memory_t* (*get_long_term_value_memory) (io_t*);
	void (*do_gc) (io_t*,int32_t);
	io_cpu_clock_pointer_t (*get_core_clock) (io_t*);
	bool (*is_first_run) (io_t*);
	bool (*clear_first_run) (io_t*);
	void (*get_stack_usage_info) (io_t*,memory_info_t*);

	//
	// identity and security
	//
	io_uid_t const* (*uid) (io_t*);
	bool (*get_shared_key) (io_t*,io_uid_t const*,io_authentication_key_t*);
	uint32_t (*get_random_u32) (io_t*);
	uint32_t (*get_next_prbs_u32) (io_t*);

	void (*sha256_start) (io_sha256_context_t*);
	void (*sha256_update) (io_sha256_context_t*,uint8_t const*,uint32_t);
	void (*sha256_finish) (io_sha256_context_t*,uint8_t[32]);
	
	//
	// communication
	//
	io_socket_t* (*get_socket) (io_t*,int32_t);
	//
	// tasks
	//
	bool (*enqueue_task) (io_t*,vref_t);
	void (*signal_task_pending) (io_t*);
	bool (*do_next_task) (io_t*);
	//
	// events
	//
	void (*dequeue_event) (io_t*,io_event_t*);
	void (*enqueue_event) (io_t*,io_event_t*);
	bool (*next_event) (io_t*);
	bool (*in_event_thread) (io_t*);
	void (*signal_event_pending) (io_t*);
	void (*wait_for_event) (io_t*);
	void (*wait_for_all_events) (io_t*);
	//
	// time
	//
	io_time_t (*get_time) (io_t*);
	void (*enqueue_alarm) (io_t*,io_alarm_t*);
	void (*dequeue_alarm) (io_t*,io_alarm_t*);
	//
	// interrupts
	//
	bool (*enter_critical_section) (io_t*);
	void (*exit_critical_section) (io_t*,bool);
	void (*register_interrupt_handler) (io_t*,int32_t,io_interrupt_action_t,void*);
	bool (*unregister_interrupt_handler) (io_t*,int32_t,io_interrupt_action_t);
	//
	// io pins
	//
	void (*set_io_pin_output) (io_t*,io_pin_t);
	void (*set_io_pin_input) (io_t*,io_pin_t);
	void (*set_io_pin_interrupt) (io_t*,io_pin_t,io_interrupt_handler_t*);
	void (*set_io_pin_alternate) (io_t*,io_pin_t);
	int32_t (*read_from_io_pin) (io_t*,io_pin_t);
	void (*write_to_io_pin) (io_t*,io_pin_t,int32_t);
	void (*toggle_io_pin) (io_t*,io_pin_t);
	bool (*valid_pin) (io_t*,io_pin_t);
	void (*release_io_pin) (io_t*,io_pin_t);
	//
	// external notifications
	//
	void (*log) (io_t*,char const*,va_list);
	void (*flush_log) (io_t*);
	void (*panic) (io_t*,int);
	//
} io_implementation_t;

void add_io_implementation_core_methods (io_implementation_t*);
void add_io_implementation_cpu_methods (io_implementation_t*);
void add_io_implementation_board_methods (io_implementation_t*);
void add_io_implementation_device_methods (io_implementation_t*);
bool add_core_value_implementations_to_hash (string_hash_table_t*);

typedef enum {
	IO_LOG_LEVEL_NO_LOGGING = 0,
	IO_ERROR_LOG_LEVEL,
	IO_WARNING_LOG_LEVEL,
	IO_INFO_LOG_LEVEL,
	IO_DETAIL_LOG_LEVEL,
} io_log_level_t;

#define IO_STRUCT_MEMBERS \
	io_implementation_t const *implementation;\
	io_event_t *events; \
	io_alarm_t *alarms; \
	uint32_t log_level;\
	/**/

struct PACK_STRUCTURE io {
	IO_STRUCT_MEMBERS
};

void	enqueue_io_event (io_t*,io_event_t*);
void	dequeue_io_event (io_t*,io_event_t*);
bool	do_next_io_event (io_t*);
void io_log_startup_message (io_t*,io_log_level_t);

int io_printf (io_t*,const char *fmt,...);
void io_log (io_t*,io_log_level_t,const char *fmt,...);
void flush_io_log (io_t*);
void initialise_io (io_t*,io_implementation_t const*);

//
// inline io implementation
//
INLINE_FUNCTION io_byte_memory_t*
io_get_byte_memory (io_t *io) {
	return io->implementation->get_byte_memory(io);
}

INLINE_FUNCTION io_value_memory_t*
io_get_short_term_value_memory (io_t *io) {
	return io->implementation->get_short_term_value_memory(io);
}

INLINE_FUNCTION io_value_memory_t*
io_get_long_term_value_memory (io_t *io) {
	return io->implementation->get_long_term_value_memory(io);
}

INLINE_FUNCTION io_value_implementation_t const*
io_get_value_implementation (io_t *io,const char *bytes,uint32_t size) {
	string_hash_table_mapping_t v;
	if (
		string_hash_table_map (
			io->implementation->value_implementation_map,bytes,size,&v
		)
	) {
		return v.ro_ptr;
	} else {
		return NULL;
	}
}

INLINE_FUNCTION uint32_t
io_get_random_u32 (io_t *io) {
	return io->implementation->get_random_u32(io);
}

INLINE_FUNCTION uint32_t
io_get_next_prbs_u32 (io_t *io) {
	return io->implementation->get_next_prbs_u32(io);
}

INLINE_FUNCTION void
io_log_message (io_t *io,char const *fmt,va_list va) {
	io->implementation->log(io,fmt,va);
}

INLINE_FUNCTION void
io_flush_log (io_t *io) {
	io->implementation->flush_log(io);
}

INLINE_FUNCTION void
io_panic (io_t *io,int code) {
	io->implementation->panic(io,code);
}

INLINE_FUNCTION void
io_wait_for_all_events (io_t *io) {
	io->implementation->wait_for_all_events (io);
}

INLINE_FUNCTION void
io_wait_for_event (io_t *io) {
	io->implementation->wait_for_event (io);
}

INLINE_FUNCTION bool
enter_io_critical_section (io_t *io) {
	return io->implementation->enter_critical_section (io);
}

INLINE_FUNCTION void
exit_io_critical_section (io_t *io,bool h) {
	io->implementation->exit_critical_section (io,h);
}

INLINE_FUNCTION void
register_io_interrupt_handler (io_t *io,int32_t h,io_interrupt_action_t a,void *u) {
	io->implementation->register_interrupt_handler (io,h,a,u);
}

INLINE_FUNCTION bool
unregister_io_interrupt_handler (io_t *io,int32_t h,io_interrupt_action_t a) {
	return io->implementation->unregister_interrupt_handler (io,h,a);
}

INLINE_FUNCTION void
signal_io_task_pending (io_t *io) {
	io->implementation->signal_task_pending (io);
}

INLINE_FUNCTION bool
io_enqueue_task (io_t *io,vref_t r_task) {
	return io->implementation->enqueue_task (io,r_task);
}

INLINE_FUNCTION bool
io_do_next_task (io_t *io) {
	return io->implementation->do_next_task (io);
}

INLINE_FUNCTION void
io_dequeue_event (io_t *io,io_event_t *ev) {
	io->implementation->dequeue_event (io,ev);
}

INLINE_FUNCTION void
signal_io_event_pending (io_t *io) {
	io->implementation->signal_event_pending (io);
}

INLINE_FUNCTION bool
next_io_event (io_t *io) {
	return io->implementation->next_event (io);
}

INLINE_FUNCTION void
io_do_gc (io_t *io,int32_t c) {
	io->implementation->do_gc (io,c);
}

INLINE_FUNCTION io_cpu_clock_pointer_t
io_get_core_clock (io_t *io) {
	return io->implementation->get_core_clock (io);
}

INLINE_FUNCTION bool
io_is_first_run (io_t *io) {
	return io->implementation->is_first_run (io);
}

INLINE_FUNCTION bool
io_clear_first_run (io_t *io) {
	return io->implementation->clear_first_run (io);
}

INLINE_FUNCTION void
io_get_stack_usage_info (io_t *io,memory_info_t *info) {
	io->implementation->get_stack_usage_info (io,info);
}

INLINE_FUNCTION io_uid_t const*
io_uid (io_t *io) {
	return io->implementation->uid(io);
}

INLINE_FUNCTION void
io_get_uid (io_t *io,io_uid_t *uid) {
	memcpy(uid->bytes,io_uid(io)->bytes,IO_UID_BYTE_LENGTH);
}

INLINE_FUNCTION bool
io_get_shared_key (io_t *io,io_uid_t const *uid,io_authentication_key_t *key) {
	return io->implementation->get_shared_key (io,uid,key);
}

INLINE_FUNCTION io_socket_t*
io_get_socket (io_t *io,int32_t s) {
	return io->implementation->get_socket (io,s);
}

INLINE_FUNCTION void
io_set_pin_to_output (io_t *io,io_pin_t p) {
	io->implementation->set_io_pin_output (io,p);
}

INLINE_FUNCTION void
io_set_pin_to_input (io_t *io,io_pin_t p) {
	io->implementation->set_io_pin_input (io,p);
}

INLINE_FUNCTION void
io_set_pin_to_alternate (io_t *io,io_pin_t p) {
	io->implementation->set_io_pin_alternate (io,p);
}

INLINE_FUNCTION void
io_set_pin_to_interrupt (io_t *io,io_pin_t p,io_interrupt_handler_t *h) {
	io->implementation->set_io_pin_interrupt (io,p,h);
}

INLINE_FUNCTION void
write_to_io_pin (io_t *io,io_pin_t p,int32_t s) {
	io->implementation->write_to_io_pin (io,p,s);
}

INLINE_FUNCTION void
toggle_io_pin (io_t *io,io_pin_t p) {
	io->implementation->toggle_io_pin (io,p);
}

INLINE_FUNCTION int32_t
io_read_pin (io_t *io,io_pin_t p) {
	return io->implementation->read_from_io_pin (io,p);
}

INLINE_FUNCTION bool
io_pin_is_valid (io_t *io,io_pin_t p) {
	return io->implementation->valid_pin (io,p);
}

INLINE_FUNCTION void
release_io_pin (io_t *io,io_pin_t p) {
	io->implementation->release_io_pin (io,p);
}

INLINE_FUNCTION bool
io_is_in_event_thread (io_t *io) {
	return io->implementation->in_event_thread (io);
}

INLINE_FUNCTION io_time_t
io_get_time (io_t *io) {
	return io->implementation->get_time (io);
}

INLINE_FUNCTION void
io_enqueue_alarm (io_t *io,io_alarm_t *a) {
	io->implementation->enqueue_alarm (io,a);
}

INLINE_FUNCTION void
io_dequeue_alarm (io_t *io,io_alarm_t *a) {
	io->implementation->dequeue_alarm (io,a);
}

INLINE_FUNCTION void
io_sha256_start (io_t *io,io_sha256_context_t *ctx) {
	io->implementation->sha256_start (ctx);
}

INLINE_FUNCTION void
io_sha256_update (io_t *io,io_sha256_context_t *ctx,uint8_t const *data,uint32_t size) {
	io->implementation->sha256_update (ctx,data,size);
}

INLINE_FUNCTION void
io_sha256_finish (io_t *io,io_sha256_context_t *ctx,uint8_t output[SHA256_SIZE]) {
	io->implementation->sha256_finish (ctx,output);
}

INLINE_FUNCTION void
set_alarm_delay_time (io_t *io,io_alarm_t *alarm,io_time_t delay) {
	io_time_t t = io_get_time (io);
	alarm->when = (io_time_t) {t.ns + delay.ns};
}

//
// declarations for base implementation and specialisation macro
//
io_byte_memory_t* io_core_get_null_byte_memory (io_t*);
io_socket_t* io_core_get_null_socket (io_t*,int32_t);
io_value_memory_t* io_core_get_null_value_memory (io_t*);
void io_no_stack_usage_info (io_t*,memory_info_t*);
void io_pin_nop (io_t*,io_pin_t);
void io_pin_interrupt_nop (io_t*,io_pin_t,io_interrupt_handler_t*);
int32_t read_from_io_pin_nop (io_t*,io_pin_t);
void write_to_io_pin_nop (io_t*,io_pin_t,int32_t);
bool io_pin_is_always_invalid (io_t*,io_pin_t);
void io_cpu_sha256_start (io_sha256_context_t*);
void io_cpu_sha256_update (io_sha256_context_t*,uint8_t const*,uint32_t);
void io_cpu_sha256_finish (io_sha256_context_t*,uint8_t[32]);
void io_no_gc (io_t*,int32_t);
io_cpu_clock_pointer_t io_no_core_clock (io_t*);
bool io_never_first_run (io_t*);
io_uid_t const* io_no_uid (io_t*);
bool io_never_get_shared_key (io_t*,io_uid_t const*,io_authentication_key_t*);
uint32_t io_no_random_u32 (io_t*);
bool io_enqueue_task_base (io_t*,vref_t);
void io_signal_task_pending (io_t*);
bool io_do_next_task_base (io_t*);
void io_no_signal_event_pending (io_t*);
bool in_not_in_event_thread (io_t*);
void io_no_wait_for_event_pending (io_t*);
io_time_t io_get_time_zero (io_t*);
void io_no_enqueue_alarm (io_t*,io_alarm_t*);
bool io_no_enter_critical_section (io_t*);
void io_no_exit_critical_section (io_t*,bool);
void io_no_register_interrupt_handler (io_t*,int32_t,io_interrupt_action_t,void*);
bool io_no_unregister_interrupt_handler (io_t*,int32_t,io_interrupt_action_t);
void io_no_log (io_t*,char const*,va_list);
void io_no_log_flush (io_t*);
void io_default_panic (io_t*,int);

#define SPECIALISE_IO_IMPLEMENTATION(S) \
	.value_implementation_map = NULL, \
	.get_byte_memory = io_core_get_null_byte_memory, \
	.get_short_term_value_memory = io_core_get_null_value_memory, \
	.get_long_term_value_memory = io_core_get_null_value_memory, \
	.get_stack_usage_info = io_no_stack_usage_info,\
	.do_gc = io_no_gc, \
	.get_core_clock = io_no_core_clock, \
	.is_first_run = io_never_first_run, \
	.clear_first_run = io_never_first_run,\
	.uid = io_no_uid, \
	.get_shared_key = io_never_get_shared_key, \
	.get_random_u32 = io_no_random_u32, \
	.get_next_prbs_u32 = io_no_random_u32, \
	.sha256_start = io_cpu_sha256_start, \
	.sha256_update = io_cpu_sha256_update, \
	.sha256_finish = io_cpu_sha256_finish, \
	.get_socket = io_core_get_null_socket, \
	.enqueue_task = io_enqueue_task_base, \
	.signal_task_pending = io_signal_task_pending, \
	.do_next_task = io_do_next_task_base, \
	.dequeue_event = dequeue_io_event, \
	.enqueue_event = enqueue_io_event, \
	.next_event = do_next_io_event, \
	.in_event_thread = in_not_in_event_thread, \
	.signal_event_pending = io_no_signal_event_pending, \
	.wait_for_event = io_no_wait_for_event_pending, \
	.wait_for_all_events = io_no_wait_for_event_pending, \
	.get_time = io_get_time_zero, \
	.enqueue_alarm = io_no_enqueue_alarm, \
	.dequeue_alarm = io_no_enqueue_alarm, \
	.enter_critical_section = io_no_enter_critical_section, \
	.exit_critical_section = io_no_exit_critical_section, \
	.register_interrupt_handler = io_no_register_interrupt_handler, \
	.unregister_interrupt_handler = io_no_unregister_interrupt_handler, \
	.set_io_pin_output = io_pin_nop, \
	.set_io_pin_input = io_pin_nop, \
	.set_io_pin_interrupt = io_pin_interrupt_nop, \
	.set_io_pin_alternate = io_pin_nop, \
	.release_io_pin = io_pin_nop, \
	.read_from_io_pin = read_from_io_pin_nop, \
	.write_to_io_pin = write_to_io_pin_nop, \
	.toggle_io_pin = io_pin_nop, \
	.valid_pin = io_pin_is_always_invalid, \
	.log = io_no_log, \
	.flush_log = io_no_log_flush,\
	.panic = io_default_panic, \
	/**/
	

#define ENTER_CRITICAL_SECTION(E)	\
	{	\
		bool __critical_handle = enter_io_critical_section(E);


#define EXIT_CRITICAL_SECTION(E) \
		exit_io_critical_section (E,__critical_handle);	\
	}

enum {
	IO_PANIC_UNRECOVERABLE_ERROR = 1,
	IO_PANIC_SOMETHING_BAD_HAPPENED,
	IO_PANIC_DEVICE_ERROR,
	IO_PANIC_OUT_OF_MEMORY,
	IO_PANIC_TIME_CLOCK_ERROR,
	IO_PANIC_INVALID_OPERATION,
};


typedef int (*quick_sort_compare_t) (void const*,void const*);
void	pq_sort_recurse (void*[],int,int,quick_sort_compare_t);

// this version of qsort requires size of all array values to be sizeof(void*)
INLINE_FUNCTION void
pq_sort (void** a,int n,quick_sort_compare_t compare) {
	pq_sort_recurse (a,0,n-1,compare);
}

#include <io_event.h>


INLINE_FUNCTION void
io_enqueue_event (io_t *io,io_event_t *ev) {
	if (io_event_is_valid (ev)) {
		io->implementation->enqueue_event (io,ev);
		io->implementation->signal_event_pending (io);
	}
}

//
// dma
//
typedef struct io_dma_channel io_dma_channel_t;
typedef struct io_dma_channel_implementation io_dma_channel_implementation_t;
typedef struct io_dma_controller io_dma_controller_t;
typedef struct io_dma_controller_implementation io_dma_controller_implementation_t;



struct io_dma_controller_implementation {
	io_dma_controller_implementation_t const *specialisation_of;

	bool (*start_controller) (io_dma_controller_t*);
	bool (*stop_controller) (io_dma_controller_t*);

	bool (*start_transfer) (io_dma_controller_t*,io_dma_channel_t*);
	bool (*stop_transfer) (io_dma_controller_t*,io_dma_channel_t*);

};

#define IO_DMA_CONTROLLER_STRUCT_MEMBERS\
	io_dma_controller_implementation_t const *implementation;\
	io_t *io;\
	/**/

struct io_dma_controller {
	IO_DMA_CONTROLLER_STRUCT_MEMBERS
};

INLINE_FUNCTION bool
io_dma_controller_start_controller (io_dma_controller_t *dmac) {
	return dmac->implementation->start_controller(dmac);
}

INLINE_FUNCTION bool
io_dma_controller_stop_controller (io_dma_controller_t *dmac) {
	return dmac->implementation->stop_controller(dmac);
}

INLINE_FUNCTION bool
io_dma_controller_start_transfer (
	io_dma_controller_t *dmac,io_dma_channel_t *c
) {
	return dmac->implementation->start_transfer(dmac,c);
}

INLINE_FUNCTION bool
io_dma_controller_stop_transfer (
	io_dma_controller_t *dmac,io_dma_channel_t *c
) {
	return dmac->implementation->stop_transfer(dmac,c);
}

bool io_dma_controller_nop (io_dma_controller_t*);
bool io_dma_controller_transfer_nop (io_dma_controller_t*,io_dma_channel_t*);

#define SPECIALISE_IO_DMA_CONTROLLER_IMPLEMENTATION(S) \
	.specialisation_of = (S), \
	.start_controller = io_dma_controller_nop,\
	.stop_controller = io_dma_controller_nop,\
	.start_transfer = io_dma_controller_transfer_nop,\
	.stop_transfer = io_dma_controller_transfer_nop,\
	/**/


struct  PACK_STRUCTURE io_dma_channel_implementation {
	io_dma_channel_implementation_t const *specialisation_of;
	void (*initialise) (io_dma_channel_t*);
	void (*transfer_from_peripheral) (io_dma_channel_t*,void*,uint32_t);
	void (*transfer_to_peripheral) (io_dma_channel_t*,void const*,uint32_t);
	void (*transfer_complete) (io_t*,io_dma_channel_t*);
};

extern io_dma_channel_t null_dma_channel;

void io_dma_channel_initialise_nop (io_dma_channel_t*);
void io_dma_channel_no_transfer_from_peripheral (io_dma_channel_t*,void*,uint32_t);
void io_dma_channel_no_transfer_to_peripheral (io_dma_channel_t*,void const*,uint32_t);
void io_dma_channel_transfer_complete_nop (io_t*,io_dma_channel_t*);

#define SPECIALISE_IO_DMA_CHANNEL_IMPLEMENTATION(S) \
	.specialisation_of = S, \
	.initialise = io_dma_channel_initialise_nop,\
	.transfer_from_peripheral = io_dma_channel_no_transfer_from_peripheral,\
	.transfer_to_peripheral = io_dma_channel_no_transfer_to_peripheral,\
	.transfer_complete = io_dma_channel_transfer_complete_nop,\
	/**/

#define IO_DMA_CHANNEL_STRUCT_MEMBERS	\
	io_dma_channel_implementation_t const *implementation;\
	io_event_t complete;	\
	io_event_t error;	\
	io_dma_channel_t *next_channel;\
	/**/

enum {
	IO_DMA_TRANSFER_MEMORY_TO_MEMORY,
	IO_DMA_TRANSFER_MEMORY_TO_PERIPHERAL,
	IO_DMA_TRANSFER_PERIPHERAL_TO_MEMORY,
};

struct PACK_STRUCTURE io_dma_channel {
	IO_DMA_CHANNEL_STRUCT_MEMBERS
};

extern EVENT_DATA io_dma_channel_implementation_t dma_channel_implementation;

//
// inline dma implementation
//
INLINE_FUNCTION void
io_dma_transfer_from_peripheral (io_dma_channel_t *channel,void *dest,uint32_t size) {
	channel->implementation->transfer_from_peripheral(channel,dest,size);
}

INLINE_FUNCTION void
io_dma_transfer_to_peripheral (io_dma_channel_t *channel,void const *src,uint32_t size) {
	channel->implementation->transfer_to_peripheral(channel,src,size);
}

INLINE_FUNCTION void
io_dma_transfer_complete (io_t *io,io_dma_channel_t *channel) {
	channel->implementation->transfer_complete (io,channel);
}

#include <io_value.h>
#include <io_sockets.h>

#ifdef IMPLEMENT_IO_CORE
//-----------------------------------------------------------------------------
//
// io core implementation (part 1)
//
//-----------------------------------------------------------------------------

static io_cpu_power_domain_t const*
ro_pd_get_as_read_only (io_cpu_power_domain_pointer_t pd) {
	return io_cpu_power_domain_pointer_ro (pd);
}

static io_cpu_power_domain_t*
ro_pd_get_as_read_write (io_cpu_power_domain_pointer_t pd) {
	return NULL;
}

EVENT_DATA io_cpu_power_domain_pointer_implementation_t 
read_only_power_domain_implementation = {
	.get_as_read_only = ro_pd_get_as_read_only,
	.get_as_read_write = ro_pd_get_as_read_write,
};

io_address_t
mk_io_address (io_byte_memory_t *bm,uint32_t size,uint8_t const *bytes) {
	switch (size) {
		case 1:
			return def_io_u8_address(*bytes);

		case 2:
			return def_io_u16_address(read_le_uint16(bytes));

		case 4:
			return def_io_u32_address(read_le_uint32(bytes));

		case IO_ADDRESS_INVALID_SIZE:
			return io_invalid_address();
			
		default:{
			return io_long_address (bm,size,bytes);
		}
	}
}

io_address_t
duplicate_io_address (io_byte_memory_t *bm,io_address_t a) {
	switch (io_address_size(a)) {
		case 1:
		case 2:
		case 4:
		case IO_ADDRESS_INVALID_SIZE:
			return a;
			
		default:
			if (io_address_is_volatile(a)) {
				return io_long_address (bm,io_address_size(a),io_address_ro_bytes(a));
			} else {
				return a;
			}
	}
}

//
// NB ptr MUST point to allocated memory of at least limit bytes
//
uint32_t
write_le_io_address (uint8_t *ptr,uint32_t limit,io_address_t address) {
	uint8_t *dest = ptr;
	uint8_t *end = dest + limit;
	uint8_t flag;
	uint32_t size = io_address_size (address);

	do {
		flag = (size > 0x7f);
		*dest++ = (size & 0x7f) | (flag << 7);
		size >>= 7;
	} while (flag && dest < end);

	size = dest - ptr;
	
	if (ptr < end) {
		uint8_t *address_cursor = get_pointer_to_io_address_value (address);
		uint8_t *address_end = address_cursor + io_address_size(address);
		
		while (address_cursor < address_end && dest < end) {
			*dest++ = *address_cursor++;
		}

		return (dest < end) ? io_address_size(address) + size : 0;
	} else {
		return 0;
	}
}

uint32_t
read_le_io_address (io_byte_memory_t *bm,uint8_t const *ptr,uint32_t limit,io_address_t *address) {
	uint8_t const *cursor = ptr;
	uint8_t const *end = cursor + limit;
	uint32_t size = 0;
	uint8_t byte = 0;

	do {
		byte = *cursor++;
		size <<= 7;
		size += (byte & 0x7f);
	} while (byte & 0x80 && cursor < end);
	
	if (cursor < end && (byte & 0x80) == 0) {
		*address = mk_io_address (bm,size,cursor);
		return (cursor - ptr) + size;
	} else {
		return 0;
	}
}

//
// size > 0
//
int32_t
compare_as_big_int_values (uint8_t const *a_bytes,uint32_t a_size,uint8_t const *b_bytes,uint32_t b_size) {
	uint8_t const *a_head = a_bytes + a_size - 1;
	uint8_t const *b_head = b_bytes + b_size - 1;
	int32_t cmp = 0;

	while (a_head > a_bytes && *a_head == 0) a_head--;
	while (b_head > b_bytes && *b_head == 0) b_head--;

	{
		uint32_t s1 = (a_head - a_bytes);
		uint32_t s2 = (b_head - b_bytes);
		
		if ( s1 > s2 ) {
			cmp = 1;
		} else if (s1 > s2) {
			cmp = -1;
		} else {
			uint8_t const *end = a_bytes - 1;
			while (a_head > end) {
				if (*a_head > *b_head) {
					cmp = 1;
					break;
				} else if (*a_head < *b_head) {
					cmp = -1;
					break;
				}
				a_head--;
				b_head--;
			}
		}
	}
	
	return cmp;
}

static int32_t
compare_io_address_values (io_address_t a,io_address_t b) {
	return compare_as_big_int_values (
		get_pointer_to_io_address_value(a),io_address_size(a),
		get_pointer_to_io_address_value(b),io_address_size(b)
	);
}

//
// compare as little endian big numbers
//
int32_t
compare_io_addresses (io_address_t a,io_address_t b) {
	int32_t cmp = 0;
	
	if (io_address_is_invalid(b)) {
		return (io_address_is_invalid(a)) ? 0 : -1;
	} else if (io_address_is_invalid(a)) {
		return (io_address_is_invalid(b)) ? 0 : 1;
	} else {
		cmp = compare_io_address_values(a,b);
	}
	
	return cmp;
}

void
io_gererate_authentication_key_pair (
	io_t *io,io_authentication_key_t *secret,io_authentication_key_t *shared
) {
	uint8_t k[IO_AUTHENTICATION_KEY_BYTE_LENGTH] = {9};

	for (int i = 0; i < IO_AUTHENTICATION_KEY_WORD_LENGTH; i++) {
		secret->words[i] = io_get_random_u32 (io);
	}

	curve25519_donna (shared->bytes,secret->bytes,k);
}

//
// io_t base
//

io_socket_t*
io_core_get_null_socket (io_t *io,int32_t h) {
	return NULL;
}

io_byte_memory_t*
io_core_get_null_byte_memory (io_t *io) {
	return NULL;
}

io_value_memory_t*
io_core_get_null_value_memory (io_t *io) {
	return NULL;
}

void
io_no_stack_usage_info (io_t *io,memory_info_t *info) {
	info->free_bytes = 0;
	info->total_bytes = 0;
	info->used_bytes = 0;
}

void
io_pin_nop (io_t *io,io_pin_t p) {
}

void
io_pin_interrupt_nop (io_t *io,io_pin_t p,io_interrupt_handler_t *h) {
}

int32_t
read_from_io_pin_nop (io_t *io,io_pin_t p) {
	return 0;
}

void
write_to_io_pin_nop (io_t *io,io_pin_t p,int32_t v) {
}

bool 
io_pin_is_always_invalid (io_t *io,io_pin_t p) {
	return false;
}

void
io_no_gc (io_t *io,int32_t count) {
}

io_cpu_clock_pointer_t
io_no_core_clock (io_t *io) {
	return NULL_IO_CLOCK;
}

bool
io_never_first_run (io_t *io) {
	return false;
}

io_uid_t const*
io_no_uid (io_t *io) {
	return NULL;
}

bool
io_never_get_shared_key (io_t *io,io_uid_t const *uid,io_authentication_key_t *key) {
	return false;
}

uint32_t
io_no_random_u32 (io_t *io) {
	return 0;
}

bool
io_enqueue_task_base (io_t *io,vref_t r_task) {
	return false;
}

void
io_signal_task_pending (io_t *io) {
}

bool
io_do_next_task_base (io_t *io) {
	return false;
}

bool
in_not_in_event_thread (io_t *io) {
	return false;
}

void
io_no_signal_event_pending (io_t *io) {
}

void
io_no_wait_for_event_pending (io_t *io) {
}

io_time_t
io_get_time_zero (io_t *io) {
	return time_zero();
}

void
io_no_enqueue_alarm (io_t *io,io_alarm_t *alarm) {
}

bool
io_no_enter_critical_section (io_t *io) {
	return false;
}

void
io_no_exit_critical_section (io_t *io,bool f) {
}

void
io_no_register_interrupt_handler (
	io_t *io,int32_t number,io_interrupt_action_t fn,void *user
) {
}

bool
io_no_unregister_interrupt_handler (io_t *io,int32_t number,io_interrupt_action_t fn) {
	return false;
}

void
io_no_log (io_t *io,char const *fmp,va_list va) {
}

void
io_no_log_flush (io_t *io) {

}

void
io_default_panic (io_t *io,int code) {
	while (1);
}

static const io_implementation_t	io_base = {
	SPECIALISE_IO_IMPLEMENTATION(NULL)
};

void 
add_io_implementation_core_methods (io_implementation_t *io_i) {	
	memcpy (io_i,&io_base,sizeof(io_implementation_t));
}

#endif /* IMPLEMENT_IO_CORE */




extern EVENT_DATA io_value_implementation_t io_symbol_value_implementation_with_const_bytes;

def_constant_symbol(cr_OPEN,		"open",4)

//
// vector
//

extern EVENT_DATA io_value_implementation_t io_vector_value_implementation;
#define declare_stack_vector(name,...) \
	struct PACK_STRUCTURE io_vector_value_stack_##name {\
		IO_VECTOR_VALUE_STRUCT_MEMBERS	\
		union PACK_STRUCTURE {\
			vref_t ro[(sizeof((vref_t[]){__VA_ARGS__})/sizeof(vref_t))];\
			vref_t rw[(sizeof((vref_t[]){__VA_ARGS__})/sizeof(vref_t))];\
		} values;\
	};\
	struct io_vector_value_stack_##name name##_v = {\
			decl_io_value (&io_vector_value_implementation,sizeof(struct io_vector_value_stack_##name)) \
			.arity = (sizeof((vref_t[]){__VA_ARGS__})/sizeof(vref_t)),\
			.values.rw = {__VA_ARGS__}\
		};\
	vref_t name = def_vref (&reference_to_c_stack_value,&name##_v);\
	UNUSED(name);

//
// text decoding
//

typedef struct io_source_decoder io_source_decoder_t;
typedef void (*io_source_decoder_input_t) (io_source_decoder_t*,io_character_t);

typedef struct PACK_STRUCTURE io_source_decoder_context {
	vref_t r_value;
	vref_t *args;
	uint32_t arity;
} io_source_decoder_context_t;

typedef vref_t (*is_symbol_t) (char const *str,size_t);

struct PACK_STRUCTURE io_source_decoder {
	io_t *io;

	io_encoding_t *buffer;
	char const *error;

	io_source_decoder_input_t reset_state;
	io_source_decoder_input_t error_state;
	is_symbol_t const *keywords;

	io_source_decoder_input_t *input_stack;
	io_source_decoder_input_t *current_input;	// top of input stack

	io_source_decoder_context_t *context_stack;
	io_source_decoder_context_t *current_context;	// top of context stack
};

#define io_source_decoder_input_depth(d)		(((d)->current_input - (d)->input_stack) + 1)
#define io_source_decoder_context_depth(d)	 	(((d)->current_context - (d)->context_stack) + 1)
#define io_source_decoder_context(this)			((this)->current_context)
#define io_source_decoder_value(this)			((this)->current_context->r_value)
#define io_source_decoder_input(this)			(*((this)->current_input))
#define io_source_decoder_goto(d,s) 			io_source_decoder_input(d) = (s)
#define io_source_decoder_io(d)					((d)->io)
#define io_source_decoder_byte_memory(d)		io_get_byte_memory(io_source_decoder_io(d))
#define io_source_decoder_set_last_error(d,m) 	(d)->error = (m)
#define io_source_decoder_get_last_error(d)	(d)->error
#define io_source_decoder_has_error(d)			(io_source_decoder_get_last_error(d) != NULL)

io_source_decoder_t* mk_io_source_decoder (
	io_t*,vref_t,io_source_decoder_input_t,io_source_decoder_input_t,is_symbol_t const*
);
void	free_io_source_decoder (io_source_decoder_t*);

bool	io_source_decoder_append_arg (io_source_decoder_t*,vref_t);
void	io_source_decoder_end_of_statement (io_source_decoder_t*);
void	io_source_decoder_next_character (io_source_decoder_t*,uint32_t);
void	io_source_decoder_remove_last_character (io_source_decoder_t*);
void	io_source_decoder_parse (io_source_decoder_t*,const char*);
void	io_source_decoder_push_input (io_source_decoder_t*,io_source_decoder_input_t);
void	io_source_decoder_pop_input (io_source_decoder_t*);
void	io_source_decoder_reset (io_source_decoder_t*);

#ifdef IMPLEMENT_IO_CORE
# define STB_SPRINTF_IMPLEMENTATION
#endif

#ifndef STB_SPRINTF_H_INCLUDE
#define STB_SPRINTF_H_INCLUDE
/*

stb_sprintf - v1.06 - public domain snprintf() implementation

Single file sprintf replacement using stb_sprintf.

EXTRAS:
=======
You can print io_values with the %v style indicator and using
a vref_t as the argument.

*/

#if defined(__has_feature)
   #if __has_feature(address_sanitizer)
      #define STBI__ASAN __attribute__((no_sanitize("address")))
   #endif
#endif
#ifndef STBI__ASAN
#define STBI__ASAN
#endif

#ifdef STB_SPRINTF_STATIC
#define STBSP__PUBLICDEC static
#define STBSP__PUBLICDEF static STBI__ASAN
#else
#ifdef __cplusplus
#define STBSP__PUBLICDEC extern "C"
#define STBSP__PUBLICDEF extern "C" STBI__ASAN
#else
#define STBSP__PUBLICDEC extern
#define STBSP__PUBLICDEF STBI__ASAN
#endif
#endif

#include <stdarg.h> // for va_list()
#include <stddef.h> // size_t, ptrdiff_t

#ifndef STB_SPRINTF_MIN
#define STB_SPRINTF_MIN 512 // how many characters per callback
#endif
typedef char *STBSP_SPRINTFCB(char *buf, void *user, int len);

#ifndef STB_SPRINTF_DECORATE
#define STB_SPRINTF_DECORATE(name) stbsp_##name // define this before including if you want to change the names
#endif

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(vsprintf)(char *buf, char const *fmt, va_list va);
STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(vsnprintf)(char *buf, int count, char const *fmt, va_list va);
STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(sprintf)(char *buf, char const *fmt, ...);
STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(snprintf)(char *buf, int count, char const *fmt, ...);

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(vsprintfcb)(STBSP_SPRINTFCB *callback, void *user, char *buf, char const *fmt, va_list va);
STBSP__PUBLICDEF void STB_SPRINTF_DECORATE(set_separators)(char comma, char period);

#endif // STB_SPRINTF_H_INCLUDE

#ifdef IMPLEMENT_IO_CORE
//-----------------------------------------------------------------------------
//
// io core implementation
//
//-----------------------------------------------------------------------------


//
// alarms
//
io_alarm_t s_null_io_alarm = {
	.at = &s_null_io_event,
	.error = &s_null_io_event,
	.when = {LLONG_MAX},
	.next_alarm = NULL,
};

void
initialise_io (io_t *io,io_implementation_t const *I) {
	io->implementation = I;
	io->events = &s_null_io_event;
	io->alarms = &s_null_io_alarm;
	io->log_level = IO_LOG_LEVEL_NO_LOGGING;
}

int
io_printf (io_t *io,const char *fmt,...) {
	io_socket_t *print = io_get_socket (io,IO_PRINTF_SOCKET);
	int result = 0;
	if (print) {
		io_encoding_t *msg = io_socket_new_message (print);
		va_list va;

		va_start(va, fmt);
		result = io_encoding_print (msg,fmt,va);
		va_end(va);

		io_socket_send_message (print,msg);
	}
	
	return result;
}

void
io_log (io_t *io,io_log_level_t level,const char *fmt,...) {
	if (io->log_level >= level) {
		va_list va;
		va_start(va, fmt);
		io_log_message (io,fmt,va);
		va_end(va);
	}
}

void
flush_io_log (io_t* io) {
	io_socket_t *print = io_get_socket (io,IO_PRINTF_SOCKET);
	if (print) {
		io_socket_flush (print);
	}
}

void
io_log_startup_message (io_t *io,io_log_level_t lvl) {
	memory_info_t info;
	io_byte_memory_get_info (io_get_byte_memory(io),&info);
	io_log (
		io,lvl,"%-*s%-*scomplete\n",
		DBP_FIELD1,DEVICE_NAME,
		DBP_FIELD2,"startup"
	);
	io_log (
		io,lvl,"%-*s%-*sio bm:  %u bytes of %u used\n",
		DBP_FIELD1,"",
		DBP_FIELD2,"""",
		info.used_bytes,info.total_bytes
	);
	io_get_stack_usage_info (io,&info);
	io_log (
		io,lvl,"%-*s%-*sc-stack:%u bytes used of %u total\n",
		DBP_FIELD1,"",
		DBP_FIELD2,"",
		info.used_bytes,info.total_bytes
	);
}

//
// clock and power
//
void
io_power_domain_no_operation (
	io_t *io,io_cpu_power_domain_pointer_t pd
) {
}

EVENT_DATA io_cpu_power_domain_implementation_t
always_on_io_power_domain_implementation = {
	.turn_off = io_power_domain_no_operation,
	.turn_on = io_power_domain_no_operation,
};

EVENT_DATA io_cpu_power_domain_t always_on_io_power_domain = {
	.implementation = &always_on_io_power_domain_implementation,
};

io_cpu_power_domain_pointer_t
get_always_on_io_power_domain (io_cpu_clock_pointer_t clock) {
	return def_io_cpu_power_domain_pointer (&always_on_io_power_domain);
}

io_cpu_clock_pointer_t
io_cpu_clock_get_input_nop (io_cpu_clock_pointer_t clock) {
	return IO_CPU_CLOCK(NULL);
}

bool
io_cpu_clock_is_derrived_from (
	io_cpu_clock_pointer_t clock,io_cpu_clock_implementation_t const *T
) {
	bool yes = false;
	while (!io_cpu_clock_is_null (clock)) {
		io_cpu_clock_implementation_t const *I = (
			io_cpu_clock_ro_pointer(clock)->implementation
		);
		if (I == T) {
			yes = true;
			break;
		}
		clock = io_cpu_clock_get_input(clock);
	};

	return yes;
}

io_cpu_clock_pointer_t
io_cpu_dependant_clock_get_input (io_cpu_clock_pointer_t clock) {
	io_cpu_dependant_clock_t const *this = (
		(io_cpu_dependant_clock_t const*) io_cpu_clock_ro_pointer (clock)
	);
	return this->input;
}

bool
io_cpu_dependant_clock_start_input (io_t *io,io_cpu_clock_pointer_t clock) {
	io_cpu_dependant_clock_t const *this = (
		(io_cpu_dependant_clock_t const*) io_cpu_clock_ro_pointer (clock)
	);

	if (io_cpu_clock_ro_pointer (this->input)) {
		return io_cpu_clock_start (io,this->input);
	} else {
		return false;
	}
}

bool
io_cpu_clock_has_implementation (io_cpu_clock_pointer_t clock,io_cpu_clock_implementation_t const *T) {
	io_cpu_clock_implementation_t const *I = io_cpu_clock_ro_pointer(clock)->implementation;
	bool yes = false;
	do {
		if (I == T) {
			yes = true;
			break;
		}
		I = I->specialisation_of;
	} while (I);

	return yes;
}

void
io_cpu_clock_stop (io_t *io,io_cpu_clock_pointer_t clock) {
	io_cpu_clock_implementation_t const *I = io_cpu_clock_ro_pointer(clock)->implementation;
	do {
		if (I->stop) {
			I->stop (io,clock);
			break;
		}
		I = I->specialisation_of;
	} while (I);
}

float64_t
io_cpu_clock_no_frequency (io_cpu_clock_pointer_t clock) {
	return 0;
}

io_cpu_clock_pointer_t
io_cpu_clock_no_input (io_cpu_clock_pointer_t clock) {
	return NULL_IO_CLOCK;
}

io_cpu_power_domain_pointer_t
io_cpu_clock_no_power_domain (io_cpu_clock_pointer_t clock) {
	return NULL_IO_POWER_DOMAIN;
}

bool
io_cpu_clock_iterate_no_outputs (
	io_cpu_clock_pointer_t clock,io_cpu_clock_iterator_t cb,void *user_value
) {
	return true;
}

bool
io_cpu_clock_always_on_start (io_t *io,io_cpu_clock_pointer_t clock) {
	return true;
}

void
io_cpu_clock_always_on_stop (io_t *io,io_cpu_clock_pointer_t clock) {

}

bool
io_cpu_clock_iterate_outputs_nop (
	io_cpu_clock_pointer_t clock ,bool (*cb) (io_cpu_clock_pointer_t,void*),void *uv
) {
	return true;
}

bool
io_cpu_clock_function_iterate_outputs (
	io_cpu_clock_pointer_t clock ,bool (*cb) (io_cpu_clock_pointer_t,void*),void *uv
) {
	io_cpu_clock_function_t const *this = (
		(io_cpu_clock_function_t const*) io_cpu_clock_ro_pointer (clock)
	);
	io_cpu_clock_pointer_t const *cursor = this->outputs;
	io_cpu_clock_t const *next;
	
	while ( (next = io_cpu_clock_ro_pointer(*cursor)) != NULL) {
		if (!cb(*cursor,uv)) {
			return false;
		}
		cursor ++;
	}

	return true;
}

EVENT_DATA io_cpu_clock_implementation_t io_cpu_clock_implementation = {
	SPECIALISE_IO_CPU_CLOCK_IMPLEMENTATION(NULL)
};

EVENT_DATA io_cpu_clock_implementation_t
io_cpu_clock_source_implementation = {
	SPECIALISE_IO_CPU_CLOCK_SOURCE_IMPLEMENTATION (
		&io_cpu_clock_implementation
	)
};

float64_t
io_dependant_cpu_clock_get_current_frequency (io_cpu_clock_pointer_t clock) {
	io_cpu_dependant_clock_t const *this = (io_cpu_dependant_clock_t const*) (
		io_cpu_clock_ro_pointer (clock)
	);
	return io_cpu_clock_get_current_frequency (this->input);
}

float64_t
io_dependant_cpu_clock_get_expected_frequency (io_cpu_clock_pointer_t clock) {
	io_cpu_dependant_clock_t const *this = (io_cpu_dependant_clock_t const*) (
		io_cpu_clock_ro_pointer (clock)
	);
	return io_cpu_clock_get_expected_frequency (this->input);
}

bool
io_dependant_cpu_clock_start (io_t *io,io_cpu_clock_pointer_t clock) {
	io_cpu_dependant_clock_t const *this = (io_cpu_dependant_clock_t const*) (
		io_cpu_clock_ro_pointer (clock)
	);
	return io_cpu_clock_start (io,this->input);
}

EVENT_DATA io_cpu_clock_implementation_t
io_dependent_clock_implementation = {
	SPECIALISE_DEPENDANT_IO_CPU_CLOCK_IMPLEMENTATION (
		&io_cpu_clock_implementation
	)
};

EVENT_DATA io_cpu_clock_implementation_t
io_cpu_clock_function_implementation = {
		SPECIALISE_IO_CPU_CLOCK_FUNCTION_IMPLEMENTATION (
		&io_dependent_clock_implementation
	)
};


//
// pipes
//

static EVENT_DATA io_pipe_implementation_t io_pipe_implementation_base = {
	.specialisation_of = NULL,
};

static bool
io_pipe_has_implememntation (
	io_pipe_t const *pipe,io_pipe_implementation_t const *T
) {
	io_pipe_implementation_t const *E = pipe->implementation;
	bool is = false;
	do {
		is = (E == T);
	} while (!is && (E = E->specialisation_of) != NULL);

	return is && (E != NULL);
}

static EVENT_DATA io_pipe_implementation_t io_byte_pipe_implementation = {
	.specialisation_of = &io_pipe_implementation_base,
};

bool
is_io_byte_pipe (io_pipe_t const *pipe) {
	return io_pipe_has_implememntation (pipe,&io_byte_pipe_implementation);
}

static EVENT_DATA io_encoding_pipe_implementation_t io_encoding_pipe_implementation = {
	.specialisation_of = &io_pipe_implementation_base,
};

bool
is_io_encoding_pipe (io_pipe_t const *pipe) {
	return io_pipe_has_implememntation (
		pipe,(io_pipe_implementation_t const*) &io_encoding_pipe_implementation
	);
}

io_encoding_pipe_t*
cast_to_io_encoding_pipe (io_pipe_t *pipe) {
	if (
			pipe != NULL
		&&	io_pipe_has_implememntation (
				pipe,(io_pipe_implementation_t const*) &io_encoding_pipe_implementation
			)
	) {
		return (io_encoding_pipe_t*) pipe;
	} else {
		return NULL;
	}
}

io_byte_pipe_t*
mk_io_byte_pipe (io_byte_memory_t *bm,uint16_t length) {
	io_byte_pipe_t *this = io_byte_memory_allocate (bm,sizeof(io_byte_pipe_t));
	
	if (this) {
		this->implementation = &io_byte_pipe_implementation,
		this->write_index = this->read_index = 0;
		this->size_of_ring = length;
		this->overrun = 0;
		this->byte_ring = io_byte_memory_allocate (bm,sizeof(uint8_t) * length);
		if (this->byte_ring == NULL) {
			io_byte_memory_free (bm,this);
			this = NULL;
		}
	}
	
	return this;
}

void
free_io_byte_pipe (io_byte_pipe_t *this,io_byte_memory_t *bm) {
	io_byte_memory_free (bm,this->byte_ring);
	io_byte_memory_free (bm,this);
}

INLINE_FUNCTION int16_t
io_byte_pipe_increment_index (io_byte_pipe_t *this,int16_t i,int16_t n) {
	i += n;
	if (i >= this->size_of_ring) {
		i -= this->size_of_ring;
	}
	return i;
}

bool
io_byte_pipe_get_byte (io_byte_pipe_t *this,uint8_t *byte) {
	if (io_byte_pipe_is_readable (this)) {
		*byte = this->byte_ring[this->read_index];
		this->read_index = io_byte_pipe_increment_index (
			this,this->read_index,1
		);
		return true;
	} else {
		return false;
	}
}

bool
io_byte_pipe_put_byte (io_byte_pipe_t *this,uint8_t byte) {
	int16_t f = io_byte_pipe_count_free_slots(this);
	if (f > 0) {
		int16_t j = this->write_index;
		int16_t i = io_byte_pipe_increment_index(this,j,1);
		this->byte_ring[j] = byte;
		this->write_index = i;
		return true;
	} else {
		return false;
	}
}

uint32_t
io_byte_pipe_put_bytes (io_byte_pipe_t *this,uint8_t const *byte,uint32_t length) {
	uint8_t const *end = byte + length;
	bool ok = true;
	while (byte < end && ok) {
		ok = io_byte_pipe_put_byte (this,*byte++);
	}
	return length - (end - byte);
}

io_encoding_pipe_t*
mk_io_encoding_pipe (io_byte_memory_t *bm,uint16_t length) {
	io_encoding_pipe_t *this = io_byte_memory_allocate (bm,sizeof(io_encoding_pipe_t));
	
	if (this) {
		this->implementation = (
			(io_pipe_implementation_t const*) &io_encoding_pipe_implementation
		),
		this->write_index = this->read_index = 0;
		this->size_of_ring = length;
		this->overrun = 0;
		this->encoding_ring = io_byte_memory_allocate (bm,sizeof(io_encoding_t*) * length);
		if (this->encoding_ring == NULL) {
			io_byte_memory_free (bm,this);
			this = NULL;
		}
	}
	
	return this;
}

void
reset_io_encoding_pipe (io_encoding_pipe_t *this) {
	while (io_encoding_pipe_pop_encoding (this)) {
	}
}

void
free_io_encoding_pipe (io_encoding_pipe_t *this,io_byte_memory_t *bm) {
	reset_io_encoding_pipe (this);
	io_byte_memory_free (bm,this->encoding_ring);
	io_byte_memory_free (bm,this);
}

INLINE_FUNCTION int16_t
io_encoding_pipe_increment_index (io_encoding_pipe_t *this,int16_t i,int16_t n) {
	i += n;
	if (i >= this->size_of_ring) {
		i -= this->size_of_ring;
	}
	return i;
}

bool
io_encoding_pipe_pop_encoding (io_encoding_pipe_t *this) {
	if (io_encoding_pipe_is_readable (this)) {
		unreference_io_encoding (this->encoding_ring[this->read_index]);
		this->read_index = io_encoding_pipe_increment_index (
			this,this->read_index,1
		);
		return true;
	} else {
		return false;
	}
}

bool
io_encoding_pipe_put_encoding (io_encoding_pipe_t *this,io_encoding_t *encoding) {
	int16_t f = io_encoding_pipe_count_free_slots(this);
	if (f > 0) {
		int16_t j = this->write_index;
		int16_t i = io_encoding_pipe_increment_index(this,j,1);
		this->encoding_ring[j] = reference_io_encoding (encoding);
		this->write_index = i;
		return true;
	} else {
		return false;
	}
}

bool
io_encoding_pipe_peek (io_encoding_pipe_t *this,io_encoding_t **encoding) {
	if (io_encoding_pipe_is_readable (this)) {
		*encoding = this->encoding_ring[this->read_index];
		return true;
	} else {
		return false;
	}
}

static EVENT_DATA io_pipe_implementation_t io_value_pipe_implementation = {
	.specialisation_of = &io_pipe_implementation_base,
};

bool
is_io_value_pipe (io_pipe_t const *pipe) {
	return io_pipe_has_implememntation (pipe,&io_value_pipe_implementation);
}

INLINE_FUNCTION int16_t
io_value_pipe_increment_index (io_value_pipe_t *this,int16_t i,int16_t n) {
	i += n;
	if (i >= this->size_of_ring) {
		i -= this->size_of_ring;
	}
	return i;
}

io_value_pipe_t*
mk_io_value_pipe (io_byte_memory_t *bm,uint16_t length) {
	io_value_pipe_t *this = io_byte_memory_allocate (bm,sizeof(io_value_pipe_t));
	
	if (this) {
		this->implementation = &io_value_pipe_implementation,
		this->size_of_ring = length;
		this->overrun = 0;
		this->value_ring = io_byte_memory_allocate (bm,sizeof(vref_t) * length);
		if (this->value_ring == NULL) {
			io_byte_memory_free (bm,this);
			this = NULL;
		}
	}
	
	return this;
}

void
free_io_value_pipe (io_value_pipe_t *this,io_byte_memory_t *bm) {
	vref_t r_value;
	
	while (io_value_pipe_get_value (this,&r_value)) {
		unreference_value (r_value);
	}
	
	io_byte_memory_free (bm,this->value_ring);
	io_byte_memory_free (bm,this);
}

bool
io_value_pipe_get_value (io_value_pipe_t *this,vref_t *r_value) {
	if (io_encoding_pipe_is_readable (this)) {
		*r_value = unreference_value (this->value_ring[this->read_index]);
		this->read_index = io_value_pipe_increment_index (
			this,this->read_index,1
		);
		return true;
	} else {
		return false;
	}
}

bool
io_value_pipe_peek (io_value_pipe_t *this,vref_t *r_value) {
	if (io_encoding_pipe_is_readable (this)) {
		*r_value = this->value_ring[this->read_index];
		return true;
	} else {
		return false;
	}
}

bool
io_value_pipe_put_value (io_value_pipe_t *this,vref_t r_value) {
	int16_t f = io_value_pipe_count_free_slots(this);
	if (f > 0) {
		int16_t j = this->write_index;
		int16_t i = io_value_pipe_increment_index(this,j,1);
		this->value_ring[j] = reference_value (r_value);
		this->write_index = i;
		return true;
	} else {
		return false;
	}
}

//
// dma
//

bool
io_dma_controller_nop (io_dma_controller_t *dmac) {
	return false;
}

bool
io_dma_controller_transfer_nop (io_dma_controller_t *dmac,io_dma_channel_t *c) {
	return true;
}

void
io_dma_channel_initialise_nop (io_dma_channel_t *channel) {
}

void
io_dma_channel_no_transfer_from_peripheral (
	io_dma_channel_t *channel,void *dest,uint32_t len
) {
}

void
io_dma_channel_no_transfer_to_peripheral (
	io_dma_channel_t *channel,void const *src,uint32_t len
) {
}

void
io_dma_channel_transfer_complete_nop (io_t *io,io_dma_channel_t *channel) {
}

EVENT_DATA io_dma_channel_implementation_t dma_channel_implementation = {
	SPECIALISE_IO_DMA_CHANNEL_IMPLEMENTATION(NULL)
};

io_dma_channel_t null_dma_channel = {
	.implementation = &dma_channel_implementation,
	.complete = def_io_event(NULL,NULL),
	.error = def_io_event(NULL,NULL),
	.next_channel = NULL,
};

//
// hash
//

uint64_t integer_hash_u64 (uint64_t key) {
	key = ~key + (key << 21);
	key = key ^ (key >> 24);
	key = key + (key << 3) + (key << 8);
	key = key ^ (key >> 14);
	key = key + (key << 2) + (key << 4);
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}

#define tommy_rot(x, k) \
	(((x) << (k)) | ((x) >> (32 - (k))))

#define tommy_mix(a, b, c) \
	do { \
		a -= c;  a ^= tommy_rot(c, 4);  c += b; \
		b -= a;  b ^= tommy_rot(a, 6);  a += c; \
		c -= b;  c ^= tommy_rot(b, 8);  b += a; \
		a -= c;  a ^= tommy_rot(c, 16);  c += b; \
		b -= a;  b ^= tommy_rot(a, 19);  a += c; \
		c -= b;  c ^= tommy_rot(b, 4);  b += a; \
	} while (0)

#define tommy_final(a, b, c) \
	do { \
		c ^= b; c -= tommy_rot(b, 14); \
		a ^= c; a -= tommy_rot(c, 11); \
		b ^= a; b -= tommy_rot(a, 25); \
		c ^= b; c -= tommy_rot(b, 16); \
		a ^= c; a -= tommy_rot(c, 4);  \
		b ^= a; b -= tommy_rot(a, 14); \
		c ^= b; c -= tommy_rot(b, 24); \
	} while (0)

uint32_t
tommy_hash_u32 (uint32_t init_val,uint8_t const *key,uint32_t key_len) {
	uint32_t a, b, c;

	a = b = c = 0xdeadbeef + ((uint32_t)key_len) + init_val;

	while (key_len > 12) {
		a += read_le_uint32(key + 0);
		b += read_le_uint32(key + 4);
		c += read_le_uint32(key + 8);

		tommy_mix(a, b, c);

		key_len -= 12;
		key += 12;
	}

	switch (key_len) {
	case 0 :
		return c; /* used only when called with a zero length */
	case 12 :
		c += read_le_uint32(key + 8);
		b += read_le_uint32(key + 4);
		a += read_le_uint32(key + 0);
		break;
	case 11 : c += ((uint32_t)key[10]) << 16; /* fallthrough */
	case 10 : c += ((uint32_t)key[9]) << 8; /* fallthrough */
	case 9 : c += key[8]; /* fallthrough */
	case 8 :
		b += read_le_uint32(key + 4);
		a += read_le_uint32(key + 0);
		break;
	case 7 : b += ((uint32_t)key[6]) << 16; /* fallthrough */
	case 6 : b += ((uint32_t)key[5]) << 8; /* fallthrough */
	case 5 : b += key[4]; /* fallthrough */
	case 4 :
		a += read_le_uint32(key + 0);
		break;
	case 3 : a += ((uint32_t)key[2]) << 16; /* fallthrough */
	case 2 : a += ((uint32_t)key[1]) << 8; /* fallthrough */
	case 1 : a += key[0]; /* fallthrough */
	}

	tommy_final(a, b, c);

	return c;
}
/*
 * Copyright (c) 2010, Andrea Mazzoleni. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

string_hash_table_entry_t*
mk_string_hash_table_entry (
	io_byte_memory_t *bm,char const *bytes,uint32_t size,string_hash_table_mapping_t map
) {
	string_hash_table_entry_t *entry = io_byte_memory_allocate (
		bm,sizeof(string_hash_table_entry_t)
	);
	
	if (entry) {
		entry->size = size;
		if (size) {
			entry->bytes = io_byte_memory_allocate (bm,size);
			if (entry->bytes != NULL) {
				entry->next_entry = NULL;
				entry->mapping = map;
				memcpy (entry->bytes,bytes,size);
			} else {
				io_byte_memory_free (bm,entry);
				entry = NULL;
			}
		} else {
			entry->bytes = NULL;
		}
	}
	
	return entry;
}

void
free_string_hash_table_entry (io_byte_memory_t *bm,string_hash_table_entry_t *entry) {
	io_byte_memory_free (bm,entry->bytes);
	io_byte_memory_free (bm,entry);
}

string_hash_table_t*
mk_string_hash_table (io_byte_memory_t *bm,uint32_t initial_size) {
	string_hash_table_t *this = io_byte_memory_allocate (
		bm,sizeof(string_hash_table_t)
	);
	
	if (this) {
		this->bm = bm;
		this->table_size = next_prime_u32_integer (initial_size);
		this->table_grow = this->table_size/2;
		this->table = io_byte_memory_allocate (
			bm,sizeof(string_hash_table_entry_t*) * this->table_size
		);
		if (this->table != NULL) {
			memset (
				this->table,0,sizeof(string_hash_table_entry_t*) * this->table_size
			);
		} else {
			io_byte_memory_free (bm,this);
			this = NULL;
		}
	}
	
	return this;
}

void
free_string_hash_table (string_hash_table_t *this) {
	uint32_t i;
	
	for (i = 0; i < this->table_size; i++) {
		string_hash_table_entry_t *next,*cursor = this->table[i];
		while (cursor != NULL) {
			next = cursor->next_entry;
			free_string_hash_table_entry (this->bm,cursor);
			cursor = next;
		}
	}

	io_byte_memory_free (this->bm,this->table);
	io_byte_memory_free (this->bm,this);
}

void
iterate_string_hash_table (
	string_hash_table_t *this,bool (*cb) (string_hash_table_entry_t*,void*),void *user_value
) {
	for (int i = 0; i < this->table_size; i++) {
		string_hash_table_entry_t *cursor = this->table[i];
		while (cursor != NULL) {
			cb (cursor,user_value);
			cursor = cursor->next_entry;
		}
	}
}

static string_hash_table_entry_t*
string_hash_table_get_entry (
	string_hash_table_t *this,const char *data,uint32_t size,int index,int *depth
) {
	string_hash_table_entry_t *cursor = this->table[index];
	if (depth) *depth = 0;
	
	while (cursor != NULL ) {
		if (depth)  (*depth) ++;
		if (cursor->size == size && (size == 0 || memcmp(cursor->bytes,data,size) == 0)) {
			break;
		}
		cursor = cursor->next_entry;
	}

	return cursor;
}

static void string_hash_table_grow (string_hash_table_t*);

bool
string_hash_table_insert (
	string_hash_table_t *this,const char *data,uint32_t size,string_hash_table_mapping_t map
) {
	uint32_t index = tommy_hash_u32 (0,(uint8_t const*) data,size) % this->table_size;
	int depth;
	
	string_hash_table_entry_t *cursor = string_hash_table_get_entry (
		this,data,size,index,&depth
	);

	if (cursor != NULL ) {
		cursor->mapping = map;
		return false;
	} else {
		if (depth > 7) {
			string_hash_table_grow (this);
			string_hash_table_insert (this,data,size,map);
		} else {
			cursor = mk_string_hash_table_entry (this->bm,data,size,map);
			cursor->next_entry = this->table[index];
			this->table[index] = cursor;
		}
		return true;
	}
}

static void
string_hash_table_grow (string_hash_table_t *this) {
	string_hash_table_entry_t **old_table = this->table;
	uint32_t old_size = this->table_size;
	
	this->table_size = next_prime_u32_integer (this->table_size + this->table_grow);
	this->table = io_byte_memory_allocate (
		this->bm,sizeof(string_hash_table_entry_t*) * this->table_size
	);
	memset (this->table,0,sizeof(string_hash_table_entry_t*) * this->table_size);
	for (uint32_t i = 0; i < old_size; i++) {
		string_hash_table_entry_t *next,*cursor = old_table[i];
		while (cursor != NULL) {
			uint32_t index = tommy_hash_u32 (0,(uint8_t const*) cursor->bytes,cursor->size) % this->table_size;
			next = cursor->next_entry;
			cursor->next_entry = this->table[index];
			this->table[index] = cursor;
			cursor = next;
		}
	}

	io_byte_memory_free (this->bm,old_table);
}

bool
string_hash_table_remove (
	string_hash_table_t *this,const char *data,uint32_t size
) {
	int index = tommy_hash_u32 (0,(uint8_t const*) data,size) % this->table_size;
	string_hash_table_entry_t **cursor = this->table + index;
	
	if (*cursor != NULL) {
		while (*cursor != NULL) {
			if (((*cursor)->size) == size &&  memcmp ((*cursor)->bytes,data,size) == 0) {
				string_hash_table_entry_t *remove = *cursor;
				*cursor = (*cursor)->next_entry;
				free_string_hash_table_entry (this->bm,remove);
				return true;
			}
			cursor = &((*cursor)->next_entry);
		}
	}
	
	return false;
}

bool
string_hash_table_map (string_hash_table_t *this,const char *data,uint32_t size,string_hash_table_mapping_t *map) {
	uint32_t index = tommy_hash_u32 (0,(uint8_t const*) data,size) % this->table_size;
	int depth;
	
	string_hash_table_entry_t *cursor = string_hash_table_get_entry (
		this,data,size,index,&depth
	);

	if (cursor != NULL ) {
		*map = cursor->mapping;
		return true;
	} else {
		return false;
	}
}

//
//
//
typedef struct vref_bucket_hash_table_entry {
	struct vref_bucket_hash_table_entry *next_entry;
	vref_t r_value;
} vref_bucket_hash_table_entry_t;

typedef struct PACK_STRUCTURE {
	VREF_HASH_TABLE_STRUCT_MEMBERS
	vref_bucket_hash_table_entry_t **table;	
	io_byte_memory_t *bm;
	uint32_t table_size;
	uint32_t table_grow;
} vref_bucket_hash_table_t;

vref_hash_table_t*
mk_vref_bucket_hash_table (io_byte_memory_t *bm,uint32_t initial_size) {
	extern EVENT_DATA vref_hash_table_implementation_t vref_bucket_hash_implementation;
	vref_bucket_hash_table_t *this = io_byte_memory_allocate (
		bm,sizeof(vref_bucket_hash_table_t)
	);
	
	if (this) {
		this->implementation = &vref_bucket_hash_implementation;
		this->bm = bm;
		this->table_size = next_prime_u32_integer (initial_size);
		this->table_grow = this->table_size/2;
		this->table = io_byte_memory_allocate (
			bm,sizeof(vref_bucket_hash_table_entry_t*) * this->table_size
		);
		if (this->table != NULL) {
			memset (
				this->table,0,sizeof(vref_bucket_hash_table_entry_t*) * this->table_size
			);
		} else {
			io_byte_memory_free (bm,this);
			this = NULL;
		}
	}
	
	return (vref_hash_table_t*) this;
}

static void
free_vref_bucket_hash_table (vref_hash_table_t *ht) {
	vref_bucket_hash_table_t *this = (vref_bucket_hash_table_t*) ht;
	uint32_t i;
	
	for (i = 0; i < this->table_size; i++) {
		vref_bucket_hash_table_entry_t *next,*cursor = this->table[i];
		while (cursor != NULL) {
			next = cursor->next_entry;
			unreference_value (cursor->r_value);
			io_byte_memory_free (this->bm,cursor);
			cursor = next;
		}
	}
	
	io_byte_memory_free (this->bm,this->table);
	io_byte_memory_free (this->bm,this);
}

uint32_t
vref_bucket_hash (vref_bucket_hash_table_t *ht,vref_t r_value) {
	return (
			integer_hash_u64 (vref_get_as_builtin_integer(r_value))
		%	ht->table_size
	);
}

static vref_bucket_hash_table_entry_t*
vref_bucket_hash_get_entry (
	vref_bucket_hash_table_t *this,vref_t r_value,int index,int *depth
) {
	vref_bucket_hash_table_entry_t *cursor = this->table[index];
	if (depth) *depth = 0;
	
	while (cursor != NULL ) {
		if (depth)  (*depth) ++;
		if (vref_is_equal_to ((cursor->r_value),r_value)) {
			break;
		}
		cursor = cursor->next_entry;
	}

	return cursor;
}

static void	vref_bucket_hash_grow (vref_bucket_hash_table_t*);

static bool
vref_bucket_hash_insert (vref_bucket_hash_table_t *this,vref_t r_value) {
	int depth,index = vref_bucket_hash (this,r_value);
	vref_bucket_hash_table_entry_t *cursor = vref_bucket_hash_get_entry (
		this,r_value,index,&depth
	);

	if (cursor != NULL ) {
		return false;
	} else {
		if (depth > 7) {
			vref_bucket_hash_grow (this);
			return vref_bucket_hash_insert (this,r_value);
		} else {
			cursor = io_byte_memory_allocate (
				this->bm,sizeof(vref_bucket_hash_table_entry_t)
			);
			memset (cursor,0,sizeof(vref_bucket_hash_table_entry_t));
			cursor->next_entry = this->table[index];
			this->table[index] = cursor;
			cursor->r_value = reference_value (r_value);
			return true;
		}
	}
}

static void
vref_bucket_hash_grow (vref_bucket_hash_table_t *this) {
	vref_bucket_hash_table_entry_t **old_table = this->table;
	uint32_t old_size = this->table_size;
	
	this->table_size = next_prime_u32_integer (this->table_size + this->table_grow);
	this->table = io_byte_memory_allocate (
		this->bm,sizeof(vref_bucket_hash_table_entry_t*) * this->table_size
	);
	memset (this->table,0,sizeof(vref_bucket_hash_table_entry_t*) * this->table_size);
	for (uint32_t i = 0; i < old_size; i++) {
		vref_bucket_hash_table_entry_t *next,*cursor = old_table[i];
		while (cursor != NULL) {
			next = cursor->next_entry;
			vref_bucket_hash_insert (this,cursor->r_value);
			unreference_value (cursor->r_value);
			io_byte_memory_free (this->bm,cursor);
			cursor = next;
		}
	}

	io_byte_memory_free (this->bm,old_table);
}

static bool
vref_bucket_hash_insert_value (vref_hash_table_t *ht,vref_t r_value) {
	vref_bucket_hash_table_t *this = (vref_bucket_hash_table_t*) ht;
	return vref_bucket_hash_insert (this,r_value);
}

static bool
vref_bucket_hash_contains (vref_hash_table_t *ht,vref_t r_value) {
	vref_bucket_hash_table_t *this = (vref_bucket_hash_table_t*) ht;
	int index = vref_bucket_hash (this,r_value);
	return NULL != vref_bucket_hash_get_entry (this,r_value,index,NULL);
}

static bool
vref_bucket_hash_remove (vref_hash_table_t *ht,vref_t r_value) {
	vref_bucket_hash_table_t *this = (vref_bucket_hash_table_t*) ht;
	int index = vref_bucket_hash (this,r_value);
	vref_bucket_hash_table_entry_t **cursor = this->table + index;
	
	if (*cursor != NULL) {
		while (*cursor != NULL) {
			if (vref_is_equal_to (((*cursor)->r_value),r_value)) {
				vref_bucket_hash_table_entry_t *remove = *cursor;
				*cursor = (*cursor)->next_entry;
				unreference_value (remove->r_value);
				io_byte_memory_free (this->bm,remove);
				return true;
			}
			cursor = &((*cursor)->next_entry);
		}
	}
	
	return false;
}

EVENT_DATA vref_hash_table_implementation_t vref_bucket_hash_implementation = {
	.free = free_vref_bucket_hash_table,
	.insert = vref_bucket_hash_insert_value,
	.contains = vref_bucket_hash_contains,
	.remove = vref_bucket_hash_remove,
};

//
// reference to umm value
//
vref_t
io_reference_to_umm_value_reference (vref_t r_value) {
	io_value_t *value = vref_cast_to_rw_pointer (r_value);
	if (value) {
		io_value_reference_count(value) ++;
	}
	return r_value;
}

static void
free_umm_value (umm_io_value_memory_t *this,io_value_t *value) {
	io_value_free (value);
	umm_free (this->bm,value);
}

static void
io_reference_to_umm_value_unreference (vref_t r_value) {
	io_value_t *value = vref_cast_to_rw_pointer (r_value);
	if (value) {
		if (io_value_reference_count(value)) {
			io_value_reference_count(value) --;
		} else {
			// a panic really
		}
	}
}

int64_t
io_reference_to_umm_value_get_as_builtin_integer (vref_t r_value) {
	return vref_expando(r_value).ptr;
}

void const*
io_reference_to_umm_value_cast_to_ro_pointer (vref_t r_value) {
	return (void const*) io_value_reference_p32_to_c_pointer(r_value);
}

void*
io_reference_to_umm_value_cast_to_rw_pointer (vref_t r_value) {
	return (void*) io_value_reference_p32_to_c_pointer(r_value);
}

io_value_memory_t*
io_reference_to_umm_value_get_containing_memory (vref_t r_value) {
	return io_get_value_memory_by_id (io_value_reference_p32_memory(r_value));
}

EVENT_DATA io_value_reference_implementation_t reference_to_umm_value = {
	.reference = io_reference_to_umm_value_reference,
	.unreference = io_reference_to_umm_value_unreference,
	.cast_to_ro_pointer = io_reference_to_umm_value_cast_to_ro_pointer,
	.cast_to_rw_pointer = io_reference_to_umm_value_cast_to_rw_pointer,
	.get_as_builtin_integer = io_reference_to_umm_value_get_as_builtin_integer,
	.get_containing_memory = io_reference_to_umm_value_get_containing_memory,
};

//
// umm-heap based io_value memory
//
#define GC_STACK_LENGTH	8

static void initialise_io_byte_memory_cursor (io_byte_memory_t*,uint16_t*);

io_value_memory_t*
mk_umm_io_value_memory (io_t *io,uint32_t size,uint32_t id) {
	umm_io_value_memory_t *this = io_byte_memory_allocate (
		io_get_byte_memory(io),sizeof(umm_io_value_memory_t)
	);
	
	if (this) {
		this->implementation = &umm_value_memory_implementation;
		this->io = io;
		this->id_ = id;
		this->bm = mk_io_byte_memory (io,size,UMM_BLOCK_SIZE_1N);
		if (this->bm != NULL) {
			initialise_io_byte_memory_cursor(this->bm,&this->gc_cursor);
			this->gc_stack_size = GC_STACK_LENGTH;
		} else {
			io_byte_memory_free (io_get_byte_memory(io),this);
			this = NULL;
		}
	}
	
	return (io_value_memory_t*) this;
}

void
umm_value_memory_free_memory (io_value_memory_t *vm) {
	umm_io_value_memory_t *this = (umm_io_value_memory_t*) vm;
	free_io_byte_memory (this->bm);
	umm_free (io_get_byte_memory(this->io),this);
}

vref_t
umm_value_memory_allocate_value (
	io_value_memory_t *vm,io_value_implementation_t const *I,size_t allocation_size
) {
	umm_io_value_memory_t *this = (umm_io_value_memory_t*) vm;
	io_value_t *new_value = umm_malloc (this->bm,allocation_size);

	*new_value = (io_value_t) {
		decl_io_value (I,allocation_size)
	};

	return umm_vref (&reference_to_umm_value,io_value_memory_id(this),new_value);
}

vref_t
umm_value_memory_new_value (
	io_value_memory_t *vm,
	io_value_implementation_t const *I,
	size_t size,
	vref_t r_base
) {
	vref_t r_new = umm_value_memory_allocate_value (vm,I,size);
	if (vref_is_valid (r_new)) {
		if (I->initialise (r_new,r_base) == NULL) {
			r_new = INVALID_VREF;
		}
	}

	return r_new;
}

struct gc_stack {
	io_value_t** cursor;
	io_value_t** end;
};

bool
umm_value_memory_gc_iterator (io_value_t *value,void *user_value) {
	struct gc_stack *stack = user_value;
	
	if (io_value_reference_count(value) == 0) {
		*stack->cursor++ = value;
	}
	
	return stack->cursor < stack->end;
}

static void
umm_value_memory_do_gc (io_value_memory_t *vm,int32_t count) {
	umm_io_value_memory_t *this = (umm_io_value_memory_t*) vm;
	io_value_t* values[this->gc_stack_size];

	if (count < 0) count = 1000000;

	while (count > 0) {
		struct gc_stack stack = {
			.cursor = values,
			.end = values + GC_STACK_LENGTH,
		};
		bool clean = true;

		incremental_iterate_io_byte_memory_allocations (
			this->bm,&this->gc_cursor,umm_value_memory_gc_iterator,&stack
		);

		{
			io_value_t** cursor = values;
			while (cursor < stack.cursor) {
				free_umm_value (this,*cursor);
				cursor++;
			}
		}

		if (clean && stack.cursor == values) {
			break;
		}
		count--;
	};
}

bool
heap_value_memory_is_persistant (io_value_memory_t *vm) {
	return false;
}

void const*
umm_value_memory_get_value_ro_pointer (io_value_memory_t *vm,vref_t r_value) {
	return vref_cast_to_ro_pointer (r_value);
}

void*
umm_value_memory_get_value_rw_pointer (io_value_memory_t *vm,vref_t r_value) {
	return vref_cast_to_rw_pointer (r_value);
}

void
umm_value_memory_get_info (io_value_memory_t *vm,memory_info_t *info) {
	umm_io_value_memory_t *this = (umm_io_value_memory_t*) vm;
	io_byte_memory_get_info (this->bm,info);
}

static io_t*
umm_value_memory_get_io (io_value_memory_t *vm) {
	umm_io_value_memory_t *this = (umm_io_value_memory_t*) vm;
	return this->io;
}

EVENT_DATA io_value_memory_implementation_t umm_value_memory_implementation = {
	.free = umm_value_memory_free_memory,
	.allocate_value = umm_value_memory_allocate_value,
	.new_value = umm_value_memory_new_value,
	.do_gc = umm_value_memory_do_gc,
	.get_info = umm_value_memory_get_info,
	.get_io = umm_value_memory_get_io,
	.is_persistant = heap_value_memory_is_persistant,
	.get_value_ro_pointer = umm_value_memory_get_value_ro_pointer,
	.get_value_rw_pointer = umm_value_memory_get_value_rw_pointer,
};

//
// reference to values in the C data section
//
static vref_t
io_reference_to_data_section_value_reference (vref_t r_value) {
	return r_value;
}

static void
io_reference_to_data_section_value_unreference (vref_t r_value) {
}

static int64_t
io_reference_to_data_section_value_get_as_builtin_integer (vref_t r_value) {
	return vref_expando(r_value).ptr;
}

static void const*
io_reference_to_data_section_value_cast_to_ro_pointer (vref_t r_value) {
	return (void const*) vref_expando(r_value).ptr;
}

static void*
io_reference_to_data_section_value_cast_to_rw_pointer (vref_t r_value) {
	return (void*) vref_expando(r_value).ptr;
}

static io_value_memory_t*
io_reference_to_value_get_null_containing_memory (vref_t r_value) {
	return NULL;
}

EVENT_DATA io_value_reference_implementation_t 
reference_to_data_section_value = {
	.reference = io_reference_to_data_section_value_reference,
	.unreference = io_reference_to_data_section_value_unreference,
	.cast_to_ro_pointer = io_reference_to_data_section_value_cast_to_ro_pointer,
	.cast_to_rw_pointer = io_reference_to_data_section_value_cast_to_rw_pointer,
	.get_as_builtin_integer = io_reference_to_data_section_value_get_as_builtin_integer,
	.get_containing_memory = io_reference_to_value_get_null_containing_memory,
};

//
// reference to constant values
//
static vref_t
io_reference_to_constant_value_reference (vref_t r_value) {
	return r_value;
}

static void
io_reference_to_constant_value_unreference (vref_t r_value) {
}

static int64_t
io_reference_to_constant_value_get_as_builtin_integer (vref_t r_value) {
	return vref_expando(r_value).ptr;
}

static void const*
io_reference_to_constant_value_cast_to_ro_pointer (vref_t r_value) {
	return (void const*) vref_expando(r_value).ptr;
}

static void*
io_reference_to_constant_value_cast_to_rw_pointer (vref_t r_value) {
	return NULL;
}

EVENT_DATA io_value_reference_implementation_t reference_to_constant_value = {
	.reference = io_reference_to_constant_value_reference,
	.unreference = io_reference_to_constant_value_unreference,
	.cast_to_ro_pointer = io_reference_to_constant_value_cast_to_ro_pointer,
	.cast_to_rw_pointer = io_reference_to_constant_value_cast_to_rw_pointer,
	.get_as_builtin_integer = io_reference_to_constant_value_get_as_builtin_integer,
	.get_containing_memory = io_reference_to_value_get_null_containing_memory,
};

//
// reference to constant values
//
static vref_t
io_reference_to_c_stack_value_reference (vref_t r_value) {
	return r_value;
}

static void
io_reference_to_c_stack_value_unreference (vref_t r_value) {
}

static int64_t
io_reference_to_c_stack_value_get_as_builtin_integer (vref_t r_value) {
	return vref_expando(r_value).ptr;
}

static void const*
io_reference_to_c_stack_value_cast_to_ro_pointer (vref_t r_value) {
	return (void const*) vref_expando(r_value).ptr;
}

static void*
io_reference_to_c_stack_value_cast_to_rw_pointer (vref_t r_value) {
	return (void*) vref_expando(r_value).ptr;
}

EVENT_DATA io_value_reference_implementation_t reference_to_c_stack_value = {
	.reference = io_reference_to_c_stack_value_reference,
	.unreference = io_reference_to_c_stack_value_unreference,
	.cast_to_ro_pointer = io_reference_to_c_stack_value_cast_to_ro_pointer,
	.cast_to_rw_pointer = io_reference_to_c_stack_value_cast_to_rw_pointer,
	.get_as_builtin_integer = io_reference_to_c_stack_value_get_as_builtin_integer,
	.get_containing_memory = io_reference_to_value_get_null_containing_memory,
};

//
// Encoding
//

io_encoding_t*
reference_io_encoding (io_encoding_t *encoding) {
	if (encoding != NULL) { 
		uint32_t new_count = (uint32_t) io_encoding_reference_count (encoding) + 1;
		if (new_count <= IO_ENCODING_REFERENCE_COUNT_LIMIT) {
			io_encoding_reference_count (encoding) = new_count;
		} else {
			io_panic (io_encoding_get_io (encoding),IO_PANIC_UNRECOVERABLE_ERROR);
		}
	}
	return encoding;
}

void
unreference_io_encoding (io_encoding_t *encoding) {
	if (io_encoding_reference_count(encoding)) {
		if (--io_encoding_reference_count(encoding) == 0) {
			io_encoding_free(encoding);
		}
	} else {
		io_panic (io_encoding_get_io (encoding),IO_PANIC_UNRECOVERABLE_ERROR);
	}
}

vref_t
io_value_encoding_decode_to_io_value (
	io_encoding_t *encoding,io_value_decoder_t decoder,io_value_memory_t *vm
) {
	return decoder (encoding,vm);
}

io_encoding_t*
mk_null_encoding (io_byte_memory_t *bm) {
	return NULL;
}

void
free_null_encoding (io_encoding_t *encoding) {
}

size_t
null_encoding_length (io_encoding_t const *encoding) {
	return 0;
}

int32_t
null_encoding_limit (void) {
	return 0;
}

uint32_t
null_encoding_grow_increment (io_encoding_t *encoding) {
	return 0;
}

io_t*
null_encoding_get_io (io_encoding_t *encoding) {
	return NULL;
}

void*
io_encoding_no_layer (io_encoding_t *encoding,io_layer_implementation_t const *L) {
	return NULL;
}

void*
io_encoding_no_byte_stream (io_encoding_t *encoding) {
	return NULL;
}

void
io_encoding_no_content (io_encoding_t *encoding,uint8_t const** b,uint8_t const** e) {
	*b = *e = NULL;
}

size_t
io_encoding_no_fill (io_encoding_t *encoding,uint8_t b,size_t s) {
	return 0;
}

bool
io_encoding_no_grow (io_encoding_t *encoding,uint32_t g) {
	return false;
}

void
io_encoding_no_reset (io_encoding_t *encoding) {
}

size_t
io_encoding_no_print (io_encoding_t *encoding,char const *fmt,va_list va) {
	return 0;
}

bool
io_encoding_no_append_byte (io_encoding_t *encoding,uint8_t byte) {
	return false;
}

bool
io_encoding_no_append_bytes (io_encoding_t *encoding,uint8_t const *b,size_t s) {
	return false;
}

bool
io_encoding_no_pop_last_byte (io_encoding_t *encoding,uint8_t *b) {
	return false;
}

uint32_t
io_encoding_no_decode_increment (io_encoding_t *encoding,uint32_t incr) {
	return 0;
}

EVENT_DATA io_encoding_layer_api_t no_packet_layer_api = {
	.get_inner_layer = NULL,
	.get_outer_layer = NULL,
	.get_layer = io_encoding_no_layer,
	.push_layer = NULL,
};

EVENT_DATA io_encoding_implementation_t io_encoding_implementation_base = {
	SPECIALISE_IO_ENCODING_IMPLEMENTATION(NULL)
};

bool
io_encoding_has_implementation (
	io_encoding_t const *encoding,io_encoding_implementation_t const *T
) {
	io_encoding_implementation_t const *E = io_encoding_implementation(encoding);
	bool is = false;
	do {
		is = (E == T);
	} while (!is && (E = E->specialisation_of) != NULL);

	return is && (E != NULL);
}

EVENT_DATA io_encoding_implementation_t 
io_value_int64_encoding_implementation = {
	SPECIALISE_IO_ENCODING_IMPLEMENTATION (&io_encoding_implementation_base)
};

bool encoding_is_io_value_int64 (io_encoding_t *encoding) {
	return io_encoding_has_implementation (
		encoding,&io_value_int64_encoding_implementation
	);
}

EVENT_DATA io_encoding_implementation_t 
io_value_float64_encoding_implementation = {
	SPECIALISE_IO_ENCODING_IMPLEMENTATION (
		&io_encoding_implementation_base
	)
};

bool
encoding_is_io_value_float64 (io_encoding_t *encoding) {
	return io_encoding_has_implementation (
		encoding,&io_value_float64_encoding_implementation
	);
}

vref_t
io_binary_encoding_decode_to_io_value (
	io_encoding_t *encoding,io_value_decoder_t decoder,io_value_memory_t *vm
) {
	return decoder (encoding,vm);
}

void*
io_binary_encoding_initialise (io_binary_encoding_t *this) {
	this->byte_stream = io_byte_memory_allocate (
		this->bm,TEXT_ENCODING_INITIAL_SIZE * sizeof(uint8_t)
	);
	if (this->byte_stream != NULL) {
		this->cursor = this->byte_stream;
		this->end = this->byte_stream + TEXT_ENCODING_INITIAL_SIZE;
		this->tag.all = 0;
	} else {
		io_byte_memory_free (this->bm,this);
		this = NULL;
	}
	return this;
};

void
io_binary_encoding_free (io_encoding_t *encoding) {
	if (encoding != NULL) {
		io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
		io_byte_memory_free (this->bm,this->byte_stream);
		io_byte_memory_free (this->bm,this);
	}
}

bool
io_binary_encoding_grow (io_encoding_t *encoding,uint32_t increase) {
	io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
	uint32_t old_size = io_binary_encoding_allocation_size (this);
	uint32_t new_size = (old_size +	(increase * sizeof(uint8_t)));
	uint32_t cursor_offset = io_binary_encoding_data_size (this);
	uint8_t *bigger = io_byte_memory_reallocate (
		this->bm,this->byte_stream,new_size
	);

	if (bigger) {
		this->byte_stream = bigger;
		this->cursor = this->byte_stream + cursor_offset;
		this->end = this->byte_stream + new_size;
		return true;
	} else {
		return false;
	}
}

bool
io_binary_encoding_append_byte (io_encoding_t *encoding,uint8_t byte) {
	io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
	if (this->cursor == this->end) {
		if (
				io_encoding_limit (encoding) < 0
			||	io_binary_encoding_data_size (this) < io_encoding_limit (encoding)
		) {
			io_encoding_grow (
				encoding,io_encoding_get_grow_increment(encoding)
			);
		}
	}

	if (this->cursor < this->end) {
		*this->cursor++ = byte;
		return true;
	} else {
		return false;
	}
}

void
io_binary_encoding_get_content (
	io_encoding_t *encoding,uint8_t const **begin,uint8_t const **end
) {
	io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
	*begin = this->byte_stream,
	*end = this->cursor;
};

void*
wordwise_32_unaligned_memset (void* s,int c,size_t sz) {
	uint32_t* p;
	uint32_t x = c & 0xff;
	uint8_t xx = c & 0xff;
	uint8_t* pp = (uint8_t*)s;
	size_t tail;

	// set up to the first alignment boundary.
	while (((unsigned int)pp & 3) && sz) {
		*pp++ = xx;
		sz--;
	}
	p = (uint32_t*)pp;

	// the number of bytes that will be trailing when the last word
	tail = sz & 3;

	x |= x << 8;
	x |= x << 16;

	sz >>= 2;

	while (sz--) {
	  *p++ = x;
	}

	// set after the last alignment boundary.
	pp = (uint8_t*)p;
	while (tail--) {
		*pp++ = xx;
	}

	return s;
}

size_t
io_binary_encoding_fill_bytes (
	io_encoding_t *encoding,uint8_t byte,size_t size
) {
	io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
	if ((this->cursor + size) >= this->end) {
		if (
				io_encoding_limit (encoding) < 0
			||	io_binary_encoding_data_size (this) < io_encoding_limit (encoding)
		) {
			io_encoding_grow (
				encoding,
				(
						(size - (this->end - this->cursor))
					+	io_encoding_get_grow_increment(encoding)
				)
			);
		} else {
			return 0;
		}
	}

/*
	uint8_t *end = this->cursor + size;
	while (this->cursor < end) {
		*this->cursor++ = byte;
	}
*/
	wordwise_32_unaligned_memset (this->cursor,byte,size);
	this->cursor += size;
	
	return size;
}

bool
io_binary_encoding_append_bytes (
	io_encoding_t *this,uint8_t const *byte,size_t size
) {
	uint8_t const *end = byte + size;
	while (byte < end) {
		if (!io_binary_encoding_append_byte (this,*byte++)) {
			return false;
		}
	}
	return true;
}

bool
io_binary_encoding_pop_last_byte (io_encoding_t *encoding,uint8_t *byte) {
	io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
	if (this->cursor > this->byte_stream) {
		this->cursor --;
		*byte = *this->cursor;
		return true;
	} else {
		return false;
	}
}

struct PACK_STRUCTURE io_binary_encoding_print_data {
	io_binary_encoding_t *this;
	char buf[STB_SPRINTF_MIN];
};

char*
io_binary_encoding_print_cb (char *buf,void *user,int len) {
	struct io_binary_encoding_print_data *info = user;
	io_binary_encoding_append_bytes ((io_encoding_t*) info->this,(uint8_t const*)info->buf,len);
	return info->buf;
}

size_t
io_binary_encoding_print (io_encoding_t *encoding,char const *fmt,va_list va) {
	struct io_binary_encoding_print_data user = {
		.this = (io_binary_encoding_t*) encoding
	};
	return STB_SPRINTF_DECORATE(vsprintfcb) (
		io_binary_encoding_print_cb,&user,user.buf,fmt,va
	);
}

bool
is_io_text_encoding (io_encoding_t const *encoding) {
	extern EVENT_DATA io_encoding_implementation_t io_text_encoding_implementation;
	return io_encoding_has_implementation (
		encoding,&io_text_encoding_implementation
	);
}

bool
io_text_encoding_iterate_characters (
	io_encoding_t *encoding,io_character_iterator_t cb,void *user_value
) {
	bool ok = true;

	if (is_io_text_encoding(encoding)) {
		const uint8_t *b,*e;

		io_encoding_get_content (encoding,&b,&e);

		while (b < e) {
			if ((*b & 0x80) == 0) {
				if ((ok &= cb(*b,user_value)) == false) break;
			} else {
				// up to U+07FF		110xxxxx 	10xxxxxx
				// up to U+FFFF 	1110xxxx 	10xxxxxx 	10xxxxxx
				// up to U+10FFFF 	11110xxx 	10xxxxxx 	10xxxxxx 	10xxxxxx
			}
			b++;
		}
	}

	return ok;
};

size_t
io_encoding_printf (io_encoding_t *encoding,const char *format, ... ) {
	size_t r = 0;

	if (is_io_binary_encoding (encoding)) {
		va_list va;
		va_start(va, format);
		r = io_encoding_print (encoding,format,va);
		va_end(va);
	}

	return r;
}

void
io_binary_encoding_reset (io_encoding_t *encoding) {
	io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
	this->cursor = this->byte_stream;
}

//
// number of bytes
//
size_t
io_binary_encoding_length (io_encoding_t const *encoding) {
	io_binary_encoding_t const *this = (io_binary_encoding_t const*) encoding;
	return io_binary_encoding_data_size (this);
}

int32_t
io_binary_encoding_nolimit (void) {
	return -1;
}

uint32_t
default_io_encoding_grow_increment (io_encoding_t *encoding) {
	return TEXT_ENCODING_GROWTH_INCREMENT;
}

io_t*
io_binary_encoding_get_io (io_encoding_t *encoding) {
	io_binary_encoding_t *this = (io_binary_encoding_t *) encoding;
	return this->bm->io;
}

void*
io_binary_encoding_get_byte_stream (io_encoding_t *encoding) {
	io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
	return this->byte_stream;
}

	
EVENT_DATA io_encoding_implementation_t 
io_binary_encoding_implementation = {
	SPECIALISE_IO_BINARY_ENCODING_IMPLEMENTATION (
		&io_encoding_implementation_base
	)
};

bool
is_io_binary_encoding (io_encoding_t const *encoding) {
	return io_encoding_has_implementation (
		encoding,&io_binary_encoding_implementation
	);
}

static io_encoding_t* 
io_text_encoding_new (io_byte_memory_t *bm) {
	io_text_encoding_t *this = io_byte_memory_allocate (
		bm,sizeof(io_text_encoding_t)
	);

	if (this != NULL) {
		extern EVENT_DATA io_encoding_implementation_t io_text_encoding_implementation;
		this->implementation = &io_text_encoding_implementation;
		this->bm = bm;
		this = io_binary_encoding_initialise ((io_binary_encoding_t*) this);
		this->visited = NULL;
	}

	return (io_encoding_t*) this;
};

static void
io_text_encoding_free (io_encoding_t *encoding) {
	if (encoding != NULL) {
		io_text_encoding_t *this = (io_text_encoding_t*) encoding;
		io_byte_memory_free (this->bm,this->byte_stream);
		if (this->visited) {
			free_vref_bucket_hash_table (this->visited);
		}
		io_byte_memory_free (this->bm,this);
	}
}

vref_hash_table_t*
io_text_encoding_get_visited (io_text_encoding_t *this) {
	if (this->visited == NULL) {
		this->visited = mk_vref_bucket_hash_table (this->bm,17);
	}
	return this->visited;
}

EVENT_DATA io_encoding_implementation_t 
io_text_encoding_implementation = {
	SPECIALISE_IO_BINARY_ENCODING_IMPLEMENTATION (
		&io_binary_encoding_implementation
	)
	.make_encoding = io_text_encoding_new,
	.free = io_text_encoding_free,
	.decode_to_io_value = io_binary_encoding_decode_to_io_value,
};

static io_encoding_t*
io_x70_encoding_new (io_byte_memory_t *bm) {
	io_binary_encoding_t *this = io_byte_memory_allocate (
		bm,sizeof(io_binary_encoding_t)
	);

	if (this != NULL) {
		extern EVENT_DATA io_encoding_implementation_t io_x70_encoding_implementation;
		this->implementation = &io_x70_encoding_implementation;
		this->bm = bm;
		this = io_binary_encoding_initialise(this);
	}

	return (io_encoding_t*) this;
};

void
io_x70_encoding_append_uint_value (io_encoding_t *encoding,uint32_t value) {
	uint8_t flag;
	io_encoding_append_byte (encoding,X70_UINT_VALUE_BYTE);
	do {
		flag = (value > 0x7f);
		io_encoding_append_byte (encoding,(value & 0x7f) | (flag << 7));
		value >>= 7;
	} while (flag);
}

int32_t
io_x70_encoding_take_uint_value (const uint8_t *b,const uint8_t *e,uint32_t *value) {
	uint8_t byte;
	int32_t c = 0;
	
	*value = 0;
	do {
		byte = *b++;
		c++;
		*value <<= 7;
		*value += (byte & 0x7f);
	} while (byte & 0x80 && b < e);
	
	return c;
}

vref_t
io_x70_decoder (io_encoding_t *encoding,io_value_memory_t *vm) {
	io_t *io = io_encoding_get_io (encoding);
	const uint8_t *b,*e;
	vref_t r_value = INVALID_VREF;
	
	io_encoding_get_content (encoding,&b,&e);
	
	while (b < e) {
		uint32_t u;
		
		switch (*b++) {
			case X70_UINT_VALUE_BYTE:
				b += io_x70_encoding_take_uint_value (b,e,&u);
				if (b < e) {
					io_value_implementation_t const *I;
					I = io_get_value_implementation (io,(const char*) b,u);
					b += u;
					if (I) {
						vref_t r_part = I->decode[IO_VALUE_ENCODING_FORMAT_X70] (vm,&b,e);
						if (vref_is_valid(r_part)) {
							r_value = r_part;
						}
					}
				}
			break;
		}
		
		b++;
	}
	
	return r_value;
}

EVENT_DATA io_encoding_implementation_t io_x70_encoding_implementation = {
	SPECIALISE_IO_BINARY_ENCODING_IMPLEMENTATION (
		&io_binary_encoding_implementation
	)
	.make_encoding = io_x70_encoding_new,
	.free = io_binary_encoding_free,
};

void
io_binary_encoding_free_memory (io_binary_encoding_t *this) {
	io_byte_memory_free (this->bm,this->byte_stream);
}

//
// Io language decoder
//

static bool io_source_decoder_push_context (io_source_decoder_t*,vref_t);

io_source_decoder_t*
mk_io_source_decoder (
	io_t *io,
	vref_t r_context,
	io_source_decoder_input_t parser,
	io_source_decoder_input_t on_error,
	is_symbol_t const *keywords
) {
	io_byte_memory_t *bm = io_get_byte_memory(io);
	io_source_decoder_t *this = io_byte_memory_allocate (
		bm,sizeof(io_source_decoder_t)
	);

	if (this) {
		this->io = io;
		this->buffer = mk_io_text_encoding (bm);
		this->error = NULL;
		this->reset_state = parser;
		this->error_state = on_error;
		this->keywords = keywords;

		this->context_stack = NULL;
		this->current_context = NULL;
		if (io_source_decoder_push_context (this,r_context)) {
			this->input_stack = io_byte_memory_allocate (
				io_source_decoder_byte_memory(this),sizeof(io_source_decoder_input_t*)
			);
			this->current_input = this->input_stack;
			if (!this->input_stack) {
				io_byte_memory_free (io_source_decoder_byte_memory(this),this->context_stack);
				io_byte_memory_free (io_source_decoder_byte_memory(this),this);
				this = NULL;
			}
		} else {
			io_byte_memory_free (io_source_decoder_byte_memory(this),this);
			this = NULL;
		}
		io_source_decoder_goto (this,this->reset_state);
	}

	return this;
}

void
free_io_source_decoder_parse_memory (io_source_decoder_t *this) {
	io_source_decoder_context_t *cursor = this->context_stack;
	while (cursor <= this->current_context) {
		unreference_value (cursor->r_value);
		io_byte_memory_free (io_source_decoder_byte_memory(this),cursor->args);
		cursor++;
	}
	io_byte_memory_free (io_source_decoder_byte_memory(this),this->context_stack);
	io_byte_memory_free (io_source_decoder_byte_memory(this),this->input_stack);
}

void
free_io_source_decoder (io_source_decoder_t *this) {
	free_io_source_decoder_parse_memory (this);
	io_encoding_free(this->buffer);
	io_byte_memory_free (io_source_decoder_byte_memory(this),this);
}

void
io_source_decoder_reset (io_source_decoder_t *this) {
	vref_t r_value = this->context_stack->r_value;

	io_encoding_reset (this->buffer);
	this->error = NULL;

	free_io_source_decoder_parse_memory (this);

	this->context_stack = NULL;
	this->current_context = NULL;
	this->input_stack = NULL;

	if (io_source_decoder_push_context (this,r_value)) {
		this->input_stack = io_byte_memory_allocate (
			io_source_decoder_byte_memory(this),sizeof(io_source_decoder_input_t*)
		);
		this->current_input = this->input_stack;
		if (this->input_stack) {
			io_source_decoder_goto (this,this->reset_state);
		} else {
			io_panic (io_source_decoder_io(this),IO_PANIC_OUT_OF_MEMORY);
		}
	}
}

void
io_source_decoder_remove_last_character (io_source_decoder_t *this) {
}

void
io_source_decoder_next_character (
	io_source_decoder_t *this,uint32_t character
) {
	io_source_decoder_input(this) (this,character);
}

void
io_source_decoder_parse (io_source_decoder_t *this,const char *src) {
	while(*src) {
		io_source_decoder_input(this) (this,*src++);
	}
}

void
io_source_decoder_push_input (io_source_decoder_t *this,io_source_decoder_input_t s) {
	uint32_t depth = io_source_decoder_input_depth(this);
	this->input_stack = io_byte_memory_reallocate (
		io_source_decoder_byte_memory(this),this->input_stack,(depth + 1) * sizeof(io_source_decoder_input_t*)
	);
	this->current_input = this->input_stack + depth;
	io_source_decoder_input(this) = (s);
}

void
io_source_decoder_pop_input (io_source_decoder_t *this) {
	uint32_t depth = io_source_decoder_input_depth(this);
	if (depth > 1) {
		this->input_stack = io_byte_memory_reallocate (
			io_source_decoder_byte_memory(this),this->input_stack,(depth - 1) * sizeof(io_source_decoder_input_t*)
		);
		this->current_input = this->input_stack + (depth - 2);
	} else {
		io_source_decoder_set_last_error (
			this,"no item to pop from stack"
		);
		io_source_decoder_goto(this,this->error_state);
	}
}

static bool
io_source_decoder_push_context (io_source_decoder_t *this,vref_t r_value) {
	if (this->context_stack == NULL) {
		this->context_stack = io_byte_memory_reallocate (
			io_source_decoder_byte_memory(this),this->context_stack,sizeof(io_source_decoder_context_t)
		);
		this->current_context = this->context_stack;
	} else {
		int32_t depth = this->current_context - this->context_stack;
		this->context_stack = io_byte_memory_reallocate (
			io_source_decoder_byte_memory(this),this->context_stack,(depth + 1) * sizeof(io_source_decoder_context_t)
		);
		this->current_context = this->context_stack + (depth + 1);
	}

	if (this->context_stack) {
		this->current_context->r_value = reference_value (r_value);
		this->current_context->args = NULL;
		this->current_context->arity = 0;
		return true;
	} else {
		return false;
	}
}

static void
io_source_decoder_pop_context (io_source_decoder_t *this) {
	io_source_decoder_context_t *ctx = io_source_decoder_context(this);
	vref_t *arg = ctx->args, *end = arg + ctx->arity;
	uint32_t depth = this->current_context - this->context_stack;

	while (arg < end) {
		unreference_value (*arg);
		arg++;
	}

	io_byte_memory_free (io_source_decoder_byte_memory(this),ctx->args);
	ctx->args = NULL;
	ctx->arity = 0;

	if (depth > 0) {
		this->context_stack = io_byte_memory_reallocate (
			io_source_decoder_byte_memory(this),
			this->context_stack,
			(depth) * sizeof(io_source_decoder_context_t)
		);
		this->current_context = this->context_stack + (depth);
	}
}

bool
io_source_decoder_append_arg (io_source_decoder_t *this,vref_t r_value) {
	io_source_decoder_context_t *ctx = io_source_decoder_context(this);

	ctx->args = io_byte_memory_reallocate (
		io_source_decoder_byte_memory(this),ctx->args,(ctx->arity + 1) * sizeof(vref_t)
	);

	if (ctx->args != NULL) {
		ctx->args[ctx->arity] = reference_value (r_value);
		ctx->arity ++;
		return true;
	} else {
		return false;
	}
}

void
io_source_decoder_end_of_statement (io_source_decoder_t *this) {
	io_source_decoder_context_t *ctx = io_source_decoder_context(this);
	if (ctx->arity) {
		io_value_sendm (io_source_decoder_io(this),ctx->r_value,ctx->arity,ctx->args);
		io_source_decoder_pop_context (this);
	} else {
		// do i pop ??
	}
}

//
// the byte memory manager
//
#define UMM_DBGLOG_DEBUG(...)
#define DBGLOG_TRACE(...)

#define UMM_FREELIST_MASK (0x8000)
#define UMM_BLOCKNO_MASK  (0x7FFF)

#define UMM_NUMBLOCKS(m) (m)->number_of_blocks
#define UMM_BLOCK_SIZE(m) (1 << (m)->block_size_n)
//#define UMM_BLOCK(m,b)  (m)->heap[b]
#define UMM_BLOCK(this,b)	((umm_block_t*)(((void*) (this)->heap) + ((b) << io_byte_memory_block_size_bits(this))))

#define UMM_NBLOCK(m,b) (UMM_BLOCK(m,b)->header.used.next)
#define UMM_PBLOCK(m,b) (UMM_BLOCK(m,b)->header.used.prev)
#define UMM_NFREE(m,b)  (UMM_BLOCK(m,b)->body.free.next)
#define UMM_PFREE(m,b)  (UMM_BLOCK(m,b)->body.free.prev)
#define UMM_DATA(m,b)   (UMM_BLOCK(m,b)->body.data)

io_byte_memory_t*
mk_io_byte_memory (io_t *io,uint32_t size,uint32_t block_size) {
	io_byte_memory_t *this = io_byte_memory_allocate (
		io_get_byte_memory (io),sizeof(io_byte_memory_t)
	);
	
	if (this) {
		this->number_of_blocks = (size >> block_size);
		size = this->number_of_blocks << block_size;

		this->heap = io_byte_memory_allocate (
			io_get_byte_memory (io),size
		);
		if (this->heap) {
			initialise_io_byte_memory (io,this,block_size);
		} else {
			io_byte_memory_free (io_get_byte_memory (io),this);
			this= NULL;
		}
	}
	
	return this;
}

void
free_io_byte_memory (io_byte_memory_t *this) {
	io_byte_memory_t *bm = io_get_byte_memory (this->io);
	io_byte_memory_free (bm,this->heap);
	io_byte_memory_free (bm,this);
}

io_byte_memory_t*
initialise_io_byte_memory (
	io_t *io,io_byte_memory_t *mem,uint32_t block_size_bits
) {
  	// init heap pointer and size, and memset it to 0
	mem->block_size_n = block_size_bits;
	io_memset (
		mem->heap,
		0x00,
		mem->number_of_blocks << io_byte_memory_block_size_bits(mem)
	);
	mem->io = io;
  /* setup initial blank heap structure */
  {
    /* index of the 0th `umm_block_t` */
    const unsigned short int block_0th = 0;
    /* index of the 1st `umm_block_t` */
    const unsigned short int block_1th = 1;
    /* index of the latest `umm_block_t` */
    const unsigned short int block_last = UMM_NUMBLOCKS(mem) - 1;

    /* setup the 0th `umm_block_t`, which just points to the 1st */
    UMM_NBLOCK(mem,block_0th) = block_1th;
    UMM_NFREE(mem,block_0th)  = block_1th;
    UMM_PFREE(mem,block_0th)  = block_1th;

    /*
     * Now, we need to set the whole heap space as a huge free block. We should
     * not touch the 0th `umm_block_t`, since it's special: the 0th `umm_block_t`
     * is the head of the free block list. It's a part of the heap invariant.
     *
     * See the detailed explanation at the beginning of the file.
     */

    /*
     * 1th `umm_block_t` has pointers:
     *
     * - next `umm_block_t`: the latest one
     * - prev `umm_block_t`: the 0th
     *
     * Plus, it's a free `umm_block_t`, so we need to apply `UMM_FREELIST_MASK`
     *
     * And it's the last free block, so the next free block is 0.
     */
    UMM_NBLOCK(mem,block_1th) = block_last | UMM_FREELIST_MASK;
    UMM_NFREE(mem,block_1th)  = 0;
    UMM_PBLOCK(mem,block_1th) = block_0th;
    UMM_PFREE(mem,block_1th)  = block_0th;

    /*
     * latest `umm_block_t` has pointers:
     *
     * - next `umm_block_t`: 0 (meaning, there are no more `umm_block_ts`)
     * - prev `umm_block_t`: the 1st
     *
     * It's not a free block, so we don't touch NFREE / PFREE at all.
     */
    UMM_NBLOCK(mem,block_last) = 0;
    UMM_PBLOCK(mem,block_last) = block_1th;
  }
  
  return mem;
}

void
io_byte_memory_get_info (io_byte_memory_t *mem,memory_info_t *info) {
	unsigned short int blockNo = UMM_NBLOCK(mem,0) & UMM_BLOCKNO_MASK;

	info->total_bytes = UMM_NUMBLOCKS(mem) * io_byte_memory_block_size(mem);
	info->free_bytes = 0;
	info->used_bytes = 0;

	while( UMM_NBLOCK(mem,blockNo) & UMM_BLOCKNO_MASK ) {
		size_t curBlocks = (UMM_NBLOCK(mem,blockNo) & UMM_BLOCKNO_MASK ) - blockNo;

		if( UMM_NBLOCK(mem,blockNo) & UMM_FREELIST_MASK ) {
			// free
			info->free_bytes += curBlocks;
		} else {
			// allocated
			info->used_bytes += curBlocks;
		}
		blockNo = UMM_NBLOCK(mem,blockNo) & UMM_BLOCKNO_MASK;
	}

	info->free_bytes *= io_byte_memory_block_size(mem);
	info->used_bytes *= io_byte_memory_block_size(mem);
}

void
initialise_io_byte_memory_cursor (io_byte_memory_t *bm,uint16_t *cursor) {
	*cursor = 0;
}

void
incremental_iterate_io_byte_memory_allocations (
	io_byte_memory_t *bm,uint16_t *cursor,bool (*cb) (io_value_t*,void*),void *user_value
) {
	uint16_t begin = *cursor;
	
	do {
		uint16_t blockNo = *cursor;
		*cursor = UMM_NBLOCK(bm,*cursor) & UMM_BLOCKNO_MASK;
		if (
				blockNo != 0															// not first
			&&	((UMM_NBLOCK(bm,blockNo) & UMM_FREELIST_MASK) == 0)		// not free
			&&	(blockNo != (UMM_NUMBLOCKS(bm) - 1))							// not last
		) {
			if (!cb ((void *)&UMM_DATA(bm,blockNo),user_value)) {
				if ((UMM_NBLOCK(bm,*cursor) & UMM_FREELIST_MASK)) {
					// do not leave cursor at start of free list as
					// assimilations will make this invalid
					*cursor = UMM_NBLOCK(bm,0) & UMM_BLOCKNO_MASK;
				}
				return;
			}
		}
	} while (*cursor != begin);
	
	*cursor = 0;
}

void
iterate_io_byte_memory_allocations (
	io_byte_memory_t *bm,bool (*cb) (void*,void*),void *user_value
) {
	uint16_t cursor = UMM_NBLOCK(bm,0) & UMM_BLOCKNO_MASK;
	while( UMM_NBLOCK(bm,cursor) & UMM_BLOCKNO_MASK ) {
		if( UMM_NBLOCK(bm,cursor) & UMM_FREELIST_MASK ) {
			if (!cb ((void *)&UMM_DATA(bm,cursor),user_value)) {
				break;
			}
		}
		cursor = UMM_NBLOCK(bm,cursor) & UMM_BLOCKNO_MASK;
	}
}

static unsigned short int 
umm_blocks (io_byte_memory_t *mem,size_t size) {
	return (
		1 + (
				(size + (sizeof(((umm_block_t *)0)->body)) - 1)
			>>	io_byte_memory_block_size_bits(mem)
		)
	);
}

/* ------------------------------------------------------------------------ */
/*
 * Split the block `c` into two blocks: `c` and `c + blocks`.
 *
 * - `new_freemask` should be `0` if `c + blocks` used, or `UMM_FREELIST_MASK`
 *   otherwise.
 *
 * Note that free pointers are NOT modified by this function.
 */
static void
umm_split_block (
	io_byte_memory_t *mem,
	unsigned short int c,
    unsigned short int blocks,
    unsigned short int new_freemask
) {
	UMM_NBLOCK(mem,c+blocks) = (UMM_NBLOCK(mem,c) & UMM_BLOCKNO_MASK) | new_freemask;
   UMM_PBLOCK(mem,c+blocks) = c;

   UMM_PBLOCK(mem,UMM_NBLOCK(mem,c) & UMM_BLOCKNO_MASK) = (c+blocks);
   UMM_NBLOCK(mem,c) = (c+blocks);
}

/* ------------------------------------------------------------------------ */

static void
umm_disconnect_from_free_list (io_byte_memory_t *mem,unsigned short int c ) {
  /* Disconnect this block from the FREE list */

  UMM_NFREE(mem,UMM_PFREE(mem,c)) = UMM_NFREE(mem,c);
  UMM_PFREE(mem,UMM_NFREE(mem,c)) = UMM_PFREE(mem,c);

  /* And clear the free block indicator */

  UMM_NBLOCK(mem,c) &= (~UMM_FREELIST_MASK);
}

/* ------------------------------------------------------------------------
 * The umm_assimilate_up() function assumes that UMM_NBLOCK(c) does NOT
 * have the UMM_FREELIST_MASK bit set!
 */

static void
umm_assimilate_up (io_byte_memory_t *mem,unsigned short int c ) {
	uint16_t next = UMM_NBLOCK(mem,UMM_NBLOCK(mem,c));
	if(next  & UMM_FREELIST_MASK ) {
		/*
		* The next block is a free block, so assimilate up and remove it from
		* the free list
		*/

		UMM_DBGLOG_DEBUG( "Assimilate up to next block, which is FREE\n" );

		/* Disconnect the next block from the FREE list */

		umm_disconnect_from_free_list(mem, UMM_NBLOCK(mem,c) );

		/* Assimilate the next block with this one */

		UMM_PBLOCK(mem,UMM_NBLOCK(mem,UMM_NBLOCK(mem,c)) & UMM_BLOCKNO_MASK) = c;
		UMM_NBLOCK(mem,c) = UMM_NBLOCK(mem,UMM_NBLOCK(mem,c)) & UMM_BLOCKNO_MASK;
	}
}


/* ------------------------------------------------------------------------
 * The umm_assimilate_down() function assumes that UMM_NBLOCK(c) does NOT
 * have the UMM_FREELIST_MASK bit set!
 */

static unsigned short int 
umm_assimilate_down (
	io_byte_memory_t *mem,unsigned short int c, unsigned short int freemask
) {
	
  UMM_NBLOCK(mem,UMM_PBLOCK(mem,c)) = UMM_NBLOCK(mem,c) | freemask;
  UMM_PBLOCK(mem,UMM_NBLOCK(mem,c)) = UMM_PBLOCK(mem,c);

  return( UMM_PBLOCK(mem,c) );
}



/* ------------------------------------------------------------------------
 * Must be called only from within critical sections guarded by
 * UMM_CRITICAL_ENTRY() and UMM_CRITICAL_EXIT().
 */

static io_memory_status_t
umm_free_core(io_byte_memory_t *mem,void *ptr) {
	io_memory_status_t result = IO_MEMORY_FREE_OK;
	unsigned short int c;

	/*
	* NOTE:  See the new umm_info() function that you can use to see if a ptr is
	*        on the free list!
	*/

	/* Figure out which block we're in. Note the use of truncated division... */

	c = (((char *)ptr)-(char *)(&(mem->heap[0]))) >> io_byte_memory_block_size_bits(mem);

	if (
			ptr < (void *) (&(mem->heap[1]))
		||	ptr >= (void *) (UMM_BLOCK(mem,io_byte_memory_number_of_blocks(mem)))
	) {
		result = IO_MEMORY_FREE_ERROR_NOT_IN_MEMORY;
	} else if (UMM_NBLOCK(mem,c) & UMM_FREELIST_MASK ) {
		result = IO_MEMORY_FREE_ERROR_ALREADY_FREE;
	} else {

		/* Now let's assimilate this block with the next one if possible. */

		umm_assimilate_up(mem,c);

		/* Then assimilate with the previous block if possible */

		if( UMM_NBLOCK(mem,UMM_PBLOCK(mem,c)) & UMM_FREELIST_MASK ) {

			UMM_DBGLOG_DEBUG( "Assimilate down to next block, which is FREE\n" );

			c = umm_assimilate_down(mem,c, UMM_FREELIST_MASK);
		} else {
			/*
			* The previous block is not a free block, so add this one to the head
			* of the free list
			*/

			UMM_DBGLOG_DEBUG( "Just add to head of free list\n" );

			UMM_PFREE(mem,UMM_NFREE(mem,0)) = c;
			UMM_NFREE(mem,c)            = UMM_NFREE(mem,0);
			UMM_PFREE(mem,c)            = 0;
			UMM_NFREE(mem,0)            = c;

			UMM_NBLOCK(mem,c) |= UMM_FREELIST_MASK;
		}
	}
  
	return result;
}

/* ------------------------------------------------------------------------ */

io_memory_status_t
umm_free(io_byte_memory_t *mem,void *ptr) {
	io_memory_status_t s;

  /* If we're being asked to free a NULL pointer, well that's just silly! */

  if( (void *)0 == ptr ) {
    UMM_DBGLOG_DEBUG( "free a null pointer -> do nothing\n" );
    return IO_MEMORY_FREE_OK;
  }

  /* Free the memory withing a protected critical section */

  UMM_CRITICAL_ENTRY(mem);

  s = umm_free_core(mem,ptr);

  UMM_CRITICAL_EXIT(mem);
  
  return s;
}

/* ------------------------------------------------------------------------
 * Must be called only from within critical sections guarded by
 * UMM_CRITICAL_ENTRY() and UMM_CRITICAL_EXIT().
 */

static void *umm_malloc_core(io_byte_memory_t *mem,size_t size) {
  unsigned short int blocks;
  unsigned short int blockSize = 0;

  unsigned short int bestSize;
  unsigned short int bestBlock;

  unsigned short int cf;

  blocks = umm_blocks(mem,size);

  /*
   * Now we can scan through the free list until we find a space that's big
   * enough to hold the number of blocks we need.
   *
   * This part may be customized to be a best-fit, worst-fit, or first-fit
   * algorithm
   */

  cf = UMM_NFREE(mem,0);

  bestBlock = UMM_NFREE(mem,0);
  bestSize  = 0x7FFF;

  while( cf ) {
	  uint16_t *ptr = &UMM_NBLOCK(mem,cf);
	  blockSize = (*ptr & UMM_BLOCKNO_MASK) - cf;
//    blockSize = (UMM_NBLOCK(mem,cf) & UMM_BLOCKNO_MASK) - cf;

    DBGLOG_TRACE( "Looking at block %6i size %6i\n", cf, blockSize );

#if defined UMM_BEST_FIT
    if( (blockSize >= blocks) && (blockSize < bestSize) ) {
      bestBlock = cf;
      bestSize  = blockSize;
    }
#elif defined UMM_FIRST_FIT
    /* This is the first block that fits! */
    if( (blockSize >= blocks) )
      break;
#else
#  error "No UMM_*_FIT is defined - check umm_malloc_cfg.h"
#endif

    cf = UMM_NFREE(mem,cf);
  }

  if( 0x7FFF != bestSize ) {
    cf        = bestBlock;
    blockSize = bestSize;
  }

  if( UMM_NBLOCK(mem,cf) & UMM_BLOCKNO_MASK && blockSize >= blocks ) {
    /*
     * This is an existing block in the memory heap, we just need to split off
     * what we need, unlink it from the free list and mark it as in use, and
     * link the rest of the block back into the freelist as if it was a new
     * block on the free list...
     */

    if( blockSize == blocks ) {
      /* It's an exact fit and we don't neet to split off a block. */
      UMM_DBGLOG_DEBUG( "Allocating %6i blocks starting at %6i - exact\n", blocks, cf );

      /* Disconnect this block from the FREE list */

      umm_disconnect_from_free_list(mem,cf );

    } else {
      /* It's not an exact fit and we need to split off a block. */
      UMM_DBGLOG_DEBUG( "Allocating %6i blocks starting at %6i - existing\n", blocks, cf );

      /*
       * split current free block `cf` into two blocks. The first one will be
       * returned to user, so it's not free, and the second one will be free.
       */
      umm_split_block(mem, cf, blocks, UMM_FREELIST_MASK /*new block is free*/ );

      /*
       * `umm_split_block()` does not update the free pointers (it affects
       * only free flags), but effectively we've just moved beginning of the
       * free block from `cf` to `cf + blocks`. So we have to adjust pointers
       * to and from adjacent free blocks.
       */

      /* previous free block */
      UMM_NFREE(mem,UMM_PFREE(mem,cf) ) = cf + blocks;
      UMM_PFREE(mem,cf + blocks ) = UMM_PFREE(mem,cf);

      /* next free block */
      UMM_PFREE(mem,UMM_NFREE(mem,cf) ) = cf + blocks;
      UMM_NFREE(mem,cf + blocks ) = UMM_NFREE(mem,cf);
    }
  } else {
    /* Out of memory */

    UMM_DBGLOG_DEBUG(  "Can't allocate %5i blocks\n", blocks );

    return( (void *)NULL );
  }

  return( (void *)&UMM_DATA(mem,cf) );
}

/* ------------------------------------------------------------------------ */

void *umm_malloc(io_byte_memory_t *mem,size_t size) {
  void *ptr = NULL;


  /*
   * the very first thing we do is figure out if we're being asked to allocate
   * a size of 0 - and if we are we'll simply return a null pointer. if not
   * then reduce the size by 1 byte so that the subsequent calculations on
   * the number of blocks to allocate are easier...
   */

  if( 0 == size ) {
    UMM_DBGLOG_DEBUG( "malloc a block of 0 bytes -> do nothing\n" );

    return( ptr );
  }

  /* Allocate the memory withing a protected critical section */

  UMM_CRITICAL_ENTRY(mem);

  ptr = umm_malloc_core(mem,size);

  UMM_CRITICAL_EXIT(mem);

  return( ptr );
}

/* ------------------------------------------------------------------------ */

void *umm_realloc(io_byte_memory_t *mem,void *ptr, size_t size) {

  unsigned short int blocks;
  unsigned short int blockSize;
  unsigned short int prevBlockSize = 0;
  unsigned short int nextBlockSize = 0;

  unsigned short int c;

  size_t curSize;

  /*
   * This code looks after the case of a NULL value for ptr. The ANSI C
   * standard says that if ptr is NULL and size is non-zero, then we've
   * got to work the same a malloc(). If size is also 0, then our version
   * of malloc() returns a NULL pointer, which is OK as far as the ANSI C
   * standard is concerned.
   */

  if( ((void *)NULL == ptr) ) {
    UMM_DBGLOG_DEBUG( "realloc the NULL pointer - call malloc()\n" );

    return umm_malloc(mem,size);
  }

  /*
   * Now we're sure that we have a non_NULL ptr, but we're not sure what
   * we should do with it. If the size is 0, then the ANSI C standard says that
   * we should operate the same as free.
   */

  if( 0 == size ) {
    UMM_DBGLOG_DEBUG( "realloc to 0 size, just free the block\n" );

    umm_free(mem,ptr);

    return( (void *)NULL );
  }

  /*
   * Otherwise we need to actually do a reallocation. A naiive approach
   * would be to malloc() a new block of the correct size, copy the old data
   * to the new block, and then free the old block.
   *
   * While this will work, we end up doing a lot of possibly unnecessary
   * copying. So first, let's figure out how many blocks we'll need.
   */

  blocks = umm_blocks(mem,size);

  /* Figure out which block we're in. Note the use of truncated division... */

  c = (((char *)ptr)-(char *)(&(mem->heap[0]))) >> io_byte_memory_block_size_bits(mem);

  /* Figure out how big this block is ... the free bit is not set :-) */

  blockSize = (UMM_NBLOCK(mem,c) - c);

  /* Figure out how many bytes are in this block */

  curSize = (
			(blockSize << io_byte_memory_block_size_bits(mem))
		-	(sizeof(((umm_block_t *)0)->header))
	);

  /* Protect the critical section... */
  UMM_CRITICAL_ENTRY(mem);

  /* Now figure out if the previous and/or next blocks are free as well as
   * their sizes - this will help us to minimize special code later when we
   * decide if it's possible to use the adjacent blocks.
   *
   * We set prevBlockSize and nextBlockSize to non-zero values ONLY if they
   * are free!
   */

  if ((UMM_NBLOCK(mem,UMM_NBLOCK(mem,c)) & UMM_FREELIST_MASK)) {
      nextBlockSize = (UMM_NBLOCK(mem,UMM_NBLOCK(mem,c)) & UMM_BLOCKNO_MASK) - UMM_NBLOCK(mem,c);
  }

  if ((UMM_NBLOCK(mem,UMM_PBLOCK(mem,c)) & UMM_FREELIST_MASK)) {
      prevBlockSize = (c - UMM_PBLOCK(mem,c));
  }

  UMM_DBGLOG_DEBUG( "realloc blocks %i blockSize %i nextBlockSize %i prevBlockSize %i\n", blocks, blockSize, nextBlockSize, prevBlockSize );

  /*
   * Ok, now that we're here we know how many blocks we want and the current
   * blockSize. The prevBlockSize and nextBlockSize are set and we can figure
   * out the best strategy for the new allocation as follows:
   *
   * 1. If the new block is the same size or smaller than the current block do
   *    nothing.
   * 2. If the next block is free and adding it to the current block gives us
   *    enough memory, assimilate the next block.
   * 3. If the prev block is free and adding it to the current block gives us
   *    enough memory, remove the previous block from the free list, assimilate
   *    it, copy to the new block.
   * 4. If the prev and next blocks are free and adding them to the current
   *    block gives us enough memory, assimilate the next block, remove the
   *    previous block from the free list, assimilate it, copy to the new block.
   * 5. Otherwise try to allocate an entirely new block of memory. If the
   *    allocation works free the old block and return the new pointer. If
   *    the allocation fails, return NULL and leave the old block intact.
   *
   * All that's left to do is decide if the fit was exact or not. If the fit
   * was not exact, then split the memory block so that we use only the requested
   * number of blocks and add what's left to the free list.
   */

    if (blockSize >= blocks) {
        UMM_DBGLOG_DEBUG( "realloc the same or smaller size block - %i, do nothing\n", blocks );
        /* This space intentionally left blank */
    } else if ((blockSize + nextBlockSize) >= blocks) {
        UMM_DBGLOG_DEBUG( "realloc using next block - %i\n", blocks );
        umm_assimilate_up(mem,c);
        blockSize += nextBlockSize;
    } else if ((prevBlockSize + blockSize) >= blocks) {
        UMM_DBGLOG_DEBUG( "realloc using prev block - %i\n", blocks );
        umm_disconnect_from_free_list(mem,UMM_PBLOCK(mem,c) );
        c = umm_assimilate_down(mem,c, 0);
        memmove( (void *)&UMM_DATA(mem,c), ptr, curSize );
        ptr = (void *)&UMM_DATA(mem,c);
        blockSize += prevBlockSize;
    } else if ((prevBlockSize + blockSize + nextBlockSize) >= blocks) {
        UMM_DBGLOG_DEBUG( "realloc using prev and next block - %i\n", blocks );
        umm_assimilate_up(mem,c );
        umm_disconnect_from_free_list(mem,UMM_PBLOCK(mem,c) );
        c = umm_assimilate_down(mem,c, 0);
        memmove( (void *)&UMM_DATA(mem,c), ptr, curSize );
        ptr = (void *)&UMM_DATA(mem,c);
        blockSize += (prevBlockSize + nextBlockSize);
    } else {
        UMM_DBGLOG_DEBUG( "realloc a completely new block %i\n", blocks );
        void *oldptr = ptr;
        if( (ptr = umm_malloc_core(mem,size )) ) {
            UMM_DBGLOG_DEBUG( "realloc %i to a bigger block %i, copy, and free the old\n", blockSize, blocks );
            memcpy( ptr, oldptr, curSize );
            umm_free_core(mem,oldptr );
        } else {
            UMM_DBGLOG_DEBUG(
            	"realloc %i to a bigger block %i failed - return NULL and leave the old block!\n",
				blockSize, blocks
            );
            /* This space intentionally left blank */
        }
        blockSize = blocks;
    }

    /* Now all we need to do is figure out if the block fit exactly or if we
     * need to split and free ...
     */

    if (blockSize > blocks ) {
        UMM_DBGLOG_DEBUG( "split and free %i blocks from %i\n", blocks, blockSize );
        umm_split_block(mem,c, blocks, 0 );
        umm_free_core(mem, (void *)&UMM_DATA(mem,c+blocks) );
    }

    /* Release the critical section... */
    UMM_CRITICAL_EXIT(mem);

    return( ptr );
}

void *umm_calloc(io_byte_memory_t *mem,size_t num, size_t item_size ) {
	void *ret = umm_malloc (mem,(size_t)(item_size * num));

	if (ret) {
	  io_memset (ret, 0x00, (size_t)(item_size * num));
	}

	return ret;
}

bool
io_byte_memory_test_block_is_free (io_byte_memory_t *this,uint16_t block) {
	return (UMM_NBLOCK(this,block) & UMM_FREELIST_MASK) == UMM_FREELIST_MASK;
}

uint16_t
io_byte_memory_test_get_block_prev (io_byte_memory_t *this,uint16_t block) {
	return UMM_PBLOCK(this,block);
}

uint16_t
io_byte_memory_test_get_block_next (io_byte_memory_t *this,uint16_t block) {
	return UMM_NBLOCK(this,block) & UMM_BLOCKNO_MASK;
}

uint16_t
io_byte_memory_test_get_block_prev_free (io_byte_memory_t *this,uint16_t block) {
	return UMM_PFREE(this,block);
}

uint16_t
io_byte_memory_test_get_block_next_free (io_byte_memory_t *this,uint16_t block) {
	return UMM_NFREE(this,block);
}

//
//
//
void
pq_sort_exchange (void* a[],int i,int j) {
	void* s = a[i];
	a[i] = a[j];
	a[j] = s;
}

int
pq_sort_partition (void* a[],int l,int h,quick_sort_compare_t cmp) {
	int i = l-1;
	int j = h;
	void* v = a[h];

	while (true) {

		while(cmp(a[++i],v) < 0);
		while(cmp(a[--j],v) > 0) if (j == i)  break;

		if (i >= j) break;

		pq_sort_exchange (a,i,j);
	}

	pq_sort_exchange (a,i,h);
	
	return i;
}

void
pq_sort_recurse (void* a[],int l,int h,quick_sort_compare_t cmp) {
	if (h <= l) {
		return;
	} else {
		int j = pq_sort_partition (a,l,h,cmp);
		pq_sort_recurse (a,l,j-1,cmp);
		pq_sort_recurse (a,j+1,h,cmp);
	}
}

//
// cht
//

static io_constrained_hash_t*	initialise_constrained_hash (io_constrained_hash_t*);
uint32_t	cht_hash1(io_constrained_hash_t*,vref_t);

#define cht_keys_equal(a,b)				vref_is_equal_to(a,b)
#define cht_values_equal(a,b)				vref_is_equal_to(a,b)
#define cht_keys_greater_than(a,b)		(vref_get_as_builtin_integer(a) > vref_get_as_builtin_integer(b))

//
// cannot reference when used to map cacheds references
//
#define CHT_REFERENCES_VALUES 0

/*
 *-----------------------------------------------------------------------------
 *
 * mk_io_constrained_hash --
 *
 *-----------------------------------------------------------------------------
 */
io_constrained_hash_t*
mk_io_constrained_hash (
	io_byte_memory_t *memory,
	uint32_t size,
	cht_begin_purge_helper_t begin_purge,
	cht_purge_entry_helper_t on_purge,
	void *user_data

) {
	io_constrained_hash_t *this = io_byte_memory_allocate (
		memory,sizeof(io_constrained_hash_t)
	);
	
	if (this) {
		this->io = memory->io;
		size = next_prime_u32_integer (size);
		this->entries = io_byte_memory_allocate (
			memory,cht_entries_size (size)
		);

		if (this->entries) {
			this->ordered = io_byte_memory_allocate (
				memory,cht_ordered_size (size)
			);
			if (this->ordered) {
				this->table_size = size;
				this->begin_purge = begin_purge;
				this->purge_callback = on_purge;
				this->user_data = user_data;
				initialise_constrained_hash (this);
			} else {
				goto error2;
			}
		} else {
			goto error1;
		}
		
	}
	
	return this;

error2:
	io_byte_memory_free (memory,this->entries);

error1:
	io_byte_memory_free (memory,this);
	
	return NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 * free_io_constrained_hash --
 *
 * Arguments
 * =========
 * memory		MUST be the memory this was allocated in
 * this			the cht to free
 *
 *-----------------------------------------------------------------------------
 */
void
free_io_constrained_hash (io_byte_memory_t *memory,io_constrained_hash_t *this) {
	#if CHT_REFERENCES_VALUES
	for(uint32_t i = 0; i < this->table_size; i++ ) {
		if (vref_is_valid(this->entries[i].key)) {
			unreference_io_value(this->entries[i].key);
		}
		if (vref_is_valid(this->entries[i].value)) {
			unreference_io_value(this->entries[i].value);
		}
	}
	#endif
	
	io_byte_memory_free (memory,this->entries);
	io_byte_memory_free (memory,this->ordered);
	io_byte_memory_free (memory,this);
}

/*
 *-----------------------------------------------------------------------------
 *
 * initialise_constrained_hash --
 *
 * Initialise a constrained hash structure. 
 *
 * Arguments
 * =========
 * this				
 *
 *-----------------------------------------------------------------------------
 */
static io_constrained_hash_t*
initialise_constrained_hash (io_constrained_hash_t *this) {
	uint32_t i;

	for( i = 0; i < this->table_size; i++ ) {
		this->entries[i].key = INVALID_VREF;
		this->entries[i].value = INVALID_VREF;
		this->entries[i].age = 0;
		this->entries[i].info.free = 1;
		this->entries[i].info.access_count = 0;
		this->entries[i].successor = NULL;
		this->entries[i].predecessor = NULL;
		this->ordered[i] = this->entries + i;
	}

	this->entry_count = 0;
	this->prune_count = this->table_size/10 + 1;		// prune 10% of entries
	this->entry_limit = this->table_size*4/5;			// allow 20% unused

	return this;
}

/*
 *-----------------------------------------------------------------------------
 *
 * murmur3_32 --
 *
 * compute hash of a value
 *
 *-----------------------------------------------------------------------------
 */
uint32_t
murmur3_32 (uint8_t const *key, size_t len) {
	uint32_t h = 0x27d4eb2d;	// seed
	if (len > 3) {
		uint32_t const* key_x4 = (uint32_t const*) key;
		size_t i = len >> 2;
		do {
			uint32_t k = *key_x4++;
			k *= 0xcc9e2d51;
			k = (k << 15) | (k >> 17);
			k *= 0x1b873593;
			h ^= k;
			h = (h << 13) | (h >> 19);
			h = (h * 5) + 0xe6546b64;
		} while (--i);
		key = (uint8_t const*) key_x4;
	}
	if (len & 3) {
		size_t i = len & 3;
		uint32_t k = 0;
		key = &key[i - 1];
		do {
			k <<= 8;
			k |= *key--;
		} while (--i);
		k *= 0xcc9e2d51;
		k = (k << 15) | (k >> 17);
		k *= 0x1b873593;
		h ^= k;
	}
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

uint32_t
cht_hash1 (io_constrained_hash_t *this,vref_t a) {
	return (
			murmur3_32 ((uint8_t const*) &a,sizeof(vref_t))
		%	this->table_size
	);
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_compare_entries_by_age --
 *
 * Compare for qsort: an entry is greater if access_count is greater or, if equal
 * age.
 *
 *-----------------------------------------------------------------------------
 */
int
cht_compare_entries_by_age (void const *a,void const *b) {
	io_constrained_hash_entry_t const *w1 = (io_constrained_hash_entry_t const*) a;
	io_constrained_hash_entry_t const *w2 = (io_constrained_hash_entry_t const*) b;
	
	if (w1->info.free || w2->info.free) {
		if (w1->info.free && w2->info.free) {
			return 0;
		} else if (w1->info.free) {
			return 1;
		} else {
			return -1;
		}
	} else if (w1->info.access_count == w2->info.access_count) {
		return (w1->age == w2->age) ?
			0
			: (w1->age > w2->age) ?
			1
			: -1;
	} else {
		return (w1->info.access_count > w2->info.access_count) ?	1 : -1;
	}
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_compare_entries_by_key --
 *
 * Compare by key for qsort (push all free entries to end of ordered list).
 *
 *-----------------------------------------------------------------------------
 */
int
cht_compare_entries_by_key (void const *a,void const *b) {
	io_constrained_hash_entry_t const *w1 = ((io_constrained_hash_entry_t const*) a);
	io_constrained_hash_entry_t const *w2 = ((io_constrained_hash_entry_t const*) b);
	if (w1->info.free || w2->info.free) {
		if (w1->info.free && w2->info.free) {
			return 0;
		} else if (w1->info.free) {
			return 1;
		} else {
			return -1;
		}
	} else {
		return cht_keys_greater_than(w1->key,w2->key )
			? 1 
			: cht_keys_equal(w1->key,w2->key) 
			? 0 
			: -1;
	}
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_sort_user --
 *
 * Create ordering based on a user supplied comparison.
 *
 *-----------------------------------------------------------------------------
 */
void
cht_sort_user (
	io_constrained_hash_t *this,int (*compare)(void const*,void const*)
) {
	pq_sort ((void**) this->ordered,this->table_size,compare);
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_sort --
 *
 * Create ordering that puts the oldest entry with least access's first.
 * The ordering is stored in the cht's ordered array and will thus be
 * overwritten by each call to sort.
 *
 *-----------------------------------------------------------------------------
 */
void
cht_sort (io_constrained_hash_t *this) {
	pq_sort (
		(void**) this->ordered,
		this->table_size,
		cht_compare_entries_by_age
	);
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_prune --
 *
 * Keep hash table size below limit.
 *
 *-----------------------------------------------------------------------------
 */
static void
cht_prune (io_constrained_hash_t *this) {
	if (this->entry_count >= this->entry_limit) {
		io_constrained_hash_entry_t **entry = this->ordered,**end = entry + this->table_size;
		uint32_t i = 0;

		#if 0 && defined(CHT_DEBUG_WORKER)
		printw(
			CHT_DEBUG_WORKER,
			"prune up to %u of %u, lim=%u\n",
			this->prune_count,
			this->entry_count,
			this->entry_limit
		);
		#endif
		
		if (this->begin_purge) {
			this->begin_purge(this->user_data);
		}
		
		cht_sort(this);
		while (entry < end && i < this->prune_count) {
			if(!(*entry)->info.free) {
				i++;
				#if 0 && defined(CHT_DEBUG_WORKER)
				printw(CHT_DEBUG_WORKER," -- %u, #%u\n",(*entry)->key,cht_hash1(this,(*entry)->key));
				#endif
				if (this->purge_callback) {
					if (this->purge_callback((*entry)->key,(*entry)->value,this->user_data)) {
						cht_unset(this,(*entry)->key);
					}
				} else {
					cht_unset(this,(*entry)->key);
				}
			}
			entry++;
		}
	}
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_find_entry --
 *
 * Retrieve a key-value pair from a hash table.  Does not count 
 * as an access.
 *
 * Arguments
 * =========
 * this			this hash
 * key			key to find
 * return		pointer to entry with key == key, or NULL
 *
 *-----------------------------------------------------------------------------
 */
io_constrained_hash_entry_t*
cht_find_entry (io_constrained_hash_t *this,vref_t key) {
	io_constrained_hash_entry_t *entry = (
		this->entries + cht_hash1(this,key)
	);
	
	if (entry->info.free == 0) {
		while (1) {
			if (cht_keys_equal(entry->key,key)) {
				return entry;
			} else if (entry->successor) {
				entry = entry->successor;
			} else {
				break;
			}
		};
	}
	
	return NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_move --
 *
 * Move an entry.
 *
 *-----------------------------------------------------------------------------
 */
void
cht_move (
	io_constrained_hash_t *this,
	io_constrained_hash_entry_t *dest,
	io_constrained_hash_entry_t *src
) {
	#ifdef CHT_DEBUG_WORKER
	io_printf (this->io,"move [%u] <- %u\n",cht_index_of_entry(this,dest),src->key);
	#endif
	
	#if CHT_REFERENCES_VALUES
	if (vref_is_valid(dest->key)) {
		unreference_io_value (dest->key);
	}
	dest->key = reference_io_value (src->key);

	if (vref_is_valid(dest->value)) {
		unreference_io_value (dest->value);
	}
	dest->value = reference_io_value (src->value);
	#else
	dest->key = src->key;
	dest->value = src->value;
	#endif
	
	dest->age = src->age;
	dest->info.access_count = src->info.access_count;
	dest->successor = src->successor;
	dest->predecessor = src->predecessor;
	if (src->successor) {
		src->successor->predecessor = dest;
	}
	if (src->predecessor) {
		src->predecessor->successor = dest;
	}
	src->info.free = 1;
	src->info.access_count = 0;
	src->successor = NULL;
	src->predecessor = NULL;
	dest->info.free = 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_longest_chain --
 *
 * Debug helper.
 *
 *-----------------------------------------------------------------------------
 */
uint32_t
cht_longest_chain (io_constrained_hash_t const *this) {
	io_constrained_hash_entry_t 
		*e,
		*entry = this->entries,
		*end = entry + this->table_size;
	uint32_t max = 0;

	while (entry < end) {
		if(!entry->info.free) {
			uint32_t count = 0;
			e = entry;
			while(e) {
				count++;
				e = e->successor;
			}
			if (count > max) max = count;
		}
		entry++;
	}
	return max;
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_unlink_entry --
 *
 *-----------------------------------------------------------------------------
 */
void
cht_unlink_entry(io_constrained_hash_entry_t *entry) {
	if(entry->predecessor) {
		entry->predecessor->successor = entry->successor;
	}
	if(entry->successor) {
		entry->successor->predecessor = entry->predecessor;
	}
	entry->predecessor = NULL;
	entry->successor = NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_find_entry_for_chain_root --
 *
 * Find an entry in e's probe chain that is not a primal-hash.
 *
 *-----------------------------------------------------------------------------
 */
io_constrained_hash_entry_t*
cht_find_entry_for_chain_root (
	io_constrained_hash_t *this,io_constrained_hash_entry_t *e,uint32_t h1
) {
	while (e) {
		if (cht_hash1(this,e->key) == h1) {
			return e;
		}
		e = e->successor;
	}
	return NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_unset --
 *
 * Remove an entry from a hash table.
 *
 *-----------------------------------------------------------------------------
 */
bool
cht_unset (io_constrained_hash_t *this,vref_t key) {
	io_constrained_hash_entry_t *free_root = cht_find_entry (this,key);
	if (free_root) {
		io_constrained_hash_entry_t *pri, *fwrd = free_root->successor;
		uint32_t free_chain_root = cht_index_of_entry(this,free_root);
		
		#ifdef CHT_DEBUG_WORKER
		printw (CHT_DEBUG_WORKER,"unset %u #%u\n",key,cht_hash1(this,free_root->key));
		#endif

		cht_unlink_entry (free_root);
		free_root->info.free = 1;
		free_root->info.user_flag1 = 0;
		free_root->info.user_flag2 = 0;

	again:
		#ifdef CHT_DEBUG_WORKER
		printw (CHT_DEBUG_WORKER,"free chain root = #%u\n",free_chain_root);
		#endif
		pri = cht_find_entry_for_chain_root (this,fwrd,free_chain_root);
		if (pri) {
			#ifdef CHT_DEBUG_WORKER
			printw(CHT_DEBUG_WORKER,"free chain entry found %u\n",pri->key);
			#endif
			fwrd = pri->successor;
			cht_move(this,free_root,pri);
			free_chain_root = cht_index_of_entry(this,pri);
			free_root = pri;
			goto again;
		}
	
		this->entry_count --;
		return true;
	} else {
		#ifdef CHT_DEBUG_WORKER
		printw (CHT_DEBUG_WORKER,"unset key %u not found\n",key);
		#endif
		return false;
	}
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_get_free_entry --
 *
 * Get free entry, helper for cht_set_value().  In general the free nodes
 * congregate at the 'bottom' of the ordered table.  So we start searching
 * from the end of this->ordered.
 *
 *-----------------------------------------------------------------------------
 */
io_constrained_hash_entry_t*
cht_get_free_entry (io_constrained_hash_t *this) {
	io_constrained_hash_entry_t **entry = this->ordered + this->table_size - 1;
	while (entry >= this->ordered) {
		if ((*entry)->info.free) {
			return *entry;
		}
		entry --;
	}

	// no free entries?
	io_panic(this->io,IO_PANIC_UNRECOVERABLE_ERROR);

	return NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_get_value --
 *
 * Retrieve a key-value pair from a hash table.
 *
 * Does count as an access.
 *
 *-----------------------------------------------------------------------------
 */
vref_t
cht_get_value (
	io_constrained_hash_t *this,vref_t key
) {
	io_constrained_hash_entry_t *entry = cht_find_entry(this,key);
	if (entry) {
		entry->info.access_count ++;
		return entry->value;
	} else {
		return INVALID_VREF;
	}
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_has_key --
 *
 * Test for existance of a key.
 *
 * Does not count as an access.
 *
 *-----------------------------------------------------------------------------
 */
bool
cht_has_key (
	io_constrained_hash_t *this,vref_t key
) {
	return cht_find_entry(this,key) != NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 * cht_set_value --
 *
 * Insert a key-value pair into a hash table.
 *
 *-----------------------------------------------------------------------------
 */
void
cht_set_value (
	io_constrained_hash_t *this,vref_t r_key,vref_t r_value
) {
	static int64_t s_age = 0;
	io_constrained_hash_entry_t *entry,*free_entry;

	// constrain number of entries
	cht_prune(this);

	entry = this->entries + cht_hash1(this,r_key);
	if (entry->info.free == 0) {
		while (1) {
			if (cht_keys_equal(entry->key,r_key)) {
				#if CHT_REFERENCES_VALUES
				unreference_io_value (entry->value);
				entry->value = reference_io_value (r_value);
				#else
				entry->value = r_value;
				#endif
				entry->info.access_count ++;
				return;
			} else if (entry->successor) {
				entry = entry->successor;
			} else {
				break;
			}
		};

		free_entry = cht_get_free_entry (this);
		free_entry->successor = NULL;
		free_entry->predecessor = entry;
		entry->successor = free_entry;
		entry = free_entry;
	}
	this->entry_count ++;
	#if 0 && defined(CHT_DEBUG_WORKER)
	printw (
		CHT_DEBUG_WORKER,
		"cht set %u, key = %u hsh = #%u\n",
		this->entry_count,
		r_key,
		cht_hash1(this,r_key)
	);
	#endif
	#if CHT_REFERENCES_VALUES
	if (vref_is_valid(entry->key)) {
		unreference_io_value (entry->key);
	}
	entry->key = reference_io_value (r_key);
	if (vref_is_valid(entry->value)) {
		unreference_io_value (entry->value);
	}
	entry->value = reference_io_value (r_value);
	#else
	entry->key = r_key;
	entry->value = r_value;
	#endif
	entry->age = s_age++;

	entry->info.access_count = 0;
	entry->info.free = 0;
	entry->info.user_flag1 = 0;
	entry->info.user_flag2 = 0;
}

#ifdef STB_SPRINTF_IMPLEMENTATION
//
// this lib can use unaligned word access which is generally not good for arm cpus
//
#define STB_SPRINTF_NOUNALIGNED

#define stbsp__uint32 unsigned int
#define stbsp__int32 signed int

#ifdef _MSC_VER
#define stbsp__uint64 unsigned __int64
#define stbsp__int64 signed __int64
#else
#define stbsp__uint64 unsigned long long
#define stbsp__int64 signed long long
#endif
#define stbsp__uint16 unsigned short

#ifndef stbsp__uintptr
#if defined(__ppc64__) || defined(__aarch64__) || defined(_M_X64) || defined(__x86_64__) || defined(__x86_64)
#define stbsp__uintptr stbsp__uint64
#else
#define stbsp__uintptr stbsp__uint32
#endif
#endif

#ifndef STB_SPRINTF_MSVC_MODE // used for MSVC2013 and earlier (MSVC2015 matches GCC)
#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define STB_SPRINTF_MSVC_MODE
#endif
#endif

#ifdef STB_SPRINTF_NOUNALIGNED // define this before inclusion to force stbsp_sprintf to always use aligned accesses
#define STBSP__UNALIGNED(code)
#else
#define STBSP__UNALIGNED(code) code
#endif

#ifndef STB_SPRINTF_NOFLOAT
// internal float utility functions
static stbsp__int32 stbsp__real_to_str(char const **start, stbsp__uint32 *len, char *out, stbsp__int32 *decimal_pos, double value, stbsp__uint32 frac_digits);
static stbsp__int32 stbsp__real_to_parts(stbsp__int64 *bits, stbsp__int32 *expo, double value);
#define STBSP__SPECIAL 0x7000
#endif

static char stbsp__period = '.';
static char stbsp__comma = ',';
static const union
{
	// force next field to be 2-byte aligned
	// modified to avoid type-punned warning
//   short temp; 
   char pair[202];
	stbsp__uint16 _p[101];
} stbsp__digitpair =
{
//  0,
   "00010203040506070809101112131415161718192021222324"
   "25262728293031323334353637383940414243444546474849"
   "50515253545556575859606162636465666768697071727374"
   "75767778798081828384858687888990919293949596979899"
};

STBSP__PUBLICDEF void STB_SPRINTF_DECORATE(set_separators)(char pcomma, char pperiod)
{
   stbsp__period = pperiod;
   stbsp__comma = pcomma;
}

#define STBSP__LEFTJUST 1
#define STBSP__LEADINGPLUS 2
#define STBSP__LEADINGSPACE 4
#define STBSP__LEADING_0X 8
#define STBSP__LEADINGZERO 16
#define STBSP__INTMAX 32
#define STBSP__TRIPLET_COMMA 64
#define STBSP__NEGATIVE 128
#define STBSP__METRIC_SUFFIX 256
#define STBSP__HALFWIDTH 512
#define STBSP__METRIC_NOSPACE 1024
#define STBSP__METRIC_1024 2048
#define STBSP__METRIC_JEDEC 4096

static void stbsp__lead_sign(stbsp__uint32 fl, char *sign)
{
   sign[0] = 0;
   if (fl & STBSP__NEGATIVE) {
      sign[0] = 1;
      sign[1] = '-';
   } else if (fl & STBSP__LEADINGSPACE) {
      sign[0] = 1;
      sign[1] = ' ';
   } else if (fl & STBSP__LEADINGPLUS) {
      sign[0] = 1;
      sign[1] = '+';
   }
}

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(vsprintfcb)(STBSP_SPRINTFCB *callback, void *user, char *buf, char const *fmt, va_list va)
{
   static char hex[] = "0123456789abcdefxp";
   static char hexu[] = "0123456789ABCDEFXP";
   char *bf;
   char const *f;
   int tlen = 0;

   bf = buf;
   f = fmt;
   for (;;) {
      stbsp__int32 fw, pr, tz;
      stbsp__uint32 fl;

      // macros for the callback buffer stuff
      #define stbsp__chk_cb_bufL(bytes)                        \
         {                                                     \
            int len = (int)(bf - buf);                         \
            if ((len + (bytes)) >= STB_SPRINTF_MIN) {          \
               tlen += len;                                    \
               if (0 == (bf = buf = callback(buf, user, len))) \
                  goto done;                                   \
            }                                                  \
         }
      #define stbsp__chk_cb_buf(bytes)    \
         {                                \
            if (callback) {               \
               stbsp__chk_cb_bufL(bytes); \
            }                             \
         }
      #define stbsp__flush_cb()                      \
         {                                           \
            stbsp__chk_cb_bufL(STB_SPRINTF_MIN - 1); \
         } // flush if there is even one byte in the buffer
      #define stbsp__cb_buf_clamp(cl, v)                \
         cl = v;                                        \
         if (callback) {                                \
            int lg = STB_SPRINTF_MIN - (int)(bf - buf); \
            if (cl > lg)                                \
               cl = lg;                                 \
         }

      // fast copy everything up to the next % (or end of string)
      for (;;) {
         while (((stbsp__uintptr)f) & 3) {
         schk1:
            if (f[0] == '%')
               goto scandd;
         schk2:
            if (f[0] == 0)
               goto endfmt;
            stbsp__chk_cb_buf(1);
            *bf++ = f[0];
            ++f;
         }
         for (;;) {
            // Check if the next 4 bytes contain %(0x25) or end of string.
            // Using the 'hasless' trick:
            // https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord
            stbsp__uint32 v, c;
            v = *(stbsp__uint32 *)f;
            c = (~v) & 0x80808080;
            if (((v ^ 0x25252525) - 0x01010101) & c)
               goto schk1;
            if ((v - 0x01010101) & c)
               goto schk2;
            if (callback)
               if ((STB_SPRINTF_MIN - (int)(bf - buf)) < 4)
                  goto schk1;
            #ifdef STB_SPRINTF_NOUNALIGNED
                if(((stbsp__uintptr)bf) & 3) {
                    bf[0] = f[0];
                    bf[1] = f[1];
                    bf[2] = f[2];
                    bf[3] = f[3];
                } else
            #endif
            {
                *(stbsp__uint32 *)bf = v;
            }
            bf += 4;
            f += 4;
         }
      }
   scandd:

      ++f;

      // ok, we have a percent, read the modifiers first
      fw = 0;
      pr = -1;
      fl = 0;
      tz = 0;

      // flags
      for (;;) {
         switch (f[0]) {
         // if we have left justify
         case '-':
            fl |= STBSP__LEFTJUST;
            ++f;
            continue;
         // if we have leading plus
         case '+':
            fl |= STBSP__LEADINGPLUS;
            ++f;
            continue;
         // if we have leading space
         case ' ':
            fl |= STBSP__LEADINGSPACE;
            ++f;
            continue;
         // if we have leading 0x
         case '#':
            fl |= STBSP__LEADING_0X;
            ++f;
            continue;
         // if we have thousand commas
         case '\'':
            fl |= STBSP__TRIPLET_COMMA;
            ++f;
            continue;
         // if we have kilo marker (none->kilo->kibi->jedec)
         case '$':
            if (fl & STBSP__METRIC_SUFFIX) {
               if (fl & STBSP__METRIC_1024) {
                  fl |= STBSP__METRIC_JEDEC;
               } else {
                  fl |= STBSP__METRIC_1024;
               }
            } else {
               fl |= STBSP__METRIC_SUFFIX;
            }
            ++f;
            continue;
         // if we don't want space between metric suffix and number
         case '_':
            fl |= STBSP__METRIC_NOSPACE;
            ++f;
            continue;
         // if we have leading zero
         case '0':
            fl |= STBSP__LEADINGZERO;
            ++f;
            goto flags_done;
         default: goto flags_done;
         }
      }
   flags_done:

      // get the field width
      if (f[0] == '*') {
         fw = va_arg(va, stbsp__uint32);
         ++f;
      } else {
         while ((f[0] >= '0') && (f[0] <= '9')) {
            fw = fw * 10 + f[0] - '0';
            f++;
         }
      }
      // get the precision
      if (f[0] == '.') {
         ++f;
         if (f[0] == '*') {
            pr = va_arg(va, stbsp__uint32);
            ++f;
         } else {
            pr = 0;
            while ((f[0] >= '0') && (f[0] <= '9')) {
               pr = pr * 10 + f[0] - '0';
               f++;
            }
         }
      }

      // handle integer size overrides
      switch (f[0]) {
      // are we halfwidth?
      case 'h':
         fl |= STBSP__HALFWIDTH;
         ++f;
         break;
      // are we 64-bit (unix style)
      case 'l':
         fl |= ((sizeof(long) == 8) ? STBSP__INTMAX : 0);
         ++f;
         if (f[0] == 'l') {
            fl |= STBSP__INTMAX;
            ++f;
         }
         break;
      // are we 64-bit on intmax? (c99)
      case 'j':
         fl |= (sizeof(size_t) == 8) ? STBSP__INTMAX : 0;
         ++f;
         break;
      // are we 64-bit on size_t or ptrdiff_t? (c99)
      case 'z':
         fl |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
         ++f;
         break;
      case 't':
         fl |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
         ++f;
         break;
      // are we 64-bit (msft style)
      case 'I':
         if ((f[1] == '6') && (f[2] == '4')) {
            fl |= STBSP__INTMAX;
            f += 3;
         } else if ((f[1] == '3') && (f[2] == '2')) {
            f += 3;
         } else {
            fl |= ((sizeof(void *) == 8) ? STBSP__INTMAX : 0);
            ++f;
         }
         break;
      default: break;
      }

      // handle each replacement
      switch (f[0]) {
         #define STBSP__NUMSZ 512 // big enough for e308 (with commas) or e-307
         char num[STBSP__NUMSZ];
         char lead[8];
         char tail[8];
         char *s;
         char const *h;
         stbsp__uint32 l, n, cs;
         stbsp__uint64 n64;
#ifndef STB_SPRINTF_NOFLOAT
         double fv;
#endif
         stbsp__int32 dp;
         char const *sn;

      case 's':
         // get the string
         s = va_arg(va, char *);
         if (s == 0)
            s = (char *)"null";
         // get the length
         sn = s;
         for (;;) {
            if ((((stbsp__uintptr)sn) & 3) == 0)
               break;
         lchk:
            if (sn[0] == 0)
               goto ld;
            ++sn;
         }
         n = 0xffffffff;
         if (pr >= 0) {
            n = (stbsp__uint32)(sn - s);
            if (n >= (stbsp__uint32)pr)
               goto ld;
            n = ((stbsp__uint32)(pr - n)) >> 2;
         }
         while (n) {
            stbsp__uint32 v = *(stbsp__uint32 *)sn;
            if ((v - 0x01010101) & (~v) & 0x80808080UL)
               goto lchk;
            sn += 4;
            --n;
         }
         goto lchk;
      ld:

         l = (stbsp__uint32)(sn - s);
         // clamp to precision
         if (l > (stbsp__uint32)pr)
            l = pr;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         // copy the string in
         goto scopy;

      case 'c': // char
         // get the character
         s = num + STBSP__NUMSZ - 1;
         *s = (char)va_arg(va, int);
         l = 1;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         goto scopy;

      case 'n': // weird write-bytes specifier
      {
         int *d = va_arg(va, int *);
         *d = tlen + (int)(bf - buf);
      } break;

#ifdef STB_SPRINTF_NOFLOAT
      case 'A':              // float
      case 'a':              // hex float
      case 'G':              // float
      case 'g':              // float
      case 'E':              // float
      case 'e':              // float
      case 'f':              // float
         va_arg(va, double); // eat it
         s = (char *)"No float";
         l = 8;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         goto scopy;
#else
      case 'A': // hex float
      case 'a': // hex float
         h = (f[0] == 'A') ? hexu : hex;
         fv = va_arg(va, double);
         if (pr == -1)
            pr = 6; // default is 6
         // read the double into a string
         if (stbsp__real_to_parts((stbsp__int64 *)&n64, &dp, fv))
            fl |= STBSP__NEGATIVE;

         s = num + 64;

         stbsp__lead_sign(fl, lead);

         if (dp == -1023)
            dp = (n64) ? -1022 : 0;
         else
            n64 |= (((stbsp__uint64)1) << 52);
         n64 <<= (64 - 56);
         if (pr < 15)
            n64 += ((((stbsp__uint64)8) << 56) >> (pr * 4));
// add leading chars

#ifdef STB_SPRINTF_MSVC_MODE
         *s++ = '0';
         *s++ = 'x';
#else
         lead[1 + lead[0]] = '0';
         lead[2 + lead[0]] = 'x';
         lead[0] += 2;
#endif
         *s++ = h[(n64 >> 60) & 15];
         n64 <<= 4;
         if (pr)
            *s++ = stbsp__period;
         sn = s;

         // print the bits
         n = pr;
         if (n > 13)
            n = 13;
         if (pr > (stbsp__int32)n)
            tz = pr - n;
         pr = 0;
         while (n--) {
            *s++ = h[(n64 >> 60) & 15];
            n64 <<= 4;
         }

         // print the expo
         tail[1] = h[17];
         if (dp < 0) {
            tail[2] = '-';
            dp = -dp;
         } else
            tail[2] = '+';
         n = (dp >= 1000) ? 6 : ((dp >= 100) ? 5 : ((dp >= 10) ? 4 : 3));
         tail[0] = (char)n;
         for (;;) {
            tail[n] = '0' + dp % 10;
            if (n <= 3)
               break;
            --n;
            dp /= 10;
         }

         dp = (int)(s - sn);
         l = (int)(s - (num + 64));
         s = num + 64;
         cs = 1 + (3 << 24);
         goto scopy;

      case 'G': // float
      case 'g': // float
         h = (f[0] == 'G') ? hexu : hex;
         fv = va_arg(va, double);
         if (pr == -1)
            pr = 6;
         else if (pr == 0)
            pr = 1; // default is 6
         // read the double into a string
         if (stbsp__real_to_str(&sn, &l, num, &dp, fv, (pr - 1) | 0x80000000))
            fl |= STBSP__NEGATIVE;

         // clamp the precision and delete extra zeros after clamp
         n = pr;
         if (l > (stbsp__uint32)pr)
            l = pr;
         while ((l > 1) && (pr) && (sn[l - 1] == '0')) {
            --pr;
            --l;
         }

         // should we use %e
         if ((dp <= -4) || (dp > (stbsp__int32)n)) {
            if (pr > (stbsp__int32)l)
               pr = l - 1;
            else if (pr)
               --pr; // when using %e, there is one digit before the decimal
            goto doexpfromg;
         }
         // this is the insane action to get the pr to match %g semantics for %f
         if (dp > 0) {
            pr = (dp < (stbsp__int32)l) ? l - dp : 0;
         } else {
            pr = -dp + ((pr > (stbsp__int32)l) ? (stbsp__int32) l : pr);
         }
         goto dofloatfromg;

      case 'E': // float
      case 'e': // float
         h = (f[0] == 'E') ? hexu : hex;
         fv = va_arg(va, double);
         if (pr == -1)
            pr = 6; // default is 6
         // read the double into a string
         if (stbsp__real_to_str(&sn, &l, num, &dp, fv, pr | 0x80000000))
            fl |= STBSP__NEGATIVE;
      doexpfromg:
         tail[0] = 0;
         stbsp__lead_sign(fl, lead);
         if (dp == STBSP__SPECIAL) {
            s = (char *)sn;
            cs = 0;
            pr = 0;
            goto scopy;
         }
         s = num + 64;
         // handle leading chars
         *s++ = sn[0];

         if (pr)
            *s++ = stbsp__period;

         // handle after decimal
         if ((l - 1) > (stbsp__uint32)pr)
            l = pr + 1;
         for (n = 1; n < l; n++)
            *s++ = sn[n];
         // trailing zeros
         tz = pr - (l - 1);
         pr = 0;
         // dump expo
         tail[1] = h[0xe];
         dp -= 1;
         if (dp < 0) {
            tail[2] = '-';
            dp = -dp;
         } else
            tail[2] = '+';
#ifdef STB_SPRINTF_MSVC_MODE
         n = 5;
#else
         n = (dp >= 100) ? 5 : 4;
#endif
         tail[0] = (char)n;
         for (;;) {
            tail[n] = '0' + dp % 10;
            if (n <= 3)
               break;
            --n;
            dp /= 10;
         }
         cs = 1 + (3 << 24); // how many tens
         goto flt_lead;

      case 'f': // float
         fv = va_arg(va, double);
      doafloat:
         // do kilos
         if (fl & STBSP__METRIC_SUFFIX) {
            double divisor;
            divisor = 1000.0f;
            if (fl & STBSP__METRIC_1024)
               divisor = 1024.0;
            while (fl < 0x4000000) {
               if ((fv < divisor) && (fv > -divisor))
                  break;
               fv /= divisor;
               fl += 0x1000000;
            }
         }
         if (pr == -1)
            pr = 6; // default is 6
         // read the double into a string
         if (stbsp__real_to_str(&sn, &l, num, &dp, fv, pr))
            fl |= STBSP__NEGATIVE;
      dofloatfromg:
         tail[0] = 0;
         stbsp__lead_sign(fl, lead);
         if (dp == STBSP__SPECIAL) {
            s = (char *)sn;
            cs = 0;
            pr = 0;
            goto scopy;
         }
         s = num + 64;

         // handle the three decimal varieties
         if (dp <= 0) {
            stbsp__int32 i;
            // handle 0.000*000xxxx
            *s++ = '0';
            if (pr)
               *s++ = stbsp__period;
            n = -dp;
            if ((stbsp__int32)n > pr)
               n = pr;
            i = n;
            while (i) {
               if ((((stbsp__uintptr)s) & 3) == 0)
                  break;
               *s++ = '0';
               --i;
            }
            while (i >= 4) {
               *(stbsp__uint32 *)s = 0x30303030;
               s += 4;
               i -= 4;
            }
            while (i) {
               *s++ = '0';
               --i;
            }
            if ((stbsp__int32)(l + n) > pr)
               l = pr - n;
            i = l;
            while (i) {
               *s++ = *sn++;
               --i;
            }
            tz = pr - (n + l);
            cs = 1 + (3 << 24); // how many tens did we write (for commas below)
         } else {
            cs = (fl & STBSP__TRIPLET_COMMA) ? ((600 - (stbsp__uint32)dp) % 3) : 0;
            if ((stbsp__uint32)dp >= l) {
               // handle xxxx000*000.0
               n = 0;
               for (;;) {
                  if ((fl & STBSP__TRIPLET_COMMA) && (++cs == 4)) {
                     cs = 0;
                     *s++ = stbsp__comma;
                  } else {
                     *s++ = sn[n];
                     ++n;
                     if (n >= l)
                        break;
                  }
               }
               if (n < (stbsp__uint32)dp) {
                  n = dp - n;
                  if ((fl & STBSP__TRIPLET_COMMA) == 0) {
                     while (n) {
                        if ((((stbsp__uintptr)s) & 3) == 0)
                           break;
                        *s++ = '0';
                        --n;
                     }
                     while (n >= 4) {
                        *(stbsp__uint32 *)s = 0x30303030;
                        s += 4;
                        n -= 4;
                     }
                  }
                  while (n) {
                     if ((fl & STBSP__TRIPLET_COMMA) && (++cs == 4)) {
                        cs = 0;
                        *s++ = stbsp__comma;
                     } else {
                        *s++ = '0';
                        --n;
                     }
                  }
               }
               cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
               if (pr) {
                  *s++ = stbsp__period;
                  tz = pr;
               }
            } else {
               // handle xxxxx.xxxx000*000
               n = 0;
               for (;;) {
                  if ((fl & STBSP__TRIPLET_COMMA) && (++cs == 4)) {
                     cs = 0;
                     *s++ = stbsp__comma;
                  } else {
                     *s++ = sn[n];
                     ++n;
                     if (n >= (stbsp__uint32)dp)
                        break;
                  }
               }
               cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
               if (pr)
                  *s++ = stbsp__period;
               if ((l - dp) > (stbsp__uint32)pr)
                  l = pr + dp;
               while (n < l) {
                  *s++ = sn[n];
                  ++n;
               }
               tz = pr - (l - dp);
            }
         }
         pr = 0;

         // handle k,m,g,t
         if (fl & STBSP__METRIC_SUFFIX) {
            char idx;
            idx = 1;
            if (fl & STBSP__METRIC_NOSPACE)
               idx = 0;
            tail[0] = idx;
            tail[1] = ' ';
            {
               if (fl >> 24) { // SI kilo is 'k', JEDEC and SI kibits are 'K'.
                  if (fl & STBSP__METRIC_1024)
                     tail[idx + 1] = "_KMGT"[fl >> 24];
                  else
                     tail[idx + 1] = "_kMGT"[fl >> 24];
                  idx++;
                  // If printing kibits and not in jedec, add the 'i'.
                  if (fl & STBSP__METRIC_1024 && !(fl & STBSP__METRIC_JEDEC)) {
                     tail[idx + 1] = 'i';
                     idx++;
                  }
                  tail[0] = idx;
               }
            }
         };

      flt_lead:
         // get the length that we copied
         l = (stbsp__uint32)(s - (num + 64));
         s = num + 64;
         goto scopy;
#endif

      case 'B': // upper binary
      case 'b': // lower binary
         h = (f[0] == 'B') ? hexu : hex;
         lead[0] = 0;
         if (fl & STBSP__LEADING_0X) {
            lead[0] = 2;
            lead[1] = '0';
            lead[2] = h[0xb];
         }
         l = (8 << 4) | (1 << 8);
         goto radixnum;

      case 'o': // octal
         h = hexu;
         lead[0] = 0;
         if (fl & STBSP__LEADING_0X) {
            lead[0] = 1;
            lead[1] = '0';
         }
         l = (3 << 4) | (3 << 8);
         goto radixnum;

      case 'p': // pointer
         fl |= (sizeof(void *) == 8) ? STBSP__INTMAX : 0;
         pr = sizeof(void *) * 2;
         fl &= ~STBSP__LEADINGZERO; // 'p' only prints the pointer with zeros
                                    // fall through - to X

      case 'X': // upper hex
      case 'x': // lower hex
         h = (f[0] == 'X') ? hexu : hex;
         l = (4 << 4) | (4 << 8);
         lead[0] = 0;
         if (fl & STBSP__LEADING_0X) {
            lead[0] = 2;
            lead[1] = '0';
            lead[2] = h[16];
         }
      radixnum:
         // get the number
         if (fl & STBSP__INTMAX)
            n64 = va_arg(va, stbsp__uint64);
         else
            n64 = va_arg(va, stbsp__uint32);

         s = num + STBSP__NUMSZ;
         dp = 0;
         // clear tail, and clear leading if value is zero
         tail[0] = 0;
         if (n64 == 0) {
            lead[0] = 0;
            if (pr == 0) {
               l = 0;
               cs = (((l >> 4) & 15)) << 24;
               goto scopy;
            }
         }
         // convert to string
         for (;;) {
            *--s = h[n64 & ((1 << (l >> 8)) - 1)];
            n64 >>= (l >> 8);
            if (!((n64) || ((stbsp__int32)((num + STBSP__NUMSZ) - s) < pr)))
               break;
            if (fl & STBSP__TRIPLET_COMMA) {
               ++l;
               if ((l & 15) == ((l >> 4) & 15)) {
                  l &= ~15;
                  *--s = stbsp__comma;
               }
            }
         };
         // get the tens and the comma pos
         cs = (stbsp__uint32)((num + STBSP__NUMSZ) - s) + ((((l >> 4) & 15)) << 24);
         // get the length that we copied
         l = (stbsp__uint32)((num + STBSP__NUMSZ) - s);
         // copy it
         goto scopy;

      //
      // special case added to support io values
      //
      case 'v': {
			struct io_binary_encoding_print_data *data = user;
			io_encoding_t *venc = mk_io_text_encoding (io_binary_encoding_byte_memory(data->this));
			vref_t r_value = va_arg(va, vref_t);
			if (io_value_encode (r_value,venc)) {
				const uint8_t *s,*e;
				io_encoding_get_content (venc,&s,&e);
				 // copy the string
				 n = e - s;
				 while (n) {
					stbsp__int32 i;
					stbsp__cb_buf_clamp(i, n);
					n -= i;
					STBSP__UNALIGNED(while (i >= 4) {
					   *(stbsp__uint32 *)bf = *(stbsp__uint32 *)s;
					   bf += 4;
					   s += 4;
					   i -= 4;
					})
					while (i) {
					   *bf++ = *s++;
					   --i;
					}
					stbsp__chk_cb_buf(1);
				 }
				 /*
				*/
			}
			io_encoding_free(venc);
		}
		break;

      case 'u': // unsigned
      case 'i':
      case 'd': // integer
         // get the integer and abs it
         if (fl & STBSP__INTMAX) {
            stbsp__int64 i64 = va_arg(va, stbsp__int64);
            n64 = (stbsp__uint64)i64;
            if ((f[0] != 'u') && (i64 < 0)) {
               n64 = (stbsp__uint64)-i64;
               fl |= STBSP__NEGATIVE;
            }
         } else {
            stbsp__int32 i = va_arg(va, stbsp__int32);
            n64 = (stbsp__uint32)i;
            if ((f[0] != 'u') && (i < 0)) {
               n64 = (stbsp__uint32)-i;
               fl |= STBSP__NEGATIVE;
            }
         }

#ifndef STB_SPRINTF_NOFLOAT
         if (fl & STBSP__METRIC_SUFFIX) {
            if (n64 < 1024)
               pr = 0;
            else if (pr == -1)
               pr = 1;
            fv = (double)(stbsp__int64)n64;
            goto doafloat;
         }
#endif

         // convert to string
         s = num + STBSP__NUMSZ;
         l = 0;

         for (;;) {
            // do in 32-bit chunks (avoid lots of 64-bit divides even with constant denominators)
            char *o = s - 8;
            if (n64 >= 100000000) {
               n = (stbsp__uint32)(n64 % 100000000);
               n64 /= 100000000;
            } else {
               n = (stbsp__uint32)n64;
               n64 = 0;
            }
            if ((fl & STBSP__TRIPLET_COMMA) == 0) {
               do {
                  s -= 2;
 //                 *(stbsp__uint16 *)s = *(stbsp__uint16 *)&stbsp__digitpair.pair[(n % 100) * 2];
                  *(stbsp__uint16 *)s = stbsp__digitpair._p[(n % 100)];
                  n /= 100;
               } while (n);
            }
            while (n) {
               if ((fl & STBSP__TRIPLET_COMMA) && (l++ == 3)) {
                  l = 0;
                  *--s = stbsp__comma;
                  --o;
               } else {
                  *--s = (char)(n % 10) + '0';
                  n /= 10;
               }
            }
            if (n64 == 0) {
               if ((s[0] == '0') && (s != (num + STBSP__NUMSZ)))
                  ++s;
               break;
            }
            while (s != o)
               if ((fl & STBSP__TRIPLET_COMMA) && (l++ == 3)) {
                  l = 0;
                  *--s = stbsp__comma;
                  --o;
               } else {
                  *--s = '0';
               }
         }

         tail[0] = 0;
         stbsp__lead_sign(fl, lead);

         // get the length that we copied
         l = (stbsp__uint32)((num + STBSP__NUMSZ) - s);
         if (l == 0) {
            *--s = '0';
            l = 1;
         }
         cs = l + (3 << 24);
         if (pr < 0)
            pr = 0;

      scopy:
         // get fw=leading/trailing space, pr=leading zeros
         if (pr < (stbsp__int32)l)
            pr = l;
         n = pr + lead[0] + tail[0] + tz;
         if (fw < (stbsp__int32)n)
            fw = n;
         fw -= n;
         pr -= l;

         // handle right justify and leading zeros
         if ((fl & STBSP__LEFTJUST) == 0) {
            if (fl & STBSP__LEADINGZERO) // if leading zeros, everything is in pr
            {
               pr = (fw > pr) ? fw : pr;
               fw = 0;
            } else {
               fl &= ~STBSP__TRIPLET_COMMA; // if no leading zeros, then no commas
            }
         }

         // copy the spaces and/or zeros
         if (fw + pr) {
            stbsp__int32 i;
            stbsp__uint32 c;

            // copy leading spaces (or when doing %8.4d stuff)
            if ((fl & STBSP__LEFTJUST) == 0)
               while (fw > 0) {
                  stbsp__cb_buf_clamp(i, fw);
                  fw -= i;
                  while (i) {
                     if ((((stbsp__uintptr)bf) & 3) == 0)
                        break;
                     *bf++ = ' ';
                     --i;
                  }
                  while (i >= 4) {
                     *(stbsp__uint32 *)bf = 0x20202020;
                     bf += 4;
                     i -= 4;
                  }
                  while (i) {
                     *bf++ = ' ';
                     --i;
                  }
                  stbsp__chk_cb_buf(1);
               }

            // copy leader
            sn = lead + 1;
            while (lead[0]) {
               stbsp__cb_buf_clamp(i, lead[0]);
               lead[0] -= (char)i;
               while (i) {
                  *bf++ = *sn++;
                  --i;
               }
               stbsp__chk_cb_buf(1);
            }

            // copy leading zeros
            c = cs >> 24;
            cs &= 0xffffff;
            cs = (fl & STBSP__TRIPLET_COMMA) ? ((stbsp__uint32)(c - ((pr + cs) % (c + 1)))) : 0;
            while (pr > 0) {
               stbsp__cb_buf_clamp(i, pr);
               pr -= i;
               if ((fl & STBSP__TRIPLET_COMMA) == 0) {
                  while (i) {
                     if ((((stbsp__uintptr)bf) & 3) == 0)
                        break;
                     *bf++ = '0';
                     --i;
                  }
                  while (i >= 4) {
                     *(stbsp__uint32 *)bf = 0x30303030;
                     bf += 4;
                     i -= 4;
                  }
               }
               while (i) {
                  if ((fl & STBSP__TRIPLET_COMMA) && (cs++ == c)) {
                     cs = 0;
                     *bf++ = stbsp__comma;
                  } else
                     *bf++ = '0';
                  --i;
               }
               stbsp__chk_cb_buf(1);
            }
         }

         // copy leader if there is still one
         sn = lead + 1;
         while (lead[0]) {
            stbsp__int32 i;
            stbsp__cb_buf_clamp(i, lead[0]);
            lead[0] -= (char)i;
            while (i) {
               *bf++ = *sn++;
               --i;
            }
            stbsp__chk_cb_buf(1);
         }

         // copy the string
         n = l;
         while (n) {
            stbsp__int32 i;
            stbsp__cb_buf_clamp(i, n);
            n -= i;
            STBSP__UNALIGNED(while (i >= 4) {
               *(stbsp__uint32 *)bf = *(stbsp__uint32 *)s;
               bf += 4;
               s += 4;
               i -= 4;
            })
            while (i) {
               *bf++ = *s++;
               --i;
            }
            stbsp__chk_cb_buf(1);
         }

         // copy trailing zeros
         while (tz) {
            stbsp__int32 i;
            stbsp__cb_buf_clamp(i, tz);
            tz -= i;
            while (i) {
               if ((((stbsp__uintptr)bf) & 3) == 0)
                  break;
               *bf++ = '0';
               --i;
            }
            while (i >= 4) {
               *(stbsp__uint32 *)bf = 0x30303030;
               bf += 4;
               i -= 4;
            }
            while (i) {
               *bf++ = '0';
               --i;
            }
            stbsp__chk_cb_buf(1);
         }

         // copy tail if there is one
         sn = tail + 1;
         while (tail[0]) {
            stbsp__int32 i;
            stbsp__cb_buf_clamp(i, tail[0]);
            tail[0] -= (char)i;
            while (i) {
               *bf++ = *sn++;
               --i;
            }
            stbsp__chk_cb_buf(1);
         }

         // handle the left justify
         if (fl & STBSP__LEFTJUST)
            if (fw > 0) {
               while (fw) {
                  stbsp__int32 i;
                  stbsp__cb_buf_clamp(i, fw);
                  fw -= i;
                  while (i) {
                     if ((((stbsp__uintptr)bf) & 3) == 0)
                        break;
                     *bf++ = ' ';
                     --i;
                  }
                  while (i >= 4) {
                     *(stbsp__uint32 *)bf = 0x20202020;
                     bf += 4;
                     i -= 4;
                  }
                  while (i--)
                     *bf++ = ' ';
                  stbsp__chk_cb_buf(1);
               }
            }
         break;

      default: // unknown, just copy code
         s = num + STBSP__NUMSZ - 1;
         *s = f[0];
         l = 1;
         fw = fl = 0;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         goto scopy;
      }
      ++f;
   }
endfmt:

   if (!callback)
      *bf = 0;
   else
      stbsp__flush_cb();

done:
   return tlen + (int)(bf - buf);
}

// cleanup
#undef STBSP__LEFTJUST
#undef STBSP__LEADINGPLUS
#undef STBSP__LEADINGSPACE
#undef STBSP__LEADING_0X
#undef STBSP__LEADINGZERO
#undef STBSP__INTMAX
#undef STBSP__TRIPLET_COMMA
#undef STBSP__NEGATIVE
#undef STBSP__METRIC_SUFFIX
#undef STBSP__NUMSZ
#undef stbsp__chk_cb_bufL
#undef stbsp__chk_cb_buf
#undef stbsp__flush_cb
#undef stbsp__cb_buf_clamp

// ============================================================================
//   wrapper functions

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(sprintf)(char *buf, char const *fmt, ...)
{
   int result;
   va_list va;
   va_start(va, fmt);
   result = STB_SPRINTF_DECORATE(vsprintfcb)(0, 0, buf, fmt, va);
   va_end(va);
   return result;
}

typedef struct stbsp__context {
   char *buf;
   int count;
   char tmp[STB_SPRINTF_MIN];
} stbsp__context;

static char *stbsp__clamp_callback(char *buf, void *user, int len)
{
   stbsp__context *c = (stbsp__context *)user;

   if (len > c->count)
      len = c->count;

   if (len) {
      if (buf != c->buf) {
         char *s, *d, *se;
         d = c->buf;
         s = buf;
         se = buf + len;
         do {
            *d++ = *s++;
         } while (s < se);
      }
      c->buf += len;
      c->count -= len;
   }

   if (c->count <= 0)
      return 0;
   return (c->count >= STB_SPRINTF_MIN) ? c->buf : c->tmp; // go direct into buffer if you can
}

static char * stbsp__count_clamp_callback( char * buf, void * user, int len )
{
   stbsp__context * c = (stbsp__context*)user;

   c->count += len;
   return c->tmp; // go direct into buffer if you can
}

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE( vsnprintf )( char * buf, int count, char const * fmt, va_list va )
{
   stbsp__context c;
   int l;

   if ( (count == 0) && !buf )
   {
      c.count = 0;

      STB_SPRINTF_DECORATE( vsprintfcb )( stbsp__count_clamp_callback, &c, c.tmp, fmt, va );
      l = c.count;
   }
   else
   {
      if ( count == 0 )
         return 0;

      c.buf = buf;
      c.count = count;

      STB_SPRINTF_DECORATE( vsprintfcb )( stbsp__clamp_callback, &c, stbsp__clamp_callback(0,&c,0), fmt, va );

      // zero-terminate
      l = (int)( c.buf - buf );
      if ( l >= count ) // should never be greater, only equal (or less) than count
         l = count - 1;
      buf[l] = 0;
   }

   return l;
}

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(snprintf)(char *buf, int count, char const *fmt, ...)
{
   int result;
   va_list va;
   va_start(va, fmt);

   result = STB_SPRINTF_DECORATE(vsnprintf)(buf, count, fmt, va);
   va_end(va);

   return result;
}

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(vsprintf)(char *buf, char const *fmt, va_list va)
{
   return STB_SPRINTF_DECORATE(vsprintfcb)(0, 0, buf, fmt, va);
}

// =======================================================================
//   low level float utility functions

#ifndef STB_SPRINTF_NOFLOAT

// copies d to bits w/ strict aliasing (this compiles to nothing on /Ox)
#define STBSP__COPYFP(dest, src)                   \
   {                                               \
      int cn;                                      \
      for (cn = 0; cn < 8; cn++)                   \
         ((char *)&dest)[cn] = ((char *)&src)[cn]; \
   }

// get float info
static stbsp__int32 stbsp__real_to_parts(stbsp__int64 *bits, stbsp__int32 *expo, double value)
{
   double d;
   stbsp__int64 b = 0;

   // load value and round at the frac_digits
   d = value;

   STBSP__COPYFP(b, d);

   *bits = b & ((((stbsp__uint64)1) << 52) - 1);
   *expo = (stbsp__int32)(((b >> 52) & 2047) - 1023);

   return (stbsp__int32)((stbsp__uint64) b >> 63);
}

static double const stbsp__bot[23] = {
   1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005, 1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
   1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017, 1e+018, 1e+019, 1e+020, 1e+021, 1e+022
};
static double const stbsp__negbot[22] = {
   1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006, 1e-007, 1e-008, 1e-009, 1e-010, 1e-011,
   1e-012, 1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018, 1e-019, 1e-020, 1e-021, 1e-022
};
static double const stbsp__negboterr[22] = {
   -5.551115123125783e-018,  -2.0816681711721684e-019, -2.0816681711721686e-020, -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
   4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026, -3.6432197315497743e-027, 6.0503030718060191e-028,  2.0113352370744385e-029,
   -3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032, 2.0902213275965398e-033,  -7.1542424054621921e-034, -7.1542424054621926e-035,
   2.4754073164739869e-036,  5.4846728545790429e-037,  9.2462547772103625e-038,  -4.8596774326570872e-039
};
static double const stbsp__top[13] = {
   1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161, 1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299
};
static double const stbsp__negtop[13] = {
   1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161, 1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299
};
static double const stbsp__toperr[13] = {
   8388608,
   6.8601809640529717e+028,
   -7.253143638152921e+052,
   -4.3377296974619174e+075,
   -1.5559416129466825e+098,
   -3.2841562489204913e+121,
   -3.7745893248228135e+144,
   -1.7356668416969134e+167,
   -3.8893577551088374e+190,
   -9.9566444326005119e+213,
   6.3641293062232429e+236,
   -5.2069140800249813e+259,
   -5.2504760255204387e+282
};
static double const stbsp__negtoperr[13] = {
   3.9565301985100693e-040,  -2.299904345391321e-063,  3.6506201437945798e-086,  1.1875228833981544e-109,
   -5.0644902316928607e-132, -6.7156837247865426e-155, -2.812077463003139e-178,  -5.7778912386589953e-201,
   7.4997100559334532e-224,  -4.6439668915134491e-247, -6.3691100762962136e-270, -9.436808465446358e-293,
   8.0970921678014997e-317
};

#if defined(_MSC_VER) && (_MSC_VER <= 1200)
static stbsp__uint64 const stbsp__powten[20] = {
   1,
   10,
   100,
   1000,
   10000,
   100000,
   1000000,
   10000000,
   100000000,
   1000000000,
   10000000000,
   100000000000,
   1000000000000,
   10000000000000,
   100000000000000,
   1000000000000000,
   10000000000000000,
   100000000000000000,
   1000000000000000000,
   10000000000000000000U
};
#define stbsp__tento19th ((stbsp__uint64)1000000000000000000)
#else
static stbsp__uint64 const stbsp__powten[20] = {
   1,
   10,
   100,
   1000,
   10000,
   100000,
   1000000,
   10000000,
   100000000,
   1000000000,
   10000000000ULL,
   100000000000ULL,
   1000000000000ULL,
   10000000000000ULL,
   100000000000000ULL,
   1000000000000000ULL,
   10000000000000000ULL,
   100000000000000000ULL,
   1000000000000000000ULL,
   10000000000000000000ULL
};
#define stbsp__tento19th (1000000000000000000ULL)
#endif

#define stbsp__ddmulthi(oh, ol, xh, yh)                            \
   {                                                               \
      double ahi = 0, alo, bhi = 0, blo;                           \
      stbsp__int64 bt;                                             \
      oh = xh * yh;                                                \
      STBSP__COPYFP(bt, xh);                                       \
      bt &= ((~(stbsp__uint64)0) << 27);                           \
      STBSP__COPYFP(ahi, bt);                                      \
      alo = xh - ahi;                                              \
      STBSP__COPYFP(bt, yh);                                       \
      bt &= ((~(stbsp__uint64)0) << 27);                           \
      STBSP__COPYFP(bhi, bt);                                      \
      blo = yh - bhi;                                              \
      ol = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo; \
   }

#define stbsp__ddtoS64(ob, xh, xl)          \
   {                                        \
      double ahi = 0, alo, vh, t;           \
      ob = (stbsp__int64)ph;                \
      vh = (double)ob;                      \
      ahi = (xh - vh);                      \
      t = (ahi - xh);                       \
      alo = (xh - (ahi - t)) - (vh + t);    \
      ob += (stbsp__int64)(ahi + alo + xl); \
   }

#define stbsp__ddrenorm(oh, ol) \
   {                            \
      double s;                 \
      s = oh + ol;              \
      ol = ol - (s - oh);       \
      oh = s;                   \
   }

#define stbsp__ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

#define stbsp__ddmultlos(oh, ol, xh, yl) ol = ol + (xh * yl);
// power can be -323 to +350
static void stbsp__raise_to_power10(double *ohi, double *olo, double d, stbsp__int32 power) 
{
   double ph, pl;
   if ((power >= 0) && (power <= 22)) {
      stbsp__ddmulthi(ph, pl, d, stbsp__bot[power]);
   } else {
      stbsp__int32 e, et, eb;
      double p2h, p2l;

      e = power;
      if (power < 0)
         e = -e;
      et = (e * 0x2c9) >> 14; /* %23 */
      if (et > 13)
         et = 13;
      eb = e - (et * 23);

      ph = d;
      pl = 0.0;
      if (power < 0) {
         if (eb) {
            --eb;
            stbsp__ddmulthi(ph, pl, d, stbsp__negbot[eb]);
            stbsp__ddmultlos(ph, pl, d, stbsp__negboterr[eb]);
         }
         if (et) {
            stbsp__ddrenorm(ph, pl);
            --et;
            stbsp__ddmulthi(p2h, p2l, ph, stbsp__negtop[et]);
            stbsp__ddmultlo(p2h, p2l, ph, pl, stbsp__negtop[et], stbsp__negtoperr[et]);
            ph = p2h;
            pl = p2l;
         }
      } else {
         if (eb) {
            e = eb;
            if (eb > 22)
               eb = 22;
            e -= eb;
            stbsp__ddmulthi(ph, pl, d, stbsp__bot[eb]);
            if (e) {
               stbsp__ddrenorm(ph, pl);
               stbsp__ddmulthi(p2h, p2l, ph, stbsp__bot[e]);
               stbsp__ddmultlos(p2h, p2l, stbsp__bot[e], pl);
               ph = p2h;
               pl = p2l;
            }
         }
         if (et) {
            stbsp__ddrenorm(ph, pl);
            --et;
            stbsp__ddmulthi(p2h, p2l, ph, stbsp__top[et]);
            stbsp__ddmultlo(p2h, p2l, ph, pl, stbsp__top[et], stbsp__toperr[et]);
            ph = p2h;
            pl = p2l;
         }
      }
   }
   stbsp__ddrenorm(ph, pl);
   *ohi = ph;
   *olo = pl;
}

// given a float value, returns the significant bits in bits, and the position of the
//   decimal point in decimal_pos.  +/-INF and NAN are specified by special values
//   returned in the decimal_pos parameter.
// frac_digits is absolute normally, but if you want from first significant digits (got %g and %e), or in 0x80000000
static stbsp__int32 stbsp__real_to_str(char const **start, stbsp__uint32 *len, char *out, stbsp__int32 *decimal_pos, double value, stbsp__uint32 frac_digits)
{
   double d;
   stbsp__int64 bits = 0;
   stbsp__int32 expo, e, ng, tens;

   d = value;
   STBSP__COPYFP(bits, d);
   expo = (stbsp__int32)((bits >> 52) & 2047);
   ng = (stbsp__int32)((stbsp__uint64) bits >> 63);
   if (ng)
      d = -d;

   if (expo == 2047) // is nan or inf?
   {
      *start = (bits & ((((stbsp__uint64)1) << 52) - 1)) ? "NaN" : "Inf";
      *decimal_pos = STBSP__SPECIAL;
      *len = 3;
      return ng;
   }

   if (expo == 0) // is zero or denormal
   {
      if ((bits << 1) == 0) // do zero
      {
         *decimal_pos = 1;
         *start = out;
         out[0] = '0';
         *len = 1;
         return ng;
      }
      // find the right expo for denormals
      {
         stbsp__int64 v = ((stbsp__uint64)1) << 51;
         while ((bits & v) == 0) {
            --expo;
            v >>= 1;
         }
      }
   }

   // find the decimal exponent as well as the decimal bits of the value
   {
      double ph, pl;

      // log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of log10 of all expos 1..2046
      tens = expo - 1023;
      tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

      // move the significant bits into position and stick them into an int
      stbsp__raise_to_power10(&ph, &pl, d, 18 - tens);

      // get full as much precision from double-double as possible
      stbsp__ddtoS64(bits, ph, pl);

      // check if we undershot
      if (((stbsp__uint64)bits) >= stbsp__tento19th)
         ++tens;
   }

   // now do the rounding in integer land
   frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1) : (tens + frac_digits);
   if ((frac_digits < 24)) {
      stbsp__uint32 dg = 1;
      if ((stbsp__uint64)bits >= stbsp__powten[9])
         dg = 10;
      while ((stbsp__uint64)bits >= stbsp__powten[dg]) {
         ++dg;
         if (dg == 20)
            goto noround;
      }
      if (frac_digits < dg) {
         stbsp__uint64 r;
         // add 0.5 at the right position and round
         e = dg - frac_digits;
         if ((stbsp__uint32)e >= 24)
            goto noround;
         r = stbsp__powten[e];
         bits = bits + (r / 2);
         if ((stbsp__uint64)bits >= stbsp__powten[dg])
            ++tens;
         bits /= r;
      }
   noround:;
   }

   // kill long trailing runs of zeros
   if (bits) {
      stbsp__uint32 n;
      for (;;) {
         if (bits <= 0xffffffff)
            break;
         if (bits % 1000)
            goto donez;
         bits /= 1000;
      }
      n = (stbsp__uint32)bits;
      while ((n % 1000) == 0)
         n /= 1000;
      bits = n;
   donez:;
   }

   // convert to string
   out += 64;
   e = 0;
   for (;;) {
      stbsp__uint32 n;
      char *o = out - 8;
      // do the conversion in chunks of U32s (avoid most 64-bit divides, worth it, constant denomiators be damned)
      if (bits >= 100000000) {
         n = (stbsp__uint32)(bits % 100000000);
         bits /= 100000000;
      } else {
         n = (stbsp__uint32)bits;
         bits = 0;
      }
      while (n) {
         out -= 2;
//         *(stbsp__uint16 *)out = *(stbsp__uint16 *)&stbsp__digitpair.pair[(n % 100) * 2];
         *(stbsp__uint16 *)out = stbsp__digitpair._p[(n % 100)];
         n /= 100;
         e += 2;
      }
      if (bits == 0) {
         if ((e) && (out[0] == '0')) {
            ++out;
            --e;
         }
         break;
      }
      while (out != o) {
         *--out = '0';
         ++e;
      }
   }

   *decimal_pos = tens;
   *start = out;
   *len = e;
   return ng;
}

#undef stbsp__ddmulthi
#undef stbsp__ddrenorm
#undef stbsp__ddmultlo
#undef stbsp__ddmultlos
#undef STBSP__SPECIAL
#undef STBSP__COPYFP

#endif // STB_SPRINTF_NOFLOAT

// clean up
#undef stbsp__uint16
#undef stbsp__uint32
#undef stbsp__int32
#undef stbsp__uint64
#undef stbsp__int64
#undef STBSP__UNALIGNED

#endif // STB_SPRINTF_IMPLEMENTATION

//
// sha256
//

#define SHA256_VALIDATE_RET(cond) 
#define SHA256_VALIDATE(cond) 

/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
do {                                                    \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
} while( 0 )
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
do {                                                    \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
} while( 0 )
#endif

void io_tls_sha256_init( io_sha256_context_t *ctx )
{
    SHA256_VALIDATE( ctx != NULL );

    memset( ctx, 0, sizeof( io_sha256_context_t ) );
}

void io_tls_sha256_free( io_sha256_context_t *ctx )
{
    if( ctx == NULL )
        return;

    memset (ctx,0,sizeof (io_sha256_context_t));
}

void
io_tls_sha256_clone (io_sha256_context_t *dst,const io_sha256_context_t *src) {
    *dst = *src;
}

int io_tls_sha256_starts_ret( io_sha256_context_t *ctx, int is224 )
{
    SHA256_VALIDATE_RET( ctx != NULL );
    SHA256_VALIDATE_RET( is224 == 0 || is224 == 1 );

    ctx->total[0] = 0;
    ctx->total[1] = 0;

    if( is224 == 0 )
    {
        /* SHA-256 */
        ctx->state[0] = 0x6A09E667;
        ctx->state[1] = 0xBB67AE85;
        ctx->state[2] = 0x3C6EF372;
        ctx->state[3] = 0xA54FF53A;
        ctx->state[4] = 0x510E527F;
        ctx->state[5] = 0x9B05688C;
        ctx->state[6] = 0x1F83D9AB;
        ctx->state[7] = 0x5BE0CD19;
    }
    else
    {
        /* SHA-224 */
        ctx->state[0] = 0xC1059ED8;
        ctx->state[1] = 0x367CD507;
        ctx->state[2] = 0x3070DD17;
        ctx->state[3] = 0xF70E5939;
        ctx->state[4] = 0xFFC00B31;
        ctx->state[5] = 0x68581511;
        ctx->state[6] = 0x64F98FA7;
        ctx->state[7] = 0xBEFA4FA4;
    }

    ctx->is224 = is224;

    return( 0 );
}

static const uint32_t K[] =
{
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};

#define  SHR(x,n) (((x) & 0xFFFFFFFF) >> (n))
#define ROTR(x,n) (SHR(x,n) | ((x) << (32 - (n))))

#define S0(x) (ROTR(x, 7) ^ ROTR(x,18) ^  SHR(x, 3))
#define S1(x) (ROTR(x,17) ^ ROTR(x,19) ^  SHR(x,10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x,13) ^ ROTR(x,22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x,11) ^ ROTR(x,25))

#define F0(x,y,z) (((x) & (y)) | ((z) & ((x) | (y))))
#define F1(x,y,z) ((z) ^ ((x) & ((y) ^ (z))))

#define R(t)                                    \
    (                                           \
        W[t] = S1(W[(t) -  2]) + W[(t) -  7] +  \
               S0(W[(t) - 15]) + W[(t) - 16]    \
    )

#define P(a,b,c,d,e,f,g,h,x,K)                          \
    do                                                  \
    {                                                   \
        temp1 = (h) + S3(e) + F1((e),(f),(g)) + (K) + (x); \
        temp2 = S2(a) + F0((a),(b),(c));                   \
        (d) += temp1; (h) = temp1 + temp2;              \
    } while( 0 )

int mbedtls_internal_sha256_process( io_sha256_context_t *ctx,
                                const unsigned char data[64] )
{
    uint32_t temp1, temp2, W[64];
    uint32_t A[8];
    unsigned int i;

    SHA256_VALIDATE_RET( ctx != NULL );
    SHA256_VALIDATE_RET( (const unsigned char *)data != NULL );

    for( i = 0; i < 8; i++ )
        A[i] = ctx->state[i];

#if defined(MBEDTLS_SHA256_SMALLER)
    for( i = 0; i < 64; i++ )
    {
        if( i < 16 )
            GET_UINT32_BE( W[i], data, 4 * i );
        else
            R( i );

        P( A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i], K[i] );

        temp1 = A[7]; A[7] = A[6]; A[6] = A[5]; A[5] = A[4]; A[4] = A[3];
        A[3] = A[2]; A[2] = A[1]; A[1] = A[0]; A[0] = temp1;
    }
#else /* MBEDTLS_SHA256_SMALLER */
    for( i = 0; i < 16; i++ )
        GET_UINT32_BE( W[i], data, 4 * i );

    for( i = 0; i < 16; i += 8 )
    {
        P( A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i+0], K[i+0] );
        P( A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[i+1], K[i+1] );
        P( A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[i+2], K[i+2] );
        P( A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[i+3], K[i+3] );
        P( A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[i+4], K[i+4] );
        P( A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[i+5], K[i+5] );
        P( A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[i+6], K[i+6] );
        P( A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[i+7], K[i+7] );
    }

    for( i = 16; i < 64; i += 8 )
    {
        P( A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(i+0), K[i+0] );
        P( A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(i+1), K[i+1] );
        P( A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(i+2), K[i+2] );
        P( A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(i+3), K[i+3] );
        P( A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(i+4), K[i+4] );
        P( A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(i+5), K[i+5] );
        P( A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(i+6), K[i+6] );
        P( A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(i+7), K[i+7] );
    }
#endif /* MBEDTLS_SHA256_SMALLER */

    for( i = 0; i < 8; i++ )
        ctx->state[i] += A[i];

    return( 0 );
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void io_tls_sha256_process( io_sha256_context_t *ctx,
                             const unsigned char data[64] )
{
    mbedtls_internal_sha256_process( ctx, data );
}
#endif

/*
 * SHA-256 process buffer
 */
int io_tls_sha256_update_ret( io_sha256_context_t *ctx,
                               const unsigned char *input,
                               size_t ilen )
{
    int ret;
    size_t fill;
    uint32_t left;

    SHA256_VALIDATE_RET( ctx != NULL );
    SHA256_VALIDATE_RET( ilen == 0 || input != NULL );

    if( ilen == 0 )
        return( 0 );

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < (uint32_t) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy( (void *) (ctx->buffer + left), input, fill );

        if( ( ret = mbedtls_internal_sha256_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 64 )
    {
        if( ( ret = mbedtls_internal_sha256_process( ctx, input ) ) != 0 )
            return( ret );

        input += 64;
        ilen  -= 64;
    }

    if( ilen > 0 )
        memcpy( (void *) (ctx->buffer + left), input, ilen );

    return( 0 );
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void io_tls_sha256_update( io_sha256_context_t *ctx,
                            const unsigned char *input,
                            size_t ilen )
{
    io_tls_sha256_update_ret( ctx, input, ilen );
}
#endif

/*
 * SHA-256 final digest
 */
int io_tls_sha256_finish_ret( io_sha256_context_t *ctx,
                               unsigned char output[32] )
{
    int ret;
    uint32_t used;
    uint32_t high, low;

    SHA256_VALIDATE_RET( ctx != NULL );
    SHA256_VALIDATE_RET( (unsigned char *)output != NULL );

    /*
     * Add padding: 0x80 then 0x00 until 8 bytes remain for the length
     */
    used = ctx->total[0] & 0x3F;

    ctx->buffer[used++] = 0x80;

    if( used <= 56 )
    {
        /* Enough room for padding + length in current block */
        memset( ctx->buffer + used, 0, 56 - used );
    }
    else
    {
        /* We'll need an extra block */
        memset( ctx->buffer + used, 0, 64 - used );

        if( ( ret = mbedtls_internal_sha256_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        memset( ctx->buffer, 0, 56 );
    }

    /*
     * Add message length
     */
    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32_BE( high, ctx->buffer, 56 );
    PUT_UINT32_BE( low,  ctx->buffer, 60 );

    if( ( ret = mbedtls_internal_sha256_process( ctx, ctx->buffer ) ) != 0 )
        return( ret );

    /*
     * Output final state
     */
    PUT_UINT32_BE( ctx->state[0], output,  0 );
    PUT_UINT32_BE( ctx->state[1], output,  4 );
    PUT_UINT32_BE( ctx->state[2], output,  8 );
    PUT_UINT32_BE( ctx->state[3], output, 12 );
    PUT_UINT32_BE( ctx->state[4], output, 16 );
    PUT_UINT32_BE( ctx->state[5], output, 20 );
    PUT_UINT32_BE( ctx->state[6], output, 24 );

    if( ctx->is224 == 0 )
        PUT_UINT32_BE( ctx->state[7], output, 28 );

    return( 0 );
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void io_tls_sha256_finish( io_sha256_context_t *ctx,
                            unsigned char output[32] )
{
    io_tls_sha256_finish_ret( ctx, output );
}
#endif

/*
 * output = SHA-256( input buffer )
 */
int io_tls_sha256_ret( const unsigned char *input,
                        size_t ilen,
                        unsigned char output[32],
                        int is224 )
{
    int ret;
    io_sha256_context_t ctx;

    SHA256_VALIDATE_RET( is224 == 0 || is224 == 1 );
    SHA256_VALIDATE_RET( ilen == 0 || input != NULL );
    SHA256_VALIDATE_RET( (unsigned char *)output != NULL );

    io_tls_sha256_init( &ctx );

    if( ( ret = io_tls_sha256_starts_ret( &ctx, is224 ) ) != 0 )
        goto exit;

    if( ( ret = io_tls_sha256_update_ret( &ctx, input, ilen ) ) != 0 )
        goto exit;

    if( ( ret = io_tls_sha256_finish_ret( &ctx, output ) ) != 0 )
        goto exit;

exit:
    io_tls_sha256_free( &ctx );

    return( ret );
}

#undef SHR
#undef ROTR
#undef S0
#undef S1
#undef S2
#undef S3
#undef F0
#undef F1
#undef R
#undef P

// end of sha256

/*
 * Common and shared functions used by multiple modules in the Mbed TLS
 * library.
 *
 *  Copyright (C) 2018, Arm Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of Mbed TLS (https://tls.mbed.org)
 */

void
io_cpu_sha256_start (io_sha256_context_t *ctx) {

	memset (ctx,0,sizeof(io_sha256_context_t));
	io_tls_sha256_starts_ret (ctx,0);
}

void
io_cpu_sha256_update (io_sha256_context_t *ctx,uint8_t const *data,uint32_t size) {
	io_tls_sha256_update_ret (ctx,data,size);
}

void
io_cpu_sha256_finish (io_sha256_context_t *ctx,uint8_t output[32]) {
	io_tls_sha256_finish_ret (ctx,output);
}

#endif /* IMPLEMENT_IO_CORE */
#endif /* io_core_H_ */
/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Gregor Bruce
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
/*
------------------------------------------------------------------------------
stb_printf software used under license
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
/*
------------------------------------------------------------------------------
umm software used under license

Copyright (c) 2015 Ralph Hempel

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
