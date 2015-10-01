#include "main.h"
#include <pebble.h>

//Global variable definitions
static Window *s_mainWindow;
static BitmapLayer *s_hawkBacker_layer;
static BitmapLayer *s_schedBacker_layer;
static Layer *s_hands_layer;
//static TextLayer *s_time_layer;
static TextLayer *s_sched_start, *s_sched_end, *s_sched_cur;
static GPath *s_minute_hand, *s_hour_hand;
static GRect s_clock_rect;
static GPoint s_sched_dep, s_sched_ret;
static GBitmap *s_hawkBacker_bitmap;
static GBitmap *s_schedBacker_short_bitmap;
static GBitmap *s_schedBacker_regu_bitmap;
static GBitmap *s_schedBacker_blank_bitmap;

static void update_hands(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(s_minute_hand, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_hand);
  gpath_draw_outline(ctx, s_minute_hand);

  gpath_rotate_to(s_hour_hand, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_hand);
  gpath_draw_outline(ctx, s_hour_hand);
  
  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);
}

static void update_time() {
  //Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  char day[3];
  strftime(day, sizeof("WED"), "%a", tick_time);
  if(strcmp(day, "Wed") == 0){
    APP_LOG(APP_LOG_LEVEL_INFO, "Wendsday");
    bitmap_layer_set_bitmap(s_schedBacker_layer, s_schedBacker_short_bitmap);
  } else if(strcmp(day, "Sat") == 0 || strcmp(day, "Sun") == 0){
    APP_LOG(APP_LOG_LEVEL_INFO, "Weekends");
    bitmap_layer_set_bitmap(s_schedBacker_layer, s_schedBacker_blank_bitmap);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Weekday");
    bitmap_layer_set_bitmap(s_schedBacker_layer, s_schedBacker_regu_bitmap);
  }
  char min[2];
  strftime(min, sizeof("00"), "%M", tick_time);
  int mini = atoi(min);
  if(mini % 2 == 0){
    layer_set_frame(bitmap_layer_get_layer(s_schedBacker_layer), GRect(s_sched_dep.x, s_sched_dep.y, 144, 161));
  } else {
    layer_set_frame(bitmap_layer_get_layer(s_schedBacker_layer), GRect(s_sched_ret.x, s_sched_ret.y, 144, 161));
  }
}

static void mainWindow_load(Window *window){
  s_clock_rect = GRect(9, 7, 126, 126);
  GRect scheduleThing = GRect(0, 7, 144, 161);
  GRect shortFrame = GRect(0, -10, 144, 168);
  Layer *mainLayer = window_get_root_layer(s_mainWindow);
  //Make everything here
  //Load bitmaps
  s_hawkBacker_bitmap = gbitmap_create_with_resource(RESOURCE_ID_HAWK_BACKER);
  s_schedBacker_regu_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SCHED_BACKER_REGU);
  s_schedBacker_short_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SCHED_BACKER_SHORT);
  s_schedBacker_blank_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SCHED_BACKER_BLANK);
  //Create bitmap layers
  s_hawkBacker_layer = bitmap_layer_create(s_clock_rect);
  bitmap_layer_set_bitmap(s_hawkBacker_layer, s_hawkBacker_bitmap);
  layer_add_child(mainLayer, bitmap_layer_get_layer(s_hawkBacker_layer));
  s_schedBacker_layer = bitmap_layer_create(scheduleThing);
  bitmap_layer_set_bitmap(s_schedBacker_layer, s_schedBacker_regu_bitmap);
  //Create time thingy
  //LOL NO
  /*
  s_time_layer = text_layer_create(GRect(9, 117, 126, 55));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(mainLayer, text_layer_get_layer(s_time_layer));
  */
  
  s_hands_layer = layer_create(shortFrame);
  layer_set_update_proc(s_hands_layer, update_hands);
  layer_add_child(mainLayer, s_hands_layer);
  
  layer_add_child(mainLayer, bitmap_layer_get_layer(s_schedBacker_layer));
  
  //Adding date info
  s_sched_start = text_layer_create(GRect(2, -4, 51, 18));
  s_sched_cur = text_layer_create(GRect(59, -3, 26, 18));
  s_sched_end = text_layer_create(GRect(93, -4, 49, 18));
  text_layer_set_background_color(s_sched_start, GColorClear);
  text_layer_set_background_color(s_sched_cur, GColorClear);
  text_layer_set_background_color(s_sched_end, GColorClear);
  text_layer_set_text_color(s_sched_start, GColorBlack);
  text_layer_set_text_color(s_sched_cur, GColorBlack);
  text_layer_set_text_color(s_sched_end, GColorBlack);
  text_layer_set_text(s_sched_start, "8:45");
  text_layer_set_text(s_sched_cur, "1st");
  text_layer_set_text(s_sched_end, "9:22");
  GFont smallGoth = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  text_layer_set_font(s_sched_start, smallGoth);
  text_layer_set_font(s_sched_cur, smallGoth);
  text_layer_set_font(s_sched_end, smallGoth);
  text_layer_set_text_alignment(s_sched_start, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_sched_cur, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_sched_end, GTextAlignmentRight);
  Layer *schedLayer = bitmap_layer_get_layer(s_schedBacker_layer);
  layer_add_child(schedLayer, text_layer_get_layer(s_sched_start));
  layer_add_child(schedLayer, text_layer_get_layer(s_sched_cur));
  layer_add_child(schedLayer, text_layer_get_layer(s_sched_end));
  update_time();
}

static void mainWindow_unload(Window *window){
  //Destroy everything in here
  gbitmap_destroy(s_hawkBacker_bitmap);
  gbitmap_destroy(s_schedBacker_regu_bitmap);
  gbitmap_destroy(s_schedBacker_short_bitmap);
  gbitmap_destroy(s_schedBacker_blank_bitmap);
  bitmap_layer_destroy(s_hawkBacker_layer);
  bitmap_layer_destroy(s_schedBacker_layer);
  text_layer_destroy(s_sched_start);
  text_layer_destroy(s_sched_cur);
  text_layer_destroy(s_sched_end);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  layer_mark_dirty(window_get_root_layer(s_mainWindow));
  update_time();
}

static void init(){
  s_mainWindow = window_create();
  s_sched_dep = GPoint(0, 7);
  s_sched_ret = GPoint(0, 146);
  
  window_set_window_handlers(s_mainWindow, (WindowHandlers){
    .load = mainWindow_load,
    .unload = mainWindow_unload
  });
  window_stack_push(s_mainWindow, true);
  
  s_minute_hand = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_hand = gpath_create(&HOUR_HAND_POINTS);
  
  Layer *window_layer = window_get_root_layer(s_mainWindow); 
  GRect bounds = layer_get_bounds(window_layer); 
  GPoint center = grect_center_point(&bounds); 
  gpath_move_to(s_minute_hand, center); 
  gpath_move_to(s_hour_hand, center); 

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  //Put registers underneath
}

static void deinit(){
  gpath_destroy(s_minute_hand);
  gpath_destroy(s_hour_hand);
  window_destroy(s_mainWindow);
}

int main(){
  init();
  app_event_loop();
  deinit();
}