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

static inline
i64 uv2v2det(UVec2 v0, Vec2 v1) {
	return (i64)v0.x*(i64)v1.y - (i64)v1.x*(i64)v0.y;
}

i64 v2uv2det(Vec2 v0, UVec2 v1) {
	return (i64)v0.x*(i64)v1.y - (i64)v1.x*(i64)v0.y;
}

/* Drawing functions */
static
void fb_clear(Fbuf fb, Pixel p) {
	for(u64 i = 0; i < (fb.sz.x * fb.sz.y); ++i)
		fb.buf[i] = p;
}

static inline
u32 i32min0(i32 a) {
	return a > 0 ? a : 0;
}

static inline
u32 i32minmax0(i32 a, u32 max) {
	return a > (i32)max ? max : i32min0(a);
}

static
void fb_draw_rect(Fbuf fb, Pixel p, Vec2 r0, UVec2 sz) {
	UVec2 r;
	for(r.y = i32min0(r0.y); r.y < i32minmax0(r0.y + sz.y, fb.sz.y); ++r.y)
		for(r.x = i32min0(r0.x); r.x < i32minmax0(r0.x + sz.x, fb.sz.x); ++r.x)
			fb_set_pix(fb, r, p);
}

static inline
u64 i32square(i32 x) {
	return (u64)((i64)x*(i64)x);
}

static inline
u32 u32min(u32 a, u32 b) {
	return a < b ? a : b;
}

static
void fb_draw_circle(Fbuf fb, Pixel p, u32 R, Vec2 r0) {
	UVec2 r;
	for(r.y = i32min0(r0.y - (i32)R); r.y < u32min(fb.sz.y, (u32)r0.y + R); ++r.y)
		for(r.x = i32min0(r0.x - (i32)R); r.x < u32min(fb.sz.x, (u32)r0.x + R); ++r.x)
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
Vec2 vec2sub(Vec2 r1, Vec2 r0) {
	return (Vec2) { r1.x - r0.x, r1.y - r0.y };
}

static
Vec2 i32minmax_3_partial(i32 a, i32 b, i32 c) {
	return b < c
		? (Vec2) {a, c}
		: (a < c) ? (Vec2) {a, b} : (Vec2) {c, b};
}

static
Vec2 i32minmax_3(i32 a, i32 b, i32 c) {
	return a < b
		? i32minmax_3_partial(a, b, c)
		: i32minmax_3_partial(b, a, c);
}

static
UVec2x2 triangle_bound(UVec2 sz, Vec2 r0, Vec2 r1, Vec2 r2) {
	Vec2 xmm = i32minmax_3(r0.x, r1.x, r2.x);
	Vec2 ymm = i32minmax_3(r0.y, r1.y, r2.y);
	return (UVec2x2) {
		{ i32min0(xmm.x), i32min0(ymm.x) },
		{ i32minmax0(sz.x, xmm.y+1), i32minmax0(sz.y, ymm.y+1) }
	};
}

static
void fb_draw_triangle_bounded(Fbuf fb, Pixel p, UVec2 B0, UVec2 B1, Vec2 r0, Vec2 dr1, Vec2 dr2) {
	i64 D = vec2det(dr1, dr2),
		D1 = uv2v2det(B0, dr2) - vec2det(r0, dr2),
		D2 = v2uv2det(dr1, B0) - vec2det(dr1, r0);
	// if(!D) return;

	B1.x -= B0.x;
	dr1.x += B1.x*dr1.y;
	dr2.x += B1.x*dr2.y;
	B1.x += B0.x;

	UVec2 r;
	for(r.y = B0.y; r.y < B1.y; ++r.y, D1 -= dr2.x, D2 += dr1.x)
		for(r.x = B0.x; r.x < B1.x; ++r.x, D1 += dr2.y, D2 -= dr1.y)
			if(!checkptstatus(D, D1, D2))
				fb_set_pix(fb, r, p);
}

// TODO bounds checking
static
void fb_draw_triangle(Fbuf fb, Pixel p, Vec2 r0, Vec2 r1, Vec2 r2) {
	UVec2x2 bound = triangle_bound(fb.sz, r0, r1, r2);
	fb_draw_triangle_bounded(fb, p, bound.r0, bound.r1, r0, vec2sub(r1, r0), vec2sub(r2, r0));
}

static inline
void fb_draw_triangle_arr(Fbuf fb, Pixel p, Triangle S) {
	fb_draw_triangle(fb, p, S[0], S[1], S[2]);
}

static inline
void fb_draw_quad(Fbuf fb, Pixel p, Quad q) {
	fb_draw_triangle(fb, p, q[0], q[1], q[2]);
	fb_draw_triangle(fb, p, q[0], q[3], q[2]);
}

static
void fb_draw_triangles(Fbuf fb, Pixel p, u32 n, Vec2 *pts) {
	for(u32 i = 0; i < n-2; i += 3)
		fb_draw_triangle(fb, p, pts[i], pts[i+1], pts[i+2]);
}

static
void fb_draw_triangles_indexed(Fbuf fb, Pixel p, u32 n, u32 *I, Vec2 *pts) {
	for(u32 i = 0; i < n-2; i += 3)
		fb_draw_triangle(fb, p, pts[I[i]], pts[I[i+1]], pts[I[i+2]]);
}

static
void fb_draw_polygon_strip(Fbuf fb, Pixel p, u32 n, Vec2 *pts) {
	for(u32 i = 0; i < n-2; ++i)
		fb_draw_triangle(fb, p, pts[i], pts[i+1], pts[i+2]);
}

static
void fb_draw_polygon_fan(Fbuf fb, Pixel p, u32 n, Vec2 *pts) {
	for(u32 i = 0; i < n-2; ++i)
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
	WinError_X status;
	Display *disp;
	Window win;
	XWindowAttributes attrs;
	XImage img;
} WinProps_X;

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
	WinProps_X wp;
	XSetErrorHandler(handle_errors_x);
	wp.disp = XOpenDisplay(NULL);
	if(!wp.disp)
		return wp.status = ERR_OPEN_DISPLAY, wp;

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

typedef struct timespec Timespec;

static
Timespec get_timespec_cur(clockid_t clk) {
	Timespec ts;
	clock_gettime(clk, &ts);
	return ts;
}

static
Timespec get_timespec_diff(Timespec t1, Timespec t0) {
	return (t1.tv_nsec < t0.tv_nsec)
		? (t1.tv_sec > t0.tv_sec)
			? (Timespec) { t1.tv_sec - t0.tv_sec - 1, t1.tv_nsec + 1000000000 - t0.tv_nsec }
			: (Timespec) { 0, 0 }
		: (t1.tv_sec >= t0.tv_sec)
			? (Timespec) { t1.tv_sec - t0.tv_sec, t1.tv_nsec - t0.tv_nsec }
			: (Timespec) { 0, 0 };
}

typedef struct drawclock_t {
	Timespec start, cur, rel, diff, interval;
} DrawClock;

static
DrawClock drawclock_init(Timespec dt) {
	Timespec t = get_timespec_cur(CLOCK_MONOTONIC);
	return (DrawClock) {
		.start = t,
		.cur = t,
		.rel = {0, 0},
		.diff = {0, 0},
		.interval = dt,
	};
}

// TODO somehow allow vsync?
static
void drawclock_wait(DrawClock *clk) {
	clk->diff = get_timespec_diff(get_timespec_cur(CLOCK_MONOTONIC), clk->cur);
	clk->diff = get_timespec_diff(clk->interval, clk->diff);
	nanosleep(&clk->diff, NULL);

	clk->cur = get_timespec_cur(CLOCK_MONOTONIC);
	clk->rel = get_timespec_diff(clk->cur, clk->start);
}

void handle_events_x(WinProps_X *wp) {
	XEvent ev;
	while(XCheckWindowEvent(wp->disp, wp->win, wp->attrs.your_event_mask, &ev))
		switch(ev.type) {
			case DestroyNotify:
				wp->closed = 1; return;
			case Expose: break;
			case KeyPress:
				printf("Key press detected!\n");
				break;
			case KeyRelease:
				printf("Key release detected!\n");
				break;
			default: break;
		}
}

const u64 font_H = 0x424242427E424242;
const u64 font_E = 0x7E0202021E02027E;
const u64 font_L = 0x7E02020202020202;
const u64 font_O = 0x3C4242424242423C;

const u64 font_W = 0x42667E5A42424242;
const u64 font_R = 0x4262321E3E42423E;
const u64 font_D = 0x1E2242424242221E;

const u64 font_excl = 0x1818001818181818;
const u64 font_blank = 0x0000000000000000;

static
void fb_draw_u64_bitmap(Fbuf fb, Pixel p, UVec2 r, u64 bmp) {
	for(u8 i = 0; i < 64; ++i)
		if((bmp >> i) & 1)
			fb_set_pix(fb, (UVec2) {r.x + i%8, r.y + i/8}, p);
}

static
void fb_draw_u64_bitmap_scaled(Fbuf fb, Pixel p, u32 scale, UVec2 r, u64 bmp) {
	for(u8 i = 0; i < 64; ++i)
		if((bmp >> i) & 1)
			for(u32 y = 0; y < scale; ++y)
				for(u32 x = 0; x < scale; ++x)
					fb_set_pix(fb, (UVec2) {r.x + (i%8)*scale + x, r.y + (i/8)*scale + y}, p);
}

static
void fb_draw_u64_bitmaps_x(Fbuf fb, Pixel p, UVec2 r, u32 n, u64 *bmps) {
	if(r.x + n*8 > fb.sz.x) return;

	for(u32 i = 0; i < n; ++i, r.x += 8)
		fb_draw_u64_bitmap(fb, p, r, bmps[i]);
}

static
void fb_draw_u64_bitmaps_x_scaled(Fbuf fb, Pixel p, u32 scale, UVec2 r, u32 n, u64 *bmps) {
	if(r.x + scale*n*8 > fb.sz.x) return;

	for(u32 i = 0; i < n; ++i, r.x += 8*scale)
		fb_draw_u64_bitmap_scaled(fb, p, scale, r, bmps[i]);
}

/* Main drawing function */
static
void draw(Fbuf fb, WinProps_X *wp) {
	Rect EyeLeft = {
		{ fb.sz.x/32, 10*fb.sz.y/48 },
		{ fb.sz.x/4, 10*fb.sz.y/48 },
	};

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

	Vec2 Nose[] = {
		{7*fb.sz.x/16, 3*fb.sz.y/8},
		{15*fb.sz.x/32, 7*fb.sz.y/16},
		{fb.sz.x/2, fb.sz.y/3},
		{17*fb.sz.x/32, 7*fb.sz.y/16},
		{9*fb.sz.x/16, 3*fb.sz.y/8},
	};

	Vec2 Teeth[] = {
		{ 7*fb.sz.x/32, fb.sz.y/2},
		{17*fb.sz.x/64, 5*fb.sz.y/8},
		{10*fb.sz.x/32, fb.sz.y/2},
		{23*fb.sz.x/64, 5*fb.sz.y/8},
		{13*fb.sz.x/32, fb.sz.y/2},
		{29*fb.sz.x/64, 5*fb.sz.y/8},
		{16*fb.sz.x/32, fb.sz.y/2},
		{35*fb.sz.x/64, 5*fb.sz.y/8},
		{19*fb.sz.x/32, fb.sz.y/2},
		{41*fb.sz.x/64, 5*fb.sz.y/8},
		{22*fb.sz.x/32, fb.sz.y/2},
		{47*fb.sz.x/64, 5*fb.sz.y/8},
		{25*fb.sz.x/32, fb.sz.y/2},
	};
	u32 ToothIndices[] = {
		0, 1, 2,
		2, 3, 4,
		4, 5, 6,
		6, 7, 8,
		8, 9, 10,
		10, 11, 12
	};

	Rect Goatee = {
		{ 7*fb.sz.x/16, 7*fb.sz.y/8 },
		{ fb.sz.x/8, fb.sz.y/2 }
	};

	Vec2 Brows[] = {
		{ 5*fb.sz.x/32, (i32)fb.sz.y/12 },
		{ fb.sz.x/32, - ((i32)fb.sz.y/24) },
		{ 9*fb.sz.x/32, - ((i32)fb.sz.y/24) },

		{ 27*fb.sz.x/32, (i32)fb.sz.y/12 },
		{ 31*fb.sz.x/32, - (i32)fb.sz.y/24 },
		{ 23*fb.sz.x/32, - (i32)fb.sz.y/24 },
	};

	UVec2 textr0 = { fb.sz.x/4, fb.sz.x/10};
	u64 txt[] = {
		font_H, font_E, font_L, font_L, font_O,
		font_blank,
		font_W, font_O, font_R, font_L, font_D,
		font_excl
	};

	Vec2 EyeballLeftPos = {   fb.sz.x/16, 5*fb.sz.y/16},
		EyeballRightPos = {15*fb.sz.x/16, 5*fb.sz.y/16};
	u32 EyeballRadius = fb.sz.x/32;
	i32 EyeLength = EyeLeft.sz.x - 2*EyeballRadius,
		tm = 0, posx = 0;

	enum { FRAME_NS = 16666667 };
	DrawClock clk = drawclock_init((struct timespec) {0, FRAME_NS});

	while(!wp->closed) {
		fb_clear(fb, 0);

		fb_draw_rect(fb, 0x00FF00, EyeLeft.r0, EyeLeft.sz);
		fb_draw_quad(fb, 0x00CF3F, EyeRight);
		fb_draw_polygon_fan(fb, 0xFF0000, sizeof(Smilepts)/sizeof(Vec2), Smilepts);
		fb_draw_polygon_strip(fb, 0xFFFF00, sizeof(Nose)/sizeof(Vec2), Nose);
		fb_draw_triangles_indexed(fb, 0xEFEFCF, sizeof(ToothIndices)/sizeof(u32), ToothIndices, Teeth);
		fb_draw_rect(fb, 0xFF00FF, (Vec2) {-200, -300}, (UVec2) {100, 200});
		fb_draw_rect(fb, 0xFFFFFF, Goatee.r0, Goatee.sz);
		fb_draw_triangles(fb, 0x7F3F00, 6, Brows);

		fb_draw_u64_bitmaps_x(fb, 0xFF00FF, (UVec2){textr0.x - 20, textr0.y - 20}, sizeof(txt)/sizeof(u64), txt);
		fb_draw_u64_bitmaps_x_scaled(fb, 0xFF00FF, 3, textr0, sizeof(txt)/sizeof(u64), txt);

		// get time in milliseconds
		EyeballLeftPos.x = EyeLeft.r0.x + EyeballRadius + posx;
		EyeballRightPos.x = fb.sz.x - EyeballLeftPos.x;

		Pixel EyeballColor = 0x00007F;
		fb_draw_circle(fb, EyeballColor, EyeballRadius, EyeballLeftPos);
		fb_draw_circle(fb, EyeballColor, EyeballRadius, EyeballRightPos);

		XPutImage(wp->disp, wp->win, DefaultGC(wp->disp, DefaultScreen(wp->disp)), &wp->img, 0, 0, 0, 0, fb.sz.x, fb.sz.y);

		enum { PERIOD = 1200 };
		drawclock_wait(&clk);
		tm = 1000*clk.rel.tv_sec + (clk.rel.tv_nsec / 1000000);
		posx = (((i64)tm % PERIOD) * EyeLength) / PERIOD;
		if((tm % (2*PERIOD)) > PERIOD) posx = EyeLength - posx;

		printf("%ld.%09ld : 0.%09ld s\r", clk.rel.tv_sec, clk.rel.tv_nsec, clk.diff.tv_nsec);
		fflush(stdout);

		handle_events_x(wp);
	}
	printf("\n");
}

int main() {
	enum win_width_e { WIDTH = 640 };
	enum win_height_e { HEIGHT = 480 };

	static Pixel fbufdata[HEIGHT*WIDTH] = {0};

	Fbuf fb = { { WIDTH, HEIGHT }, fbufdata };

	WinProps_X wp = window_init_x(fb);
	if(wp.status == ERR_SUCCESS)
		draw(fb, &wp);

	XCloseDisplay(wp.disp);
	render_to_ppm(fb);

	return wp.status;
}

