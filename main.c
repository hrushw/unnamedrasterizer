#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef struct color_t {
	uint8_t r, g, b;
} Color;

typedef struct vec2_t {
	uint32_t x, y;
} Vec2;

typedef struct rect_t {
	Vec2 r0, sz;
} Rect;

typedef struct fbuf_t {
	Vec2 sz;
	uint8_t	*buf;
} Fbuf;

void fb_set_pix(Fbuf *fb, Vec2 r, Color clr) {
	size_t index = 3*(r.y*fb->sz.x + r.x);
	fb->buf[index] = clr.r;
	fb->buf[index + 1] = clr.g;
	fb->buf[index + 2] = clr.b;
}

void fb_draw_rect(Fbuf *fb, Rect S, Color clr) {
	Vec2 r;
	for(r.y = S.r0.y; r.y < S.r0.y + S.sz.y; ++r.y)
		for(r.x = S.r0.x; r.x < S.r0.x + S.sz.x; ++r.x)
			fb_set_pix(fb, r, clr);
}

// This function is Hermetian
void fb_mirror_rect_x(Fbuf *fb, Rect *rect) {
	rect->r0.x = fb->sz.x - rect->r0.x - rect->sz.x;
}

void fb_draw_rect_mirrored_x(Fbuf *fb, Rect rect, Color clr) {
	fb_draw_rect(fb, rect, clr);
	fb_mirror_rect_x(fb, &rect);
	fb_draw_rect(fb, rect, clr);
}

uint32_t square(uint32_t x) {
	return x*x;
}

void fb_draw_parabola(Fbuf *fb, Rect bound, Vec2 origin, uint32_t a, Color clr) {
	Vec2 r;
	for(r.y = bound.r0.y; r.y < bound.r0.y + bound.sz.y; ++r.y)
		for(r.x = bound.r0.x; r.x < bound.r0.x + bound.sz.x; ++r.x)
			if (a*(r.y - origin.y) > square(r.x - origin.x))
				fb_set_pix(fb, r, clr);
}


uint32_t det(Vec2 v0, Vec2 v1){
	return v0.x*v1.y - v0.y*v1.x; 
}


enum { WIDTH = 640 };
enum { HEIGHT = 480 };

uint8_t fbufdata[HEIGHT*WIDTH*3] = {0};
const char* fname = "gaem.ppm";

int main() {
	Fbuf fb = { {WIDTH, HEIGHT}, fbufdata };
	FILE* f = fopen(fname, "wb");
	fprintf(f, "P6\n%d %d 255\n", WIDTH, HEIGHT);

	Rect EyeLeft = {
		{ 20, 100 },
		{ 160, 100 },
	};
	Color EyeClr = {0, 0xFF, 0};
	fb_draw_rect_mirrored_x(&fb, EyeLeft, EyeClr);

	Rect SmileBound = {
		{0, HEIGHT/2},
		{WIDTH, HEIGHT/3}
	};
	Vec2 SmileOrigin = {WIDTH/2, 5*HEIGHT/6};
	Color SmileClr = {0xFF, 0, 0};
	fb_draw_parabola(&fb, SmileBound, SmileOrigin, -256, SmileClr);

	fwrite(fb.buf, 1, WIDTH*HEIGHT*3, f);

	return 0;
}
