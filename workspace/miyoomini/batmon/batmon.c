#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/input.h>
#include <pthread.h>

static int is_charging = 1;
static int screen_on = 0;
static unsigned long screen_start;
void screenOn(void) {
	screen_start = SDL_GetTicks();
	if (!screen_on) system("echo 100 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle");
	screen_on = 1;
}
void screenOff(void) {
	system("echo 0 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle");
	screen_on = 0;
}
void checkCharging(void) {
	int i = 0;
	FILE *file = fopen("/sys/devices/gpiochip0/gpio/gpio59/value", "r");
	if (file!=NULL) {
		fscanf(file, "%i", &i);
		fclose(file);
	}
	is_charging = i;
}
static pthread_t charging_pt;
void* chargingThread(void* arg) {
	while (1) {
		sleep(1);
		checkCharging();
	}
}

#define	BUTTON_L1		KEY_E
#define	BUTTON_R1		KEY_T
#define	BUTTON_L2		KEY_TAB
#define	BUTTON_R2		KEY_BACKSPACE

static int launch = 0;
static int input_fd;
static pthread_t input_pt;
void* inputThread(void* arg) {
	struct input_event	event;
	uint32_t L_pressed = 0;
	uint32_t R_pressed = 0;
	while (read(input_fd, &event, sizeof(event))==sizeof(event)) {
		if (event.type!=EV_KEY || event.value>1) continue;
		if (event.type==EV_KEY) screenOn();
		
		if (event.code==BUTTON_L1) L_pressed = event.value;
		else if (event.code==BUTTON_R1) R_pressed = event.value;
		else if (event.code==BUTTON_L2 || event.code==BUTTON_R2) continue;
		else if (L_pressed && R_pressed && event.value>0) {
			launch = 1;
			// is_charging = 0; // not necessary anymore
		}
	}	
}

int main(void) {
	int fb0_fd = open("/dev/fb0", O_RDWR);
	struct fb_var_screeninfo vinfo;
	ioctl(fb0_fd, FBIOGET_VSCREENINFO, &vinfo);
	int map_size = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8); // 640x480x4
	// fprintf(stdout, "fb width:%d height:%d bpp:%d size:%d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, map_size);
	char* fb0_map = (char*)mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb0_fd, 0);
	
	memset(fb0_map, 0, map_size); // clear screen
	
	char charging_path[128];
	sprintf(charging_path, "%s/.system/shared/images/640x480/charging.png", getenv("SDCARD_PATH"));
	SDL_Surface* img = IMG_Load(charging_path); // 24-bit opaque png
	// SDL_PixelFormat* fmt = img->format;
	// fprintf(stdout, "img width:%d height:%d bpp:%d pitch:%d\n", img->w, img->h, fmt->BitsPerPixel, img->pitch);
	// fprintf(stdout, "img rmask:%04x gmask:%04x bmask:%04x amask:%04x\n", fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
	
	uint8_t* dst = (uint8_t*)fb0_map; // rgba
	uint8_t* src = (uint8_t*)img->pixels; // bgr
	src += ((img->h * img->w) - 1) * 3;
	for (int y=0; y<img->h; y++) {
		for (int x=0; x<img->w; x++) {
			*(dst+0) = *(src+2); // r
			*(dst+1) = *(src+1); // g
			*(dst+2) = *(src+0); // b
			*(dst+3) = 0xf; // alpha
			dst += 4;
			src -= 3;
		}
	}
	SDL_FreeSurface(img);
	
	screenOn();
	
	input_fd = open("/dev/input/event0", O_RDONLY);
	pthread_create(&input_pt, NULL, &inputThread, NULL);
	
	pthread_create(&charging_pt, NULL, &chargingThread, NULL);
	
	while (!launch && is_charging) {
		if (screen_on && SDL_GetTicks()-screen_start>=3000) screenOff();
	}
	
	close(input_fd);
	
	pthread_cancel(input_pt);
	pthread_join(input_pt, NULL);
	
	pthread_cancel(charging_pt);
	pthread_join(charging_pt, NULL);

	munmap(fb0_map, map_size);
	close(fb0_fd);
	
	if (!launch) system("reboot");
	
    return EXIT_SUCCESS;
}
