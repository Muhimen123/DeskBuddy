#include <lvgl.h>
#include <TFT_eSPI.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <XPT2046_Touchscreen.h>

#include "font/passion_one_64.c"
#include "font/passion_one_24.c"
#include "font/fontawesome.c"
LV_FONT_DECLARE(fontawesome)

#define CUSTOM_CLOCK_SYMBOL "\xEF\x80\x97"


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


uint32_t dark_blue = 0xB4BEFE;

static lv_style_t label_style, label_style2, label_style3;

lv_obj_t *hour_label, *minute_label, *day_label, *meridian_label;

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

lv_obj_t* add_rounded_rectangle(short x, short y, short ht, short wt, uint32_t color) {
  lv_obj_t *rect = lv_obj_create(lv_scr_act());

  lv_obj_set_size(rect, wt, ht);  
  lv_obj_set_style_bg_color(rect, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_radius(rect, 10, LV_PART_MAIN);
  lv_obj_set_style_border_width(rect, 0, LV_PART_MAIN);
  lv_obj_align(rect, LV_ALIGN_TOP_LEFT, x, y); 
  lv_obj_set_style_pad_all(rect, 0, LV_PART_MAIN);

  return rect;
}

lv_obj_t* add_rounded_button(short x, short y, short ht, short wt, uint32_t color) {
  lv_obj_t *btn = lv_button_create(lv_scr_act());  // Create a button
  lv_obj_set_size(btn, wt, ht);  // Set button size
  lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN);  // Set button color
  lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);  // Set rounded corners
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);  // Remove border
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x, y);  // Position the button
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);  // Remove padding

  return btn;
}

//BUTTON CALL BACKS
static void clock_button_cb(lv_event_t * e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_clean(lv_scr_act());
    clock_state = true;
    clock_gui();
  }
}


void dashboard_gui() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x181825), LV_PART_MAIN);

  // Clock button
  lv_obj_t *clock_button_bg = add_rounded_button(65, 77, 86, 86, dark_blue);
  lv_obj_t *clock_symbol = lv_label_create(clock_button_bg);
  lv_label_set_text(clock_symbol, CUSTOM_CLOCK_SYMBOL);
  lv_obj_set_style_text_font(clock_symbol, &fontawesome, 0);
  lv_obj_set_style_text_color(clock_symbol, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_align(clock_symbol, LV_ALIGN_CENTER, 0, 0);

  lv_obj_add_event_cb(clock_button_bg, clock_button_cb, LV_EVENT_CLICKED, NULL);


  // pomodoro button
  // lv_obj_t *pomodoro_button_bg = add_rounded_rectangle(169, 77, 86, 86, dark_blue);
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
  //clock_gui();
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
