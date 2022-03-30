#ifndef Math3D_h
#define Math3D_h

#include <math.h>

// vec2 operations
typedef struct vec2 { float x, y; } vec2_t;

static inline vec2_t vec2_add(vec2_t a, vec2_t b) { return (vec2_t){ a.x + b.x, a.y + b.y }; }
static inline vec2_t vec2_sub(vec2_t a, vec2_t b) { return (vec2_t){ a.x - b.x, a.y - b.y }; }
static inline vec2_t vec2_mul(vec2_t a, vec2_t b) { return (vec2_t){ a.x * b.x, a.y * b.y }; }
static inline vec2_t vec2_div(vec2_t a, vec2_t b) { return (vec2_t){ a.x / b.x, a.y / b.y }; }
static inline vec2_t vec2_neg(vec2_t a) { return (vec2_t){ -a.x, -a.y }; }

static inline vec2_t vec2_addf(vec2_t a, float b) { return (vec2_t){ a.x + b, a.y + b }; }
static inline vec2_t vec2_subf(vec2_t a, float b) { return (vec2_t){ a.x - b, a.y - b }; }
static inline vec2_t vec2_mulf(vec2_t a, float b) { return (vec2_t){ a.x * b, a.y * b }; }
static inline vec2_t vec2_divf(vec2_t a, float b) { return (vec2_t){ a.x / b, a.y / b }; }

static inline float vec2_dot(vec2_t a, vec2_t b) { return a.x * b.x + a.y * b.y; }
static inline float vec2_len(vec2_t a) { return sqrtf(vec2_dot(a, a)); }
static inline float vec2_dist(vec2_t a, vec2_t b) { return vec2_len(vec2_sub(b, a)); }
static inline vec2_t vec2_normalize(vec2_t a) { return vec2_divf(a, vec2_len(a)); }

// vec3 operations
typedef struct vec3 { float x, y, z; } vec3_t;

static inline vec3_t vec3_add(vec3_t a, vec3_t b) { return (vec3_t){ a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline vec3_t vec3_sub(vec3_t a, vec3_t b) { return (vec3_t){ a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline vec3_t vec3_mul(vec3_t a, vec3_t b) { return (vec3_t){ a.x * b.x, a.y * b.y, a.z * b.z }; }
static inline vec3_t vec3_div(vec3_t a, vec3_t b) { return (vec3_t){ a.x / b.x, a.y / b.y, a.z / b.z }; }
static inline vec3_t vec3_neg(vec3_t a) { return (vec3_t){ -a.x, -a.y, -a.z }; }

static inline vec3_t vec3_addf(vec3_t a, float b) { return (vec3_t){ a.x + b, a.y + b, a.z + b }; }
static inline vec3_t vec3_subf(vec3_t a, float b) { return (vec3_t){ a.x - b, a.y - b, a.z - b }; }
static inline vec3_t vec3_mulf(vec3_t a, float b) { return (vec3_t){ a.x * b, a.y * b, a.z * b }; }
static inline vec3_t vec3_divf(vec3_t a, float b) { return (vec3_t){ a.x / b, a.y / b, a.z / b }; }

static inline float vec3_dot(vec3_t a, vec3_t b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float vec3_len(vec3_t a) { return sqrtf(vec3_dot(a, a)); }
static inline float vec3_dist(vec3_t a, vec3_t b) { return vec3_len(vec3_sub(b, a)); }
static inline vec3_t vec3_normalize(vec3_t a) { return vec3_divf(a, vec3_len(a)); }
static inline vec3_t vec3_cross(vec3_t a, vec3_t b) { return (vec3_t){ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }

// vec4 operations
typedef struct vec4 { float x, y, z, w; } vec4_t;

static inline vec4_t vec4_add(vec4_t a, vec4_t b) { return (vec4_t){ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
static inline vec4_t vec4_sub(vec4_t a, vec4_t b) { return (vec4_t){ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
static inline vec4_t vec4_mul(vec4_t a, vec4_t b) { return (vec4_t){ a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w }; }
static inline vec4_t vec4_div(vec4_t a, vec4_t b) { return (vec4_t){ a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w }; }
static inline vec4_t vec4_neg(vec4_t a) { return (vec4_t){ -a.x, -a.y, -a.z, -a.w }; }

static inline vec4_t vec4_addf(vec4_t a, float b) { return (vec4_t){ a.x + b, a.y + b, a.z + b, a.w + b }; }
static inline vec4_t vec4_subf(vec4_t a, float b) { return (vec4_t){ a.x - b, a.y - b, a.z - b, a.w - b }; }
static inline vec4_t vec4_mulf(vec4_t a, float b) { return (vec4_t){ a.x * b, a.y * b, a.z * b, a.w * b }; }
static inline vec4_t vec4_divf(vec4_t a, float b) { return (vec4_t){ a.x / b, a.y / b, a.z / b, a.w / b }; }

static inline float vec4_dot(vec4_t a, vec4_t b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
static inline float vec4_len(vec4_t a) { return sqrtf(vec4_dot(a, a)); }
static inline float vec4_dist(vec4_t a, vec4_t b) { return vec4_len(vec4_sub(b, a)); }
static inline vec4_t vec4_normalize(vec4_t a) { return vec4_divf(a, vec4_len(a)); }

// vec casting (z -> 0.0f, w -> 1.0f for matrix mult purposes)
static inline vec3_t vec2_to_vec3(vec2_t a) { return (vec3_t){ a.x, a.y, 0.0f }; }
static inline vec4_t vec2_to_vec4(vec2_t a) { return (vec4_t){ a.x, a.y, 0.0f, 1.0f }; }
static inline vec2_t vec3_to_vec2(vec3_t a) { return (vec2_t){ a.x, a.y }; }
static inline vec4_t vec3_to_vec4(vec3_t a) { return (vec4_t){ a.x, a.y, a.z, 1.0f }; }
static inline vec2_t vec4_to_vec2(vec4_t a) { return (vec2_t){ a.x, a.y }; }
static inline vec3_t vec4_to_vec3(vec4_t a) { return (vec3_t){ a.x, a.y, a.z }; }

// vec constants
#define vec2_one     ((vec2_t){  1.0f,  1.0f })
#define vec2_up      ((vec2_t){  0.0f,  1.0f })
#define vec2_down    ((vec2_t){  0.0f, -1.0f })
#define vec2_right   ((vec2_t){  1.0f,  0.0f })
#define vec2_left    ((vec2_t){ -1.0f,  0.0f })

#define vec3_one     ((vec3_t){  1.0f,  1.0f,  1.0f })
#define vec3_up      ((vec3_t){  0.0f,  1.0f,  0.0f })
#define vec3_down    ((vec3_t){  0.0f, -1.0f,  0.0f })
#define vec3_right   ((vec3_t){  1.0f,  0.0f,  0.0f })
#define vec3_left    ((vec3_t){ -1.0f,  0.0f,  0.0f })
#define vec3_forward ((vec3_t){  0.0f,  0.0f,  1.0f })
#define vec3_back    ((vec3_t){  0.0f,  0.0f, -1.0f })

#define vec4_one     ((vec4_t){  1.0f,  1.0f,  1.0f,  1.0f })
#define vec4_up      ((vec4_t){  0.0f,  1.0f,  0.0f,  0.0f })
#define vec4_down    ((vec4_t){  0.0f, -1.0f,  0.0f,  0.0f })
#define vec4_right   ((vec4_t){  1.0f,  0.0f,  0.0f,  0.0f })
#define vec4_left    ((vec4_t){ -1.0f,  0.0f,  0.0f,  0.0f })
#define vec4_forward ((vec4_t){  0.0f,  0.0f,  1.0f,  0.0f })
#define vec4_back    ((vec4_t){  0.0f,  0.0f, -1.0f,  0.0f })
#define vec4_w       ((vec4_t){  0.0f,  0.0f,  0.0f,  1.0f })
#define vec4_nw      ((vec4_t){  0.0f,  0.0f,  0.0f, -1.0f })

// mat4 operations
typedef struct mat4 { vec4_t a, b, c, d; } mat4_t;

static inline mat4_t mat4_add(mat4_t m, mat4_t n)
{
	return (mat4_t)
	{
		vec4_add(m.a, n.a),
		vec4_add(m.b, n.b),
		vec4_add(m.c, n.c),
		vec4_add(m.d, n.d),
	};
}

static inline mat4_t mat4_neg(mat4_t m) { return (mat4_t){ vec4_neg(m.a), vec4_neg(m.b), vec4_neg(m.c), vec4_neg(m.d) }; }

static inline vec4_t mat4_mulv(mat4_t m, vec4_t v)
{
	return (vec4_t)
	{
		vec4_dot((vec4_t){ m.a.x, m.b.x, m.c.x, m.d.x }, v),
		vec4_dot((vec4_t){ m.a.y, m.b.y, m.c.y, m.d.y }, v),
		vec4_dot((vec4_t){ m.a.z, m.b.z, m.c.z, m.d.z }, v),
		vec4_dot((vec4_t){ m.a.w, m.b.w, m.c.w, m.d.w }, v),
	};
}

static inline mat4_t mat4_mul(mat4_t m, mat4_t n)
{
	return (mat4_t)
	{
		mat4_mulv(m, n.a),
		mat4_mulv(m, n.b),
		mat4_mulv(m, n.c),
		mat4_mulv(m, n.d),
	};
}

// mat4 transformations
static inline mat4_t mat4_translate(mat4_t m, vec3_t v)
{
	mat4_t n = (mat4_t)
	{
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f, 0.0f },
		{ v.x,  v.y,  v.z,  1.0f },
	};
	return mat4_mul(n, m);
}

static inline mat4_t mat4_scale(mat4_t m, vec3_t v)
{
	mat4_t n = (mat4_t)
	{
		{ v.x,  0.0f, 0.0f, 0.0f },
		{ 0.0f, v.y,  0.0f, 0.0f },
		{ 0.0f, 0.0f, v.z,  0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f },
	};
	return mat4_mul(n, m);
}

static inline mat4_t mat4_rotate_x(mat4_t m, float a)
{
	float c = cosf(a), s = sinf(a);
	mat4_t n = (mat4_t)
	{
		{ 1.0f,  0.0f,  0.0f, 0.0f },
		{ 0.0f,  c,    -s,    0.0f },
		{ 0.0f, -s,     c,    0.0f },
		{ 0.0f,  0.0f,  0.0f, 1.0f },
	};
	return mat4_mul(n, m);
};

static inline mat4_t mat4_rotate_y(mat4_t m, float a)
{
	float c = cosf(a), s = sinf(a);
	mat4_t n = (mat4_t)
	{
		{ c,    0.0f, -s,    0.0f },
		{ 0.0f, 1.0f,  0.0f, 0.0f },
		{ s,    0.0f,  c,    0.0f },
		{ 0.0f, 0.0f,  0.0f, 1.0f },
	};
	return mat4_mul(n, m);
}

static inline mat4_t mat4_rotate_z(mat4_t m, float a)
{
	float c = cosf(a), s = sinf(a);
	mat4_t n = (mat4_t)
	{
		{  c,    s,    0.0f, 0.0f },
		{ -s,    c,    0.0f, 0.0f },
		{  0.0f, 0.0f, 1.0f, 0.0f },
		{  0.0f, 0.0f, 0.0f, 1.0f },
	};
	return mat4_mul(n, m);
}

static inline mat4_t mat4_rotate_euler(mat4_t m, vec3_t v)
{
	return mat4_rotate_x(mat4_rotate_y(mat4_rotate_z(m, v.z), v.y), v.x);
}

static inline mat4_t mat4_rotate_axis(mat4_t m, vec3_t v, float a)
{
	float c = cosf(a), s = sinf(a);
	float C = 1.0f - c;
	mat4_t n = (mat4_t)
	{
		{ c + v.x * v.x * C, v.y * v.x * C + v.z * s, v.z * v.x * C - v.y * s, 0.0f },
		{ v.x * v.y * C - v.z * s, c + v.y * v.y * C, v.z * v.y * C + v.x * s, 0.0f },
		{ v.x * v.z * C + v.y * s, v.y * v.z * C - v.x * s, c + v.z * v.z * C, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f },
	};
	return mat4_mul(n, m);
}

static inline mat4_t mat4_lookat(vec3_t pos, vec3_t at, vec3_t up)
{
	vec3_t f = vec3_normalize(vec3_sub(at, pos));
	vec3_t r = vec3_normalize(vec3_cross(up, f));
	vec3_t u = vec3_normalize(vec3_cross(f, r));
	return (mat4_t)
	{
		{ r.x, r.y, r.z, -vec3_dot(r, pos) },
		{ u.x, u.y, u.z, -vec3_dot(u, pos) },
		{ f.x, f.y, f.z, -vec3_dot(f, pos) },
		{ 0.0f, 0.0f, 0.0f, 1.0f },
	};
}

#define mat4_identity ((mat4_t){{1.0f,0.0f,0.0f,0.0f},{0.0f,1.0f,0.0f,0.0f},{0.0f,0.0f,1.0f,0.0f},{0.0f,0.0f,0.0f,1.0f}})

#endif
