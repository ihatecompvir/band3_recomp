#include <rex/ppc/context.h>
#include <rex/ppc/function.h>
#include <cmath>

// native replacements for recompiled PPC math functions

// helpers for guest memory float access
static inline float lf(uint8_t* base, uint32_t addr) {
	PPCRegister t{};
	t.u32 = PPC_LOAD_U32(addr);
	return t.f32;
}

static inline void sf(uint8_t* base, uint32_t addr, float val) {
	PPCRegister t{};
	t.f32 = val;
	PPC_STORE_U32(addr, t.u32);
}

static void normalize_vec3(uint8_t* base, uint32_t src, uint32_t dst) {
	float x = lf(base, src);
	float y = lf(base, src + 0x04);
	float z = lf(base, src + 0x08);

	if (x == 0.0f && y == 0.0f && z == 0.0f) {
		sf(base, dst, 0.0f);
		sf(base, dst + 0x04, 0.0f);
		sf(base, dst + 0x08, 0.0f);
		return;
	}

	float inv = 1.0f / std::sqrt(x * x + y * y + z * z);
	sf(base, dst, x * inv);
	sf(base, dst + 0x04, y * inv);
	sf(base, dst + 0x08, z * inv);
}

// Matrix/vector math stuff
// TODO: add proper matrix/vector3 structures to make this look cleaner

// Normalize(Vector3* src, Vector3* dst)
extern "C" PPC_FUNC(Normalize_Vector3) {
	normalize_vec3(base, ctx.r3.u32, ctx.r4.u32);
}

// Normalize(Matrix3* src, Matrix3* dst)
extern "C" PPC_FUNC(Normalize_Matrix3) {
	uint32_t src = ctx.r3.u32;
	uint32_t dst = ctx.r4.u32;

	normalize_vec3(base, src + 0x10, dst + 0x10);

	float r1x = lf(base, dst + 0x10);
	float r1y = lf(base, dst + 0x14);
	float r1z = lf(base, dst + 0x18);
	float r2x = lf(base, src + 0x20);
	float r2y = lf(base, src + 0x24);
	float r2z = lf(base, src + 0x28);

	sf(base, dst + 0x00, r1y * r2z - r1z * r2y);
	sf(base, dst + 0x04, r1z * r2x - r1x * r2z);
	sf(base, dst + 0x08, r1x * r2y - r1y * r2x);
	normalize_vec3(base, dst, dst);

	float r0x = lf(base, dst);
	float r0y = lf(base, dst + 0x04);
	float r0z = lf(base, dst + 0x08);

	sf(base, dst + 0x20, r0y * r1z - r0z * r1y);
	sf(base, dst + 0x24, r0z * r1x - r0x * r1z);
	sf(base, dst + 0x28, r0x * r1y - r0y * r1x);
	normalize_vec3(base, dst + 0x20, dst + 0x20);
}

// Multiply(Matrix3* A, Matrix3* B, Matrix3* C)
extern "C" PPC_FUNC(Multiply_Matrix3) {
	uint32_t a_addr = ctx.r3.u32;
	uint32_t b_addr = ctx.r4.u32;
	uint32_t c_addr = ctx.r5.u32;

	float a[3][3], b[3][3];
	for (int i = 0; i < 3; i++) {
		uint32_t ar = a_addr + i * 0x10;
		uint32_t br = b_addr + i * 0x10;
		a[i][0] = lf(base, ar);       a[i][1] = lf(base, ar + 4);  a[i][2] = lf(base, ar + 8);
		b[i][0] = lf(base, br);       b[i][1] = lf(base, br + 4);  b[i][2] = lf(base, br + 8);
	}

	for (int i = 0; i < 3; i++) {
		uint32_t cr = c_addr + i * 0x10;
		sf(base, cr,     a[i][0]*b[0][0] + a[i][1]*b[1][0] + a[i][2]*b[2][0]);
		sf(base, cr + 4, a[i][0]*b[0][1] + a[i][1]*b[1][1] + a[i][2]*b[2][1]);
		sf(base, cr + 8, a[i][0]*b[0][2] + a[i][1]*b[1][2] + a[i][2]*b[2][2]);
	}
}

// Interp(Vector3* a, Vector3* b, float t, Vector3* dst)
extern "C" PPC_FUNC(Interp_Vector3) {
	uint32_t a = ctx.r3.u32;
	uint32_t b = ctx.r4.u32;
	float t = float(ctx.f1.f64);
	uint32_t dst = ctx.r6.u32;

	if (t == 0.0f) {
		PPC_STORE_U32(dst,      PPC_LOAD_U32(a));
		PPC_STORE_U32(dst + 4,  PPC_LOAD_U32(a + 4));
		PPC_STORE_U32(dst + 8,  PPC_LOAD_U32(a + 8));
		PPC_STORE_U32(dst + 12, PPC_LOAD_U32(a + 12));
		return;
	}

	if (t == 1.0f) {
		PPC_STORE_U32(dst,      PPC_LOAD_U32(b));
		PPC_STORE_U32(dst + 4,  PPC_LOAD_U32(b + 4));
		PPC_STORE_U32(dst + 8,  PPC_LOAD_U32(b + 8));
		PPC_STORE_U32(dst + 12, PPC_LOAD_U32(b + 12));
		return;
	}

	float ax = lf(base, a), ay = lf(base, a + 4), az = lf(base, a + 8);
	float bx = lf(base, b), by = lf(base, b + 4), bz = lf(base, b + 8);
	sf(base, dst,     (bx - ax) * t + ax);
	sf(base, dst + 4, (by - ay) * t + ay);
	sf(base, dst + 8, (bz - az) * t + az);
}

extern "C" PPC_FUNC(_cos) {
    ctx.f1.f64 = std::cos(ctx.f1.f64);
}

extern "C" PPC_FUNC(_tan) {
    ctx.f1.f64 = std::tan(ctx.f1.f64);
}

extern "C" PPC_FUNC(_floor) {
    ctx.f1.f64 = std::floor(ctx.f1.f64);
}

extern "C" PPC_FUNC(_fmod) {
    ctx.f1.f64 = std::fmod(ctx.f1.f64, ctx.f2.f64);
}

extern "C" PPC_FUNC(_asin) {
    ctx.f1.f64 = std::asin(ctx.f1.f64);
}

extern "C" PPC_FUNC(_acos) {
    ctx.f1.f64 = std::acos(ctx.f1.f64);
}

extern "C" PPC_FUNC(_atan) {
    ctx.f1.f64 = std::atan(ctx.f1.f64);
}

extern "C" PPC_FUNC(_pow) {
    ctx.f1.f64 = std::pow(ctx.f1.f64, ctx.f2.f64);
}

extern "C" PPC_FUNC(_atan2) {
    ctx.f1.f64 = std::atan2(ctx.f1.f64, ctx.f2.f64);
}
