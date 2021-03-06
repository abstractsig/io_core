/*
 *
 * io_graphics.h
 *
 * LICENSE
 * =======
 * See end of file for license terms.
 *
 */
#ifndef io_graphics_H_
#define io_graphics_H_
#include <io_core.h>

#ifdef IO_GRAPHICS_FLOAT_IS_FLOAT32
# include <math.h>
# include <float.h>

typedef float32_t io_graphics_float_t;
# define io_graphics_float_abs	fabsf
# define FLOAT_COMPARE_EPSILON	0.0001
# define IO_GRAPHICS_FLOAT_MAX	FLT_MAX
# define IO_GRAPHICS_FLOAT_MIN	FLT_MIN
# define IO_GRAPHICS_FLOAT_MIN_NORMAL	(IO_GRAPHICS_FLOAT_MIN * 1e5)

#define double_to_io_graphics_float(d)	(d)
#define io_graphics_float_to_double(d)	(d)

bool io_graphics_compare_float32_equal (float32_t,float32_t,float32_t);
int	io_graphics_compare_float32 (float32_t,float32_t,float32_t);
# define io_graphics_float_compare(a,b) io_graphics_compare_float32(a,b,FLOAT_COMPARE_EPSILON)

#else
# error "you need to select a type of float for io graphics"
#endif


typedef struct io_i32_point {
	int32_t x;
	int32_t y;
} io_i32_point_t;

#define def_i32_point(x,y) 			((io_i32_point_t){x,y})
#define io_points_equal(a,b)		((a).x == (b).x && (a).y == (b).y)
#define io_points_not_equal(a,b)	((a).x != (b).x || (a).y != (b).y)

//
// pixels
//
typedef union PACK_STRUCTURE io_pixel {
	uint32_t all;
	struct PACK_STRUCTURE {
		uint32_t mono:1;
		uint32_t :23;
		uint8_t a;
	} monochrome_1bit;
	struct PACK_STRUCTURE {
		uint32_t mono:8;
		uint32_t :16;
		uint8_t a;
	} monochrome;
	struct PACK_STRUCTURE {
		uint32_t r:2;
		uint32_t :6;
		uint32_t g:2;
		uint32_t :6;
		uint32_t b:2;
		uint32_t :6;
		uint8_t a;
	} colour_2bit;
	struct PACK_STRUCTURE {
		uint32_t r:4;
		uint32_t :4;
		uint32_t g:4;
		uint32_t :4;
		uint32_t b:4;
		uint32_t :4;
		uint8_t a;
	} colour_4bit;
	struct PACK_STRUCTURE {
		uint32_t r:6;
		uint32_t :2;
		uint32_t g:6;
		uint32_t :2;
		uint32_t b:6;
		uint32_t :2;
		uint8_t a;
	} colour_6bit;
	struct PACK_STRUCTURE {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} colour_8bit;
} io_pixel_t;

#define io_pixel_alpha(px)					(px).colour_8bit.a

#define def_io_pixel_1bit_monochrome(bit)		((io_pixel_t){.monochrome_1bit = {.mono = bit,.a = 0,}})

#define def_io_pixel_2bita(r,g,b,a)		((io_pixel_t){.colour_2bit = {r,g,b,a}})
#define def_io_pixel_2bit(r,g,b)			def_io_pixel_2bita(r,g,b,0xff)

#define def_io_pixel_4bita(r,g,b,a)		((io_pixel_t){.colour_4bit = {r,g,b,a}})
#define def_io_pixel_4bit(r,g,b)			def_io_pixel_4bita(r,g,b,0xff)

#define def_io_pixel_6bita(r,g,b,a)		((io_pixel_t){.colour_6bit = {r,g,b,a}})
#define def_io_pixel_6bit(r,g,b)			def_io_pixel_6bita(r,g,b,0xff)

#define def_io_pixel_8bita(r,g,b,a)		((io_pixel_t){.colour_8bit = {r,g,b,a}})
#define def_io_pixel_8bit(r,g,b)			def_io_pixel_8bita(r,g,b,0xff)

#define io_pixel_colour_2bit_red(px)	(px).colour_2bit.r
#define io_pixel_colour_2bit_blue(px)	(px).colour_2bit.b
#define io_pixel_colour_2bit_green(px)	(px).colour_2bit.g

#define io_pixel_colour_4bit_red(px)	(px).colour_4bit.r
#define io_pixel_colour_4bit_blue(px)	(px).colour_4bit.b
#define io_pixel_colour_4bit_green(px)	(px).colour_4bit.g

#define io_pixel_colour_6bit_red(px)	(px).colour_6bit.r
#define io_pixel_colour_6bit_blue(px)	(px).colour_6bit.b
#define io_pixel_colour_6bit_green(px)	(px).colour_6bit.g

#define io_pixel_colour_red(px)			(px).colour_8bit.r
#define io_pixel_colour_blue(px)			(px).colour_8bit.b
#define io_pixel_colour_green(px)		(px).colour_8bit.g
#define io_pixel_colour_monochrome(px)	(px).monochrome.mono

#define are_pixel_levels_equal(a,b) (((a).all & 0x00ffffff) == ((b).all & 0x00ffffff))

typedef enum {
	IO_COLOUR_BLACK,
	IO_COLOUR_WHITE,

	IO_COLOUR_RED,
	IO_COLOUR_GREEN,
	IO_COLOUR_BLUE,

	IO_COLOUR_MAX_INDEX
} io_colour_id_t;

typedef struct io_colour_mix_implementation io_colour_mix_implementation_t;
typedef struct io_colour_mix io_colour_mix_t;
typedef struct io_colour io_colour_t;

typedef union colour_mix_bit_depth {
	struct  {
		uint32_t red:8;
		uint32_t green:8;
		uint32_t blue:8;
		uint32_t :8;
	} part;
	uint32_t all;
} colour_mix_bit_depth_t;

#define def_colour_mix_bit_depth(r,g,b)	((colour_mix_bit_depth_t){.part = {r,g,b}})

#define IO_COLOUR_MIX_IMPLEMENTATION_STRUCT_MEMBERS \
	io_colour_mix_implementation_t const *specialisation_of;	\
	colour_mix_bit_depth_t (*bit_depth) (void); \
	uint8_t (*maximum_bit_depth) (void); \
	uint8_t (*get_monochrome_level) (io_pixel_t); \
	uint8_t (*get_red_level) (io_pixel_t); \
	uint8_t (*get_green_level) (io_pixel_t); \
	uint8_t (*get_blue_level) (io_pixel_t); \
	io_pixel_t (*mk_pixel) (uint8_t,uint8_t,uint8_t); \
	io_pixel_t (*convert_to_8bit) (io_pixel_t); \
	io_pixel_t (*convert_from_8bit) (io_pixel_t); \
	io_colour_t (*get_standard_colour) (io_colour_id_t); \
	/**/

struct io_colour_mix_implementation {
	IO_COLOUR_MIX_IMPLEMENTATION_STRUCT_MEMBERS
};

struct io_colour_mix {
	io_colour_mix_implementation_t const *implementation;
};

//
// inline colour mix methods
//
INLINE_FUNCTION colour_mix_bit_depth_t
io_colour_mix_bit_depth (io_colour_mix_t const *cm) {
	return cm->implementation->bit_depth ();
}

INLINE_FUNCTION uint8_t
io_colour_mix_maximum_bit_depth (io_colour_mix_t const *cm) {
	return cm->implementation->maximum_bit_depth ();
}

INLINE_FUNCTION uint8_t
io_colour_mix_get_pixel_monochrome_level (io_colour_mix_t const *cm,io_pixel_t px) {
	return cm->implementation->get_monochrome_level (px);
}

INLINE_FUNCTION uint8_t
io_colour_mix_get_pixel_red_level (io_colour_mix_t const *cm,io_pixel_t px) {
	return cm->implementation->get_red_level (px);
}

INLINE_FUNCTION uint8_t
io_colour_mix_get_pixel_green_level (io_colour_mix_t const *cm,io_pixel_t px) {
	return cm->implementation->get_green_level (px);
}

INLINE_FUNCTION uint8_t
io_colour_mix_get_pixel_blue_level (io_colour_mix_t const *cm,io_pixel_t px) {
	return cm->implementation->get_blue_level (px);
}

INLINE_FUNCTION io_pixel_t
io_colour_mix_make_pixel (io_colour_mix_t const *cm,uint8_t r,uint8_t g,uint8_t b) {
	return cm->implementation->mk_pixel (r,g,b);
}

INLINE_FUNCTION io_pixel_t
io_colour_mix_convert_pixel_to_8bit (io_colour_mix_t const *cm,io_pixel_t px) {
	return cm->implementation->convert_to_8bit (px);
}

INLINE_FUNCTION io_pixel_t
io_colour_mix_convert_pixel_from_8bit (io_colour_mix_t const *cm,io_pixel_t px) {
	return cm->implementation->convert_from_8bit (px);
}

extern EVENT_DATA io_colour_mix_t io_colour_mix_8bit;
extern EVENT_DATA io_colour_mix_t io_colour_mix_6bit;
extern EVENT_DATA io_colour_mix_t io_colour_mix_4bit;
extern EVENT_DATA io_colour_mix_t io_colour_mix_2bit;
extern EVENT_DATA io_colour_mix_t io_colour_mix_1bit_monochrome;

struct PACK_STRUCTURE io_colour {
	io_colour_mix_t const *mix;
	io_pixel_t level;
};

#define io_colour_mix(c)	(c).mix
#define io_colour_level(c)	(c).level

#define decl_8bit_colour(r,g,b)	{.mix = &io_colour_mix_8bit,.level = def_io_pixel_8bit(r,g,b)}
#define def_8bit_colour(r,g,b)	((io_colour_t) decl_8bit_colour(r,g,b))

#define decl_6bit_colour(r,g,b)	{.mix = &io_colour_mix_6bit,.level = def_io_pixel_6bit(r,g,b)}
#define def_6bit_colour(r,g,b)	((io_colour_t) decl_6bit_colour(r,g,b))

#define decl_4bit_colour(r,g,b)	{.mix = &io_colour_mix_4bit,.level = def_io_pixel_4bit(r,g,b)}
#define decl_2bit_colour(r,g,b)	{.mix = &io_colour_mix_2bit,.level = def_io_pixel_2bit(r,g,b)}

#define decl_1bit_monochrome_colour(bit)	{.mix = &io_colour_mix_1bit_monochrome,.level = def_io_pixel_1bit_monochrome(bit)}

// and mix==?
#define colours_are_equal(c1,c2) are_pixel_levels_equal(io_colour_level(c1),io_colour_level(c2))

#define io_colour_get_std_colour(c,id) io_colour_mix_get_standard_colour(io_colour_mix(c),id)

io_colour_t convert_io_colour (io_colour_t from,io_colour_t to);
io_pixel_t	io_colour_get_composite_level (io_colour_t);
io_colour_t scale_io_colour (io_colour_t base,io_colour_t by);

INLINE_FUNCTION io_colour_t
io_colour_mix_get_standard_colour (io_colour_mix_t const *cm,io_colour_id_t id) {
	return cm->implementation->get_standard_colour (id);
}

INLINE_FUNCTION uint8_t
io_colour_monochrome_level (io_colour_t c) {
	return io_colour_mix(c)->implementation->get_monochrome_level (io_colour_level(c));
}

INLINE_FUNCTION uint8_t
io_colour_get_red_level (io_colour_t c) {
	return io_colour_mix(c)->implementation->get_red_level (io_colour_level(c));
}

INLINE_FUNCTION uint8_t
io_colour_get_green_level (io_colour_t c) {
	return io_colour_mix(c)->implementation->get_green_level (io_colour_level(c));
}

INLINE_FUNCTION uint8_t
io_colour_get_blue_level (io_colour_t c) {
	return io_colour_mix(c)->implementation->get_blue_level (io_colour_level(c));
}

INLINE_FUNCTION uint8_t
io_colour_get_maximum_bit_depth (io_colour_t c) {
	return io_colour_mix_maximum_bit_depth(io_colour_mix(c));
}

// the standard colours

#define IO_BLACK_8BIT					io_colour_mix_get_standard_colour(&io_colour_mix_8bit,IO_COLOUR_BLACK)
#define IO_WHITE_8BIT					io_colour_mix_get_standard_colour(&io_colour_mix_8bit,IO_COLOUR_WHITE)
#define IO_8BIT_COLOUR_RED				io_colour_mix_get_standard_colour(&io_colour_mix_8bit,IO_COLOUR_RED)
#define IO_8BIT_COLOUR_GREEN			io_colour_mix_get_standard_colour(&io_colour_mix_8bit,IO_COLOUR_GREEN)
#define IO_8BIT_COLOUR_BLUE			io_colour_mix_get_standard_colour(&io_colour_mix_8bit,IO_COLOUR_BLUE)

#define IO_BLACK_6BIT					io_colour_mix_get_standard_colour(&io_colour_mix_6bit,IO_COLOUR_BLACK)
#define IO_WHITE_6BIT					io_colour_mix_get_standard_colour(&io_colour_mix_6bit,IO_COLOUR_WHITE)
#define IO_6BIT_COLOUR_RED				io_colour_mix_get_standard_colour(&io_colour_mix_6bit,IO_COLOUR_RED)
#define IO_6BIT_COLOUR_GREEN			io_colour_mix_get_standard_colour(&io_colour_mix_6bit,IO_COLOUR_GREEN)
#define IO_6BIT_COLOUR_BLUE			io_colour_mix_get_standard_colour(&io_colour_mix_6bit,IO_COLOUR_BLUE)

#define IO_BLACK_4BIT					io_colour_mix_get_standard_colour(&io_colour_mix_4bit,IO_COLOUR_BLACK)
#define IO_WHITE_4BIT					io_colour_mix_get_standard_colour(&io_colour_mix_4bit,IO_COLOUR_WHITE)
#define IO_4BIT_COLOUR_RED				io_colour_mix_get_standard_colour(&io_colour_mix_4bit,IO_COLOUR_RED)
#define IO_4BIT_COLOUR_GREEN			io_colour_mix_get_standard_colour(&io_colour_mix_4bit,IO_COLOUR_GREEN)
#define IO_4BIT_COLOUR_BLUE			io_colour_mix_get_standard_colour(&io_colour_mix_4bit,IO_COLOUR_BLUE)

#define IO_BLACK_2BIT					io_colour_mix_get_standard_colour(&io_colour_mix_2bit,IO_COLOUR_BLACK)
#define IO_WHITE_2BIT					io_colour_mix_get_standard_colour(&io_colour_mix_2bit,IO_COLOUR_WHITE)

#define IO_BLACK_1BIT_MONOCHROME		io_colour_mix_get_standard_colour(&io_colour_mix_1bit_monochrome,IO_COLOUR_BLACK)
#define IO_WHITE_1BIT_MONOCHROME		io_colour_mix_get_standard_colour(&io_colour_mix_1bit_monochrome,IO_COLOUR_WHITE)

//
// graphics font
//

typedef struct io_graphics_font io_graphics_font_t;
typedef struct io_graphics_context io_graphics_context_t;

typedef struct io_character_bitmap {
	int32_t baseline;
	uint8_t *bytes;
} io_character_bitmap_t;

typedef struct PACK_STRUCTURE {
	void (*free) (io_graphics_font_t*);
	uint32_t (*get_pixel_height) (io_graphics_font_t*);
	void (*set_pixel_height) (io_graphics_font_t*,uint32_t);
	io_character_bitmap_t (*get_codepoint_bitmap) (io_graphics_font_t*,int,int*,int*,int*,int*);
} io_graphics_font_implementation_t;

#define IO_GRAPHICS_FONT_STRUCT_MEMBERS \
	io_graphics_font_implementation_t const *implementation;\
	const char *name; \
	/**/

struct PACK_STRUCTURE io_graphics_font {
	IO_GRAPHICS_FONT_STRUCT_MEMBERS
};

//
// inline graphics font methods
//
INLINE_FUNCTION void
free_io_graphics_font (io_graphics_font_t *font) {
	font->implementation->free (font);
}

INLINE_FUNCTION io_character_bitmap_t
io_graphics_font_get_codepoint_bitmap (io_graphics_font_t *font,int codepoint,int *width,int *height,int *xoff,int *yoff) {
	return font->implementation->get_codepoint_bitmap(font,codepoint,width,height,xoff,yoff);
}

INLINE_FUNCTION void
io_graphics_font_set_pixel_height (io_graphics_font_t *font,uint32_t h) {
	font->implementation->set_pixel_height (font,h);
}

INLINE_FUNCTION uint32_t
io_graphics_font_get_pixel_height (io_graphics_font_t *font) {
	return font->implementation->get_pixel_height (font);
}

//
// graphics command
//

typedef struct io_graphics_command io_graphics_command_t;
typedef struct io_graphics_command_implementation {
	void (*free) (io_byte_memory_t*,io_graphics_command_t*);
	void (*run) (io_graphics_command_t*,io_graphics_context_t*);
} io_graphics_command_implementation_t;

#define IO_GRAPHICS_COMMAND_STRUCT_MEMBERS \
	io_graphics_command_implementation_t const *implementation;\
	/**/

struct PACK_STRUCTURE io_graphics_command {
	IO_GRAPHICS_COMMAND_STRUCT_MEMBERS
};


//
// inline graphics command methods
//
INLINE_FUNCTION void
free_io_graphics_command (io_byte_memory_t *bm,io_graphics_command_t *cmd) {
	cmd->implementation->free (bm,cmd);
}

INLINE_FUNCTION void
run_io_graphics_command (io_graphics_command_t *cmd,io_graphics_context_t *gfx) {
	cmd->implementation->run (cmd,gfx);
}

typedef struct io_graphics_command_stack {
	io_byte_memory_t *bm;
	io_graphics_command_t **commands;
	io_graphics_command_t **cursor;
	io_graphics_command_t **end_of_allocation;
	uint32_t grow;
} io_graphics_command_stack_t;

#define io_graphics_command_stack_begin(b)	(b)->commands
#define io_graphics_command_stack_end(b)		(b)->cursor
#define io_graphics_command_stack_memory(b)	(b)->bm

io_graphics_command_stack_t* mk_io_graphics_command_stack (io_byte_memory_t*,uint32_t);
void free_io_graphics_command_stack (io_graphics_command_stack_t*);
void reset_io_graphics_command_stack (io_graphics_command_stack_t*);

bool io_graphics_stack_append_line (io_graphics_command_stack_t*,io_i32_point_t,io_i32_point_t);
bool io_graphics_stack_append_text (io_graphics_command_stack_t*,io_character_t*,uint32_t,io_i32_point_t);
bool io_graphics_stack_append_circle (io_graphics_command_stack_t*,io_i32_point_t,uint32_t,bool);
bool io_graphics_stack_append_rectangle (io_graphics_command_stack_t*,io_i32_point_t,io_i32_point_t,bool);

/*
enum gfx_command_type {
    NK_COMMAND_NOP,
    NK_COMMAND_SCISSOR,
    NK_COMMAND_LINE,
    NK_COMMAND_CURVE,
    NK_COMMAND_RECT,
    NK_COMMAND_RECT_FILLED,
    NK_COMMAND_RECT_MULTI_COLOR,
    NK_COMMAND_CIRCLE,
    NK_COMMAND_CIRCLE_FILLED,
    NK_COMMAND_ARC,
    NK_COMMAND_ARC_FILLED,
    NK_COMMAND_TRIANGLE,
    NK_COMMAND_TRIANGLE_FILLED,
    NK_COMMAND_POLYGON,
    NK_COMMAND_POLYGON_FILLED,
    NK_COMMAND_POLYLINE,
    NK_COMMAND_TEXT,
    NK_COMMAND_IMAGE,
    NK_COMMAND_CUSTOM
};
*/

//
// graphics context
//

typedef struct PACK_STRUCTURE {
	const char *name;
	uint8_t const *ttf;
} io_ttf_data_t;

extern const io_ttf_data_t io_graphics_ttf_fonts[];

typedef struct PACK_STRUCTURE {
	void (*free) (io_graphics_context_t*);
	bool (*select_font_by_name) (io_graphics_context_t*,uint8_t const*,uint32_t);
	void (*set_drawing_colour) (io_graphics_context_t*,io_colour_t);
	io_colour_t (*get_drawing_colour) (io_graphics_context_t*);
	void (*set_gamma_correction) (io_graphics_context_t*,io_graphics_float_t);
	io_graphics_float_t (*get_gamma_correction) (io_graphics_context_t*);
	bool (*get_pixel) (io_graphics_context_t*,io_i32_point_t,io_pixel_t*);
	uint32_t (*get_pixel_height) (io_graphics_context_t*);
	uint32_t (*get_pixel_width) (io_graphics_context_t*);
	io_graphics_command_stack_t* (*get_command_stack) (io_graphics_context_t*);
	void (*draw_pixel) (io_graphics_context_t*,io_i32_point_t);
	int32_t (*draw_character) (io_graphics_context_t*,io_character_t,io_i32_point_t);
	void (*fill_rectangle) (io_graphics_context_t*,io_i32_point_t,io_i32_point_t);
	void (*begin) (io_graphics_context_t*);
	void (*run) (io_graphics_context_t*);
	void (*render) (io_graphics_context_t*);

	void (*fill) (io_graphics_context_t*,io_colour_t);
	void (*draw_ascii_text) (io_graphics_context_t*,char const*,io_i32_point_t);
	void (*draw_circle) (io_graphics_context_t*,io_i32_point_t,int);
	void (*draw_filled_circle) (io_graphics_context_t*,io_i32_point_t,int);
	void (*draw_line) (io_graphics_context_t*,io_i32_point_t,io_i32_point_t);
	void (*draw_rectangle) (io_graphics_context_t*,io_i32_point_t,io_i32_point_t);
	void (*draw_filled_rectangle) (io_graphics_context_t*,io_i32_point_t,io_i32_point_t);
} io_graphics_context_implementation_t;

#define IO_GRAPHICS_CONTEXT_STRUCT_MEMBERS \
	io_graphics_context_implementation_t const *implementation;\
	io_byte_memory_t *bm;\
	io_ttf_data_t const *ttf_font_data;\
	io_graphics_font_t *current_font;\
	/**/

struct PACK_STRUCTURE io_graphics_context {
	IO_GRAPHICS_CONTEXT_STRUCT_MEMBERS
};

#define io_graphics_context_current_font(gfx)	(gfx)->current_font
#define io_graphics_context_io(g)				(g)->bm->io

void initialise_io_graphics_context (io_graphics_context_t*,io_byte_memory_t*,io_ttf_data_t const*);
void io_graphics_context_circle (io_graphics_context_t*,io_i32_point_t,int);
void io_graphics_context_fill_circle (io_graphics_context_t*,io_i32_point_t,int);
void io_graphics_context_one_pixel_line (io_graphics_context_t*,io_i32_point_t,io_i32_point_t);
int32_t io_graphics_context_draw_character_with_current_font (io_graphics_context_t*,io_character_t,io_i32_point_t);
void io_graphics_context_fill_with_colour (io_graphics_context_t*,io_colour_t);
void io_graphics_context_draw_rectangle_base (io_graphics_context_t*,io_i32_point_t,io_i32_point_t);
void io_graphics_context_draw_filled_rectangle_base (io_graphics_context_t*,io_i32_point_t,io_i32_point_t);
void io_graphics_context_draw_draw_ascii_text_base (io_graphics_context_t*,char const*,io_i32_point_t);
bool io_graphics_context_select_ttf_font_by_name (io_graphics_context_t*,uint8_t const*,uint32_t);

//
// inline graphics context methods
//
INLINE_FUNCTION void
free_io_graphics_context (io_graphics_context_t *gfx) {
	gfx->implementation->free(gfx);
}

INLINE_FUNCTION void
io_graphics_context_fill (io_graphics_context_t *gfx,io_colour_t p) {
	gfx->implementation->fill(gfx,p);
}

INLINE_FUNCTION void
io_graphics_context_fill_rectangle (io_graphics_context_t *gfx,io_i32_point_t p1,io_i32_point_t p2) {
	gfx->implementation->fill_rectangle(gfx,p1,p2);
}

INLINE_FUNCTION void
io_graphics_context_draw_character (io_graphics_context_t *gfx,io_character_t c,io_i32_point_t p) {
	gfx->implementation->draw_character(gfx,c,p);
}

INLINE_FUNCTION void
io_graphics_context_draw_ascii_text (io_graphics_context_t *gfx,char const *t,io_i32_point_t p) {
	gfx->implementation->draw_ascii_text(gfx,t,p);
}

INLINE_FUNCTION io_graphics_command_stack_t*
io_graphics_context_get_command_stack (io_graphics_context_t *gfx) {
	return gfx->implementation->get_command_stack(gfx);
}

INLINE_FUNCTION uint32_t
io_graphics_context_get_height_in_pixels (io_graphics_context_t *gfx) {
	return gfx->implementation->get_pixel_height(gfx);
}

INLINE_FUNCTION uint32_t
io_graphics_context_get_width_in_pixels (io_graphics_context_t *gfx) {
	return gfx->implementation->get_pixel_width(gfx);
}

INLINE_FUNCTION void
io_graphics_context_set_gamma_correction (io_graphics_context_t *gfx,io_graphics_float_t g) {
	gfx->implementation->set_gamma_correction(gfx,g);
}

INLINE_FUNCTION io_graphics_float_t
io_graphics_context_get_gamma_correction (io_graphics_context_t *gfx) {
	return gfx->implementation->get_gamma_correction(gfx);
}

INLINE_FUNCTION void
io_graphics_context_set_colour (io_graphics_context_t *gfx,io_colour_t c) {
	gfx->implementation->set_drawing_colour(gfx,c);
}

INLINE_FUNCTION void
io_graphics_context_set_drawing_colour (io_graphics_context_t *gfx,io_colour_t c) {
	gfx->implementation->set_drawing_colour(gfx,c);
}

INLINE_FUNCTION io_colour_t
io_graphics_context_get_drawing_colour (io_graphics_context_t *gfx) {
	return gfx->implementation->get_drawing_colour(gfx);
}

INLINE_FUNCTION void
io_graphics_context_draw_pixel (io_graphics_context_t *gfx,io_i32_point_t p) {
	gfx->implementation->draw_pixel(gfx,p);
}

INLINE_FUNCTION void
io_graphics_context_draw_circle (io_graphics_context_t *gfx,io_i32_point_t pt,int r) {
	gfx->implementation->draw_circle(gfx,pt,r);
}

INLINE_FUNCTION void
io_graphics_context_draw_filled_circle (io_graphics_context_t *gfx,io_i32_point_t pt,int r) {
	gfx->implementation->draw_filled_circle(gfx,pt,r);
}

INLINE_FUNCTION void
io_graphics_context_draw_line (io_graphics_context_t *gfx,io_i32_point_t p1,io_i32_point_t p2) {
	gfx->implementation->draw_line(gfx,p1,p2);
}

INLINE_FUNCTION bool
io_graphics_context_get_pixel (io_graphics_context_t *gfx,io_i32_point_t at,io_pixel_t *px) {
	return gfx->implementation->get_pixel(gfx,at,px);
}

INLINE_FUNCTION void
io_graphics_context_begin (io_graphics_context_t *gfx) {
	gfx->implementation->begin(gfx);
}

INLINE_FUNCTION void
io_graphics_context_run (io_graphics_context_t *gfx) {
	gfx->implementation->run(gfx);
}

INLINE_FUNCTION void
io_graphics_context_render (io_graphics_context_t *gfx) {
	gfx->implementation->render(gfx);
}

INLINE_FUNCTION void
io_graphics_context_draw_rectangle (io_graphics_context_t *gfx,io_i32_point_t p1,io_i32_point_t p2) {
	gfx->implementation->draw_rectangle(gfx,p1,p2);
}

INLINE_FUNCTION void
io_graphics_context_draw_filled_rectangle (io_graphics_context_t *gfx,io_i32_point_t p1,io_i32_point_t p2) {
	gfx->implementation->draw_filled_rectangle(gfx,p1,p2);
}

INLINE_FUNCTION bool
io_graphics_context_select_font_by_name (io_graphics_context_t *gfx,uint8_t const *n,uint32_t length) {
	return gfx->implementation->select_font_by_name (gfx,n,length);
}

//
//-----------------------------------------------------------------------------
//
// Implementation
//
//-----------------------------------------------------------------------------
//
#ifdef IMPLEMENT_IO_GRAPHICS


static colour_mix_bit_depth_t
cm_8bit_bit_depth (void) {
	return def_colour_mix_bit_depth(8,8,8);
}

static uint8_t
cm_8bit_maximum_bit_depth (void) {
	return 8;
}

static uint8_t
cm_8bit_get_monochrome_level (io_pixel_t px) {
	uint16_t c = (
			io_pixel_colour_red(px)
		+	io_pixel_colour_green(px)
		+	io_pixel_colour_blue(px)
	);
	return (uint8_t) (c/3);
}

static uint8_t
cm_8bit_get_red_level (io_pixel_t px) {
	return io_pixel_colour_red(px);
}

static uint8_t
cm_8bit_get_green_level (io_pixel_t px) {
	return io_pixel_colour_green(px);
}

static uint8_t
cm_8bit_get_blue_level (io_pixel_t px) {
	return io_pixel_colour_blue(px);
}

static io_pixel_t
cm_8bit_make_pixel (uint8_t r,uint8_t g,uint8_t b) {
	return def_io_pixel_8bit (r,g,b);
}

static io_pixel_t
cm_8bit_convert_to_8bit (io_pixel_t px) {
	return px;
}

io_colour_t
cm_8bit_get_standard_colour (io_colour_id_t id) {
	static const io_colour_t c[] = {
		/* IO_COLOUR_BLACK */	decl_8bit_colour(0,0,0),
		/* IO_COLOUR_WHITE */	decl_8bit_colour(0xff,0xff,0xff),
		/* IO_COLOUR_RED */		decl_8bit_colour(0xff,0,0),
		/* IO_COLOUR_GREEN */	decl_8bit_colour(0,0xff,0),
		/* IO_COLOUR_BLUE */	decl_8bit_colour(0,0,0xff),
	};
	if (id < IO_COLOUR_MAX_INDEX) {
		return c[id];
	} else {
		return c[0];
	}
}

EVENT_DATA io_colour_mix_implementation_t colour_mix_8bit_implementation = {
	.specialisation_of = NULL,
	.bit_depth = cm_8bit_bit_depth,
	.maximum_bit_depth = cm_8bit_maximum_bit_depth,
	.mk_pixel = cm_8bit_make_pixel,
	.get_monochrome_level = cm_8bit_get_monochrome_level,
	.get_red_level = cm_8bit_get_red_level,
	.get_green_level = cm_8bit_get_green_level,
	.get_blue_level = cm_8bit_get_blue_level,
	.convert_to_8bit = cm_8bit_convert_to_8bit,
	.convert_from_8bit = cm_8bit_convert_to_8bit,
	.get_standard_colour = cm_8bit_get_standard_colour,
};

EVENT_DATA io_colour_mix_t io_colour_mix_8bit = {
	.implementation = &colour_mix_8bit_implementation,
};

static colour_mix_bit_depth_t
cm_6bit_bit_depth (void) {
	return def_colour_mix_bit_depth(6,6,6);
}

static uint8_t
cm_6bit_maximum_bit_depth (void) {
	return 6;
}

static uint8_t
cm_6bit_get_red_level (io_pixel_t px) {
	return io_pixel_colour_6bit_red(px);
}

static uint8_t
cm_6bit_get_green_level (io_pixel_t px) {
	return io_pixel_colour_6bit_green(px);
}

static uint8_t
cm_6bit_get_blue_level (io_pixel_t px) {
	return io_pixel_colour_6bit_blue(px);
}

static io_pixel_t
cm_6bit_make_pixel (uint8_t r,uint8_t g,uint8_t b) {
	return def_io_pixel_6bit (r,g,b);
}

static io_pixel_t
cm_6bit_convert_to_8bit (io_pixel_t px) {
	return def_io_pixel_8bit (
		io_pixel_colour_red(px) << 2,
		io_pixel_colour_green(px) << 2,
		io_pixel_colour_blue(px) << 2
	);
}

static io_pixel_t
cm_6bit_convert_from_8bit (io_pixel_t px) {
	return def_io_pixel_6bit (
		io_pixel_colour_red(px) >> 2,
		io_pixel_colour_green(px) >> 2,
		io_pixel_colour_blue(px) >> 2
	);
}

static io_colour_t
cm_6bit_get_standard_colour (io_colour_id_t id) {
	static const io_colour_t c[] = {
		/* IO_COLOUR_BLACK */	decl_6bit_colour(0,0,0),
		/* IO_COLOUR_WHITE */	decl_6bit_colour(0x3f,0x3f,0x3f),
		/* IO_COLOUR_RED */		decl_6bit_colour(0x3f,0,0),
		/* IO_COLOUR_GREEN */	decl_6bit_colour(0,0x3f,0),
		/* IO_COLOUR_BLUE */	decl_6bit_colour(0,0,0x3f),
	};
	if (id < IO_COLOUR_MAX_INDEX) {
		return c[id];
	} else {
		return c[0];
	}
}

EVENT_DATA io_colour_mix_implementation_t colour_mix_6bit_implementation = {
	.specialisation_of = NULL,
	.bit_depth = cm_6bit_bit_depth,
	.maximum_bit_depth = cm_6bit_maximum_bit_depth,
	.mk_pixel = cm_6bit_make_pixel,
	.get_monochrome_level = cm_8bit_get_monochrome_level,
	.get_red_level = cm_6bit_get_red_level,
	.get_green_level = cm_6bit_get_green_level,
	.get_blue_level = cm_6bit_get_blue_level,
	.convert_to_8bit = cm_6bit_convert_to_8bit,
	.convert_from_8bit = cm_6bit_convert_from_8bit,
	.get_standard_colour = cm_6bit_get_standard_colour,
};

EVENT_DATA io_colour_mix_t io_colour_mix_6bit = {
	.implementation = &colour_mix_6bit_implementation,
};

/////////////
static colour_mix_bit_depth_t
cm_4bit_bit_depth (void) {
	return def_colour_mix_bit_depth(4,4,4);
}

static uint8_t
cm_4bit_maximum_bit_depth (void) {
	return 4;
}

static uint8_t
cm_4bit_get_red_level (io_pixel_t px) {
	return io_pixel_colour_4bit_red(px);
}

static uint8_t
cm_4bit_get_green_level (io_pixel_t px) {
	return io_pixel_colour_4bit_green(px);
}

static uint8_t
cm_4bit_get_blue_level (io_pixel_t px) {
	return io_pixel_colour_4bit_blue(px);
}

static io_pixel_t
cm_4bit_make_pixel (uint8_t r,uint8_t g,uint8_t b) {
	return def_io_pixel_4bit (r,g,b);
}

static io_pixel_t
cm_4bit_convert_to_8bit (io_pixel_t px) {
	return def_io_pixel_8bit (
		io_pixel_colour_red(px) << 4,
		io_pixel_colour_green(px) << 4,
		io_pixel_colour_blue(px) << 4
	);
}

static io_pixel_t
cm_4bit_convert_from_8bit (io_pixel_t px) {
	return def_io_pixel_4bit (
		io_pixel_colour_red(px) >> 4,
		io_pixel_colour_green(px) >> 4,
		io_pixel_colour_blue(px) >> 4
	);
}

static io_colour_t
cm_4bit_get_standard_colour (io_colour_id_t id) {
	static const io_colour_t c[] = {
		/* IO_COLOUR_BLACK */	decl_4bit_colour(0,0,0),
		/* IO_COLOUR_WHITE */	decl_4bit_colour(0xf,0xf,0xf),
		/* IO_COLOUR_RED */		decl_4bit_colour(0xf,0,0),
		/* IO_COLOUR_GREEN */	decl_4bit_colour(0,0xf,0),
		/* IO_COLOUR_BLUE */	decl_4bit_colour(0,0,0xf),
	};
	if (id < IO_COLOUR_MAX_INDEX) {
		return c[id];
	} else {
		return c[0];
	}
}

EVENT_DATA io_colour_mix_implementation_t colour_mix_4bit_implementation = {
	.specialisation_of = NULL,
	.bit_depth = cm_4bit_bit_depth,
	.maximum_bit_depth = cm_4bit_maximum_bit_depth,
	.mk_pixel = cm_4bit_make_pixel,
	.get_monochrome_level = cm_8bit_get_monochrome_level,
	.get_red_level = cm_4bit_get_red_level,
	.get_green_level = cm_4bit_get_green_level,
	.get_blue_level = cm_4bit_get_blue_level,
	.convert_to_8bit = cm_4bit_convert_to_8bit,
	.convert_from_8bit = cm_4bit_convert_from_8bit,
	.get_standard_colour = cm_4bit_get_standard_colour,
};

EVENT_DATA io_colour_mix_t io_colour_mix_4bit = {
	.implementation = &colour_mix_4bit_implementation,
};

/////////////
static colour_mix_bit_depth_t
cm_2bit_bit_depth (void) {
	return def_colour_mix_bit_depth(2,2,2);
}

static uint8_t
cm_2bit_maximum_bit_depth (void) {
	return 2;
}

static uint8_t
cm_2bit_get_red_level (io_pixel_t px) {
	return io_pixel_colour_2bit_red(px);
}

static uint8_t
cm_2bit_get_green_level (io_pixel_t px) {
	return io_pixel_colour_2bit_green(px);
}

static uint8_t
cm_2bit_get_blue_level (io_pixel_t px) {
	return io_pixel_colour_2bit_blue(px);
}

static io_pixel_t
cm_2bit_make_pixel (uint8_t r,uint8_t g,uint8_t b) {
	return def_io_pixel_2bit (r,g,b);
}

static io_pixel_t
cm_2bit_convert_to_8bit (io_pixel_t px) {
	return def_io_pixel_8bit (
		io_pixel_colour_red(px) << 6,
		io_pixel_colour_green(px) << 6,
		io_pixel_colour_blue(px) << 6
	);
}

static io_pixel_t
cm_2bit_convert_from_8bit (io_pixel_t px) {
	return def_io_pixel_2bit (
		io_pixel_colour_red(px) >> 6,
		io_pixel_colour_green(px) >> 6,
		io_pixel_colour_blue(px) >> 6
	);
}

static io_colour_t
cm_2bit_get_standard_colour (io_colour_id_t id) {
	static const io_colour_t c[] = {
		/* IO_COLOUR_BLACK */	decl_2bit_colour(0,0,0),
		/* IO_COLOUR_WHITE */	decl_2bit_colour(0x3,0x3,0x3),
		/* IO_COLOUR_RED */		decl_2bit_colour(0x3,0,0),
		/* IO_COLOUR_GREEN */	decl_2bit_colour(0,0x3,0),
		/* IO_COLOUR_BLUE */		decl_2bit_colour(0,0,0x3),
	};
	if (id < IO_COLOUR_MAX_INDEX) {
		return c[id];
	} else {
		return c[0];
	}
}

EVENT_DATA io_colour_mix_implementation_t colour_mix_2bit_implementation = {
	.specialisation_of = NULL,
	.bit_depth = cm_2bit_bit_depth,
	.maximum_bit_depth = cm_2bit_maximum_bit_depth,
	.mk_pixel = cm_2bit_make_pixel,
	.get_monochrome_level = cm_8bit_get_monochrome_level,
	.get_red_level = cm_2bit_get_red_level,
	.get_green_level = cm_2bit_get_green_level,
	.get_blue_level = cm_2bit_get_blue_level,
	.convert_to_8bit = cm_2bit_convert_to_8bit,
	.convert_from_8bit = cm_2bit_convert_from_8bit,
	.get_standard_colour = cm_2bit_get_standard_colour,
};

EVENT_DATA io_colour_mix_t io_colour_mix_2bit = {
	.implementation = &colour_mix_2bit_implementation,
};

/////////////
static colour_mix_bit_depth_t
cm_1bit_monochrome_bit_depth (void) {
	return def_colour_mix_bit_depth(0,0,0);
}

static uint8_t
cm_1bit_monochrome_maximum_bit_depth (void) {
	return 1;
}

static uint8_t
cm_1bit_monochrome_get_monochrome_level (io_pixel_t px) {
	return io_pixel_colour_monochrome(px);
}

static uint8_t
cm_1bit_monochrome_get_red_level (io_pixel_t px) {
	return 0;
}

static uint8_t
cm_1bit_monochrome_get_green_level (io_pixel_t px) {
	return 0;
}

static uint8_t
cm_1bit_monochrome_get_blue_level (io_pixel_t px) {
	return 0;
}

static io_pixel_t
cm_1bit_monochrome_convert_to_8bit (io_pixel_t px) {
	if (io_pixel_colour_monochrome(px)) {
		return def_io_pixel_8bit(0xff,0xff,0xff);
	} else {
		return def_io_pixel_8bit(0,0,0);
	}
}

static io_pixel_t
cm_1bit_monochrome_convert_from_8bit (io_pixel_t px) {
	if (
			io_pixel_colour_red(px) > 0
		|| io_pixel_colour_green(px) > 0
		|| io_pixel_colour_blue(px) > 0
	) {
		return def_io_pixel_1bit_monochrome(1);
	} else {
		return def_io_pixel_1bit_monochrome(0);
	}
}

static io_colour_t
cm_1bit_monochrome_get_standard_colour (io_colour_id_t id) {
	static const io_colour_t c[] = {
		/* IO_COLOUR_BLACK */	decl_1bit_monochrome_colour(0),
		/* IO_COLOUR_WHITE */	decl_1bit_monochrome_colour(1),
		/* IO_COLOUR_RED */		decl_1bit_monochrome_colour(1),
		/* IO_COLOUR_GREEN */	decl_1bit_monochrome_colour(1),
		/* IO_COLOUR_BLUE */		decl_1bit_monochrome_colour(1),
	};
	if (id < IO_COLOUR_MAX_INDEX) {
		return c[id];
	} else {
		return c[0];
	}
}

static io_pixel_t
cm_1bit_monochrome_make_pixel (uint8_t r,uint8_t g,uint8_t b) {
	if (
			r > 0
		|| g > 0
		|| b > 0
	) {
		return def_io_pixel_1bit_monochrome(1);
	} else {
		return def_io_pixel_1bit_monochrome(0);
	}
}

EVENT_DATA io_colour_mix_implementation_t 
colour_mix_1bit_monochrome_implementation = {
	.specialisation_of = NULL,
	.bit_depth = cm_1bit_monochrome_bit_depth,
	.maximum_bit_depth = cm_1bit_monochrome_maximum_bit_depth,
	.mk_pixel = cm_1bit_monochrome_make_pixel,
	.get_monochrome_level = cm_1bit_monochrome_get_monochrome_level,
	.get_red_level = cm_1bit_monochrome_get_red_level,
	.get_green_level = cm_1bit_monochrome_get_green_level,
	.get_blue_level = cm_1bit_monochrome_get_blue_level,
	.convert_to_8bit = cm_1bit_monochrome_convert_to_8bit,
	.convert_from_8bit = cm_1bit_monochrome_convert_from_8bit,
	.get_standard_colour = cm_1bit_monochrome_get_standard_colour,
};

EVENT_DATA io_colour_mix_t io_colour_mix_1bit_monochrome = {
	.implementation = &colour_mix_1bit_monochrome_implementation,
};


io_colour_t
convert_io_colour (io_colour_t from,io_colour_t to) {
	io_colour_level (to) = io_colour_mix_convert_pixel_from_8bit (
		io_colour_mix (to),
		io_colour_mix_convert_pixel_to_8bit (
			io_colour_mix (from),io_colour_level (from)
		)
	);

	return to;
}

//
// produce a colour like base but scalled by with
//
io_colour_t
scale_io_colour (io_colour_t base,io_colour_t by) {
	io_pixel_t p8 = io_colour_mix_convert_pixel_to_8bit (
		io_colour_mix (by),io_colour_level (by)
	);

	io_pixel_t b8 = io_colour_mix_convert_pixel_to_8bit (
		io_colour_mix (base),io_colour_level (base)
	);

	io_pixel_t blend = def_io_pixel_8bit (
		((uint32_t) io_pixel_colour_red(p8) * (uint32_t) io_pixel_colour_red(b8)) / 255,
		((uint32_t) io_pixel_colour_green(p8) * (uint32_t) io_pixel_colour_green(b8)) / 255,
		((uint32_t) io_pixel_colour_blue(p8) * (uint32_t) io_pixel_colour_blue(b8)) / 255
	);

	io_colour_level (base) = io_colour_mix_convert_pixel_from_8bit (
		io_colour_mix (base),blend
	);

	return base;
}

io_pixel_t
io_colour_get_composite_level (io_colour_t c) {
	return io_colour_mix_make_pixel (
		io_colour_mix(c),
		io_colour_get_red_level(c),
		io_colour_get_green_level(c),
		io_colour_get_blue_level(c)
	);
}

bool
io_graphics_compare_float32_equal (float32_t a,float32_t b,float32_t epsilon) {
	float32_t diff = io_graphics_float_abs (a - b);
	if (a == b) {
		// shortcut, handles infinities
		return true;
	} else if (a == 0 || b == 0) {
		return diff < epsilon;
	} else {
		float32_t absA = io_graphics_float_abs (a);
		float32_t absB = io_graphics_float_abs (b);
		return (diff / (absA + absB)) < epsilon;
	}
}

int
io_graphics_compare_float32 (float32_t a,float32_t b,float32_t epsilon) {
	if (io_graphics_compare_float32_equal(a,b,epsilon)) {
		return 0;
	} else {
		return (a < b) ? -1 : 1;
	}
}

//
// command stack
//

io_graphics_command_stack_t*
mk_io_graphics_command_stack (io_byte_memory_t *bm,uint32_t grow) {
	io_graphics_command_stack_t *this = io_byte_memory_allocate (
		bm,sizeof(io_graphics_command_stack_t)
	);
	
	if (this) {
		this->bm = bm;
		this->grow = grow;
		this->commands = io_byte_memory_allocate (
			bm,sizeof(io_graphics_command_stack_t*) * grow
		);
		if (this->commands) {
			this->cursor = this->commands;
			this->end_of_allocation = this->commands + grow;
		} else {
			io_byte_memory_free(bm,this);
			this = NULL;
		}
	} 

	return this;
}

void
reset_io_graphics_command_stack (io_graphics_command_stack_t *this) {
	io_graphics_command_t **cursor = io_graphics_command_stack_begin(this);
	
	while (cursor < io_graphics_command_stack_end(this)) {
		free_io_graphics_command (this->bm,*cursor);
		cursor++;
	}

	this->cursor = this->commands;	
}

void
free_io_graphics_command_stack (io_graphics_command_stack_t *this) {
	reset_io_graphics_command_stack (this);
	io_byte_memory_free(this->bm,this->commands);
	io_byte_memory_free(this->bm,this);
}

bool
io_graphics_command_stack_append (
	io_graphics_command_stack_t *this,io_graphics_command_t *cmd
) {
	if (this->cursor == this->end_of_allocation) {
		uint32_t length = (this->end_of_allocation - this->commands);
		io_graphics_command_t **buf = io_byte_memory_reallocate (
			this->bm,this->commands,sizeof(io_graphics_command_stack_t*) * (length + this->grow)
		);
		if (buf) {
			this->commands = buf;
			this->cursor = this->commands + length;
			this->end_of_allocation = this->commands + length + this->grow;
		} else {
			return false;
		}
	}
	
	*(this->cursor)++ = cmd;
	
	return true;
}

//
// commands
//

typedef struct PACK_STRUCTURE io_graphics_line_command {
	IO_GRAPHICS_COMMAND_STRUCT_MEMBERS
	io_i32_point_t p1,p2;
} io_graphics_line_command_t;

static void
free_io_graphics_line_command (io_byte_memory_t *bm,io_graphics_command_t *cmd) {
	io_byte_memory_free (bm,cmd);
}

static void
io_graphics_command_line_run (io_graphics_command_t *cmd,io_graphics_context_t *gfx) {
	io_graphics_line_command_t *this = (io_graphics_line_command_t*) cmd;
	io_graphics_context_one_pixel_line (gfx,this->p1,this->p2);
}

static EVENT_DATA io_graphics_command_implementation_t 
io_graphics_command_line_implementation = {
	.free = free_io_graphics_line_command,
	.run = io_graphics_command_line_run,
};

bool
io_graphics_stack_append_line (
	io_graphics_command_stack_t *stack,io_i32_point_t p1,io_i32_point_t p2
) {
	io_graphics_line_command_t *line = io_byte_memory_allocate (
		io_graphics_command_stack_memory(stack),sizeof(io_graphics_line_command_t)
	);
	
	if (line) {
		line->implementation = &io_graphics_command_line_implementation;
		line->p1 = p1;
		line->p2 = p2;
		return io_graphics_command_stack_append (stack,(io_graphics_command_t*) line);
	} else {
		return false;
	}
}

typedef struct PACK_STRUCTURE io_graphics_text_command {
	IO_GRAPHICS_COMMAND_STRUCT_MEMBERS
	io_character_t *text;
	uint32_t length;
	io_i32_point_t at;
} io_graphics_text_command_t;

static void
free_io_graphics_text_command (io_byte_memory_t *bm,io_graphics_command_t *cmd) {
	io_graphics_text_command_t *this = (io_graphics_text_command_t*) cmd;
	io_byte_memory_free (bm,this->text);
	io_byte_memory_free (bm,cmd);
}

static void
io_graphics_command_text_run (io_graphics_command_t *cmd,io_graphics_context_t *gfx) {
	io_graphics_text_command_t *this = (io_graphics_text_command_t*) cmd;
	io_character_t *cursor = this->text;
	io_character_t *end = cursor + this->length;
	io_i32_point_t at = this->at;
	
	while (cursor < end) {
		at.x += io_graphics_context_draw_character_with_current_font (
			gfx,*cursor++,at
		);
		// text spacing from font...
		at.x += 1;
	}

}

static EVENT_DATA io_graphics_command_implementation_t 
io_graphics_command_text_implementation = {
	.free = free_io_graphics_text_command,
	.run = io_graphics_command_text_run,
};

bool
io_graphics_stack_append_text (
	io_graphics_command_stack_t *stack,io_character_t *text,uint32_t length,io_i32_point_t at
) {
	io_graphics_text_command_t *command = io_byte_memory_allocate (
		io_graphics_command_stack_memory(stack),sizeof(io_graphics_text_command_t)
	);
	
	if (command) {
		command->implementation = &io_graphics_command_text_implementation;
		command->length = length;
		command->at = at;
		command->text = io_byte_memory_allocate (
			io_graphics_command_stack_memory(stack),sizeof(io_character_t) * length
		);
		if (command->text) {
			memcpy (command->text,text,sizeof(io_character_t) * length);
			return io_graphics_command_stack_append (stack,(io_graphics_command_t*) command);
		} else {
			io_byte_memory_free (io_graphics_command_stack_memory(stack),command);
			return false;
		}
	} else {
		return false;
	}
}

typedef struct PACK_STRUCTURE io_graphics_circle_command {
	IO_GRAPHICS_COMMAND_STRUCT_MEMBERS
	uint32_t fill;
	uint32_t radius;
	io_i32_point_t center;
} io_graphics_circle_command_t;

static void
free_io_graphics_circle_command (io_byte_memory_t *bm,io_graphics_command_t *cmd) {
	io_byte_memory_free (bm,cmd);
}

static void
io_graphics_command_circle_run (io_graphics_command_t *cmd,io_graphics_context_t *gfx) {
	io_graphics_circle_command_t *this = (io_graphics_circle_command_t*) cmd;

	if (this->fill) {
		io_graphics_context_fill_circle (gfx,this->center,this->radius);
	} else {
		io_graphics_context_circle (gfx,this->center,this->radius);
	}
}

static EVENT_DATA io_graphics_command_implementation_t 
io_graphics_command_circle_implementation = {
	.free = free_io_graphics_circle_command,
	.run = io_graphics_command_circle_run,
};

bool
io_graphics_stack_append_circle (
	io_graphics_command_stack_t *stack,io_i32_point_t at,uint32_t radius,bool fill
) {
	io_graphics_circle_command_t *line = io_byte_memory_allocate (
		io_graphics_command_stack_memory(stack),sizeof(io_graphics_circle_command_t)
	);
	
	if (line) {
		line->implementation = &io_graphics_command_circle_implementation;
		line->center = at;
		line->radius = radius;
		line->fill = fill;
		return io_graphics_command_stack_append (stack,(io_graphics_command_t*) line);
	} else {
		return false;
	}
}

typedef struct PACK_STRUCTURE io_graphics_rectangle_command {
	IO_GRAPHICS_COMMAND_STRUCT_MEMBERS
	uint32_t fill;
	io_i32_point_t p1;
	io_i32_point_t p2;
} io_graphics_rectangle_command_t;


static void
free_io_graphics_rectangle_command (io_byte_memory_t *bm,io_graphics_command_t *cmd) {
	io_byte_memory_free (bm,cmd);
}

static void
io_graphics_command_rectangle_run (io_graphics_command_t *cmd,io_graphics_context_t *gfx) {
	io_graphics_rectangle_command_t *this = (io_graphics_rectangle_command_t*) cmd;

	if (this->fill) {
		io_graphics_context_draw_filled_rectangle_base (gfx,this->p1,this->p2);
	} else {
		io_graphics_context_draw_rectangle_base (gfx,this->p1,this->p2);
	}
}

static EVENT_DATA io_graphics_command_implementation_t 
io_graphics_command_rectangle_implementation = {
	.free = free_io_graphics_rectangle_command,
	.run = io_graphics_command_rectangle_run,
};

bool
io_graphics_stack_append_rectangle (
		io_graphics_command_stack_t *stack,io_i32_point_t p1,io_i32_point_t p2,bool fill
) {
	io_graphics_rectangle_command_t *line = io_byte_memory_allocate (
		io_graphics_command_stack_memory(stack),sizeof(io_graphics_rectangle_command_t)
	);
	
	if (line) {
		line->implementation = &io_graphics_command_rectangle_implementation;
		line->p1 = p1;
		line->p2 = p2;
		line->fill = fill;
		return io_graphics_command_stack_append (stack,(io_graphics_command_t*) line);
	} else {
		return false;
	}
}

//
// need stb tt
//
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u)  umm_malloc(u,x)
#define STBTT_free(x,u)    umm_free(u,x)
#define STBTT_SMALL_MEMORY
//
// we see arsserts caused by out-of-memory in rasterizer but it
// looks like stb_tt copes with these, perhaps at the cost of
// degraded bitmat quality
// 
#define STBTT_assert(x)
#include <stb_truetype.h>
#include <font/Proggy-Small.h>

#ifdef WITH_IO_GRAPHICS_FONT_ROBOTO_BOLD
#include <font/Roboto-Bold.h>
#endif

const io_ttf_data_t io_graphics_ttf_fonts[] = {
	{"ProggySmall",ProggySmall_ttf},
	#ifdef WITH_IO_GRAPHICS_FONT_ROBOTO_BOLD
	{"RobotoBold",Roboto_Bold_ttf},
	#endif
	{NULL}
};

typedef struct {
	IO_GRAPHICS_FONT_STRUCT_MEMBERS
	io_graphics_context_t *gfx;
	uint32_t pixel_height;
	stbtt_fontinfo stb;
} stbtt_io_graphics_font_t;

io_graphics_font_t*
mk_stbtt_io_graphics_font (
	io_graphics_context_t *gfx,const char *name,const unsigned char *data,int offset
) {
	stbtt_io_graphics_font_t *this = io_byte_memory_allocate (
		gfx->bm,sizeof(stbtt_io_graphics_font_t)
	);

	if (this) {
		extern EVENT_DATA io_graphics_font_implementation_t stbtt_io_font_implementation;
		this->implementation = &stbtt_io_font_implementation;
		this->gfx = gfx;
		this->name = name;
		this->pixel_height = 12;
		this->stb.userdata = gfx->bm;
		stbtt_InitFont (
			&this->stb,data,stbtt_GetFontOffsetForIndex(data,0)
		);
	}

	return (io_graphics_font_t*) this;
}

void
free_stbtt_font (io_graphics_font_t *font) {
	stbtt_io_graphics_font_t *this = (stbtt_io_graphics_font_t*) font;
	io_byte_memory_free (this->stb.userdata,font);
}

//
// the returned bitmap has been allocated in the gfx context's byte memory
// and must be freed by the caller
//
static io_character_bitmap_t
stbtt_font_get_codepoint_bitmap (
	io_graphics_font_t *font,int codepoint,int *width,int *height,int *xoff,int *yoff
) {
	stbtt_io_graphics_font_t *this = (stbtt_io_graphics_font_t*) font;
	unsigned char *bitmap;
	int ascent,baseline;
	float scale;

	scale = stbtt_ScaleForPixelHeight (
		&this->stb, io_graphics_font_get_pixel_height(font)
	);
	stbtt_GetFontVMetrics(&this->stb, &ascent,0,0);
	baseline = (int) (ascent*scale);

	bitmap = stbtt_GetCodepointBitmap (
		&this->stb,0,scale,codepoint,width,height,xoff,yoff
	);

	return (io_character_bitmap_t) {.bytes = bitmap,.baseline = baseline};
};

static uint32_t
stbtt_font_get_pixel_height (io_graphics_font_t *font) {
	stbtt_io_graphics_font_t *this = (stbtt_io_graphics_font_t*) font;
	return this->pixel_height;
}

static void
stbtt_font_set_pixel_height (io_graphics_font_t *font,uint32_t height) {
	stbtt_io_graphics_font_t *this = (stbtt_io_graphics_font_t*) font;
	this->pixel_height = height;
}

EVENT_DATA io_graphics_font_implementation_t stbtt_io_font_implementation = {
	.free = free_stbtt_font,
	.get_codepoint_bitmap = stbtt_font_get_codepoint_bitmap,
	.get_pixel_height = stbtt_font_get_pixel_height,
	.set_pixel_height = stbtt_font_set_pixel_height,
};

void
initialise_io_graphics_context (
	io_graphics_context_t *gfx,io_byte_memory_t *bm,io_ttf_data_t const *font_sources
) {
	gfx->bm = bm;
	gfx->ttf_font_data = font_sources;
	gfx->current_font = mk_stbtt_io_graphics_font (
		gfx,gfx->ttf_font_data->name,gfx->ttf_font_data->ttf,0
	);
}

io_ttf_data_t const*
io_graphics_context_find_font (
	io_graphics_context_t *gfx,uint8_t const *name,uint32_t name_length
) {
	io_ttf_data_t const *f = gfx->ttf_font_data;

	 while (f->name != NULL) {
		if (
				strlen (f->name) == name_length
			&&	strncmp(f->name,(const char*) name,name_length) == 0
		) {
			return f;
		}
		f++;
	};

	return NULL;
}

bool
io_graphics_context_select_ttf_font_by_name (
	io_graphics_context_t *gfx,uint8_t const *name,uint32_t name_length
) {
	io_ttf_data_t const *new_font = io_graphics_context_find_font (
		gfx,name,name_length
	);

	if (new_font) {
		uint32_t height = stbtt_font_get_pixel_height (gfx->current_font);

		free_io_graphics_font (gfx->current_font);
		gfx->current_font = mk_stbtt_io_graphics_font (
			gfx,new_font->name,new_font->ttf,0
		);

		stbtt_font_set_pixel_height (gfx->current_font,height);

		return true;
	} else {
		return false;
	}
}

void
io_graphics_context_fill_with_colour (io_graphics_context_t *gfx,io_colour_t colour) {
	uint32_t w = io_graphics_context_get_width_in_pixels(gfx);
	uint32_t h = io_graphics_context_get_height_in_pixels(gfx);
	uint32_t x,y;

	io_colour_t push = io_graphics_context_get_drawing_colour (gfx);

	io_graphics_context_set_colour (gfx,colour);

	for (x = 0; x < w; x++) {
		for (y = 0; y < h; y++) {
			io_i32_point_t at = {x,y};
			io_graphics_context_draw_pixel(gfx,at);
		}
	}

	io_graphics_context_set_colour (gfx,push);
}

int32_t
io_graphics_context_draw_character_with_current_font (
	io_graphics_context_t *gfx,io_character_t character,io_i32_point_t pt
) {
	io_character_bitmap_t bitmap;
	int width,height,i,j,xoff,yoff;
	io_graphics_font_t *font = io_graphics_context_current_font(gfx);

	io_colour_t colour = io_graphics_context_get_drawing_colour (gfx);

	bitmap = io_graphics_font_get_codepoint_bitmap (
		font,
		character,
		&width,
		&height,
		&xoff,
		&yoff
	);

	yoff += bitmap.baseline;
	pt.y += yoff;

	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			uint8_t c = bitmap.bytes[(j * width) + i];
			io_graphics_context_set_colour (
				gfx,scale_io_colour (colour,def_8bit_colour(c,c,c))
			);
			io_graphics_context_draw_pixel (gfx,(io_i32_point_t) {pt.x + i,pt.y + j});
		}
	}

	stbtt_FreeBitmap (bitmap.bytes,gfx->bm);
	io_graphics_context_set_drawing_colour (gfx,colour);

	return width;
}

void
io_graphics_context_draw_draw_ascii_text_base (
	io_graphics_context_t *gfx,char const *text,io_i32_point_t at
) {
	while (*text) {
		at.x += io_graphics_context_draw_character_with_current_font (
			gfx,*text++,at
		);
		// text spacing from font...
		at.x += 1;
	}
}

void
io_graphics_context_one_pixel_line (io_graphics_context_t *gfx,io_i32_point_t p1,io_i32_point_t p2) {
	float32_t x = p1.x;
	float32_t y = p1.y;
	float32_t dx = p2.x - x;
	float32_t dy = p2.y - y;
	float32_t xincr,yincr;
	uint32_t end,count;

	if (fabsf(dx) > fabsf(dy)) {
		end = round((dx < 0) ? -dx : dx) + 1;
		xincr = ((dx < 0) ? -1 : 1);
		yincr = dy/fabsf(dx);
	} else {
		end = round((dy < 0) ? -dy : dy) + 1;
		xincr = dx/fabsf(dy);
		yincr = ((dy < 0) ? -1 : 1);
	}

	count = 0;
	while(count < end) {
		//
		// whichever incr is not unity, we 'overshoot' with a 50% pixel in
		// that direction when the rounded value increments to next whole number
		//
		io_i32_point_t p = (io_i32_point_t) {round_float_to_int(x),round_float_to_int(y)};
		io_graphics_context_draw_pixel (gfx,p);

		x += xincr;
		y += yincr;
		count++;
	}
}

//
//
//
void
io_graphics_context_ellipse (io_graphics_context_t *gfx,io_i32_point_t p1,io_i32_point_t p2) {
	int posX =  (p1.x + p2.x)/2;
	int posY =  (p1.y + p2.y)/2;
	int a = (p2.x - p1.x)/2;
	int b = (p2.y - p1.y)/2;
	int dx = 0;
	int dy = b;
	int a2 = a*a;
	int b2 = b*b;
	int err = b2-(2*b-1)*a2;
	int e2;

	do {
		io_graphics_context_draw_pixel (gfx,def_i32_point(posX+dx,posY+dy));
		io_graphics_context_draw_pixel (gfx,def_i32_point(posX-dx,posY+dy));
		io_graphics_context_draw_pixel (gfx,def_i32_point(posX+dx,posY-dy));
		io_graphics_context_draw_pixel (gfx,def_i32_point(posX-dx,posY-dy));
		e2 = 2*err;
		if (e2 <  (2*dx+1)*b2) { dx++; err += (2*dx+1)*b2; }
		if (e2 > -(2*dy-1)*a2) { dy--; err -= (2*dy-1)*a2; }
	} while (dy >= 0);

	while (dx++ < a) { // erroneous termination in flat ellipses (b=1)
	   io_graphics_context_draw_pixel(gfx,def_i32_point(posX+dx,posY));
	   io_graphics_context_draw_pixel(gfx,def_i32_point(posX-dx,posY));
	}
}

void
io_graphics_context_circle (io_graphics_context_t *gfx,io_i32_point_t pt,int radius) {
	io_graphics_context_ellipse (
		gfx,
		def_i32_point(pt.x - radius,pt.y - radius),
		def_i32_point(pt.x + radius,pt.y + radius)
	);
}

void
io_graphics_context_fill_ellipse (io_graphics_context_t *gfx,io_i32_point_t p1,io_i32_point_t p2) {
	int32_t posX = (p1.x + p2.x)/2;
	int posY = (p1.y + p2.y)/2;
	int a = (p2.x - p1.x)/2;
	int b = (p2.y - p1.y)/2;
	int dx = 0;
	int dy = b;
	int a2 = a*a;
	int b2 = b*b;
	int err = b2-(2*b-1)*a2;
	int e2;

	do {
		io_graphics_context_fill_rectangle(gfx,def_i32_point(posX+dx,posY+dy),def_i32_point(posX-dx,posY+dy));
		io_graphics_context_fill_rectangle(gfx,def_i32_point(posX+dx,posY-dy),def_i32_point(posX-dx,posY-dy));
	   e2 = 2*err;
	   if (e2 <  (2*dx+1)*b2) { dx++; err += (2*dx+1)*b2; }
	   if (e2 > -(2*dy-1)*a2) { dy--; err -= (2*dy-1)*a2; }
	} while (dy >= 0);

	while (dx++ < a) {// erroneous termination in flat ellipses(b=1)
		io_graphics_context_fill_rectangle (
			gfx,def_i32_point(posX+dx,posY),def_i32_point(posX-dx,posY)
		);
	}
}

void
io_graphics_context_fill_circle (io_graphics_context_t *gfx,io_i32_point_t pt,int radius) {
	io_graphics_context_fill_ellipse (
		gfx,
		def_i32_point(pt.x - radius,pt.y - radius),
		def_i32_point(pt.x + radius,pt.y + radius)
	);
}

void
io_graphics_context_draw_rectangle_base (io_graphics_context_t *gfx,io_i32_point_t p1,io_i32_point_t p2) {
	io_graphics_context_fill_rectangle(gfx,def_i32_point(p1.x,p1.y),def_i32_point(p2.x,p1.y));
	io_graphics_context_fill_rectangle(gfx,def_i32_point(p2.x,p1.y),def_i32_point(p2.x,p2.y));
	io_graphics_context_fill_rectangle(gfx,def_i32_point(p1.x,p2.y),def_i32_point(p2.x,p2.y));
	io_graphics_context_fill_rectangle(gfx,def_i32_point(p1.x,p2.y),def_i32_point(p1.x,p1.y));
}

void
io_graphics_context_draw_filled_rectangle_base (io_graphics_context_t *gfx,io_i32_point_t p1,io_i32_point_t p2) {
	io_graphics_context_fill_rectangle (gfx,p1,p2);
}

#endif /* IMPLEMENT_IO_GRAPHICS */
#endif /* io_graphics_H_ */
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
