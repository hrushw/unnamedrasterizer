#include "main.c"

/* Main drawing function */
static
void draw(Fbuf fb, WinProps_X *wp) {
	enum { FRAME_NS = 16666667 };
	DrawClock clk = drawclock_init((struct timespec) {0, FRAME_NS});
	while(!wp->closed) {
		fb_clear(fb, 0xFF0000);

		XPutImage(wp->disp, wp->win, DefaultGC(wp->disp, DefaultScreen(wp->disp)), &wp->img, 0, 0, 0, 0, fb.sz.x, fb.sz.y);

		drawclock_wait(&clk);
		handle_events_x(wp);
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

	wp.img.data = NULL;
	XCloseDisplay(wp.disp);
	render_to_ppm(fopen("test00.ppm", "wb"), fb);

	return wp.status;
}
