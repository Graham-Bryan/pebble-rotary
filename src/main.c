#include <pebble.h>
  
static Window *s_main_window;
static  Layer *window_layer;
static GRect window_bounds;
static GPoint window_center;

static TextLayer *s_hour_layer;
static TextLayer *s_minute_layer;

static TextLayer *s_date_layer;

static int32_t minute_angle;
static Animation *s_minute_hand_animation;

static PropertyAnimation *s_date_enter_animation;
static PropertyAnimation *s_date_exit_animation;

#ifdef PBL_COLOR
static const int bg_color_count = 9;
static GColor bg_colors[9];
static int current_color;
#endif

static GSize minute_size;

static GFont large_font;
static GFont small_font;

static int anim_percentage(AnimationProgress dist_normalized, int max) {
  return (int)(float)(((float)dist_normalized / (float)ANIMATION_NORMALIZED_MAX) * (float)max);
}


void anim_callback(Animation *anim, AnimationProgress dist_normalized){
  int step = anim_percentage(dist_normalized, 360);

  float t = (float)step / 360.0f;

  int anim_angle = (int)((float)TRIG_MAX_ANGLE * t);
  anim_angle += minute_angle;
  GPoint minute_hand = {
    .x = ((int16_t)(sin_lookup(anim_angle) * (int32_t)60 / TRIG_MAX_RATIO) + window_center.x) - (minute_size.w / 2),
    .y = ((int16_t)(-cos_lookup(anim_angle) * (int32_t)60 / TRIG_MAX_RATIO) + window_center.y) - (minute_size.h / 2),
  };
  
  layer_set_frame((Layer *)s_minute_layer, GRect(minute_hand.x,minute_hand.y, minute_size.w, minute_size.h));
}


static void show_date()
{
  GRect from_frame = GRect(0,0,window_bounds.size.w,window_bounds.size.h);
  GRect to_frame = GRect(0,-22,window_bounds.size.w,window_bounds.size.h + 22);

  s_date_enter_animation = property_animation_create_layer_frame((Layer*)window_layer, &from_frame, &to_frame);
  animation_set_curve((Animation*)s_date_enter_animation, AnimationCurveEaseInOut);  
  animation_set_duration((Animation*)s_date_enter_animation, 500);

  s_date_exit_animation = property_animation_create_layer_frame((Layer*)window_layer, &to_frame, &from_frame);
  animation_set_curve((Animation*)s_date_exit_animation, AnimationCurveEaseInOut);  
  animation_set_duration((Animation*)s_date_exit_animation, 500);
  animation_set_delay((Animation*)s_date_exit_animation, 2500);    
  
  animation_schedule((Animation*) s_date_enter_animation);  
  animation_schedule((Animation*) s_date_exit_animation);    
}

static void update_time() {

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char hour_buffer[] = "00";
  static char minute_buffer[] = "00";
  static char date_buffer[] = "AAA BBB 00";  


  if(clock_is_24h_style() == true) {
    strftime(hour_buffer, sizeof("00"), "%H", tick_time);
  } else {
    if (tick_time->tm_hour <= 12)
      snprintf (hour_buffer, 3, "%d", tick_time->tm_hour);
    else
      snprintf (hour_buffer, 3, "%d", tick_time->tm_hour - 12);
  }
  strftime(minute_buffer, sizeof("00"), "%M", tick_time);  
  strftime(date_buffer, sizeof("AAA BBB 00"), "%a %b %e", tick_time); 

  text_layer_set_text(s_hour_layer, hour_buffer);
  text_layer_set_text(s_minute_layer, minute_buffer);  
  text_layer_set_text(s_date_layer, date_buffer);    
 
  minute_size = graphics_text_layout_get_content_size(text_layer_get_text(s_minute_layer), small_font, GRect(0,0,144,35), GTextOverflowModeWordWrap, GTextAlignmentLeft);
  GSize hour_size = graphics_text_layout_get_content_size(text_layer_get_text(s_hour_layer), large_font, GRect(0,0,144,70), GTextOverflowModeWordWrap, GTextAlignmentLeft);
  hour_size.h += 20;

  GRect frame = GRect((window_bounds.size.w / 2) - (hour_size.w / 2),(window_bounds.size.h / 2) - (hour_size.h / 2), hour_size.w, hour_size.h);
  layer_set_frame((Layer *)s_hour_layer, frame);

  minute_angle = TRIG_MAX_ANGLE * ((float)tick_time->tm_min / 60.0f);
  GPoint minute_hand = {
    .x = ((int16_t)(sin_lookup(minute_angle) * (int32_t)60 / TRIG_MAX_RATIO) + window_center.x) - (minute_size.w / 2),
    .y = ((int16_t)(-cos_lookup(minute_angle) * (int32_t)60 / TRIG_MAX_RATIO) + window_center.y) - (minute_size.h / 2),
  };

  frame = GRect(minute_hand.x,minute_hand.y, minute_size.w, minute_size.h);
  layer_set_frame((Layer *)s_minute_layer, frame);

  #ifdef PBL_COLOR
    current_color = (tick_time->tm_hour % bg_color_count);
    window_set_background_color(s_main_window, bg_colors[current_color]);
  #else
    window_set_background_color(s_main_window, GColorBlack);
  #endif
}

static const PropertyAnimationImplementation animimp = {
  .base = {
    .update = (AnimationUpdateImplementation) anim_callback,
  },
};

static void animate_time()
{

  animation_destroy(s_minute_hand_animation);
  s_minute_hand_animation = (Animation*)property_animation_create(&animimp, s_minute_layer, NULL, NULL);
  animation_set_duration((Animation*)s_minute_hand_animation, 2000);

  animation_set_curve((Animation*)s_minute_hand_animation, AnimationCurveEaseInOut);
  animation_schedule((Animation*)s_minute_hand_animation);  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
   animate_time();
}

static void main_window_load(Window *window) {

  window_layer = window_get_root_layer(s_main_window);
  window_bounds = layer_get_bounds(window_layer);
  window_center = grect_center_point (&window_bounds);  
  
  large_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_74));
  small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MINUTE_18));  

  s_hour_layer = text_layer_create(GRect(0,0,100,70));
  text_layer_set_background_color(s_hour_layer, GColorClear);
  text_layer_set_text_color(s_hour_layer, GColorWhite);
  text_layer_set_font(s_hour_layer, large_font);
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentLeft);
  
  s_minute_layer = text_layer_create(GRect(0,0,50,35));
  text_layer_set_background_color(s_minute_layer, GColorClear);
  text_layer_set_text_color(s_minute_layer, GColorWhite);
  text_layer_set_font(s_minute_layer, small_font);
  text_layer_set_text_alignment(s_minute_layer, GTextAlignmentLeft);
  
  s_date_layer = text_layer_create(GRect(0,window_bounds.size.h,window_bounds.size.w,35));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, small_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter); 

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_hour_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_minute_layer));  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
}

static void main_window_unload(Window *window) {

}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  show_date();
}

static void init() {

  #ifdef PBL_COLOR
    current_color = 0;
    bg_colors[0] = GColorOrange ;
    bg_colors[1] = GColorDarkGreen ;
    bg_colors[2] = GColorDukeBlue ;
    bg_colors[3] =  GColorDarkCandyAppleRed ;
    bg_colors[4] =  GColorImperialPurple ;
    bg_colors[5] =  GColorJaegerGreen ;
    bg_colors[6] =  GColorOxfordBlue ;
    bg_colors[7] =  GColorBulgarianRose ;
    bg_colors[8] =  GColorCobaltBlue  ;
  #endif
    
    







    
  s_main_window = window_create();

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  
  accel_tap_service_subscribe(tap_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  update_time(); 
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}