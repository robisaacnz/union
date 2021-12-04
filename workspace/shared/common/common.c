#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#include "common.h"
#include "platform.h"

// TODO: move any functions not also used by libmmenu out of here
// TODO: one wrinkle, functions that require static variables defined in here...

// TODO: standardize function casing, eg. Object_methodName vs function_name vs functionName
// NOTE: currently thinking it should be Object_methodName() and functionName() but instance->property_name and variable_name
// NOTE: constants can be CONSTANT_NAME or kConstantName, not sure what distinguishes the two types...
// NOTE: CONSTANT_NAME matches C and feels lower level, kConstantName is recognized by my IDE's C syntax coloring :sweat_smile:

///////////////////////////////////////

struct ButtonState {
	int is_pressed;
	int just_pressed;
	int just_repeated;
	int just_released;
};
static struct ButtonState buttons[kButtonCount];

void Input_reset(void) {
	// reset all
	for (int i=0; i<kButtonCount; i++) {
		buttons[i].just_pressed = 0;
		buttons[i].is_pressed = 0;
		buttons[i].just_repeated = 0;
		buttons[i].just_released = 0;
	}
}
void Input_poll(void) {
	// reset transient values
	for (int i=0; i<kButtonCount; i++) {
		buttons[i].just_pressed = 0;
		buttons[i].just_repeated = 0;
		buttons[i].just_released = 0;
	}
	
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		ButtonIndex i;
		if (event.type==SDL_KEYDOWN || event.type==SDL_KEYUP) {
			SDLKey key = event.key.keysym.sym;
			// NOTE: can't use switch because all missing buttons have the same value
				 if (key==MINUI_A) 		i = kButtonA;
			else if (key==MINUI_B) 		i = kButtonB;
			else if (key==MINUI_X) 		i = kButtonX;
			else if (key==MINUI_Y) 		i = kButtonY;
			else if (key==MINUI_START) 	i = kButtonStart;
			else if (key==MINUI_SELECT)	i = kButtonSelect;
			else if (key==MINUI_UP)		i = kButtonUp;
			else if (key==MINUI_DOWN)	i = kButtonDown;
			else if (key==MINUI_LEFT)	i = kButtonLeft;
			else if (key==MINUI_RIGHT)	i = kButtonRight;
			else if (key==MINUI_L)		i = kButtonL;
			else if (key==MINUI_R)		i = kButtonR;
			else if (key==MINUI_MENU)	i = kButtonMenu;
			else if (key==MINUI_L2)		i = kButtonL2;
			else if (key==MINUI_R2)		i = kButtonR2;
			else if (key==MINUI_L3)		i = kButtonL3;
			else if (key==MINUI_R3)		i = kButtonR3;
			else if (key==MINUI_POWER)	i = kButtonPower;
			else continue;
			
			if (event.type==SDL_KEYDOWN) {
				buttons[i].just_repeated = 1;
				if (!buttons[i].is_pressed) {
					buttons[i].just_pressed = 1;
					buttons[i].is_pressed = 1;
				}
			}
			else {
				buttons[i].is_pressed = 0;
				buttons[i].just_released = 1;
			}
		}
	}
}
int Input_anyPressed(void) {
	for (int i=0; i<kButtonCount; i++) {
		if (buttons[i].is_pressed) return 1;
	}
	return 0;
}
int Input_justPressed(ButtonIndex btn) 	{ return  buttons[btn].just_pressed; }
int Input_justRepeated(ButtonIndex btn) { return  buttons[btn].just_repeated; }
int Input_isPressed(ButtonIndex btn) 	{ return  buttons[btn].is_pressed; }
int Input_justReleased(ButtonIndex btn) { return  buttons[btn].just_released; }

///////////////////////////////////////

int prefixMatch(char* pre, char* str) {
	return (strncasecmp(pre,str,strlen(pre))==0);
}
int suffixMatch(char* suf, char* str) {
	int len = strlen(suf);
	int offset = strlen(str)-len;
	return (offset>=0 && strncasecmp(suf, str+offset, len)==0);
}
int exactMatch(char* str1, char* str2) {
	int len1 = strlen(str1);
	if (len1!=strlen(str2)) return 0;
	return  (strncmp(str1,str2,len1)==0);
}
int hide(char* file_name) {
	if (file_name[0]=='.') return 1;
	return 0;
}

void getDisplayName(const char* in_name, char* out_name) {
	char* tmp;
	strcpy(out_name, in_name);
	
	// extract just the filename if necessary
	tmp = strrchr(in_name, '/');
	if (tmp) strcpy(out_name, tmp+1);
	
	// remove extension
	tmp = strrchr(out_name, '.');
	if (tmp) tmp[0] = '\0';
	
	// remove trailing parens (round and square)
	char safe_name[256];
	strcpy(safe_name,out_name);
	while ((tmp=strrchr(out_name, '('))!=NULL || (tmp=strrchr(out_name, '['))!=NULL) {
		tmp[0] = '\0';
		tmp = out_name;
	}
	
	// make sure we haven't nuked the entire name
	if (out_name[0]=='\0') strcpy(out_name, safe_name);
	
	// remove trailing whitespace
	tmp = out_name + strlen(out_name) - 1;
    while(tmp>out_name && isspace((unsigned char)*tmp)) tmp--;
    tmp[1] = '\0';
}
void getEmuName(const char* in_name, char* out_name) {
	char* tmp;
	strcpy(out_name, in_name);
	tmp = out_name;
	
	// extract just the Roms folder name if necessary
	if (prefixMatch(kRomPath, tmp)) {
		tmp += strlen(kRomPath) + 1;
		char* tmp2 = strchr(tmp, '/');
		if (tmp2) tmp2[0] = '\0';
	}

	// finally extract pak name from parenths if present
	tmp = strchr(tmp, '(');
	if (tmp) {
		tmp += 1;
		strcpy(out_name, tmp);
		tmp = strchr(out_name,')');
		tmp[0] = '\0';
	}
}

void normalizeNewline(char* line) {
	int len = strlen(line);
	if (len>1 && line[len-1]=='\n' && line[len-2]=='\r') { // windows!
		line[len-2] = '\n';
		line[len-1] = '\0';
	}
}
void trimTrailingNewlines(char* line) {
	int len = strlen(line);
	while (len>0 && line[len-1]=='\n') {
		line[len-1] = '\0'; // trim newline
		len -= 1;
	}
}

///////////////////////////////////////

int exists(char* path) {
	return access(path, F_OK)==0;
}
void putFile(char* path, char* contents) {
	FILE* file = fopen(path, "w");
	if (file) {
		fputs(contents, file);
		fclose(file);
	}
}
void getFile(char* path, char* buffer) {
	FILE *file = fopen(path, "r");
	if (file) {
		fseek(file, 0L, SEEK_END);
		size_t size = ftell(file);
		rewind(file);
		fread(buffer, sizeof(char), size, file);
		fclose(file);
		buffer[size] = '\0';
	}
}
int getInt(char* path) {
	int i = 0;
	FILE *file = fopen(path, "r");
	if (file!=NULL) {
		fscanf(file, "%i", &i);
		fclose(file);
	}
	return i;
}
void putInt(char* path, int value) {
	char buffer[8];
	sprintf(buffer, "%d", value);
	putFile(path, buffer);
}

///////////////////////////////////////

static TTF_Font* font_s;
static TTF_Font* font_m;
static TTF_Font* font_l;
static SDL_Surface* screen;
static SDL_Surface* rule;
static SDL_Surface* button;
static SDL_Surface* slot_overlay;
static SDL_Surface* bg_white;
static SDL_Surface* bg_black;
static SDL_Surface* alt_enabled;
static SDL_Surface* alt_disabled;
static SDL_Surface* settings_bar_full;
static SDL_Surface* settings_bar_empty;
static SDL_Surface* settings_brightness;
static SDL_Surface* settings_volume;
static SDL_Surface* settings_mute;

static SDL_Color gold = {GOLD_TRIAD};
static SDL_Color bronze = {BRONZE_TRIAD};
static SDL_Color white = {WHITE_TRIAD};
static SDL_Color gray = {GRAY_TRIAD};
static SDL_Color shadow25 = {SHADOW25_TRIAD};
static SDL_Color shadow50 = {SHADOW50_TRIAD};

static SDL_Surface* battery[6];
void GFX_init(char* font_path) {
	TTF_Init();
	font_s = TTF_OpenFont(font_path, FONT_S);
	font_m = TTF_OpenFont(font_path, FONT_M);
	font_l = TTF_OpenFont(font_path, FONT_L);
	
	if (!font_s || !font_m || !font_l) printf("TTF_OpenFont: %s\n", TTF_GetError());

	rule = GFX_loadImage("rule.png");
	button = GFX_loadImage("btn.png");
	bg_white = GFX_loadImage("bg-white.png");
	bg_black = GFX_loadImage("bg-black.png");
	
	settings_bar_full = GFX_loadImage("settings-bar-full.png");
	settings_bar_empty = GFX_loadImage("settings-bar-empty.png");
	settings_brightness = GFX_loadImage("settings-brightness.png");
	settings_volume = GFX_loadImage("settings-volume.png");
	settings_mute = GFX_loadImage("settings-mute.png");
	
	alt_enabled = GFX_loadImage("alt-enabled.png");
	alt_disabled = GFX_loadImage("alt-disabled.png");
	
	char battery_path[256];
	for (int i=0; i<=5; i++) {
		sprintf(battery_path, "battery-%i.png", i);
		battery[i] = GFX_loadImage(battery_path);
	}
	
	puts("GFX_init");
	fflush(stdout);
}
void GFX_ready(void) {
	screen = SDL_GetVideoSurface(); // :cold_sweat:
}
SDL_Surface* GFX_loadImage(char* path) {
	static char full_path[256];
	sprintf(full_path, "%s/%s", kImagePath, path);
	
	SDL_Surface* image = IMG_Load(full_path);
	if (!image) printf("IMG_Load: %s\n", IMG_GetError());
	return image;
}

void GFX_quit(void) {
	SDL_FreeSurface(rule);
	SDL_FreeSurface(button);
	SDL_FreeSurface(bg_white);
	SDL_FreeSurface(bg_black);
	SDL_FreeSurface(settings_bar_full);
	SDL_FreeSurface(settings_bar_empty);
	SDL_FreeSurface(settings_brightness);
	SDL_FreeSurface(settings_volume);
	SDL_FreeSurface(settings_mute);
	SDL_FreeSurface(alt_enabled);
	SDL_FreeSurface(alt_disabled);
	
	for (int i=0; i<6; i++) {
		SDL_FreeSurface(battery[i]);
	}
	
	TTF_CloseFont(font_s);
	TTF_CloseFont(font_m);
	TTF_CloseFont(font_l);
	
	TTF_Quit();
}

void GFX_clear(void) {
	SDL_FillRect(screen, NULL, 0);
	SDL_Flip(screen);
}
void GFX_blitRule(SDL_Surface* surface, int y) {
	SDL_BlitSurface(rule, NULL, surface, &(SDL_Rect){0,y});
}
int GFX_blitHint(SDL_Surface* surface, char* htxt, int x, int y) {
	SDL_Surface* hint_text = TTF_RenderUTF8_Blended(font_m, htxt, white);
	SDL_BlitSurface(hint_text, NULL, surface, &(SDL_Rect){x,y});
	SDL_FreeSurface(hint_text);
}
int GFX_blitPill(SDL_Surface* surface, char* btxt, char* htxt, int x, int y) {
	SDL_Surface* button_text = TTF_RenderUTF8_Blended(font_s, btxt, bronze);
	SDL_Surface* hint_text = TTF_RenderUTF8_Blended(font_m, htxt, white);

	int pill_width = PILL_PAD + button_text->w + PILL_PAD;
	int total_width = pill_width + HINT_OX + hint_text->w;
	if (x<0) x += SCREEN_WIDTH - total_width;

	int hw = BUTTON_SIZE/2;
	int fill_width = pill_width - hw - hw;
	SDL_BlitSurface(button, &(SDL_Rect){0,0,hw,BUTTON_SIZE}, surface, &(SDL_Rect){x,y});
	SDL_FillRect(surface, &(SDL_Rect){x+hw,y,fill_width,BUTTON_SIZE}, SDL_MapRGB(button->format, WHITE_TRIAD));
	SDL_BlitSurface(button, &(SDL_Rect){hw,0,hw,BUTTON_SIZE}, surface, &(SDL_Rect){x+hw+fill_width,y});
	SDL_BlitSurface(button_text, NULL, surface, &(SDL_Rect){x+PILL_PAD,y+PILL_TEXT_OY});
	SDL_BlitSurface(hint_text, NULL, surface, &(SDL_Rect){x+pill_width+HINT_OX,y+HINT_TEXT_OY});
	SDL_FreeSurface(button_text);
	SDL_FreeSurface(hint_text);
	return total_width;
}
int GFX_blitButton(SDL_Surface* surface, char* btxt, char* htxt, int x, int y, int bx) {
	SDL_Surface* button_text = TTF_RenderUTF8_Blended(font_l, btxt, bronze);
	SDL_Surface* hint_text = TTF_RenderUTF8_Blended(font_m, htxt, white);

	int total_width = BUTTON_SIZE + HINT_OX + hint_text->w;
	if (x<0) x += SCREEN_WIDTH - total_width;

	int button_text_ox = (BUTTON_SIZE - button_text->w) / 2 + bx;
	SDL_BlitSurface(button, NULL, surface, &(SDL_Rect){x,y});
	SDL_BlitSurface(button_text, NULL, surface, &(SDL_Rect){x+button_text_ox,y+BUTTON_TEXT_OY});
	SDL_BlitSurface(hint_text, NULL, surface, &(SDL_Rect){x+BUTTON_SIZE+HINT_OX,y+HINT_TEXT_OY});
	SDL_FreeSurface(button_text);
	SDL_FreeSurface(hint_text);
	return total_width;
}
void GFX_blitMenu(SDL_Surface* surface, char* name, char* path, int conflict, int row, int selected_row, int has_alt, int use_alt) {
	int max_width = SCREEN_WIDTH - (2 * LIST_OX);
	SDL_Surface* text;
	char* fullname = strrchr(path, '/')+1;
	if (row==selected_row) {
		char* display_name = conflict ? fullname : name;

		// bar
		SDL_FillRect(surface, &(SDL_Rect){0,LIST_Y+(row*LIST_ROW_HEIGHT),SCREEN_WIDTH,LIST_ROW_HEIGHT}, SDL_MapRGB(surface->format, GOLD_TRIAD));
		
		// shadow
		text = TTF_RenderUTF8_Blended(font_l, display_name, shadow50);
		
		// if (text->w>max_width) needs_scrolling = 1; // TODO: restore
		SDL_BlitSurface(text, &(SDL_Rect){0,0,max_width,text->h}, surface, &(SDL_Rect){LIST_OX+SHADOW_OX,LIST_Y+(row*LIST_ROW_HEIGHT)+LIST_OY+SHADOW_OY});
		SDL_FreeSurface(text);
		
		text = TTF_RenderUTF8_Blended(font_l, display_name, white);
		SDL_BlitSurface(text, &(SDL_Rect){0,0,max_width,text->h}, surface, &(SDL_Rect){LIST_OX,LIST_Y+(row*LIST_ROW_HEIGHT)+LIST_OY});
		SDL_FreeSurface(text);
		
		if (has_alt) {
			// TODO: tmp
			SDL_BlitSurface(use_alt ? alt_enabled : alt_disabled, NULL, surface, &(SDL_Rect){SCREEN_WIDTH-LIST_ALT_OX, LIST_Y+(row*LIST_ROW_HEIGHT)+LIST_ALT_OY});
		}
	}
	else {
		if (conflict) {
			text = TTF_RenderUTF8_Blended(font_l, fullname, gray);
			SDL_BlitSurface(text, &(SDL_Rect){0,0,max_width,text->h}, surface, &(SDL_Rect){LIST_OX,LIST_Y+(row*LIST_ROW_HEIGHT)+LIST_OY});
			SDL_FreeSurface(text);
		}
	
		text = TTF_RenderUTF8_Blended(font_l, name, white);
		SDL_BlitSurface(text, &(SDL_Rect){0,0,max_width,text->h}, surface, &(SDL_Rect){LIST_OX,LIST_Y+(row*LIST_ROW_HEIGHT)+LIST_OY});
		SDL_FreeSurface(text);
	}
}
void GFX_blitWindow(SDL_Surface* surface, int x, int y, int width, int height, int color) {
	SDL_Surface* bg = color ? bg_white : bg_black;
	int color_rgb = color ? SDL_MapRGB(bg->format, WHITE_TRIAD) : 0;
	int hw = (bg->w / 2);
	int hh = (bg->h / 2);
	
	// corners
	SDL_BlitSurface(bg, &(SDL_Rect){0,0,hw,hh}, surface, &(SDL_Rect){x,y});
	SDL_BlitSurface(bg, &(SDL_Rect){hw,0,hw,hh}, surface, &(SDL_Rect){x+width-hw,y});
	SDL_BlitSurface(bg, &(SDL_Rect){0,hh,hw,hh}, surface, &(SDL_Rect){x,y+height-hh});
	SDL_BlitSurface(bg, &(SDL_Rect){hw,hh,hw,hh}, surface, &(SDL_Rect){x+width-hw,y+height-hh});
	
	// fill
	SDL_FillRect(surface, &(SDL_Rect){x+hw,y,width-bg->w,hh}, color_rgb);
	SDL_FillRect(surface, &(SDL_Rect){x,y+hh,width,height-bg->h}, color_rgb);
	SDL_FillRect(surface, &(SDL_Rect){x+hw,y+height-hh,width-bg->w,hh}, color_rgb);
}

void GFX_blitBodyCopy(SDL_Surface* surface, char* str, int ox, int oy, int width, int height) {
	SDL_Surface* text;
#define kTextBoxMaxRows 16
	char* rows[kTextBoxMaxRows];
	int row_count = 0;

	char* tmp;
	rows[row_count++] = str;
	while ((tmp=strchr(rows[row_count-1], '\n'))!=NULL) {
		if (row_count+1>=kTextBoxMaxRows) return; // TODO: bail
		rows[row_count++] = tmp+1;
	}
	
	int rendered_height = TEXTBOX_LINE_HEIGHT * row_count;
	int y = oy;
	y += (height - rendered_height) / 2;
	
	char line[256];
	for (int i=0; i<row_count; i++) {
		int len;
		if (i+1<row_count) {
			len = rows[i+1]-rows[i]-1;
			if (len) strncpy(line, rows[i], len);
			line[len] = '\0';
		}
		else {
			len = strlen(rows[i]);
			strcpy(line, rows[i]);
		}
		
		
		if (len) {
			text = TTF_RenderUTF8_Blended(font_l, line, gold);
			int x = ox;
			x += (width - text->w) / 2;
			SDL_BlitSurface(text, NULL, surface, &(SDL_Rect){x,y});
			SDL_FreeSurface(text);
		}
		// printf("%i [%s]\n",i,line);
		y += TEXTBOX_LINE_HEIGHT;
	}
}
int GFX_blitText(SDL_Surface* surface, char* str, int size, int x, int y, int width, int color, int has_shadow) {
	// size=0:small,1:medium,2:large width=0:left,>0:center,<0:right, color=0:white,1:gold, no multiline
	SDL_Surface* text;
	
	TTF_Font* font = size==0?font_s:(size==1?font_m:font_l);
	int oy = size=0?FONT_S_OY:(size==1?FONT_M_OY:FONT_L_OY);
	text = TTF_RenderUTF8_Blended(font, str, has_shadow?(x==MENU_LIST_X?shadow25:shadow50):(color?gold:white));
	int w = text->w;
	
	if (width>0) x += (width - w) / 2;
	else if (width<0) x += width - w;
	
	if (has_shadow) {
		SDL_BlitSurface(text, NULL, surface, &(SDL_Rect){x+SHADOW_OX,y+SHADOW_OY}); 
		SDL_FreeSurface(text);
		text = TTF_RenderUTF8_Blended(font, str, color?gold:white);
	}
	
	// SDL_FillRect(surface, &(SDL_Rect){x,y,text->w,text->h}, white_rgb); // TODO: tmp
	
	SDL_BlitSurface(text, NULL, surface, &(SDL_Rect){x,y});
	SDL_FreeSurface(text);
	return w;
}

SDL_Surface* GFX_getText(char* text, int size, int color) {
	TTF_Font* font = size==0?font_s:(size==1?font_m:font_l);
	return TTF_RenderUTF8_Blended(font, text, color?gold:white);
}

void GFX_blitBattery(SDL_Surface* surface, int x, int y) {
	int charge = getSmoothBatteryLevel();
	SDL_BlitSurface(battery[charge], NULL, surface, &(SDL_Rect){x,y});
}
void GFX_blitSettings(SDL_Surface* surface, int x, int y, int icon, int value, int min_value, int max_value) {
	if (x==MENU_SETTINGS_X) GFX_blitWindow(surface, x,y,SETTINGS_WIDTH,SETTINGS_HEIGHT, 0);
	SDL_BlitSurface(icon==0?settings_brightness:(icon==1?settings_volume:settings_mute), NULL, surface, &(SDL_Rect){x+SETTINGS_ICON_OX,y+SETTINGS_ICON_OY});
	SDL_BlitSurface(settings_bar_empty, NULL, surface, &(SDL_Rect){x+SETTINGS_BAR_OX,y+SETTINGS_BAR_OY});
	int w = SETTINGS_BAR_WIDTH * ((float)(value-min_value) / (max_value-min_value));
	SDL_BlitSurface(settings_bar_full, &(SDL_Rect){0,0,w,4}, surface, &(SDL_Rect){x+SETTINGS_BAR_OX,y+SETTINGS_BAR_OY,w,4});
}

///////////////////////////////////////

void waitForWake(void) {
	static int pressed[kButtonCount];
	for (int i=0; i<kButtonCount; i++) {
		pressed[i] = 0;
	}

	// NOTE: simplified version of Input_poll(), keep in sync!
	inline void poll(SDL_Event* event) {
		int i;
		if (event->type==SDL_KEYDOWN || event->type==SDL_KEYUP) {
			SDLKey key = event->key.keysym.sym;
			// NOTE: can't use switch because all missing buttons have the same value
				 if (key==MINUI_A) 		i = kButtonA;
			else if (key==MINUI_B) 		i = kButtonB;
			else if (key==MINUI_X) 		i = kButtonX;
			else if (key==MINUI_Y) 		i = kButtonY;
			else if (key==MINUI_START) 	i = kButtonStart;
			else if (key==MINUI_SELECT)	i = kButtonSelect;
			else if (key==MINUI_UP)		i = kButtonUp;
			else if (key==MINUI_DOWN)	i = kButtonDown;
			else if (key==MINUI_LEFT)	i = kButtonLeft;
			else if (key==MINUI_RIGHT)	i = kButtonRight;
			else if (key==MINUI_L)		i = kButtonL;
			else if (key==MINUI_R)		i = kButtonR;
			else if (key==MINUI_MENU)	i = kButtonMenu;
			else return;
		
			pressed[i] = event->type==SDL_KEYDOWN;
		}
	}

	SDL_Event event;
	int sleeping = 1;
	int wake = 0;
	while (!wake) {
		SDL_Delay(16); // TODO: can this be increased again?
		// SDL_Delay(1000);
		while (SDL_PollEvent(&event)) {
			poll(&event);
			if (sleeping) {
				if (event.type==SDL_KEYDOWN) {
					SDLKey btn = event.key.keysym.sym;
					if (btn==MINUI_L || btn==MINUI_R || btn==MINUI_L2 || btn==MINUI_R2); // buh
					else if (pressed[kButtonL] && pressed[kButtonR]) sleeping = 0;
				}
			}
			else {
				int count = 0;
				for (int i=0; i<kButtonCount; i++) {
					if (pressed[i]) {
						count += 1;
						break;
					}
				}
				if (count==0) wake = 1;
			}
		}
	}
	
	// Input_reset();
}

void fauxSleep(void) {
	GFX_clear();
	Input_reset();
	
	enterSleep();
	system("killall -s STOP keymon");
	waitForWake();
	system("killall -s CONT keymon");
	exitSleep();
}

int getSmoothBatteryLevel(void) {	
	#define kBatteryReadings 10
	
	static int values[kBatteryReadings];
	static int total = 0;
	static int i = 0;
	static int ready = 0;
	static unsigned long last_ticks = 0;
	
	int value = getBatteryLevel();
	unsigned long now_ticks = SDL_GetTicks();
	if (now_ticks-last_ticks>1000*10) {
		ready = 0;
		last_ticks = now_ticks;
	}

	// first run (or first in the last 10 seconds), fill up the buffer
	if (!ready) {
		for (i=0; i<kBatteryReadings; i++) {
			values[i] = value;
		}
		total = value * kBatteryReadings;
		ready = 1;
		i = 0;
	}
	// subsequent calls, update average
	else {
		total -= values[i];
		values[i] = value;
		total += value;
		i += 1;
		if (i>=kBatteryReadings) i -= kBatteryReadings;
		value = roundf((float)total / kBatteryReadings);
	}
	if (value<0) value = 0;
	if (value>5) value = 5;
	return value;
	#undef kBatteryReadings
}