#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// TODO win32

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t i32;
typedef int64_t i64;

typedef uint32_t Pixel;

// TODO signed integers are a CPU bug and should not be used :P
typedef struct vec2_t {
	i32 x, y;
} Vec2;

typedef struct vec3b_t {
	u32 b0, b1, b2;
} Vec3B;

typedef struct rect_t {
	Vec2 r0;
	Vec2 sz;
} Rect;

typedef struct triangle_t {
	Vec2 r0, r1, r2;
} Triangle;

typedef struct vec3_pixel_t {
	Pixel p0, p1, p2;
} Vec3Pixel;

typedef struct fbuf_t {
	Vec2 	sz;
	Pixel *buf;
} Fbuf;


// get pixel alpha value
u8 pixA(Pixel p) {
	return (p >> 24) & 0xFF;
}

// get pixel red value
u8 pixR(Pixel p) {
	return (p >> 16) & 0xFF;
}

// get pixel green value
u8 pixG(Pixel p) {
	return (p >> 8) & 0xFF;
}

// get pixel blue value
u8 pixB(Pixel p) {
	return p & 0xFF;
}

Pixel argb_to_pixel(u8 a, u8 r, u8 g, u8 b) {
	return
		  ((Pixel)a << 24)
		| ((Pixel)r << 16)
		| ((Pixel)g <<  8)
		|  (Pixel)b
	;
}

void fb_set_pix(Fbuf *fb, Vec2 r, Pixel p) {
	fb->buf[r.y*fb->sz.x + r.x] = p;
}

Pixel fb_get_pix(Fbuf *fb, Vec2 r) {
	return fb->buf[r.y*fb->sz.x + r.x];
}

void fb_draw_rect(Fbuf *fb, Rect S, Pixel p) {
	Vec2 r;
	for(r.y = S.r0.y; r.y < S.r0.y + S.sz.y; ++r.y)
		for(r.x = S.r0.x; r.x < S.r0.x + S.sz.x; ++r.x)
			fb_set_pix(fb, r, p);
}

void fb_mirror_rect_x(Fbuf *fb, Rect *rect) {
	rect->r0.x = fb->sz.x - rect->r0.x - rect->sz.x;
}

void fb_draw_rect_mirrored_x(Fbuf *fb, Rect rect, Pixel p) {
	fb_draw_rect(fb, rect, p);
	fb_mirror_rect_x(fb, &rect);
	fb_draw_rect(fb, rect, p);
}

i32 square(i32 x) {
	return x*x;
}

void fb_draw_parabola(Fbuf *fb, Rect bound, Vec2 origin, int a, Pixel p) {
	Vec2 r;
	for(r.y = bound.r0.y; r.y < bound.r0.y + bound.sz.y; ++r.y)
		for(r.x = bound.r0.x; r.x < bound.r0.x + bound.sz.x; ++r.x)
			if (a*(r.y - origin.y) > square(r.x - origin.x))
				fb_set_pix(fb, r, p);
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

	if (
		   i32cmpsign(D, D1)
		|| i32cmpsign(D, D2)
		|| i32modcmp(D1, D)
		|| i32modcmp(D2, D)
	) return -1;

	B->b1 = ((i64)D1 * (i64)0xFFFFFFFF) / (i64)D;
	B->b2 = ((i64)D2 * (i64)0xFFFFFFFF) / (i64)D;

	if(B->b1 + B->b2 < B->b1) return -2;
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

Pixel lerp(Vec3B B, Vec3Pixel P) {
	return argb_to_pixel(
		u8lerp(B, pixA(P.p0), pixA(P.p1), pixA(P.p2)),
		u8lerp(B, pixR(P.p0), pixR(P.p1), pixR(P.p2)),
		u8lerp(B, pixG(P.p0), pixG(P.p1), pixG(P.p2)),
		u8lerp(B, pixB(P.p0), pixB(P.p1), pixB(P.p2))
	);
}

// TODO faster function for monochrome triangles?
void fb_draw_triangle(Fbuf *fb, Rect bound, Triangle S, Vec3Pixel P) {
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
				fb_set_pix(fb, r, lerp(B, P));
}

void ximgsetpix(XImage *img, size_t i, Pixel p) {
	img->data[i  ] = pixB(p);
	img->data[i+1] = pixG(p);
	img->data[i+2] = pixR(p);
	img->data[i+3] = pixA(p);
}

void fbtoximg(Fbuf *fb, XImage *img) {
	Vec2 r;
	for(r.y = 0; r.y < fb->sz.y; ++r.y)
		for(r.x = 0; r.x < fb->sz.x; ++r.x)
			ximgsetpix(
				img,
				4*(r.y*img->width + r.x),
				fb_get_pix(fb, r)
			);
}

// TODO drawing independent of framebuffer size
void draw(Fbuf *fb) {
	Rect EyeLeft = {
		{ 20, 100 },
		{ 160, 100 },
	};
	Pixel EyeClr = 0x00FF00;
	fb_draw_rect_mirrored_x(fb, EyeLeft, EyeClr);

	Rect SmileBound = {
		{0, fb->sz.y/2},
		{fb->sz.x, fb->sz.y/3}
	};

	Vec2 SmileOrigin = {fb->sz.x/2, 5*fb->sz.y/6};
	Pixel SmileClr = 0xFF0000;
	fb_draw_parabola(fb, SmileBound, SmileOrigin, -256, SmileClr);

	Rect FbBound = {
		{0, 0},
		fb->sz
	};
	Triangle Nose = {
		{fb->sz.x/2, 160},
		{(fb->sz.x/2) - 60, 210},
		{(fb->sz.x/2) + 60, 210},
	};
	Vec3Pixel NoseColors = {
		0xFFFF00,
		0xFF7F3F,
		0x7FFF3F,
	};
	fb_draw_triangle(fb, FbBound, Nose, NoseColors);
}

void fb_to_ppm(FILE *f, Fbuf *fb) {
	Pixel p;
	uint8_t pixbuf[3];
	for(i32 i = 0; i < fb->sz.x*fb->sz.y; ++i) {
		p = fb->buf[i];
		pixbuf[0] = pixR(p);
		pixbuf[1] = pixG(p);
		pixbuf[2] = pixB(p);
		fwrite(pixbuf, 3, 1, f);
	}
}

void render_to_ppm(Fbuf *fb) {
	const char* fname = "gaem.ppm";
	FILE* f = fopen(fname, "wb");
	fprintf(f, "P6\n%d %d 255\n", fb->sz.x, fb->sz.y);
	fb_to_ppm(f, fb);
}

Window crwin_X(
	Display *disp, Visual *vis,
	unsigned long width, unsigned long height
) {
	XSetWindowAttributes xswattrs = {
		.background_pixel = BlackPixel(disp, DefaultScreen(disp)),
		.event_mask = StructureNotifyMask | ExposureMask,
	};

	Window win = XCreateWindow(
		disp, DefaultRootWindow(disp),
		0, 0, width, height,
		0, 24, InputOutput,
		vis, CWEventMask | CWBackPixel, &xswattrs
	);
	XMapWindow(disp, win);

	return win;
}

int render_to_x(Fbuf *fb) {
	Display *disp = XOpenDisplay(NULL);

	XVisualInfo vinfo = {0};
	if(!XMatchVisualInfo(disp, DefaultScreen(disp), 24, TrueColor, &vinfo))
		return fprintf(stderr, "Unable to get proper visual!\n"), -1;

	Window win = crwin_X(disp, vinfo.visual, fb->sz.x, fb->sz.y);

	XWindowAttributes attrs = {0};
	XGetWindowAttributes(disp, win, &attrs);

	uint32_t* buf = calloc(fb->sz.y*fb->sz.x, 4);
	XImage *img = XCreateImage(disp, attrs.visual, attrs.depth, ZPixmap, 0, (char*)buf, fb->sz.x, fb->sz.y, 32, 0);
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
		// TODO nonblocking event handling
		XNextEvent(disp, &ev);
		if(ev.type == DestroyNotify)
			break;

		if(ev.type == Expose)
			XPutImage(disp, win, DefaultGC(disp, DefaultScreen(disp)), img, 0, 0, 0, 0, fb->sz.x, fb->sz.y);

		// TODO keyboard handling
	}

	free(buf);
	XCloseDisplay(disp);
	return 0;
}

int main() {
	enum { WIDTH = 640 };
	enum { HEIGHT = 480 };

	static Pixel fbufdata[HEIGHT*WIDTH] = {0};

	Fbuf fb = { {WIDTH, HEIGHT}, fbufdata };
	draw(&fb);
	render_to_ppm(&fb);
	if(render_to_x(&fb)) return -1;

	return 0;
}
