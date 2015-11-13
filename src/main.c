#include "main.h"
#include "scheduleInfo.c"
#include <pebble.h>

//Global variable definitions
static Window *s_mainWindow;
static BitmapLayer *s_hawkBacker_layer;
static BitmapLayer *s_schedBacker_layer;
#if PBL_SDK_2
static InverterLayer *s_battery_inverter;
#endif
static Layer *s_hands_layer, *s_battery_layer, *s_remainder_ind;
static bool deployed;
static bool isSchoolDay;
static bool drawRemaining;
static TextLayer *s_sched_start, *s_sched_end, *s_sched_cur, *s_battery_text, *s_time_text, *s_rem_text;
static PropertyAnimation *s_deploy_animation, *s_return_animation;
static GPath *s_minute_hand, *s_hour_hand, *s_rem_indicator;
static GRect s_clock_rect, s_batteryBack_rect;
static GPoint s_sched_dep, s_sched_ret;
static GBitmap *s_hawkBacker_bitmap;
static GBitmap *s_schedBacker_short_bitmap;
static GBitmap *s_schedBacker_regu_bitmap;
static GBitmap *s_schedBacker_blank_bitmap;
static int s_battery_level;
static int period, remInts;
static int s_degree_start = 0, s_degree_end = 1, s_degree_current = 1;
static char remLabel[15];
static char timeLabel[9];

void stopped_return_animation(Animation *animation, void *data){
  deployed = false;
  #if PBL_SDK_2
  property_animation_destroy(s_return_animation);
  #endif
}

static void trigger_return_animation(){
  if(!deployed){
    return;
  }
  
  //Setting up the animation
  GRect from_frame = GRect(s_sched_dep.x, s_sched_dep.y, 144, 161);
  GRect to_frame = GRect(s_sched_ret.x, s_sched_ret.y, 144, 161);
  s_return_animation = property_animation_create_layer_frame(bitmap_layer_get_layer(s_schedBacker_layer), &from_frame, &to_frame);
  animation_set_duration((Animation*)s_deploy_animation, 750);
  animation_set_delay((Animation*)s_deploy_animation, 3000);
  animation_set_curve((Animation*)s_deploy_animation, AnimationCurveEaseIn);
  animation_set_handlers((Animation*)s_deploy_animation, (AnimationHandlers){
    .stopped = (AnimationStoppedHandler)stopped_return_animation
  }, NULL);
  
  animation_schedule((Animation*)s_return_animation);
}

void stopped_deploy_animation(Animation *animation, void *data){
  deployed = true;
  property_animation_destroy(s_deploy_animation);
  trigger_return_animation();
}

void started_deploy_animation(Animation *animation, void *data){
  deployed = true;
}

static void trigger_deploy_animation(){
  if(deployed){
    return;
  }
  if(isSchoolDay){
    //Setting up the animation
    GRect from_frame = GRect(s_sched_ret.x, s_sched_ret.y, 144, 161);
    GRect to_frame = GRect(s_sched_dep.x, s_sched_dep.y, 144, 161);
    s_deploy_animation = property_animation_create_layer_frame(bitmap_layer_get_layer(s_schedBacker_layer), &from_frame, &to_frame);
    animation_set_duration((Animation*)s_deploy_animation, 750);
    animation_set_curve((Animation*)s_deploy_animation, AnimationCurveEaseIn);
    animation_set_handlers((Animation*)s_deploy_animation, (AnimationHandlers){
      .stopped = (AnimationStoppedHandler)stopped_deploy_animation,
      .started = (AnimationStartedHandler)started_deploy_animation
    }, NULL);
  
    animation_schedule((Animation*) s_deploy_animation);
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction){
  switch(axis){
    case ACCEL_AXIS_X:
      break;
    case ACCEL_AXIS_Y:
      trigger_deploy_animation();
      break;
    case ACCEL_AXIS_Z:
      break;
  }
}

static void update_remainders(Layer *layer, GContext *ctx){
  if (drawRemaining && isSchoolDay){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
    gpath_rotate_to(s_rem_indicator, s_degree_end);
    gpath_draw_filled(ctx, s_rem_indicator);
    gpath_draw_outline(ctx, s_rem_indicator);
    #if PBL_SDK_3
    graphics_context_set_stroke_color(ctx, GColorYellow);
    graphics_context_set_stroke_width(ctx, 3);
    graphics_draw_arc(ctx, GRect(7, 17, 132, 132), GOvalScaleModeFitCircle, s_degree_start, s_degree_end);
    #endif
  }
}

static void update_hands(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  // minute/hour hand
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorYellow, GColorWhite));
  graphics_context_set_stroke_color(ctx, GColorBlack);
  
  gpath_rotate_to(s_minute_hand, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_hand);
  gpath_draw_outline(ctx, s_minute_hand);
  
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorYellow, GColorWhite));
  gpath_rotate_to(s_hour_hand, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_hand);
  gpath_draw_outline(ctx, s_hour_hand);
  
  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);
  
  
}

static void battery_callback(BatteryChargeState state){
  s_battery_level = state.charge_percent;
  char battText[5] = "00%%";
  snprintf(battText, sizeof(battText), "%d%%", s_battery_level);
  text_layer_set_text(s_battery_text, battText);
  #if PBL_SDK_2
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 144.0F);
  GRect newBounds = layer_get_bounds(inverter_layer_get_layer(s_battery_inverter));
  newBounds.size.w = width;
  newBounds.origin.y = 132;
  layer_set_frame(inverter_layer_get_layer(s_battery_inverter), newBounds);
  #else
  layer_mark_dirty(s_battery_layer);
  #endif
}

static void get_remaining_time(int curT, int nexT){
  int nextPerM = nexT % 100;
  int nextPerH = nexT / 100;
  int timeM = curT % 100;
  int timeH = curT / 100;
  int nextF = (nextPerH * 60) + nextPerM;
  int timeF = (timeH * 60) + timeM;
  int remTime = nextF - timeF;
  if(remTime > 60 && remTime > 0) {
    snprintf(remLabel, 12, ">60 mins");
  } else if (remTime == 0){
    snprintf(remLabel, 12, "<0 mins");
  } else {
    snprintf(remLabel, 12, "%d mins", remTime);
  }
  drawRemaining = true;
  if(nexT >= 2400){
    snprintf(remLabel, 12, "       ");
    drawRemaining = false;
  }
  remInts = remTime;
}

static void update_time() {
  //Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  if(clock_is_24h_style() == true){
    strftime(timeLabel, sizeof(timeLabel), "%H:%M", tick_time);
  } else {
    strftime(timeLabel, sizeof(timeLabel), "%I:%M %p", tick_time);
  }
  text_layer_set_text(s_time_text, timeLabel);
  char day[4];
  strftime(day, sizeof("WED"), "%a", tick_time);
  char time[5];
  strftime(time, sizeof("0000"), "%H%M", tick_time);
  int timeI = atoi(time);
  if(strcmp(day, "Wed") == 0){
    //APP_LOG(APP_LOG_LEVEL_INFO, "Wednesday");
    isSchoolDay = true;
    bitmap_layer_set_bitmap(s_schedBacker_layer, s_schedBacker_short_bitmap);
    for (int i = 0; i < 10; i++){
      if(timeI >= sched_shr_times[i]){
        continue;
      } else {
        
        get_remaining_time(timeI, sched_shr_times[i]);
        
        
        if(i >= 1){
          i -= 1;
        }
  
        text_layer_set_text(s_sched_start, sched_shr_p_start[i]);
        text_layer_set_text(s_sched_cur, sched_p_label[i]);
        text_layer_set_text(s_sched_end, sched_shr_p_end[i]);
        text_layer_set_text(s_rem_text, remLabel);
        
        s_degree_current = TRIG_MAX_ANGLE * tick_time->tm_min / 60;
        s_degree_start = s_degree_current;
        s_degree_end = s_degree_current + (TRIG_MAX_ANGLE * remInts / 60);
        layer_mark_dirty(s_remainder_ind);
        
        break;
      }
    }
  } else if(strcmp(day, "Sat") == 0 || strcmp(day, "Sun") == 0){
    //APP_LOG(APP_LOG_LEVEL_INFO, "Weekend");
    bitmap_layer_set_bitmap(s_schedBacker_layer, s_schedBacker_blank_bitmap);
    text_layer_set_text(s_sched_start, "N/A");
    text_layer_set_text(s_sched_cur, "N/A");
    text_layer_set_text(s_sched_end, "N/A");
    snprintf(remLabel, 12, "       ");
    text_layer_set_text(s_rem_text, "");
    isSchoolDay = false;
  } else {
    isSchoolDay = true;
    //APP_LOG(APP_LOG_LEVEL_INFO, "Weekday");
    bitmap_layer_set_bitmap(s_schedBacker_layer, s_schedBacker_regu_bitmap);
    for (int i = 0; i < 10; i++){
      if(timeI >= sched_reg_times[i]){
        continue;
      
      } else {
        get_remaining_time(timeI, sched_reg_times[i]);
        if(i >= 1){
          i -= 1;
        }
        text_layer_set_text(s_sched_start, sched_reg_p_start[i]);
        text_layer_set_text(s_sched_cur, sched_p_label[i]);
        text_layer_set_text(s_sched_end, sched_reg_p_end[i]);
        text_layer_set_text(s_rem_text, remLabel);
        period = i;
        
        s_degree_current = TRIG_MAX_ANGLE * tick_time->tm_min / 60;
        s_degree_start = s_degree_current;
        s_degree_end = s_degree_current + (TRIG_MAX_ANGLE * remInts / 60);
        //APP_LOG(APP_LOG_LEVEL_INFO, "End: %d, remaining: %d, current: %d, maximum: %d, time: %d", s_degree_end, remInts, s_degree_current, TRIGANGLE_TO_DEG(TRIG_MAX_ANGLE), tick_time->tm_min);
        layer_mark_dirty(s_remainder_ind);
        break;
      }
    }
  }
  
}
/*
#if PBL_SDK_3
static void drawBatteryBack(Layer *layer, GContext *ctx){
  graphics_context_set_fill_color(ctx, GColorGreen);
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 144.0F);
  graphics_fill_rect(ctx, GRect(0, 132, width, 15), 0, GCornerNone);
}
#endif
*/
static void mainWindow_load(Window *window){
  s_clock_rect = GRect(9, 7, 126, 126);
  s_batteryBack_rect = GRect(0, 132, 144, 15);
  GRect scheduleThing = GRect(0, 146, 144, 161);
  GRect shortFrame = GRect(0, -10, 144, 168);
  Layer *mainLayer = window_get_root_layer(s_mainWindow);
  //Make everything here
  //Load bitmaps
  #if PBL_BW
  s_hawkBacker_bitmap = gbitmap_create_with_resource(RESOURCE_ID_HAWK_BACKER);
  #else
  s_hawkBacker_bitmap = gbitmap_create_with_resource(RESOURCE_ID_HAWK_BACKER_COLOR);
  
  #endif
  s_schedBacker_regu_bitmap = gbitmap_create_with_resource(PBL_IF_BW_ELSE(RESOURCE_ID_SCHED_BACKER_REGU, RESOURCE_ID_COLOR_SCHED_BACKER_BLANK));
  s_schedBacker_short_bitmap = gbitmap_create_with_resource(PBL_IF_BW_ELSE(RESOURCE_ID_SCHED_BACKER_SHORT, RESOURCE_ID_COLOR_SCHED_BACKER_BLANK));
  s_schedBacker_blank_bitmap = gbitmap_create_with_resource(PBL_IF_BW_ELSE(RESOURCE_ID_SCHED_BACKER_BLANK, RESOURCE_ID_COLOR_SCHED_BACKER_BLANK));
  //Create bitmap layers
  #if PBL_BW
  s_hawkBacker_layer = bitmap_layer_create(s_clock_rect);
  #else
  s_hawkBacker_layer = bitmap_layer_create(GRect(0, 0, 144, 144));
  bitmap_layer_set_compositing_mode(s_hawkBacker_layer, GCompOpSet);
  #endif
  bitmap_layer_set_bitmap(s_hawkBacker_layer, s_hawkBacker_bitmap);
  layer_add_child(mainLayer, bitmap_layer_get_layer(s_hawkBacker_layer));
  s_schedBacker_layer = bitmap_layer_create(scheduleThing);
  bitmap_layer_set_bitmap(s_schedBacker_layer, s_schedBacker_regu_bitmap);
  
  s_remainder_ind = layer_create(shortFrame);
  layer_set_update_proc(s_remainder_ind, update_remainders);
  layer_add_child(mainLayer, s_remainder_ind);
  
  s_hands_layer = layer_create(shortFrame);
  layer_set_update_proc(s_hands_layer, update_hands);
  layer_add_child(mainLayer, s_hands_layer);
  
  #if PBL_SDK_2
  GFont BatteryFont = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  #elif PBL_SDK_3
  GFont BatteryFont = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  GColor BatteryColor = GColorYellow; //For ease of access
  //layer_set_update_proc(s_battery_layer, drawBatteryBack);
  #endif
  
  s_battery_text = text_layer_create(s_batteryBack_rect);
  text_layer_set_text_alignment(s_battery_text, GTextAlignmentCenter);
  text_layer_set_text(s_battery_text, "100%");
  text_layer_set_text_color(s_battery_text, GColorBlack);
  text_layer_set_font(s_battery_text, BatteryFont);
  text_layer_set_background_color(s_battery_text, GColorClear);
  #if PBL_SDK_3
  text_layer_set_text_color(s_battery_text, BatteryColor);
  #endif
  layer_add_child(mainLayer, text_layer_get_layer(s_battery_text));
  
  
  s_time_text = text_layer_create(s_batteryBack_rect);
  text_layer_set_text_alignment(s_time_text, GTextAlignmentRight);
  text_layer_set_text(s_time_text, "");
  text_layer_set_text_color(s_time_text, GColorBlack);
  text_layer_set_font(s_time_text, BatteryFont);
  text_layer_set_background_color(s_time_text, GColorClear);
  #if PBL_SDK_3
  text_layer_set_text_color(s_time_text, BatteryColor);
  #endif
  layer_add_child(mainLayer, text_layer_get_layer(s_time_text));
  
  
  s_rem_text = text_layer_create(s_batteryBack_rect);
  text_layer_set_text_alignment(s_rem_text, GTextAlignmentLeft);
  text_layer_set_text(s_rem_text, "");
  text_layer_set_text_color(s_rem_text, GColorBlack);
  text_layer_set_font(s_rem_text, BatteryFont);
  text_layer_set_background_color(s_rem_text, GColorClear);
  #if PBL_SDK_3
  text_layer_set_text_color(s_rem_text, BatteryColor);
  #endif
  layer_add_child(mainLayer, text_layer_get_layer(s_rem_text));
  
  #if PBL_SDK_2
  s_battery_inverter = inverter_layer_create(s_batteryBack_rect);
  layer_add_child(mainLayer, inverter_layer_get_layer(s_battery_inverter));
  #endif
  
  layer_add_child(mainLayer, bitmap_layer_get_layer(s_schedBacker_layer));
  
  
  //Adding period info
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
  #if PBL_COLOR
  text_layer_set_text_color(s_sched_start, GColorYellow);
  text_layer_set_text_color(s_sched_cur, GColorYellow);
  text_layer_set_text_color(s_sched_end, GColorYellow);
  #endif
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
  text_layer_destroy(s_battery_text);
  text_layer_destroy(s_rem_text);
  text_layer_destroy(s_time_text);
  #if PBL_SDK_2
  inverter_layer_destroy(s_battery_inverter);
  #endif
  layer_destroy(s_hands_layer);
  layer_destroy(s_battery_layer);
  layer_destroy(s_remainder_ind);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  layer_mark_dirty(window_get_root_layer(s_mainWindow));
  update_time();
}

static void init(){
  s_mainWindow = window_create();
  s_sched_dep = GPoint(0, 7);
  s_sched_ret = GPoint(0, 146);
  deployed = false;
  window_set_window_handlers(s_mainWindow, (WindowHandlers){
    .load = mainWindow_load,
    .unload = mainWindow_unload
  });
  window_set_background_color(s_mainWindow, COLOR_FALLBACK(GColorDarkGreen, GColorWhite));
  window_stack_push(s_mainWindow, true);
  
  s_minute_hand = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_hand = gpath_create(&HOUR_HAND_POINTS);
  s_rem_indicator = gpath_create(&PER_END_POINTS);
  
  Layer *window_layer = window_get_root_layer(s_mainWindow); 
  GRect bounds = layer_get_bounds(window_layer); 
  GPoint center = grect_center_point(&bounds); 
  gpath_move_to(s_minute_hand, center); 
  gpath_move_to(s_hour_hand, center); 
  gpath_move_to(s_rem_indicator, center);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  //Put registers underneath
  
  //Registering the tapper
  accel_tap_service_subscribe(tap_handler);

  //Registering the battery
  battery_callback(battery_state_service_peek());
}

static void deinit(){
  gpath_destroy(s_minute_hand);
  gpath_destroy(s_hour_hand);
  gpath_destroy(s_rem_indicator);
  window_destroy(s_mainWindow);
}

int main(){
  init();
  app_event_loop();
  deinit();
}