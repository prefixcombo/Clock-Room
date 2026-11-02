#include <pebble.h>

// Window globals
static Window *s_main_window;
static Layer *s_window_layer;
static GRect s_window_layer_bounds;
static GPoint s_center;

// Bitmap globals
static BitmapLayer *s_background_layer;
static GBitmap *s_background;
static RotBitmapLayer *s_hour_layer;
static GBitmap *s_hour;
static RotBitmapLayer *s_minute_layer;
static GBitmap *s_minute;
static BitmapLayer *s_goat_layer;
static GBitmap *s_goat;

// Time globals
static int s_time_hour = 0;
static int s_time_minute = 0;

// Calculate the minute hand angle based on the minute
// Formula: (6 degrees * minute) * 182
// The 182 comes from 65536 / 360 degrees
// The rot_bitmap_layer_set_angle function sets angle with
// values between 0 and 0x10000
static int calculate_minute_hand_angle(int minute)
{
  return (6 * minute) * 182;
}

// Calculate the hour hand angle based on the hour and minute
// Formula: (0.5 degrees * ((60 * hour) + minute)) * 182
// The 182 comes from 65536 / 360 degrees
// The rot_bitmap_layer_set_angle function sets angle with
// values between 0 and 0x10000
static int calculate_hour_hand_angle(int hour, int minute)
{
  return (0.5 * ((60 * hour) + minute)) * 182;
}

// Get current time and set the hour and minute globals so hands can update
static void update_time(struct tm* tick_time)
{
  s_time_hour = tick_time->tm_hour;
  s_time_minute = tick_time->tm_min;
}

// Set the hand angles based on the time globals
static void update_hands()
{
  rot_bitmap_layer_set_angle(s_hour_layer, calculate_hour_hand_angle(s_time_hour, s_time_minute));
  rot_bitmap_layer_set_angle(s_minute_layer, calculate_minute_hand_angle(s_time_minute));
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  // Need to update the time globals first then update the hands
  update_time(tick_time);
  update_hands();
}

// Center a frame's points on the window given its size
static void center_frame_points(GPoint* point, GSize size)
{
  point->x = s_center.x - (size.w / 2);
  point->y = s_center.y - (size.h / 2);
}

// Get a frame centered on the window for a particular bitmap
static GRect get_centered_frame(GBitmap* bitmap)
{
  GRect frame = {0};
  GRect bounds = gbitmap_get_bounds(bitmap);

  // Center frame and set size
  center_frame_points(&frame.origin, bounds.size);
  frame.size.w = bounds.size.w;
  frame.size.h = bounds.size.h;

  return frame;
}

// Load a rotbitmaplayer for a hand and add it to the window layer
static void load_hand_layer(uint32_t resource_id, GBitmap** bitmap, RotBitmapLayer** rotbitmap_layer)
{
  // Create hour hand layer
  *bitmap = gbitmap_create_with_resource(resource_id);
  *rotbitmap_layer = rot_bitmap_layer_create(*bitmap);
  
  // Use GCompOpSet alignment otherwise the sprite transparency is not in effect causing black bars
  rot_bitmap_set_compositing_mode(*rotbitmap_layer, GCompOpSet);
  
  // Get the bounds of the original bitmap to calculate the rotation point. Rotation point for the hands
  // is width / 2 of sprite, and 26 pixels less than height
  GRect bitmap_frame = gbitmap_get_bounds(*bitmap);
  GPoint rotation_point = { .x = (bitmap_frame.size.w / 2), .y = (bitmap_frame.size.h - 26)};
  rot_bitmap_set_src_ic(*rotbitmap_layer, rotation_point);
  
  // Need to center the hand on the clock face in the middle of the screen. 
  // I couldn't get the bitmap to show up on Basalt and Chalk unless I set 
  // a frame before adding the rotbitmap layer to the window layer
  GRect frame = layer_get_frame((Layer *)*rotbitmap_layer);
  center_frame_points(&frame.origin, frame.size);
  layer_set_frame((Layer *)*rotbitmap_layer, frame);
  layer_add_child(s_window_layer, (Layer *)*rotbitmap_layer);
}

// Load a bitmaplayer centered on the window
static void load_centered_bitmap_layer(uint32_t resource_id, GBitmap** bitmap, BitmapLayer** bitmap_layer)
{
  *bitmap = gbitmap_create_with_resource(resource_id);
  *bitmap_layer = bitmap_layer_create(get_centered_frame(*bitmap));
  bitmap_layer_set_compositing_mode(*bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(*bitmap_layer, *bitmap);
  layer_add_child(s_window_layer, (Layer *)*bitmap_layer);
}

static void load_background_layer()
{
  // Determine which background will be loaded based on window layer size.
  // There are two background versions, on Basalt and Chalk the 260x260 
  // version couldn't be loaded into memory so I needed to make a 180x180 
  // version for the older platforms
  bool full_size = (s_window_layer_bounds.size.w > 180);

  // Get the background size
  int background_w = full_size ? 260 : 180;
  int background_h = full_size ? 260 : 180;

  // If the window size is smaller than the background, it needs to be centered, 
  // offset for the background is calculated with the following formula:
  // (window size - backgound size) / 2
  int x_offset = (s_window_layer_bounds.size.w - background_w) / 2;
  int y_offset = (s_window_layer_bounds.size.h - background_h) / 2;
  
  // Create background layer based on screen size and offset it to center the background on the window
  s_background = gbitmap_create_with_resource(full_size ? RESOURCE_ID_CLOCK_ROOM_FULL : RESOURCE_ID_CLOCK_ROOM_SMALL);
  s_background_layer = bitmap_layer_create(GRect(x_offset, y_offset, background_w, background_h));
  bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_background_layer, s_background);
  layer_add_child(s_window_layer, (Layer *)s_background_layer);
}

static void main_window_load(Window *window)
{
  // Get information about the Window
  s_window_layer = window_get_root_layer(window);
  s_window_layer_bounds = layer_get_bounds(s_window_layer);
  s_center = grect_center_point(&s_window_layer_bounds);
  
  // Create the background layer
  load_background_layer();

  // Create the hour hand layer
  load_hand_layer(RESOURCE_ID_HOUR_HAND, &s_hour, &s_hour_layer);
  
  // Create the minute hand layer
  load_hand_layer(RESOURCE_ID_MINUTE_HAND, &s_minute, &s_minute_layer);
  
  // Create the goat layer centered on the window
  load_centered_bitmap_layer(RESOURCE_ID_GOAT, &s_goat, &s_goat_layer);
}

static void main_window_unload(Window *window)
{
  // Unload the bitmaps and layers
  gbitmap_destroy(s_background);
  bitmap_layer_destroy(s_background_layer);
  gbitmap_destroy(s_hour);
  rot_bitmap_layer_destroy(s_hour_layer);
  gbitmap_destroy(s_minute);
  rot_bitmap_layer_destroy(s_minute_layer);
  gbitmap_destroy(s_goat);
  bitmap_layer_destroy(s_goat_layer);
}

static void init()
{
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set the background color
  window_set_background_color(s_main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers)
  {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  time_t temp = time(NULL);
  struct tm *initial_time = localtime(&temp);
  update_time(initial_time);
  update_hands();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit()
{
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}