#include <lvgl.h>
#include <TFT_eSPI.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <XPT2046_Touchscreen.h>

#include "font/passion_one_128.c"
#include "font/passion_one_64.c"
#include "font/passion_one_24.c"
#include "font/passion_one_16.c"
#include "font/fontawesome.c"

#define CUSTOM_CLOCK_SYMBOL "\xEF\x80\x97"
#define CUSTOM_POMODORO_SYMBOL "\xEF\x89\x94"

#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define SSID "ssid"
#define PASSWORD "password"

int x, y, z;

bool clock_state = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 21600, 60000);

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];


uint32_t purple = 0xB4BEFE;
uint32_t light_red = 0xF6A0A7;
uint32_t red = 0x620101;
uint32_t dark_blue = 0x11111B;
uint32_t light_green = 0xB9DEAD;
uint32_t green = 0x2F8F10;

static lv_style_t label_style, label_style2, label_style3, label_style4, label_style5;

lv_obj_t *hour_label, *minute_label, *day_label, *meridian_label;
lv_obj_t *pomo_minute_label, *pomo_second_label;
lv_obj_t *pomo_break_minute_label, *pomo_break_second_label;

void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    data->point.x = x;
    data->point.y = y;
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

lv_obj_t* add_rounded_rectangle(short x, short y, short ht, short wt, uint32_t color, short radius = 10) {
  lv_obj_t *rect = lv_obj_create(lv_scr_act());

  lv_obj_set_size(rect, wt, ht);  
  lv_obj_set_style_bg_color(rect, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_radius(rect, radius, LV_PART_MAIN);
  lv_obj_set_style_border_width(rect, 0, LV_PART_MAIN);
  lv_obj_align(rect, LV_ALIGN_TOP_LEFT, x, y); 
  lv_obj_set_style_pad_all(rect, 0, LV_PART_MAIN);

  return rect;
}

lv_obj_t* add_rounded_button(short x, short y, short ht, short wt, uint32_t color, short radius = 10) {
  lv_obj_t *btn = lv_button_create(lv_scr_act());  // Create a button
  lv_obj_set_size(btn, wt, ht);  // Set button size
  lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN);  // Set button color
  lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);  // Set rounded corners
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);  // Remove border
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x, y);  // Position the button
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);  // Remove padding

  return btn;
}

lv_obj_t* add_home_button(uint32_t bg_color, uint32_t text_color) {
  lv_obj_t* button_bg = add_rounded_button(5, 200, 35, 35, bg_color, 5);

  lv_obj_t *home_symbol_label = lv_label_create(button_bg);
  lv_label_set_text(home_symbol_label, LV_SYMBOL_HOME);
  lv_obj_set_style_text_font(home_symbol_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(home_symbol_label, lv_color_hex(text_color), LV_PART_MAIN);
  lv_obj_align(home_symbol_label, LV_ALIGN_CENTER, 0, 0);

  lv_obj_add_event_cb(button_bg, home_button_cb, LV_EVENT_CLICKED, NULL);

  return button_bg;
}

lv_obj_t* add_icon_button(short x, short y, short ht, short wt, const char* icon, uint32_t bg_color, uint32_t icon_color, short radius = 5) {
  lv_obj_t *button_bg = add_rounded_button(x, y, ht, wt, bg_color, radius);

  lv_obj_t *button_label = lv_label_create(button_bg);
  lv_label_set_text(button_label, icon);
  lv_obj_set_style_text_font(button_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(button_label, lv_color_hex(icon_color), LV_PART_MAIN);
  lv_obj_align(button_label, LV_ALIGN_CENTER, 0, 0);

  return button_bg;
}

//BUTTON CALL BACKS
static void clock_button_cb(lv_event_t * e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_clean(lv_scr_act());
    clock_state = true;
    clock_gui();
  }
}

static void home_button_cb(lv_event_t * e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_clean(lv_scr_act());
    clock_state = false;
    dashboard_gui();
  }
}

static void pomodoro_focus_button_cb(lv_event_t * e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_clean(lv_scr_act());
    clock_state = false;
    pomodoro_focus_gui();
  }
}

static void pomodoro_focus_pause_button_cb(lv_event_t * e) {
  Serial.println("Button Pressed");
}

static void pomodoro_focus_next_button_cb(lv_event_t * e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_clean(lv_scr_act());
    clock_state = false;
    pomodoro_break_gui();
  }
}

static void pomodoro_break_pause_button_cb(lv_event_t * e) {
  Serial.println("Button Pressed");
}

static void pomodoro_break_next_button_cb(lv_event_t * e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_clean(lv_scr_act());
    clock_state = false;
    pomodoro_focus_gui();
  }
}


void dashboard_gui() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x181825), LV_PART_MAIN);

  // Clock button
  lv_obj_t *clock_button_bg = add_rounded_button(65, 77, 86, 86, purple);
  lv_obj_t *clock_symbol = lv_label_create(clock_button_bg);
  lv_label_set_text(clock_symbol, CUSTOM_CLOCK_SYMBOL);
  lv_obj_set_style_text_font(clock_symbol, &fontawesome, 0);
  lv_obj_set_style_text_color(clock_symbol, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_align(clock_symbol, LV_ALIGN_CENTER, 0, 0);

  lv_obj_add_event_cb(clock_button_bg, clock_button_cb, LV_EVENT_CLICKED, NULL);

  // pomodoro button
  lv_obj_t *pomodoro_button_bg = add_rounded_rectangle(169, 77, 86, 86, purple);
  lv_obj_t *pomodoro_symbol = lv_label_create(pomodoro_button_bg);
  lv_label_set_text(pomodoro_symbol, CUSTOM_POMODORO_SYMBOL);
  lv_obj_set_style_text_font(pomodoro_symbol, &fontawesome, 0);
  lv_obj_set_style_text_color(pomodoro_symbol, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_align(pomodoro_symbol, LV_ALIGN_CENTER, 0, 0);

  lv_obj_add_event_cb(pomodoro_button_bg, pomodoro_focus_button_cb, LV_EVENT_CLICKED, NULL);
}

void clock_gui(void) {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x181825), LV_PART_MAIN);  

  // Hour section
  lv_obj_t *hour_bg = add_rounded_rectangle(36, 68, 104, 86, 0xB4BEFE);

  hour_label = lv_label_create(hour_bg);
  lv_label_set_text(hour_label, "00");
  lv_obj_align(hour_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(hour_label, &label_style, 0);

  // Minute section
  lv_obj_t *minute_bg = add_rounded_rectangle(136, 68, 104, 86, 0xB4BEFE);

  minute_label = lv_label_create(minute_bg);
  lv_label_set_text(minute_label, "00");
  lv_obj_align(minute_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(minute_label, &label_style, 0);

  // for day box
  lv_obj_t *day_bg = add_rounded_rectangle(236, 71, 45, 62, 0xA6E3A1);

  day_label = lv_label_create(day_bg);
  lv_label_set_text(day_label, "SSS");
  lv_obj_align(day_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(day_label, &label_style2, 0);

  // for am/pm box
  lv_obj_t *meridian_bg = add_rounded_rectangle(236, 123, 45, 62, 0x89DCEB);

  meridian_label = lv_label_create(meridian_bg);
  lv_label_set_text(meridian_label, "##");
  lv_obj_align(meridian_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(meridian_label, &label_style2, 0);

  // home button
  lv_obj_t *home_button = add_home_button(purple, dark_blue);  
}

void pomodoro_focus_gui() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(light_red), LV_PART_MAIN);  

  // Minute label
  pomo_minute_label = lv_label_create(lv_scr_act());
  lv_label_set_text(pomo_minute_label, "23");
  lv_obj_align(pomo_minute_label, LV_ALIGN_TOP_LEFT, 26, 75);
  lv_obj_add_style(pomo_minute_label, &label_style4, 0);

  // Colon
  lv_obj_t* colon_label = lv_label_create(lv_scr_act());
  lv_label_set_text(colon_label, ":");
  lv_obj_align_to(colon_label, pomo_minute_label, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
  lv_obj_add_style(colon_label, &label_style4, 0);

  // Sec label
  pomo_second_label = lv_label_create(lv_scr_act());
  lv_label_set_text(pomo_second_label, "55");
  lv_obj_align(pomo_second_label, LV_ALIGN_TOP_LEFT, 185, 75);
  lv_obj_add_style(pomo_second_label, &label_style4, 0);

  // Focus label
  lv_obj_t *focus_bg = add_rounded_rectangle(23, 28, 20, 45, red, 5);

  lv_obj_t *focus_label = lv_label_create(focus_bg);
  lv_label_set_text(focus_label, "FOCUS");
  lv_obj_align(focus_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(focus_label, &label_style, 0);
  lv_obj_set_style_text_font(focus_label, &passion_one_16, 0);
  lv_obj_set_style_text_color(focus_label, lv_color_hex(light_red), LV_PART_MAIN);

  // Pause button
  lv_obj_t *pause_button = add_icon_button(224, 176, 24, 24, LV_SYMBOL_PAUSE, red, light_red);
  lv_obj_add_event_cb(pause_button, pomodoro_focus_pause_button_cb, LV_EVENT_CLICKED, NULL);

  // Next button
  lv_obj_t *next_button = add_icon_button(257, 176, 24, 24, LV_SYMBOL_NEXT, red, light_red);
  lv_obj_add_event_cb(next_button, pomodoro_focus_next_button_cb, LV_EVENT_CLICKED, NULL);

  // home button
  lv_obj_t *home_button = add_home_button(red, light_red);
}

void pomodoro_break_gui() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(light_green), LV_PART_MAIN);  

  // Minute label
  pomo_break_minute_label = lv_label_create(lv_scr_act());
  lv_label_set_text(pomo_break_minute_label, "04");
  lv_obj_align(pomo_break_minute_label, LV_ALIGN_TOP_LEFT, 26, 75);
  lv_obj_add_style(pomo_break_minute_label, &label_style5, 0);

  // Colon
  lv_obj_t* colon_label = lv_label_create(lv_scr_act());
  lv_label_set_text(colon_label, ":");
  lv_obj_align_to(colon_label, pomo_break_minute_label, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
  lv_obj_add_style(colon_label, &label_style5, 0);

  // Sec label
  pomo_break_second_label = lv_label_create(lv_scr_act());
  lv_label_set_text(pomo_break_second_label, "25");
  lv_obj_align(pomo_break_second_label, LV_ALIGN_TOP_LEFT, 185, 75);
  lv_obj_add_style(pomo_break_second_label, &label_style5, 0);

  // Focus label
  lv_obj_t *break_bg = add_rounded_rectangle(23, 28, 20, 45, green, 5);

  lv_obj_t *break_label = lv_label_create(break_bg);
  lv_label_set_text(break_label, "BREAK");
  lv_obj_align(break_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(break_label, &label_style, 0);
  lv_obj_set_style_text_font(break_label, &passion_one_16, 0);
  lv_obj_set_style_text_color(break_label, lv_color_hex(light_green), LV_PART_MAIN);

  // Pause button
  lv_obj_t *pause_button = add_icon_button(224, 176, 24, 24, LV_SYMBOL_PAUSE, green, light_green);
  lv_obj_add_event_cb(pause_button, pomodoro_break_pause_button_cb, LV_EVENT_CLICKED, NULL);

  // Next button
  lv_obj_t *next_button = add_icon_button(257, 176, 24, 24, LV_SYMBOL_NEXT, green, light_green);
  lv_obj_add_event_cb(next_button, pomodoro_break_next_button_cb, LV_EVENT_CLICKED, NULL);

  // home button
  lv_obj_t *home_button = add_home_button(green, light_green);
}

void setup() {
  lv_style_init(&label_style);
  lv_style_set_text_font(&label_style, &passion_one_64);
  lv_style_set_text_color(&label_style, lv_color_black());

  lv_style_init(&label_style3);
  lv_style_set_text_font(&label_style3, &fontawesome);
  lv_style_set_text_color(&label_style3, lv_color_black());

  lv_style_init(&label_style2);
  lv_style_set_text_font(&label_style2, &passion_one_24);
  lv_style_set_text_color(&label_style2, lv_color_black());

  lv_style_init(&label_style4);
  lv_style_set_text_font(&label_style4, &passion_one_128);
  lv_style_set_text_color(&label_style4, lv_color_hex(red));

  lv_style_init(&label_style5);
  lv_style_set_text_font(&label_style5, &passion_one_128);
  lv_style_set_text_color(&label_style5, lv_color_hex(green));

  Serial.begin(115200);
  
  WiFi.begin(SSID, PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(2);

  lv_display_t * disp;
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
    
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  timeClient.begin();
  dashboard_gui();
}

void loop() {
  if(clock_state) {
    timeClient.update();

    time_t epoch_time = timeClient.getEpochTime();

    struct tm *ptm = localtime(&epoch_time);

    char buffer[10];

    int hour = ptm->tm_hour;
    if(hour > 12) hour = hour - 12;
    if(hour == 0) hour = 12;
    sprintf(buffer, "%02d", hour);
    lv_label_set_text(hour_label, buffer);

    sprintf(buffer, "%02d", ptm->tm_min);
    lv_label_set_text(minute_label, buffer);

    char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    char* day_name =days[ptm->tm_wday];
    lv_label_set_text(day_label, day_name);

    const char* meri = (ptm->tm_hour < 12) ? "AM" : "PM";
    lv_label_set_text(meridian_label, meri);
  }
  
  lv_task_handler(); 
  lv_tick_inc(5);
  delay(5);
}
