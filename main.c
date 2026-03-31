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

typedef struct i32x2_t {
	int32_t n0, n1;
} i32x2;

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

typedef struct uvec3b_t {
	u32 b0, b1, b2;
} UVec3B;

typedef struct triangle_t {
	Vec2 r0, r1, r2;
} Triangle;

typedef struct vec3_pixel_t {
	Pixel p0, p1, p2;
} Vec3Pixel;

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

/* Integer operations */
static inline
u32 i32restrict0(i32 a, u32 max) {
	return a > 0 ? (a < (i32)max ? a : (i32)max-1) : 0;
}

static inline
u64 i32square(i32 x) {
	return (u64)((i64)x*(i64)x);
}

/* Vector Operations */
static inline
i32 vec2det(Vec2 v0, Vec2 v1) {
	return v0.x*v1.y - v1.x*v0.y;
}

static inline
Vec2 vec2sub(Vec2 r0, Vec2 r1) {
	return (Vec2) {r0.x - r1.x, r0.y - r1.y};
}

static inline
Vec2 uv2v2sub(UVec2 r0, Vec2 r1) {
	return (Vec2) {(i32)r0.x - r1.x, (i32)r0.y - r1.y};
}

/* Linear interpolation functions */
static
u8 u8lerp(UVec3B B, u8 x0, u8 x1, u8 x2) {
	return (
		(u64)x0*(u64)B.b0 +
		(u64)x1*(u64)B.b1 +
		(u64)x2*(u64)B.b2
	) >> 31;
}

static
Pixel lerp(UVec3B B, Vec3Pixel P) {
	return argb_to_pixel(
		u8lerp(B, pixA(P.p0), pixA(P.p1), pixA(P.p2)),
		u8lerp(B, pixR(P.p0), pixR(P.p1), pixR(P.p2)),
		u8lerp(B, pixG(P.p0), pixG(P.p1), pixG(P.p2)),
		u8lerp(B, pixB(P.p0), pixB(P.p1), pixB(P.p2))
	);
}

/* Drawing functions */
UVec2x2 rect_bound_uvec2x2(Fbuf fb, Rect S) {
	UVec2x2 S_;
	S_.r0.x = S.r0.x < 0 ? 0 : S.r0.x;
	S_.r0.y = S.r0.y < 0 ? 0 : S.r0.y;

	S.r0.x += S.sz.x;
	S.r0.y += S.sz.y;

	S_.r1.x = S.r0.x < 0 ? 0 : S.r0.x;
	S_.r1.y = S.r0.y < 0 ? 0 : S.r0.y;

	S_.r1.x = S_.r1.x > fb.sz.x ? fb.sz.x : S_.r1.x;
	S_.r1.y = S_.r1.y > fb.sz.y ? fb.sz.y : S_.r1.y;

	return S_;
}

void fb_draw_rect_uvec2x2(Fbuf fb, UVec2x2 S, Pixel p) {
	UVec2 r;
	for(r.y = S.r0.y; r.y < S.r1.y; ++r.y)
		for(r.x = S.r0.x; r.x < S.r1.x; ++r.x)
			fb_set_pix(fb, r, p);
}

void fb_draw_rect(Fbuf fb, Rect S, Pixel p) {
	fb_draw_rect_uvec2x2(fb, rect_bound_uvec2x2(fb, S), p);
}

void fb_draw_circle(Fbuf fb, Circle S, Pixel p) {
	UVec2 r;
	for(r.y = 0; r.y < fb.sz.y; ++r.y)
		for(r.x = 0; r.x < fb.sz.x; ++r.x)
			if(i32square(r.y - S.r0.y) + i32square(r.x - S.r0.x) < i32square(S.R))
				fb_set_pix(fb, r, p);
}

bool checkptstatus(i32 D, i32 D1, i32 D2) {
	if(D < 0) D = -D, D1 = -D1, D2 = -D2;
	return (D1 < 0 || D2 < 0 || (u32)D1 + (u32)D2 > (u32)D)
		? false : true ;
}

UVec3B getlerpweights(i32 D, i32 D1, i32 D2) {
	UVec3B B;
	B.b1 = ((i64)D1 << 31) / (i64)D;
	B.b2 = ((i64)D2 << 31) / (i64)D;
	B.b0 = (1 << 31) - B.b1 - B.b2;

	return B;
}

static
bool checkptstatus_dr(i32 D, Vec2 dr, Vec2 dr1, Vec2 dr2) {
	return checkptstatus(D, vec2det(dr1, dr), vec2det(dr, dr2));
}

static
void fb_set_pix_lerped(Fbuf fb, UVec2 r, Vec3Pixel P, i32 D, i32 D1, i32 D2) {
	if(checkptstatus(D, D1, D2))
		fb_set_pix(fb, r, lerp(getlerpweights(D, D1, D2), P));
}

static
void fb_set_pix_lerped_dr(Fbuf fb, UVec2 r, Vec3Pixel P, i32 D, Vec2 dr, Vec2 dr1, Vec2 dr2) {
	fb_set_pix_lerped(fb, r, P, D, vec2det(dr1, dr), vec2det(dr, dr2));
}

void fb_draw_triangle(Fbuf fb, Triangle S, Pixel p) {
	S.r1 = vec2sub(S.r1, S.r0);
	S.r2 = vec2sub(S.r2, S.r0);
	i32 D = vec2det(S.r1, S.r2);
	if(!D) return;

	UVec2 r;
	for(r.y = 0; r.y < fb.sz.y; ++r.y)
		for(r.x = 0; r.x < fb.sz.x; ++r.x)
			if(checkptstatus_dr(D, uv2v2sub(r, S.r0), S.r1, S.r2))
				fb_set_pix(fb, r, p);
}

void fb_draw_triangle_lerped(Fbuf fb, Triangle S, Vec3Pixel P) {
	S.r1 = vec2sub(S.r1, S.r0);
	S.r2 = vec2sub(S.r2, S.r0);
	i32 D = vec2det(S.r1, S.r2);
	if(!D) return;

	UVec2 r;
	for(r.y = 0; r.y < fb.sz.y; ++r.y)
		for(r.x = 0; r.x < fb.sz.x; ++r.x)
			fb_set_pix_lerped_dr(fb, r, P, D, uv2v2sub(r, S.r0), S.r1, S.r2);
}

static
Rect fb_mirror_rect_x(Fbuf fb, Rect R) {
	return (Rect) {
		{ (i32)fb.sz.x - R.r0.x - R.sz.x, R.r0.y },
		R.sz
	};
}

void fb_draw_parabola_bounded(Fbuf fb, Rect bound, Vec2 origin, i32 a, Pixel p) {
	UVec2 r;
	for(r.y = bound.r0.y; r.y < bound.r0.y + bound.sz.y; ++r.y)
		for(r.x = bound.r0.x; r.x < bound.r0.x + bound.sz.x; ++r.x)
			if ((i64)a*((i64)r.y - (i64)origin.y) > (i64)i32square(r.x - origin.x))
				fb_set_pix(fb, r, p);
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
	int dir = 1;
	while(!handle_events_x(disp, win, evmask)) {
		// Eye motion logic
		fb_draw_circle(fb, EyeballLeft, 0x00FF00);
		fb_draw_circle(fb, EyeballRight, 0x00FF00);
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
		/* the processing gap between the event handler and XPutImage means *
		 * that this may be called before window destruction is detected */
		XPutImage(disp, win, DefaultGC(disp, DefaultScreen(disp)), img, 0, 0, 0, 0, fb.sz.x, fb.sz.y);

		nanosleep(&dt, NULL);
	}
}

int crwin_x(
	Window *win, XWindowAttributes *attrs,
	Display *disp,
	unsigned long width, unsigned long height
) {
	long evmask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask;
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

int render_to_x_win(
	Fbuf fb, Display *disp, Window win,
	XWindowAttributes attrs
) {
	XImage *img = XCreateImage(
		disp, attrs.visual, attrs.depth, ZPixmap, 0,
		calloc(fb.sz.y*fb.sz.x, 4),
		fb.sz.x, fb.sz.y, 32, 0
	);
	XInitImage(img);
	if(
		   img->bits_per_pixel != 32
		|| img->red_mask != 0xFF0000
		|| img->blue_mask != 0xFF
		|| img->green_mask != 0xFF00
	)
		return fprintf(stderr, "Invalid image format!\n"), -1;

	render_to_x_win_img(fb, disp, win, img, attrs.your_event_mask);

	free(img->data);
	return 0;
}

int render_to_x_disp(Fbuf fb, Display *disp) {
	Window win;
	XWindowAttributes attrs;

	if(crwin_x(&win, &attrs, disp, fb.sz.x, fb.sz.y))
		return -1;
	return render_to_x_win(fb, disp, win, attrs);
}

int render_to_x(Fbuf fb) {
	Display *disp = XOpenDisplay(NULL);

	if(!disp)
		return fprintf(stderr, "Unable to open display!\n"), -1;

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
	Pixel EyeClr = 0x00FF00;
	fb_draw_rect(fb, EyeLeft, EyeClr);

	Rect EyeRight = fb_mirror_rect_x(fb, EyeLeft);
	fb_draw_rect(fb, EyeRight, EyeClr);

	Rect SmileBound = {
		{0, fb.sz.y/2},
		{fb.sz.x, fb.sz.y/3}
	};

	Vec2 SmileOrigin = {fb.sz.x/2, 5*fb.sz.y/6};
	Pixel SmileClr = 0xFF0000;

	fb_draw_parabola_bounded(fb, SmileBound, SmileOrigin, -256, SmileClr);

	Triangle Nose = {
		{fb.sz.x/2, fb.sz.y/3},
		{13*fb.sz.x/32, 7*fb.sz.y/16},
		{19*fb.sz.x/32, 7*fb.sz.y/16},
	};
	Vec3Pixel NoseColors = {
		0xFFFF00,
		0xFF7F3F,
		0x7FFF3F,
	};

	fb_draw_triangle_lerped(fb, Nose, NoseColors);

	Rect outofboundrect = {
		{ -200, -300 },
		{ 100, 200 }
	};

	fb_draw_rect(fb, outofboundrect, 0xFF00FF);

	Rect Goatee = {
		{ 7*fb.sz.x/16, 7*fb.sz.y/8 },
		{ fb.sz.x/8, fb.sz.y/2 }
	};
	fb_draw_rect(fb, Goatee, 0xFFFFFF);

	Triangle BrowLeft = {
		{ 5*fb.sz.x/32, (i32)fb.sz.y/12 },
		{ fb.sz.x/32, -1*(i32)fb.sz.y/24 },
		{ 9*fb.sz.x/32, -1*(i32)fb.sz.y/24 },
	}, BrowRight = BrowLeft;
	BrowRight.r0.x = fb.sz.x - BrowLeft.r0.x;
	BrowRight.r1.x = fb.sz.x - BrowLeft.r1.x;
	BrowRight.r2.x = fb.sz.x - BrowLeft.r2.x;

	Pixel BrowColor = 0x7F3F00;
	fb_draw_triangle(fb, BrowLeft, BrowColor);
	fb_draw_triangle(fb, BrowRight, BrowColor);
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
