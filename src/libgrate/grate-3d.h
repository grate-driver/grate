#ifndef GRATE_LIBGRATE_3D_H
#define GRATE_LIBGRATE_3D_H 1

#include <stdint.h>

#define log2_size(s)		(31 - __builtin_clz(s))

struct host1x_bo;
struct cgc_shader;

struct grate_attribute {
	int position;
	const char *name;
};

struct grate_uniform {
	int position;
	const char *name;
};

struct grate_shader {
	struct cgc_shader *cgc;
	unsigned num_words;
	uint32_t *words;

	union {
		struct {
			unsigned alu_buf_size;
			unsigned pseq_inst_nb;
			unsigned pseq_to_dw_nb;
		};

		struct {
			unsigned used_tram_rows_nb;
			unsigned linker_inst_nb;
		};
	};
};

struct grate_program {
	struct grate_shader *vs;
	struct grate_shader *fs;
	struct grate_shader *linker;

	struct grate_attribute *attributes;
	unsigned num_attributes;
	uint32_t attributes_use_mask;

	struct grate_uniform *vs_uniforms;
	unsigned num_vs_uniforms;

	struct grate_uniform *fs_uniforms;
	unsigned num_fs_uniforms;

	uint32_t vs_constants[256 * 4];
	uint32_t fs_constants[32];
};

struct grate_render_target {
	struct host1x_pixelbuffer *pixbuf;
	bool dither_enabled;
};

struct grate_vtx_attribute {
	struct host1x_bo *bo;
	unsigned long stride;
	unsigned size;
	unsigned type;
};

struct grate_texture {
	struct host1x_pixelbuffer *pixbuf;
	struct host1x_pixelbuffer *mipmap_pixbuf;
	unsigned wrap_mode;
	unsigned max_lod;
	bool mag_filter_enabled;
	bool min_filter_enabled;
	bool mip_filter_enabled;
	bool mipmap_enabled;
};

struct grate_3d_ctx {
	uint32_t vs_uniforms[256 * 4];
	uint32_t fs_uniforms[32];

	struct grate *grate;
	struct grate_program *program;

	struct grate_render_target render_targets[16];
	struct grate_vtx_attribute *vtx_attributes[16];
	struct grate_texture *textures[16];

	float depth_range_near;
	float depth_range_far;
	float point_size;
	float point_coord_range_min_s;
	float point_coord_range_min_t;
	float point_coord_range_max_s;
	float point_coord_range_max_t;
	float line_width;
	float viewport_x_bias;
	float viewport_y_bias;
	float viewport_z_bias;
	float viewport_x_scale;
	float viewport_y_scale;
	float viewport_z_scale;
	float polygon_offset_units;
	float polygon_offset_factor;
	bool guarband_enabled;
	bool provoking_vtx_last;
	bool tri_face_front_cw;
	bool depth_test;
	bool depth_write;
	enum grate_3d_ctx_cull_face cull_face;
	enum grate_3d_ctx_depth_function depth_func;
	uint32_t dither_unk;
	uint32_t point_params;
	uint32_t line_params;
	uint16_t scissor_x;
	uint16_t scissor_y;
	uint16_t scissor_width;
	uint16_t scissor_heigth;

	uint16_t attributes_enable_mask;
	uint16_t render_targets_enable_mask;
};

#endif
