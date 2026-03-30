#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// TODO win32

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t i32;
typedef int64_t i64;

typedef uint32_t Pixel;

#define U32MAX UINT32_MAX

/* Structure declarations */
typedef struct vec2_t {
	int32_t x, y;
} Vec2;

typedef struct uvec2_t {
	u32 x, y;
} UVec2;

typedef struct vec3b_t {
	u32 b0, b1, b2;
} Vec3B;

typedef struct rect_t {
	Vec2 r0;
	UVec2 sz;
} Rect;

typedef struct triangle_t {
	Vec2 r0, r1, r2;
} Triangle;

typedef struct vec3_pixel_t {
	Pixel p0, p1, p2;
} Vec3Pixel;

typedef struct fbuf_t {
	UVec2 	sz;
	Pixel *buf;
} Fbuf;

typedef struct circle_t {
	Vec2 r0;
	u32 R;
} Circle;

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
void fb_set_pix(Fbuf *fb, UVec2 r, Pixel p) {
	fb->buf[r.y*fb->sz.x + r.x] = p;
}

static inline
Pixel fb_get_pix(Fbuf *fb, UVec2 r) {
	return fb->buf[r.y*fb->sz.x + r.x];
}

/* Integer operations */
static inline
i32 i32min0(i32 a) {
	return a > 0 ? a : 0;
}

/*
static
int i32cmpsign(i32 x, i32 y) {
	return (x > 0 && y < 0) || (x < 0 && y > 0);
}

static
int i32cmpsign3(i32 x, i32 y, i32 z) {
	return !(
		   (x > 0 && y > 0 && z > 0)
		|| (x < 0 && y < 0 && z < 0)
	);
}

static
int i32modcmp(i32 x, i32 y) {
	return x > 0 ? x > y : x < y;
}
*/

static inline
void i32signrestrict(i32 *r, i32 *x1, i32 *x2) {
	if(*r < 0) {
		*r = - *r;
		*x1 = - *x1;
		*x2 = - *x2;
	}
}

static inline
i32 i32square(i32 x) {
	return x*x;
}

static inline
u32 u32chkaddoverflow(u32 a, u32 b) {
	return a + b < a;
}

/*
static inline
u32 absdiff(u32 a, u32 b) {
	return a > b ? a - b : b - a;
}

static inline
u64 u32square(u32 a) {
	return (u64)a * (u64)a;
}
*/

/* Vector Operations */
static inline
i32 vec2det(Vec2 v0, Vec2 v1) {
	return v0.x*v1.y - v1.x*v0.y;
}

static
Vec2 vec2sub(Vec2 r0, Vec2 r1) {
	return (Vec2) {r0.x - r1.x, r0.y - r1.y};
}

/*
static
Vec2 uvec2sub(UVec2 r0, UVec2 r1) {
	return (Vec2) {(i32)r0.x - (i32)r1.x, (i32)r0.y - (i32)r1.y};
}
*/

/* Linear interpolation functions */
static
u8 u8lerp(Vec3B B, u8 x0, u8 x1, u8 x2) {
	return (
		(u64)x0*(u64)B.b0 +
		(u64)x1*(u64)B.b1 +
		(u64)x2*(u64)B.b2
	) / (u64)U32MAX;
}

static
Pixel lerp(Vec3B B, Vec3Pixel P) {
	return argb_to_pixel(
		u8lerp(B, pixA(P.p0), pixA(P.p1), pixA(P.p2)),
		u8lerp(B, pixR(P.p0), pixR(P.p1), pixR(P.p2)),
		u8lerp(B, pixG(P.p0), pixG(P.p1), pixG(P.p2)),
		u8lerp(B, pixB(P.p0), pixB(P.p1), pixB(P.p2))
	);
}

/* Drawing functions */
void fb_draw_rect(Fbuf *fb, Rect S, Pixel p) {
	UVec2 r;
	for(r.y = i32min0(S.r0.y); r.y < S.r0.y + S.sz.y; ++r.y)
		for(r.x = i32min0(S.r0.x); r.x < S.r0.x + S.sz.x; ++r.x)
			fb_set_pix(fb, r, p);
}

void fb_draw_circle(Fbuf *fb, Circle S, Pixel p) {
	UVec2 r;
	for(r.y = 0; r.y < fb->sz.y; ++r.y)
		for(r.x = 0; r.x < fb->sz.x; ++r.x)
			if(i32square(r.y - S.r0.y) + i32square(r.x - S.r0.x) < i32square(S.R))
				fb_set_pix(fb, r, p);
}

typedef enum ptstatus_e {
	PT_IN = 0,
	PT_OUT = -1
} PtStatus;

// find barycentric coordinates from given determinants
PtStatus getlerpweights(Vec3B *B, i32 D, i32 D1, i32 D2) {
	i32signrestrict(&D, &D1, &D2);

	if(D1 < 0 || D2 < 0) return PT_OUT;
	if(D1 > D || D2 > D) return PT_OUT;

	B->b1 = ((i64)D1 * (i64)U32MAX) / (i64)D;
	B->b2 = ((i64)D2 * (i64)U32MAX) / (i64)D;
	B->b0 = U32MAX - B->b1 - B->b2;

	if(u32chkaddoverflow(B->b1, B->b2)) return PT_OUT;
	return PT_IN;
}

// TODO faster function for monochrome triangles?
void fb_draw_triangle(Fbuf *fb, Triangle S, Vec3Pixel P) {
	Vec2 dr1 = vec2sub(S.r1, S.r0);
	Vec2 dr2 = vec2sub(S.r2, S.r0);
	i32 D = vec2det(dr1, dr2);
	UVec2 r;
	Vec2 dr;
	Vec3B B;

	for(
		r.y = 0, dr.y = (i32)r.y - (i32)S.r0.y;
		r.y < fb->sz.y;
		++r.y, ++dr.y
	)
		for(
			r.x = 0, dr.x = (i32)r.x - (i32)S.r0.x;
			r.x < fb->sz.x;
			++r.x, ++dr.x
		)
			if(getlerpweights(&B, D, vec2det(dr1, dr), vec2det(dr, dr2)) == PT_IN)
				fb_set_pix(fb, r, lerp(B, P));
}

Rect fb_mirror_rect_x(Fbuf *fb, Rect R) {
	return (Rect) {
		(Vec2) { (i32)fb->sz.x - R.r0.x - R.sz.x, R.r0.y },
		R.sz
	};
}

void fb_draw_parabola_bounded(Fbuf *fb, Rect bound, UVec2 origin, i32 a, Pixel p) {
	UVec2 r;
	for(r.y = bound.r0.y; r.y < bound.r0.y + bound.sz.y; ++r.y)
		for(r.x = bound.r0.x; r.x < bound.r0.x + bound.sz.x; ++r.x)
			if (a*((i32)r.y - (i32)origin.y) > i32square(r.x - origin.x))
				fb_set_pix(fb, r, p);
}

/* framebuffer export functions (to X11 window or PPM file) */
void ximgsetpix_bgra(XImage *img, size_t i, Pixel p) {
	img->data[i  ] = pixB(p);
	img->data[i+1] = pixG(p);
	img->data[i+2] = pixR(p);
	img->data[i+3] = pixA(p);
}

void fbtoximg(Fbuf *fb, XImage *img) {
	UVec2 r;
	for(r.y = 0; r.y < fb->sz.y; ++r.y)
		for(r.x = 0; r.x < fb->sz.x; ++r.x)
			ximgsetpix_bgra(
				img,
				4*(r.y*img->width + r.x),
				fb_get_pix(fb, r)
			);
}

void fb_to_ppm(FILE *f, Fbuf *fb) {
	Pixel p;
	u8 pixbuf[3];
	for(u32 i = 0; i < fb->sz.x*fb->sz.y; ++i) {
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

// get a valid window and its attributes
int crwin_X(
	Window *win, XWindowAttributes *attrs,
	Display *disp, long evmask,
	unsigned long width, unsigned long height
) {
	XVisualInfo vinfo = {0};
	XSetWindowAttributes xswattrs = {
		.background_pixel = BlackPixel(disp, DefaultScreen(disp)),
		.event_mask = evmask
	};

	if(!XMatchVisualInfo(disp, DefaultScreen(disp), 24, TrueColor, &vinfo))
		return fprintf(stderr, "Unable to get proper visual!\n"), -1;

	*win = XCreateWindow(
		disp, DefaultRootWindow(disp),
		0, 0, width, height,
		0, 24, InputOutput,
		vinfo.visual, CWEventMask | CWBackPixel, &xswattrs
	);
	XGetWindowAttributes(disp, *win, attrs);

	XMapWindow(disp, *win);
	return 0;
}

int render_to_x_disp(Fbuf *fb, Display *disp) {
	Window win;
	XWindowAttributes attrs;
	long evmask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask;
	u32 *buf;
	XImage *img;
	XEvent ev;
	struct timespec dt = { 0, 166666667 };

	if(crwin_X(
		&win, &attrs,
		disp, evmask,
		fb->sz.x, fb->sz.y
	)) return -1;

	// Create XImage structure 
	buf = calloc(fb->sz.y*fb->sz.x, 4);
	img = XCreateImage(disp, attrs.visual, attrs.depth, ZPixmap, 0, (char*)buf, fb->sz.x, fb->sz.y, 32, 0);
	XInitImage(img);
	if(
		   img->bits_per_pixel != 32
		|| img->red_mask != 0xFF0000
		|| img->blue_mask != 0xFF
		|| img->green_mask != 0xFF00
	)
		return fprintf(stderr, "Invalid image format!\n"), -2;

	Circle EyeballLeft = { {40, 150}, 20 }, EyeballRight = EyeballLeft;
	int dir = 1;
	while(1) {
		// Handle events
		while(XCheckWindowEvent(disp, win, evmask, &ev)) {
			if(ev.type == DestroyNotify)
				goto end;

			if(ev.type == Expose)
				(void)0;

			if(ev.type == KeyPress)
				printf("Key press detected!\n");

			if(ev.type == KeyRelease)
				printf("Key release detected!\n");
		}

		// Eye motion logic
		fb_draw_circle(fb, EyeballLeft, 0x00FF00);
		fb_draw_circle(fb, EyeballRight, 0x00FF00);
		if(dir) EyeballLeft.r0.x += 10; else EyeballLeft.r0.x -= 10;
		if(EyeballLeft.r0.x > 180 - (i32)EyeballLeft.R)
			EyeballLeft.r0.x = 180 - (i32)EyeballLeft.R,
			dir = 0;
		else if(EyeballLeft.r0.x < 40)
			EyeballLeft.r0.x = 40,
			dir = 1;
		EyeballRight.r0.x = fb->sz.x - EyeballLeft.r0.x;
		fb_draw_circle(fb, EyeballLeft, 0x00007F);
		fb_draw_circle(fb, EyeballRight, 0x00007F);

		// draw image to window
		fbtoximg(fb, img);
		XPutImage(disp, win, DefaultGC(disp, DefaultScreen(disp)), img, 0, 0, 0, 0, fb->sz.x, fb->sz.y);

		// sleep for 1 frame
		nanosleep(&dt, NULL);
	}

	end:
	free(buf);
	return 0;
}

int render_to_x(Fbuf *fb) {
	Display *disp = XOpenDisplay(NULL);

	if(render_to_x_disp(fb, disp)) return -1;

	XCloseDisplay(disp);
	return 0;
}

/* Main drawing function */
// TODO drawing independent of framebuffer size
void draw(Fbuf *fb) {
	Rect EyeLeft = {
		{ 20, 100 },
		{ 160, 100 },
	};
	Pixel EyeClr = 0x00FF00;
	fb_draw_rect(fb, EyeLeft, EyeClr);

	Rect EyeRight = fb_mirror_rect_x(fb, EyeLeft);
	fb_draw_rect(fb, EyeRight, EyeClr);

	Rect SmileBound = {
		{0, fb->sz.y/2},
		{fb->sz.x, fb->sz.y/3}
	};

	UVec2 SmileOrigin = {fb->sz.x/2, 5*fb->sz.y/6};
	Pixel SmileClr = 0xFF0000;

	fb_draw_parabola_bounded(fb, SmileBound, SmileOrigin, -256, SmileClr);

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

	fb_draw_triangle(fb, Nose, NoseColors);

}

int main() {
	enum win_width_e { WIDTH = 640 };
	enum win_height_e { HEIGHT = 480 };

	static Pixel fbufdata[HEIGHT*WIDTH] = {0};

	Fbuf fb = { { WIDTH, HEIGHT }, fbufdata };
	draw(&fb);
	render_to_ppm(&fb);
	if(render_to_x(&fb)) return -1;

	return 0;
}
