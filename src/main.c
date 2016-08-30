#include <pebble.h>
#include <ctype.h>
#include "main.h"
#include "health.h"

static Window *s_main_window;
static Layer *s_bg_layer, *s_date_layer, *s_hands_layer;

static TextLayer *s_clock_label, *s_date_label, *s_weather_label, *s_steps_label;
static GFont s_clock_font, s_date_font, s_weather_font, s_steps_font;

static GBitmap *s_bt_icon_bitmap;
static BitmapLayer *s_bt_icon_layer;

static char s_date_buffer[12];
static int s_temp_c;
static char s_weather_condition[32];

static int32_t colorcode_background, colorcode_clock, colorcode_steps, colorcode_weather, colorcode_date;

/* *** Color Configuration *** */

static void setup_colors() {
  colorcode_background = persist_read_int(KEY_COLOR_BACKGROUND) ? persist_read_int(KEY_COLOR_BACKGROUND) : COLOR_DEFAULT_BACKGROUND;
  colorcode_clock = persist_read_int(KEY_COLOR_CLOCK) ? persist_read_int(KEY_COLOR_CLOCK) : COLOR_DEFAULT_CLOCK;
  colorcode_steps = persist_read_int(KEY_COLOR_STEPS) ? persist_read_int(KEY_COLOR_STEPS) : COLOR_DEFAULT_STEPS;
  colorcode_weather = persist_read_int(KEY_COLOR_WEATHER) ? persist_read_int(KEY_COLOR_WEATHER) : COLOR_DEFAULT_WEATHER;
  colorcode_date = persist_read_int(KEY_COLOR_DATE) ? persist_read_int(KEY_COLOR_DATE) : COLOR_DEFAULT_DATE;
}

static void update_colors() {
  setup_colors();
  window_set_background_color(s_main_window, GColorFromHEX(colorcode_background));
  text_layer_set_text_color(s_clock_label, GColorFromHEX(colorcode_clock));
  text_layer_set_text_color(s_date_label, GColorFromHEX(colorcode_date));
  text_layer_set_text_color(s_steps_label, GColorFromHEX(colorcode_steps));
  text_layer_set_text_color(s_weather_label, GColorFromHEX(colorcode_weather));  
}

/* *** Bluetooth Callback *** */

// Bluetooth connection indicator (shown when disconnected)
static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

/* *** proc layer updates *** */

static void proc_date_update (Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %d", t);
  char *s = s_date_buffer;
  while(*s) *s++ = toupper((int)*s);
  text_layer_set_text(s_date_label, s_date_buffer);
}

/* *** focus handler *** */

static void handle_focus(bool focus) {
  if (focus) {
    layer_mark_dirty(window_get_root_layer(s_main_window));
  }
}

/* *** Update Steps *** */

static void update_steps_label(bool is_enabled) {
  if(is_enabled) {
    static char steps_buffer[] = MAX_HEALTH_STR;
    update_health(steps_buffer);
    text_layer_set_text(s_steps_label, steps_buffer);
  } else {
    text_layer_set_text(s_steps_label, "");
  }
}

/* *** Update Weather *** */

static void update_weather() {
  static char temperature_buffer[8];
  static char weather_label_buffer[32];
  
  //if(s_temp_c && s_weather_condition != null) {
    if(persist_read_bool(KEY_IS_FAHRENHEIT)) {          
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", (int) (s_temp_c * 9 / 5 + 32)); // Fahrenheit  
    } else {
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)s_temp_c); // Celsius by default    
    }
    snprintf(weather_label_buffer, sizeof(weather_label_buffer), "%s\n%s", temperature_buffer, s_weather_condition);
    APP_LOG(APP_LOG_LEVEL_INFO, "Weather: %s", weather_label_buffer);
    text_layer_set_text(s_weather_label, weather_label_buffer);
  //}
}

/* *** callbacks *** */

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "in inbox_received_callback");

  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  Tuple *is_fahrenheit_tuple = dict_find(iterator, MESSAGE_KEY_IS_FAHRENHEIT);
  Tuple *is_steps_enabled_tuple = dict_find(iterator, MESSAGE_KEY_IS_STEPS_ENABLED);
  
  Tuple *color_background_tuple = dict_find(iterator, MESSAGE_KEY_COLOR_BACKGROUND);
  Tuple *color_clock_tuple = dict_find(iterator, MESSAGE_KEY_COLOR_CLOCK);
  Tuple *color_steps_tuple = dict_find(iterator, MESSAGE_KEY_COLOR_STEPS);
  Tuple *color_weather_tuple = dict_find(iterator, MESSAGE_KEY_COLOR_WEATHER);
  Tuple *color_date_tuple = dict_find(iterator, MESSAGE_KEY_COLOR_DATE);
  
  if(color_background_tuple ) { persist_write_int(KEY_COLOR_BACKGROUND, color_background_tuple->value->int32); }
  if(color_clock_tuple)            { persist_write_int(KEY_COLOR_CLOCK, color_clock_tuple->value->int32); }
  if(color_steps_tuple)          { persist_write_int(KEY_COLOR_STEPS, color_steps_tuple->value->int32); }
  if(color_weather_tuple)     { persist_write_int(KEY_COLOR_WEATHER, color_weather_tuple->value->int32); }
  if(color_date_tuple)    { persist_write_int(KEY_COLOR_DATE, color_date_tuple->value->int32); }
  
  if(color_background_tuple || color_clock_tuple || color_steps_tuple || color_weather_tuple || color_date_tuple ) {
    update_colors();
    layer_mark_dirty(window_get_root_layer(s_main_window));
  }
  
  if(is_steps_enabled_tuple) {
    if(is_steps_enabled_tuple->value->int8 > 0) {
      persist_write_bool(KEY_IS_STEPS_ENABLED, true);
      update_steps_label(true);
    } else {
      persist_write_bool(KEY_IS_STEPS_ENABLED, false);
      update_steps_label(false);
    }
    layer_mark_dirty(window_get_root_layer(s_main_window));
  }
  
  if(is_fahrenheit_tuple) {
    if (is_fahrenheit_tuple->value->int8 > 0) {
      persist_write_bool(KEY_IS_FAHRENHEIT, true);
    } else {
      persist_write_bool(KEY_IS_FAHRENHEIT, false);
    }
    update_weather();
    layer_mark_dirty(window_get_root_layer(s_main_window));
  }
  
  if(temp_tuple && conditions_tuple) {
    s_temp_c = temp_tuple->value->int32;
    snprintf(s_weather_condition, sizeof(s_weather_condition), "%s", conditions_tuple->value->cstring);  
    APP_LOG(APP_LOG_LEVEL_INFO, "weather update from tuple: %s %d", s_weather_condition, s_temp_c);
    update_weather();
  }
  
}
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void setup_callbacks() {
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  //app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}

/* *** Tick handlers *** */

static void update_time() {
  time_t now = time(NULL);  
  struct tm *t = localtime(&now);
  static char s_time_buffer[10];
  strftime(s_time_buffer, sizeof(s_time_buffer), "%H:%M", t);
  text_layer_set_text(s_clock_label, s_time_buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  if(tick_time->tm_sec % 20 == 0) {  
    update_time();
    //layer_mark_dirty(window_get_root_layer(s_main_window));
  }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Update Health Steps every minute
  
  bool is_steps_enabled = persist_exists(KEY_IS_STEPS_ENABLED) ? persist_read_bool(KEY_IS_STEPS_ENABLED) : true;
  update_steps_label(is_steps_enabled);
  
  // Get weather update every WEATHER_UPDATE_INTERVAL_MINUTES minutes. default would be 30 min (0 and 30 of each hour)
  if(tick_time->tm_min % WEATHER_UPDATE_INTERVAL_MINUTES == 0) { 
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, 0, 0);
    app_message_outbox_send();
  }
}

/* *** main window load & unload *** */

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Read color code
  setup_colors();
  
  // Create date layer and add to main layer
  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, proc_date_update);
  layer_add_child(window_layer, s_date_layer);

  s_date_label = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(115, 95), bounds.size.w, 24));

  GColor color_date = GColorFromHEX(colorcode_date);
  text_layer_set_text(s_date_label, s_date_buffer);
  text_layer_set_background_color(s_date_label, GColorClear);
  text_layer_set_text_color(s_date_label, color_date);
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_INFO_20));
  text_layer_set_font(s_date_label, s_date_font);
  text_layer_set_text_alignment(s_date_label, GTextAlignmentCenter);
  layer_add_child(s_date_layer, text_layer_get_layer(s_date_label));
  
  // Create weather and templature layer
  s_weather_label = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(28,14), bounds.size.w, 50));

  GColor color_weather = GColorFromHEX(colorcode_weather);  
  text_layer_set_background_color(s_weather_label, GColorClear);
  text_layer_set_text_color(s_weather_label, color_weather);
  text_layer_set_text_alignment(s_weather_label, GTextAlignmentCenter);
  text_layer_set_text(s_weather_label, "LOADING");
  
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_INFO_20));
  text_layer_set_font(s_weather_label, s_weather_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_label));
  
  // Create Health Steps layer
  s_steps_label = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(138, 110), bounds.size.w, 24));
  
  GColor color_steps = GColorFromHEX(colorcode_steps);  
  text_layer_set_background_color(s_steps_label, GColorClear);
  text_layer_set_text_color(s_steps_label, color_steps);
  text_layer_set_text_alignment(s_steps_label, GTextAlignmentCenter);
  s_steps_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_INFO_20));
  text_layer_set_font(s_steps_label, s_steps_font);
  text_layer_set_text(s_steps_label, "00000");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_steps_label));

  // Create Clock Layer 
  GColor color_clock = GColorFromHEX(colorcode_clock);
  s_clock_label = text_layer_create(GRect(0, bounds.size.h/2-42/2, bounds.size.w, 42));  
  text_layer_set_background_color(s_clock_label, GColorClear);
  text_layer_set_text_color(s_clock_label, color_clock); // Set Clock Color
  text_layer_set_text_alignment(s_clock_label, GTextAlignmentCenter);
  s_clock_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CLOCK_42));
  text_layer_set_font(s_clock_label, s_clock_font);
  layer_add_child(window_layer, text_layer_get_layer(s_clock_label));
  update_time();
}

static void main_window_unload(Window *window) {  
  
  fonts_unload_custom_font(s_clock_font);
  text_layer_destroy(s_clock_label);
  
}

static void init(void) {
  s_main_window = window_create();
  
  setup_colors();
  GColor color_background = GColorFromHEX(colorcode_background);
  window_set_background_color(s_main_window, color_background); 

  setup_callbacks();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload  
  });
  
  window_stack_push(s_main_window, true);
  
  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  
  // App Forcus Handler (case Watch received notification over watchface and back to get focus)
  app_focus_service_subscribe_handlers((AppFocusHandlers) {
    .did_focus = handle_focus,
  });
  
  // Subscribe Tick handlers
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}