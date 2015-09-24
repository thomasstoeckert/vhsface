#include <pebble.h>

//Global variable definitions
static Window *s_mainWindow;

static void mainWindow_load(Window *window){
  //Make everything here
}

static void mainWindow_unload(Window *window){
  //Destroy everything in here
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