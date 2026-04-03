#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

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

static inline
u32 u32min(u32 a, u32 b) {
	return a < b ? a : b;
}

static inline
i32 u32min0(i32 a) {
	return a > 0 ? a : 0;
}

static
void fb_draw_circle(Fbuf fb, Pixel p, u32 R, Vec2 r0) {
	UVec2 r;
	for(r.y = u32min0(r0.y - (i32)R); r.y < u32min(fb.sz.y, (u32)r0.y + R); ++r.y)
		for(r.x = u32min0(r0.x - (i32)R); r.x < u32min(fb.sz.x, (u32)r0.x + R); ++r.x)
			if(i32square(r.y - r0.y) + i32square(r.x - r0.x) < i32square(R))
				fb_set_pix(fb, r, p);
}

// 0 is success
static
int checkptstatus(i64 D, i64 D1, i64 D2) {
	if(D < 0) D = -D, D1 = -D1, D2 = -D2;
	return (D1 < 0 || D2 < 0 || (u64)D1 + (u64)D2 > (u64)D);
}

// TODO bounds checking
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

/* framebuffer export to ppm */
static
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

static
void render_to_ppm(Fbuf fb) {
	const char* fname = "img.ppm";
	FILE* f = fopen(fname, "wb");

	fprintf(f, "P6\n%d %d 255\n", fb.sz.x, fb.sz.y);
	fb_to_ppm(f, fb);
}

/* X11 setup */
typedef enum window_init_stage_x_e {
	STAGE_NONE = 0,
	STAGE_DISPLAY,
} InitStage_X;

typedef enum possible_errors_x_e {
	ERR_SUCCESS = 0,
	ERR_OPEN_DISPLAY,
	ERR_GET_VISUAL,
	ERR_GETWINATTRS,
	ERR_VISUAL_FMT,
	ERR_MAP_WIN,
	ERR_IMG_INIT,
} WinError_X;

typedef struct window_properties_x_t {
	int closed;
	InitStage_X stage;
	WinError_X status;
	Display *disp;
	Window win;
	XWindowAttributes attrs;
	XImage img;
} WinProps_X;

static
void window_cleanup_x(WinProps_X *wp) {
	switch(wp->stage) {
	case STAGE_DISPLAY:
		XCloseDisplay(wp->disp);
		/* fallthrough */
	case STAGE_NONE:
	default: break;
	}
}

/* X11 errors are expected and normal
 * the window may be destroyed just before XPutImage is called, resulting in an invalid operation
 * setting this function as the error handler allows the program to continue in that case
 * and perform necessary cleanup */
static
int handle_errors_x(Display *disp, XErrorEvent *err) {
	(void)disp;
	(void)err;
	printf("X11 Error (unspecified due to laziness)\n");
	return 0;
}

static
WinProps_X window_init_x(Fbuf fb) {
	WinProps_X wp = { .stage = STAGE_NONE };
	XSetErrorHandler(handle_errors_x);
	wp.disp = XOpenDisplay(NULL);
	if(!wp.disp)
		return wp.status = ERR_OPEN_DISPLAY, wp;
	wp.stage = STAGE_DISPLAY;

	XVisualInfo vinfo = {0};
	if(!XMatchVisualInfo(wp.disp, DefaultScreen(wp.disp), 24, TrueColor, &vinfo))
		return wp.status = ERR_GET_VISUAL, wp;

	XSetWindowAttributes xswattrs = {
		.background_pixel = BlackPixel(wp.disp, DefaultScreen(wp.disp)),
		.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask
	};
	wp.win = XCreateWindow(
		wp.disp, DefaultRootWindow(wp.disp),
		0, 0, fb.sz.x, fb.sz.y, 0,
		24, InputOutput, vinfo.visual,
		CWBackPixel | CWEventMask, &xswattrs
	);

	if(!XGetWindowAttributes(wp.disp, wp.win, &wp.attrs))
		return wp.status = ERR_GETWINATTRS, wp;

	if(wp.attrs.visual->red_mask != 0xFF0000
		|| wp.attrs.visual->green_mask != 0xFF00
		|| wp.attrs.visual->blue_mask != 0xFF
	) return wp.status = ERR_VISUAL_FMT, wp;

	if(!XMapWindow(wp.disp, wp.win))
		return wp.status = ERR_MAP_WIN, wp;

	wp.img = (XImage) {
		.width = (int)fb.sz.x,
		.height = (int)fb.sz.y,
		.xoffset = 0,
		.format = ZPixmap,
		.data = (char*)fb.buf,

		.byte_order = LSBFirst,
		.bitmap_unit = 32,
		.bitmap_bit_order = LSBFirst,
		.bitmap_pad = 32,

		.depth = wp.attrs.depth,
		.bytes_per_line = 0,
		.bits_per_pixel = 32,

		.red_mask = wp.attrs.visual->red_mask,
		.blue_mask = wp.attrs.visual->blue_mask,
		.green_mask = wp.attrs.visual->green_mask,
	};

	if(!XInitImage(&wp.img))
		return wp.status = ERR_IMG_INIT, wp;

	return wp.status = ERR_SUCCESS, wp;
}

void get_time_diff(clockid_t clk, struct timespec *t) {
	struct timespec end;
	clock_gettime(clk, &end);
	t->tv_sec = end.tv_sec - t->tv_sec;
	if(t->tv_nsec > end.tv_nsec)
		t->tv_nsec = end.tv_nsec + 1000000000 - t->tv_nsec;
	else
		t->tv_nsec = end.tv_nsec - t->tv_nsec;
}

void get_timespec_diff(struct timespec *t, u32 nsmax) {
	if(t->tv_sec || t->tv_nsec > nsmax) t->tv_sec = t->tv_nsec = 0;
	else t->tv_nsec = nsmax - t->tv_nsec;
}

/* Main drawing function */
static
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

	enum { FRAME_NS = 16666667 };
	struct timespec dt;

	Vec2 EyeballLeftOrigin = {fb.sz.x/16, 5*fb.sz.y/16};
	u32 EyeballRadius = fb.sz.x/32;
	Vec2 EyeballRightOrigin = EyeballLeftOrigin;
	Pixel EyeballColor = 0x00007F;
	EyeballRightOrigin.x = fb.sz.x - EyeballLeftOrigin.x;
	int dir = 1;

	clock_gettime(CLOCK_MONOTONIC, &dt);
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

		XPutImage(wp->disp, wp->win, DefaultGC(wp->disp, DefaultScreen(wp->disp)), &wp->img, 0, 0, 0, 0, fb.sz.x, fb.sz.y);

		get_time_diff(CLOCK_MONOTONIC, &dt);
		get_timespec_diff(&dt, FRAME_NS);
		// printf("0.%09ld s\r", dt.tv_nsec);
		// fflush(stdout);

		nanosleep(&dt, NULL);
		clock_gettime(CLOCK_MONOTONIC, &dt);

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
}

int main() {
	enum win_width_e { WIDTH = 640 };
	enum win_height_e { HEIGHT = 480 };

	static Pixel fbufdata[HEIGHT*WIDTH] = {0};

	Fbuf fb = { { WIDTH, HEIGHT }, fbufdata };

	WinProps_X wp = window_init_x(fb);
	if(wp.status == ERR_SUCCESS)
		draw(fb, &wp);

	window_cleanup_x(&wp);
	render_to_ppm(fb);

	return wp.status;
}

