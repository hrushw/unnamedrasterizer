#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t i32;
typedef int64_t i64;

typedef struct color_t {
	u8 r, g, b;
} Color;

typedef struct vec2_t {
	i32 x, y;
} Vec2;

typedef struct vec3b_t {
	u32 b0, b1, b2;
} Vec3B;

typedef struct rect_t {
	Vec2 r0, sz;
} Rect;

typedef struct triangle_t {
	Vec2 r0, r1, r2;
} Triangle;

typedef struct vec3_color_t {
	Color c0, c1, c2;
} Vec3Color;

typedef struct fbuf_t {
	Vec2 sz;
	u8	*buf;
} Fbuf;

void fb_set_pix(Fbuf *fb, Vec2 r, Color clr) {
	size_t index = 3*(r.y*fb->sz.x + r.x);
	fb->buf[index] = clr.r;
	fb->buf[index + 1] = clr.g;
	fb->buf[index + 2] = clr.b;
}

Color fb_get_pix(Fbuf *fb, Vec2 r) {
	size_t index = 3*(r.y*fb->sz.x + r.x);
	return (Color) { fb->buf[index], fb->buf[index+1], fb->buf[index+2] };
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

i32 square(i32 x) {
	return x*x;
}

void fb_draw_parabola(Fbuf *fb, Rect bound, Vec2 origin, int a, Color clr) {
	Vec2 r;
	for(r.y = bound.r0.y; r.y < bound.r0.y + bound.sz.y; ++r.y)
		for(r.x = bound.r0.x; r.x < bound.r0.x + bound.sz.x; ++r.x)
			if (a*(r.y - origin.y) > square(r.x - origin.x))
				fb_set_pix(fb, r, clr);
}

i32 det(Vec2 v0, Vec2 v1) {
	return v0.x*v1.y - v1.x*v0.y;
}

Vec2 vec2sub(Vec2 r0, Vec2 r1) {
	return (Vec2) {r0.x - r1.x, r0.y - r1.y};
}

int i32cmpsign(i32 x, i32 y) {
	return (x > 0 && y < 0) || (x < 0 && y > 0);
}

int i32modcmp(i32 x, i32 y) {
	return x > 0 ? x > y : x < y;
}

int barycentric_coords(Vec3B *B, Vec2 dr, Vec2 dr1, Vec2 dr2, i32 D) {
	i32 D1 = det(dr1, dr);
	i32 D2 = det(dr, dr2);

	if (i32cmpsign(D, D1) || i32cmpsign(D, D2)) return -1;
	if (i32modcmp(D1, D) || i32modcmp(D2, D)) return -2;
	B->b1 = ((i64)D1 * (i64)0xFFFFFFFF) / (i64)D;
	B->b2 = ((i64)D2 * (i64)0xFFFFFFFF) / (i64)D;
	if(B->b1 + B->b2 < B->b1) return -1;
	B->b0 = 0xFFFFFFFF - B->b1 - B->b2;
	return 0;
}

u8 u8lerp(Vec3B B, u8 x0, u8 x1, u8 x2) {
	return (
		(u64)x0*(u64)B.b0 +
		(u64)x1*(u64)B.b1 +
		(u64)x2*(u64)B.b2
	) / (u64)0xFFFFFFFF;
}

Color lerp(Vec3B B, Vec3Color colors) {
	return (Color) {
		u8lerp(B, colors.c0.r, colors.c1.r, colors.c2.r),
		u8lerp(B, colors.c0.g, colors.c1.g, colors.c2.g),
		u8lerp(B, colors.c0.b, colors.c1.b, colors.c2.b)
	};
}

void fb_draw_triangle(Fbuf *fb, Rect bound, Triangle S, Vec3Color colors) {
	Vec2 dr1 = vec2sub(S.r1, S.r0);
	Vec2 dr2 = vec2sub(S.r2, S.r0);
	i32 D = det(dr1, dr2);
	Vec2 r, dr;
	Vec3B B;
	for(
		r.y = bound.r0.y, dr.y = r.y - S.r0.y;
		r.y < bound.r0.y + bound.sz.y;
		++r.y, ++dr.y
	)
		for(
			r.x = bound.r0.x, dr.x = r.x - S.r0.x;
			r.x < bound.r0.x + bound.sz.x;
			++r.x, ++dr.x
		)
			if(!barycentric_coords(&B, dr, dr1, dr2, D))
				fb_set_pix(fb, r, lerp(B, colors));
}

void ximgsetpix(Vec2 r, Color clr, XImage *img) {
	size_t i = 4*(r.y*img->width + r.x);
	img->data[i  ] = clr.b;
	img->data[i+1] = clr.g;
	img->data[i+2] = clr.r;
}

void fbtoximg(Fbuf *fb, XImage *img) {
	Vec2 r;
	for(r.y = 0; r.y < fb->sz.y; ++r.y)
		for(r.x = 0; r.x < fb->sz.x; ++r.x)
			ximgsetpix(r, fb_get_pix(fb, r), img);
}

enum { WIDTH = 640 };
enum { HEIGHT = 480 };

u8 fbufdata[HEIGHT*WIDTH*3] = {0};

void draw(Fbuf *fb) {
	Rect EyeLeft = {
		{ 20, 100 },
		{ 160, 100 },
	};
	Color EyeClr = {0x00, 0xFF, 0x00};
	fb_draw_rect_mirrored_x(fb, EyeLeft, EyeClr);

	Rect SmileBound = {
		{0, HEIGHT/2},
		{WIDTH, HEIGHT/3}
	};

	Vec2 SmileOrigin = {WIDTH/2, 5*HEIGHT/6};
	Color SmileClr = {0xFF, 0, 0};
	fb_draw_parabola(fb, SmileBound, SmileOrigin, -256, SmileClr);

	Rect FbBound = {
		{0, 0},
		{WIDTH, HEIGHT}
	};
	Triangle Nose = {
		{WIDTH/2, 160},
		{(WIDTH/2) - 60, 210},
		{(WIDTH/2) + 60, 210},
	};
	Vec3Color NoseColors = {
		{0xFF, 0xFF, 0x00},
		{0xFF, 0x7F, 0x3F},
		{0x7F, 0xFF, 0x3F},
	};
	fb_draw_triangle(fb, FbBound, Nose, NoseColors);
}

void render_to_ppm(Fbuf *fb) {
	const char* fname = "gaem.ppm";
	FILE* f = fopen(fname, "wb");
	fprintf(f, "P6\n%d %d 255\n", WIDTH, HEIGHT);
	fwrite(fb->buf, 1, WIDTH*HEIGHT*3, f);
}

int render_to_x(Fbuf *fb) {
	Display *disp = XOpenDisplay(NULL);
	Window root = DefaultRootWindow(disp);
	int scrnu = DefaultScreen(disp);
	unsigned long blackpix = BlackPixel(disp, scrnu);
	GC gc = DefaultGC(disp, scrnu);

	XVisualInfo vinfo = {0};
	if(!XMatchVisualInfo(disp, scrnu, 24, TrueColor, &vinfo))
		return fprintf(stderr, "Unable to get proper visual!\n"), -1;

	XSetWindowAttributes xswattrs = {0};
	xswattrs.background_pixel = blackpix;
	xswattrs.event_mask = StructureNotifyMask | ExposureMask;

	Window win = XCreateWindow(
		disp, root,
		0, 0, WIDTH, HEIGHT,
		0, 24, InputOutput,
		vinfo.visual /*change*/, CWEventMask | CWBackPixel, &xswattrs
	);
	XMapWindow(disp, win);
	XSync(disp, False);

	XWindowAttributes attrs = {0};
	XGetWindowAttributes(disp, win, &attrs);

	uint8_t* buf = calloc(4*fb->sz.y*fb->sz.x, 1);
	XImage *img = XCreateImage(disp, attrs.visual, attrs.depth, ZPixmap, 0, (char*)buf, WIDTH, HEIGHT, 32, 0);
	fbtoximg(fb, img);
	XInitImage(img);
	if(
		img->bits_per_pixel != 32
		|| img->red_mask != 0xFF0000
		|| img->blue_mask != 0xFF
		|| img->green_mask != 0xFF00
	)
		return fprintf(stderr, "Invalid image format!\n"), -2;

	XEvent ev;
	while(1) {
		XNextEvent(disp, &ev);
		if(ev.type == DestroyNotify)
			break;

		if(ev.type == Expose)
			XPutImage(disp, win, gc, img, 0, 0, 0, 0, WIDTH, HEIGHT);
	}

	free(buf);
	XCloseDisplay(disp);
	return 0;
}

int main() {
	Fbuf fb = { {WIDTH, HEIGHT}, fbufdata };
	draw(&fb);
	render_to_ppm(&fb);
	if(render_to_x(&fb)) return -1;

	return 0;
}
