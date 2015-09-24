#include <pebble.h>

//Global variable definitions
static Window *s_mainWindow;
static BitmapLayer *s_hawkBacker_layer;
static GBitmap *s_hawkBacker_bitmap;


static void mainWindow_load(Window *window){
  Layer *mainLayer = window_get_root_layer(s_mainWindow);
  GRect fullFrame = GRect(0, 0, 144, 168);
  //Make everything here
  //Load bitmaps
  s_hawkBacker_bitmap = gbitmap_create_with_resource(RESOURCE_ID_HAWK_BACKER);
  //Create bitmap layers
  s_hawkBacker_layer = bitmap_layer_create(GRect(71, 65, 125, 125));
  bitmap_layer_set_bitmap(s_hawkBacker_layer, s_hawkBacker_bitmap);
  layer_add_child(mainLayer, bitmap_layer_get_layer(s_hawkBacker_layer));
}

static void mainWindow_unload(Window *window){
  //Destroy everything in here
  gbitmap_destroy(s_hawkBacker_bitmap);
  bitmap_layer_destroy(s_hawkBacker_layer);
}

static void init(){
  s_mainWindow = window_create();
  
  window_set_window_handlers(s_mainWindow, (WindowHandlers){
    .load = mainWindow_load,
    .unload = mainWindow_unload
  });
  
  window_stack_push(s_mainWindow, true);
  
  //Put registers underneath
}

static void deinit(){
  window_destroy(s_mainWindow);
}

int main(){
  init();
  app_event_loop();
  deinit();
}