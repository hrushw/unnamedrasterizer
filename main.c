#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// TODO win32
// TODO line drawing
// TODO rotation

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t i32;
typedef int64_t i64;

typedef uint32_t Pixel;
typedef uint32_t Angle;

/* Structure declarations */
typedef struct vec2_t {
	int32_t x, y;
} Vec2;
typedef Vec2 i32Complex;

typedef struct uvec2_t {
	u32 x, y;
} UVec2;

typedef struct uvec2x2_t {
	UVec2 r0, r1;
} UVec2x2;

typedef struct fbuf_t {
	UVec2 	sz;
	Pixel *buf;
} Fbuf;

typedef struct rect_t {
	Vec2 r0;
	UVec2 sz;
} Rect;

typedef struct triangle_t {
	Vec2 r0, r1, r2;
} Triangle;

typedef struct quad_t {
	Vec2 r0, r1, r2, r3;
} Quad;

typedef struct circle_t {
	Vec2 r0;
	u32 R;
} Circle;

i32Complex i32complex_mul_norm(i32Complex z1, i32Complex z2) {
	return (i32Complex) {
		((i64)z1.x * (i64)z2.x - (i64)z1.y * (i64)z2.y) >> 30,
		((i64)z1.x * (i64)z2.y + (i64)z1.y * (i64)z2.x) >> 30,
	};
}

/* Pixel ARGB access functions */
static inline
u8 pixA(Pixel p) {
	return (p >> 24) & 0xFF;
}

static inline
u8 pixR(Pixel p) {
	return (p >> 16) & 0xFF;
}

static inline
u8 pixG(Pixel p) {
	return (p >> 8) & 0xFF;
}

static inline
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

/* Framebuffer pixel access functions */
static inline
void fb_set_pix(Fbuf fb, UVec2 r, Pixel p) {
	fb.buf[r.y*fb.sz.x + r.x] = p;
}

static inline
Pixel fb_get_pix(Fbuf fb, UVec2 r) {
	return fb.buf[r.y*fb.sz.x + r.x];
}

/* Vector Operations */
static inline
i64 vec2det(Vec2 v0, Vec2 v1) {
	return (i64)v0.x*(i64)v1.y - (i64)v1.x*(i64)v0.y;
}

/* Drawing functions */
UVec2 rect_bound_min0(Vec2 r0) {
	return (UVec2) {
		r0.x < 0 ? 0 : (u32)r0.x,
		r0.y < 0 ? 0 : (u32)r0.y,
	};
}

UVec2 rect_bound_max(Fbuf fb, Vec2 r0, UVec2 sz) {
	return (UVec2) {(
		r0.x += sz.x,
		r0.x > (i32)fb.sz.x
			? (i32)fb.sz.x
			: r0.x < 0 ? 0 : r0.x
	), (
		r0.y += sz.y,
		r0.y > (i32)fb.sz.y
			? (i32)fb.sz.y
			: r0.y < 0 ? 0 : r0.y
	)};
}

void fb_draw_rect_uvec2x2(Fbuf fb, Pixel p, UVec2 r0, UVec2 r1) {
	UVec2 r;
	for(r.y = r0.y; r.y < r1.y; ++r.y)
		for(r.x = r0.x; r.x < r1.x; ++r.x)
			fb_set_pix(fb, r, p);
}

void fb_draw_rect(Fbuf fb, Pixel p, Vec2 r0, UVec2 sz) {
	fb_draw_rect_uvec2x2(fb, p, rect_bound_min0(r0), rect_bound_max(fb, r0, sz));
}

static inline
u64 i32square(i32 x) {
	return (u64)((i64)x*(i64)x);
}

void fb_draw_circle(Fbuf fb, Circle S, Pixel p) {
	UVec2 r;
	for(r.y = 0; r.y < fb.sz.y; ++r.y)
		for(r.x = 0; r.x < fb.sz.x; ++r.x)
			if(i32square(r.y - S.r0.y) + i32square(r.x - S.r0.x) < i32square(S.R))
				fb_set_pix(fb, r, p);
}

bool checkptstatus(i64 D, i64 D1, i64 D2) {
	if(D < 0) D = -D, D1 = -D1, D2 = -D2;
	return !(D1 < 0 || D2 < 0 || (u64)D1 + (u64)D2 > (u64)D);
}

void fb_draw_triangle(Fbuf fb, Pixel p, Vec2 r0, Vec2 r1, Vec2 r2) {
	i64 D1 = - vec2det(r0, r2);
	i64 D2 = - vec2det(r1, r0);
	i64 D = vec2det(r1, r2) + D1 + D2;
	if(!D) return;

	r1.y -= r0.y;
	r2.y -= r0.y;
	r1.x += fb.sz.x*r1.y - r0.x;
	r2.x += fb.sz.x*r2.y - r0.x;

	UVec2 r;
	for(r.y = 0; r.y < fb.sz.y; ++r.y, D1 -= r2.x, D2 += r1.x)
		for(r.x = 0; r.x < fb.sz.x; ++r.x, D1 += r2.y, D2 -= r1.y)
			if(checkptstatus(D, D1, D2))
				fb_set_pix(fb, r, p);
}

void fb_draw_quad(Fbuf fb, Quad S, Pixel p) {
	fb_draw_triangle(fb, p, S.r0, S.r1, S.r2);
	fb_draw_triangle(fb, p, S.r0, S.r3, S.r2);
}

void fb_draw_polygon_strip(Fbuf fb, Pixel p, size_t n, Vec2 *pts) {
	for(size_t i = 0; i < n-2; ++i)
		fb_draw_triangle(fb, p, pts[i], pts[i+1], pts[i+2]);
}

void fb_draw_polygon_fan(Fbuf fb, Pixel p, size_t n, Vec2 *pts) {
	for(size_t i = 0; i < n-2; ++i)
		fb_draw_triangle(fb, p, pts[0], pts[i+1], pts[i+2]);
}

/* framebuffer export functions (to X11 window or PPM file) */
void fb_to_ppm(FILE *f, Fbuf fb) {
	Pixel p;
	u8 pixbuf[3];
	for(u32 i = 0; i < fb.sz.x*fb.sz.y; ++i) {
		p = fb.buf[i];
		pixbuf[0] = pixR(p);
		pixbuf[1] = pixG(p);
		pixbuf[2] = pixB(p);
		fwrite(pixbuf, 3, 1, f);
	}
}

void render_to_ppm(Fbuf fb) {
	const char* fname = "gaem.ppm";
	FILE* f = fopen(fname, "wb");

	fprintf(f, "P6\n%d %d 255\n", fb.sz.x, fb.sz.y);
	fb_to_ppm(f, fb);
}

static
void ximgsetpix_bgra(XImage *img, size_t i, Pixel p) {
	img->data[i  ] = pixB(p);
	img->data[i+1] = pixG(p);
	img->data[i+2] = pixR(p);
	img->data[i+3] = pixA(p);
}

void fbtoximg(Fbuf fb, XImage *img) {
	UVec2 r;
	for(r.y = 0; r.y < fb.sz.y; ++r.y)
		for(r.x = 0; r.x < fb.sz.x; ++r.x)
			ximgsetpix_bgra(
				img,
				4*(r.y*img->width + r.x),
				fb_get_pix(fb, r)
			);
}

typedef enum possible_errors_x_e {
	ERR_SUCCESS = 0,
	ERR_OPEN_DISPLAY,
	ERR_GET_VISUAL,
	ERR_GETWINATTRS,
	ERR_MAP_WIN,
	ERR_CR_IMG,
	ERR_IMG_INIT,
	ERR_IMG_FMT,
} WIN_ERROR_X;

/* X11 handling functions */
int handle_events_x(Display *disp, Window win, long evmask) {
	XEvent ev;

	while(XCheckWindowEvent(disp, win, evmask, &ev))
	switch(ev.type) {
		case DestroyNotify: return -1;
		case Expose: break;
		case KeyPress:
			printf("Key press detected!\n"); break;
		case KeyRelease:
			printf("Key release detected!\n"); break;
		default: break;
	}

	return 0;
}

void render_to_x_win_img(
	Fbuf fb, Display *disp, Window win,
	XImage *img, long evmask
) {
	struct timespec dt = { 0, 166666667 };
	Circle EyeballLeft = { {fb.sz.x/16, 5*fb.sz.y/16}, fb.sz.x/32 }, EyeballRight = EyeballLeft;
	EyeballRight.r0.x = fb.sz.x - EyeballLeft.r0.x;
	int dir = 1;
	while(!handle_events_x(disp, win, evmask)) {
		// Eye motion logic
		fb_draw_circle(fb, EyeballLeft, 0x00FF00);
		fb_draw_circle(fb, EyeballRight, 0x00CF3F);
		if(dir) EyeballLeft.r0.x += 10; else EyeballLeft.r0.x -= 10;
		if(EyeballLeft.r0.x > 9*(i32)fb.sz.x/32 - (i32)EyeballLeft.R)
			EyeballLeft.r0.x = 9*fb.sz.x/32 - (i32)EyeballLeft.R,
			dir = 0;
		else if(EyeballLeft.r0.x < (i32)fb.sz.x/16)
			EyeballLeft.r0.x = fb.sz.x/16,
			dir = 1;
		EyeballRight.r0.x = fb.sz.x - EyeballLeft.r0.x;
		fb_draw_circle(fb, EyeballLeft, 0x00007F);
		fb_draw_circle(fb, EyeballRight, 0x00007F);

		fbtoximg(fb, img);
		XPutImage(disp, win, DefaultGC(disp, DefaultScreen(disp)), img, 0, 0, 0, 0, fb.sz.x, fb.sz.y);

		nanosleep(&dt, NULL);
	}
}

int render_to_x_win(
	Fbuf fb, Display *disp, Window win,
	long evmask, XImage *img
) {
	if(!XInitImage(img))
		return ERR_IMG_INIT;
	if(img->bits_per_pixel != 32
		|| img->red_mask != 0xFF0000
		|| img->blue_mask != 0xFF
		|| img->green_mask != 0xFF00
	) return ERR_IMG_FMT;

	render_to_x_win_img(fb, disp, win, img, evmask);

	free(img->data);
	img->data = NULL; // don't touch my data Xlib you disgusting creature, you did not allocate it

	return ERR_SUCCESS;
}

int render_to_x_disp(Fbuf fb, Display *disp) {
	XVisualInfo vinfo = {0};
	XWindowAttributes attrs;

	if(!XMatchVisualInfo(disp, DefaultScreen(disp), 24, TrueColor, &vinfo))
		return ERR_GET_VISUAL;

	Window win = XCreateSimpleWindow(
		disp, DefaultRootWindow(disp),
		0, 0, fb.sz.x, fb.sz.y, 0, 0,
		BlackPixel(disp, DefaultScreen(disp))
	);
	XSelectInput(disp, win, StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask);

	if(!XGetWindowAttributes(disp, win, &attrs))
		return ERR_GETWINATTRS;
	if(!XMapWindow(disp, win))
		return ERR_MAP_WIN;

	XImage *img = XCreateImage(
		disp, attrs.visual, attrs.depth, ZPixmap, 0,
		calloc(fb.sz.y*fb.sz.x, 4),
		fb.sz.x, fb.sz.y, 32, 0
	);
	if(!img) return ERR_CR_IMG;

	int ret = render_to_x_win(
		fb, disp, win,
		attrs.your_event_mask, img
	);

	XDestroyImage(img);
	return ret;
}

int render_to_x(Fbuf fb) {
	Display *disp = XOpenDisplay(NULL);
	if(!disp) return ERR_OPEN_DISPLAY;

	int ret = render_to_x_disp(fb, disp);

	XCloseDisplay(disp);
	return ret;
}

/* Main drawing function */
void draw(Fbuf fb) {
	Rect EyeLeft = {
		{ fb.sz.x/32, 10*fb.sz.y/48 },
		{ fb.sz.x/4, 10*fb.sz.y/48 },
	};
	Pixel EyeLeftClr = 0x00FF00;
	fb_draw_rect(fb, EyeLeftClr, EyeLeft.r0, EyeLeft.sz);

	Vec2 EyeRightOrigin = {
		(i32)fb.sz.x - EyeLeft.r0.x - EyeLeft.sz.x,
		EyeLeft.r0.y
	};
	Quad EyeRightQuad = {
		EyeRightOrigin,
		{ EyeRightOrigin.x - fb.sz.x/64,   EyeRightOrigin.y + EyeLeft.sz.y },
		{ EyeRightOrigin.x + EyeLeft.sz.x, EyeRightOrigin.y + EyeLeft.sz.y - fb.sz.x/32 },
		{ EyeRightOrigin.x + EyeLeft.sz.x, EyeRightOrigin.y },
	};
	Pixel EyeRightClr = 0x00CF3F;
	fb_draw_quad(fb, EyeRightQuad, EyeRightClr);

	Vec2 Smilepts[] = {
		{fb.sz.x/2, fb.sz.y/2},
		{3*fb.sz.x/16, fb.sz.y/2},
		{7*fb.sz.x/32, 5*fb.sz.y/8},
		{fb.sz.x/4, 11*fb.sz.y/16},
		{5*fb.sz.x/16, 9*fb.sz.y/12},
		{13*fb.sz.x/32, 49*fb.sz.y/60},
		{15*fb.sz.x/32, 5*fb.sz.y/6},
		{17*fb.sz.x/32, 5*fb.sz.y/6},
		{19*fb.sz.x/32, 49*fb.sz.y/60},
		{11*fb.sz.x/16, 9*fb.sz.y/12},
		{3*fb.sz.x/4, 11*fb.sz.y/16},
		{25*fb.sz.x/32, 5*fb.sz.y/8},
		{13*fb.sz.x/16, fb.sz.y/2},
	};
	Pixel SmileClr = 0xFF0000;

	fb_draw_polygon_fan(fb, SmileClr, sizeof(Smilepts)/sizeof(Vec2), Smilepts);

	Triangle Nose = {
		{fb.sz.x/2, fb.sz.y/3},
		{13*fb.sz.x/32, 7*fb.sz.y/16},
		{19*fb.sz.x/32, 7*fb.sz.y/16},
	};

	Pixel NoseColor = 0xFFFF00;
	fb_draw_triangle(fb, NoseColor, Nose.r0, Nose.r1, Nose.r2);

	Rect outofboundrect = {
		{ -200, -300 },
		{ 100, 200 }
	};

	fb_draw_rect(fb, 0xFF00FF, outofboundrect.r0, outofboundrect.sz);

	Rect Goatee = {
		{ 7*fb.sz.x/16, 7*fb.sz.y/8 },
		{ fb.sz.x/8, fb.sz.y/2 }
	};
	fb_draw_rect(fb, 0xFFFFFF, Goatee.r0, Goatee.sz);

	Triangle BrowLeft = {
		{ 5*fb.sz.x/32, (i32)fb.sz.y/12 },
		{ fb.sz.x/32, -1*(i32)fb.sz.y/24 },
		{ 9*fb.sz.x/32, -1*(i32)fb.sz.y/24 },
	}, BrowRight = BrowLeft;
	BrowRight.r0.x = fb.sz.x - BrowLeft.r0.x;
	BrowRight.r1.x = fb.sz.x - BrowLeft.r1.x;
	BrowRight.r2.x = fb.sz.x - BrowLeft.r2.x;

	Pixel BrowColor = 0x7F3F00;
	fb_draw_triangle(fb, BrowColor, BrowLeft.r0, BrowLeft.r1, BrowLeft.r2);
	fb_draw_triangle(fb, BrowColor, BrowRight.r0, BrowRight.r1, BrowRight.r2);
}

int main() {
	enum win_width_e { WIDTH = 640 };
	enum win_height_e { HEIGHT = 480 };

	static Pixel fbufdata[HEIGHT*WIDTH] = {0};

	i32Complex I = { 0, 1 << 30 };
	i32Complex z0 = { 300, 400 };
	i32Complex z1 = i32complex_mul_norm(z0, I);
	printf("%d %d\n", z1.x, z1.y);

	Fbuf fb = { { WIDTH, HEIGHT }, fbufdata };
	draw(fb);
	render_to_ppm(fb);
	if(render_to_x(fb)) return -1;

	return 0;
}
