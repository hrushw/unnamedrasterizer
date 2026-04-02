#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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
typedef Vec2 Triangle[3];
typedef Vec2 Quad[4];

typedef struct uvec2_t {
	u32 x, y;
} UVec2;

typedef struct fbuf_t {
	UVec2 	sz;
	Pixel *buf;
} Fbuf;

typedef struct rect_t {
	Vec2 r0;
	UVec2 sz;
} Rect;

i32Complex i32complex_mul_norm(i32Complex z1, i32Complex z2) {
	return (i32Complex) {
		((i64)z1.x * (i64)z2.x - (i64)z1.y * (i64)z2.y) >> 30,
		((i64)z1.x * (i64)z2.y + (i64)z1.y * (i64)z2.x) >> 30,
	};
}

/* Pixel ARGB access functions */
static inline
u8 pixA(Pixel p) { return (p >> 24) & 0xFF; }
static inline
u8 pixR(Pixel p) { return (p >> 16) & 0xFF; }
static inline
u8 pixG(Pixel p) { return (p >> 8) & 0xFF; }
static inline
u8 pixB(Pixel p) { return p & 0xFF; }

/* Framebuffer pixel access functions */
static inline
void fb_set_pix(Fbuf fb, UVec2 r, Pixel p) {
	fb.buf[r.y*fb.sz.x + r.x] = p;
}

static inline
Pixel fb_get_pix(Fbuf fb, UVec2 r) {
	return fb.buf[r.y*fb.sz.x + r.x];
}

// determinant
static inline
i64 vec2det(Vec2 v0, Vec2 v1) {
	return (i64)v0.x*(i64)v1.y - (i64)v1.x*(i64)v0.y;
}

/* Drawing functions */
static
UVec2 rect_bound_min0(Vec2 r0) {
	return (UVec2) {
		r0.x < 0 ? 0 : (u32)r0.x,
		r0.y < 0 ? 0 : (u32)r0.y,
	};
}

static
UVec2 rect_bound_max(Fbuf fb, Vec2 r0, UVec2 sz) {
	return (UVec2) {(
		r0.x += sz.x,
		r0.x > (i32)fb.sz.x
			? fb.sz.x
			: r0.x < 0 ? 0 : (u32)r0.x
	), (
		r0.y += sz.y,
		r0.y > (i32)fb.sz.y
			? fb.sz.y
			: r0.y < 0 ? 0 : (u32)r0.y
	)};
}

static
void fb_draw_rect_uvec2x2(Fbuf fb, Pixel p, UVec2 r0, UVec2 r1) {
	UVec2 r;
	for(r.y = r0.y; r.y < r1.y; ++r.y)
		for(r.x = r0.x; r.x < r1.x; ++r.x)
			fb_set_pix(fb, r, p);
}

static
void fb_draw_rect(Fbuf fb, Pixel p, Vec2 r0, UVec2 sz) {
	fb_draw_rect_uvec2x2(fb, p, rect_bound_min0(r0), rect_bound_max(fb, r0, sz));
}

static inline
u64 i32square(i32 x) {
	return (u64)((i64)x*(i64)x);
}

static
void fb_draw_circle(Fbuf fb, Pixel p, u32 R, Vec2 r0) {
	UVec2 r;
	for(r.y = 0; r.y < fb.sz.y; ++r.y)
		for(r.x = 0; r.x < fb.sz.x; ++r.x)
			if(i32square(r.y - r0.y) + i32square(r.x - r0.x) < i32square(R))
				fb_set_pix(fb, r, p);
}

// 0 is success
static
int checkptstatus(i64 D, i64 D1, i64 D2) {
	if(D < 0) D = -D, D1 = -D1, D2 = -D2;
	return (D1 < 0 || D2 < 0 || (u64)D1 + (u64)D2 > (u64)D);
}

static
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
			if(!checkptstatus(D, D1, D2))
				fb_set_pix(fb, r, p);
}

static
void fb_draw_triangle_arr(Fbuf fb, Pixel p, Triangle S) {
	fb_draw_triangle(fb, p, S[0], S[1], S[2]);
}

static
void fb_draw_quad(Fbuf fb, Pixel p, Quad q) {
	fb_draw_triangle(fb, p, q[0], q[1], q[2]);
	fb_draw_triangle(fb, p, q[0], q[3], q[2]);
}

static
void fb_draw_polygon_strip(Fbuf fb, Pixel p, size_t n, Vec2 *pts) {
	for(size_t i = 0; i < n-2; ++i)
		fb_draw_triangle(fb, p, pts[i], pts[i+1], pts[i+2]);
}

static
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
	const char* fname = "img.ppm";
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

typedef enum window_init_stage_x_e {
	STAGE_NONE = 0,
	STAGE_DISPLAY,
	STAGE_IMG
} InitStage_X;

typedef enum possible_errors_x_e {
	ERR_SUCCESS = 0,
	ERR_OPEN_DISPLAY,
	ERR_GET_VISUAL,
	ERR_GETWINATTRS,
	ERR_MAP_WIN,
	ERR_CR_IMG,
	ERR_IMG_INIT,
	ERR_IMG_FMT,
} WinError_X;

typedef struct window_properties_x_t {
	int closed;
	InitStage_X stage;
	WinError_X status;
	Display *disp;
	Window win;
	XWindowAttributes attrs;
	XImage *img;
} WinProps_X;

void window_cleanup_x(WinProps_X *wp) {
	switch(wp->stage) {
	case STAGE_IMG:
		free(wp->img->data);
		wp->img->data = NULL; // don't touch my data Xlib you disgusting creature, you did not allocate it
		XDestroyImage(wp->img);
	case STAGE_DISPLAY:
		XCloseDisplay(wp->disp);
	case STAGE_NONE:
	default: break;
	}
}

int handle_errors_x(Display *disp, XErrorEvent *err) {
	(void)disp;
	(void)err;
	printf("X11 Error (unspecified due to laziness)\n");
	return 0;
}

WinProps_X window_init_x(unsigned int width, unsigned int height) {
	WinProps_X wp = { .stage = STAGE_NONE };
	XSetErrorHandler(handle_errors_x);
	wp.disp = XOpenDisplay(NULL);
	if(!wp.disp) return wp.status = ERR_OPEN_DISPLAY, wp;
	wp.stage = STAGE_DISPLAY;

	XVisualInfo vinfo = {0};
	if(!XMatchVisualInfo(wp.disp, DefaultScreen(wp.disp), 24, TrueColor, &vinfo))
		return wp.status = ERR_GET_VISUAL, wp;

	wp.win = XCreateSimpleWindow(
		wp.disp, DefaultRootWindow(wp.disp),
		0, 0, width, height, 0, 0,
		BlackPixel(wp.disp, DefaultScreen(wp.disp))
	);
	XSelectInput(wp.disp, wp.win, StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask);

	if(!XGetWindowAttributes(wp.disp, wp.win, &wp.attrs))
		return wp.status = ERR_GETWINATTRS, wp;
	if(!XMapWindow(wp.disp, wp.win))
		return wp.status = ERR_MAP_WIN, wp;

	wp.img = XCreateImage(
		wp.disp, wp.attrs.visual, wp.attrs.depth, ZPixmap, 0,
		calloc(width*height, 4),
		width, height, 32, 0
	);
	if(!wp.img) return wp.status = ERR_CR_IMG, wp;
	wp.stage = STAGE_IMG;

	if(!XInitImage(wp.img))
		return wp.status = ERR_IMG_INIT, wp;
	if(wp.img->bits_per_pixel != 32
		|| wp.img->red_mask != 0xFF0000
		|| wp.img->blue_mask != 0xFF
		|| wp.img->green_mask != 0xFF00
	) return wp.status = ERR_IMG_FMT, wp;

	return wp;
}

/* X11 handling functions */
void handle_events_x(WinProps_X *wp) {
	XEvent ev;

	while(XCheckWindowEvent(wp->disp, wp->win, wp->attrs.your_event_mask, &ev))
	switch(ev.type) {
		case DestroyNotify:
			wp->closed = 1;
			return;
		case Expose: break;
		case KeyPress:
			printf("Key press detected!\n"); break;
		case KeyRelease:
			printf("Key release detected!\n"); break;
		default: break;
	}
}

/* Main drawing function */
void draw(Fbuf fb, WinProps_X *wp) {
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
	Quad EyeRight = {
		EyeRightOrigin,
		{ EyeRightOrigin.x - fb.sz.x/64,   EyeRightOrigin.y + EyeLeft.sz.y },
		{ EyeRightOrigin.x + EyeLeft.sz.x, EyeRightOrigin.y + EyeLeft.sz.y - fb.sz.x/32 },
		{ EyeRightOrigin.x + EyeLeft.sz.x, EyeRightOrigin.y },
	};
	Pixel EyeRightClr = 0x00CF3F;
	fb_draw_quad(fb, EyeRightClr, EyeRight);

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
	fb_draw_triangle_arr(fb, NoseColor, Nose);

	fb_draw_rect(fb, 0xFF00FF, (Vec2) {-200, -300}, (UVec2) {100, 200});

	Rect Goatee = {
		{ 7*fb.sz.x/16, 7*fb.sz.y/8 },
		{ fb.sz.x/8, fb.sz.y/2 }
	};
	fb_draw_rect(fb, 0xFFFFFF, Goatee.r0, Goatee.sz);

	Triangle BrowLeft = {
		{ 5*fb.sz.x/32, (i32)fb.sz.y/12 },
		{ fb.sz.x/32, - ((i32)fb.sz.y/24) },
		{ 9*fb.sz.x/32, - ((i32)fb.sz.y/24) },
	}, BrowRight = {
		{ fb.sz.x - BrowLeft[0].x, BrowLeft[0].y },
		{ fb.sz.x - BrowLeft[1].x, BrowLeft[1].y },
		{ fb.sz.x - BrowLeft[2].x, BrowLeft[2].y },
	};

	Pixel BrowColor = 0x7F3F00;
	fb_draw_triangle_arr(fb, BrowColor, BrowLeft);
	fb_draw_triangle_arr(fb, BrowColor, BrowRight);

	struct timespec dt = { 0, 16666667 };
	Vec2 EyeballLeftOrigin = {fb.sz.x/16, 5*fb.sz.y/16};
	u32 EyeballRadius = fb.sz.x/32;
	Vec2 EyeballRightOrigin = EyeballLeftOrigin;
	Pixel EyeballColor = 0x00007F;
	EyeballRightOrigin.x = fb.sz.x - EyeballLeftOrigin.x;
	int dir = 1;

	while(!wp->closed) {
		// Eye motion logic
		fb_draw_circle(fb, EyeLeftClr, EyeballRadius, EyeballLeftOrigin);
		fb_draw_circle(fb, EyeRightClr, EyeballRadius, EyeballRightOrigin);
		if(dir) EyeballLeftOrigin.x += 2; else EyeballLeftOrigin.x -= 2;
		if(EyeballLeftOrigin.x > 9*(i32)fb.sz.x/32 - (i32)EyeballRadius)
			EyeballLeftOrigin.x = 9*fb.sz.x/32 - (i32)EyeballRadius,
			dir = 0;
		else if(EyeballLeftOrigin.x < (i32)fb.sz.x/16)
			EyeballLeftOrigin.x = fb.sz.x/16,
			dir = 1;
		EyeballRightOrigin.x = fb.sz.x - EyeballLeftOrigin.x;
		fb_draw_circle(fb, EyeballColor, EyeballRadius, EyeballLeftOrigin);
		fb_draw_circle(fb, EyeballColor, EyeballRadius, EyeballRightOrigin);

		fbtoximg(fb, wp->img);
		XPutImage(wp->disp, wp->win, DefaultGC(wp->disp, DefaultScreen(wp->disp)), wp->img, 0, 0, 0, 0, fb.sz.x, fb.sz.y);

		nanosleep(&dt, NULL);
		handle_events_x(wp);
	}
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

	WinProps_X wp = window_init_x(fb.sz.x, fb.sz.y);
	if(wp.status == ERR_SUCCESS)
		draw(fb, &wp);

	window_cleanup_x(&wp);
	render_to_ppm(fb);

	return wp.status;
}

