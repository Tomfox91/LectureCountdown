#include "pebble.h"





Window * window;
TextLayer * countdown_layer;
TextLayer * to_layer;
TextLayer * clock_layer;
TextLayer * date_layer;
TextLayer * bt_layer;
TextLayer * battery_layer;




enum com_codes {
	CODE_MON = 0,
	CODE_TUE = 1,
	CODE_WED = 2,
	CODE_THU = 3,
	CODE_FRI = 4,
	CODE_SAT = 5,
	CODE_SUN = 6,
	CODE_OVERRIDE = 10,
	CODE_OVERRIDE_FULL = 11
};




#define SCHEDULE_SIZE 10
short schedule[SCHEDULE_SIZE] = {};
int schedule_day = -1;
int override = -1;

static void load_schedule(int weekday) {
	int code;
	
	APP_LOG(APP_LOG_LEVEL_INFO, "Loading day %d", weekday);
	
	switch (weekday) {
		case 0:
			code = CODE_SUN;
			break;
			
		case 1:
			code = CODE_MON;
			break;
			
		case 2:
			code = CODE_TUE;
			break;
			
		case 3:
			code = CODE_WED;
			break;
			
		case 4:
			code = CODE_THU;
			break;
			
		case 5:
			code = CODE_FRI;
			break;
			
		case 6:
			code = CODE_SAT;
			break;
			
		default:
			code = -1;
			break;
	}
	
	if (!persist_exists(code)) {
		for (int i = 0; i < SCHEDULE_SIZE; i++) {
			schedule	[i] = 0;
		}
	} else {
		persist_read_data(code, &schedule, sizeof(schedule));
		schedule_day = weekday;
	}
	schedule_day = weekday;
}

static short time_to_min(struct tm* t) {
	return (t->tm_hour) * 60 + (t->tm_min);
}

static void write_countdown(struct tm* tick_time) {
	static char countdown_text[] = "- 0000";
	static char to_text[] = "(override)";

	static int deb_cont = 0;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Minute tick %d. Current day: %d Loaded: %d", deb_cont++, tick_time->tm_wday, schedule_day);
	
	if (schedule_day != tick_time->tm_wday) {
		load_schedule(tick_time->tm_wday);
	}
	
	short min = time_to_min(tick_time);
	
	if (override >= 0) {
		if (override < min) {
			override = -1;
			persist_write_int(CODE_OVERRIDE, -1);
			persist_delete(CODE_OVERRIDE_FULL);
		} else {
			snprintf(countdown_text, sizeof(countdown_text), "%d", -(override - min));
			snprintf(to_text, sizeof(to_text), "(override)");
			text_layer_set_text(countdown_layer, countdown_text);
			text_layer_set_text(to_layer, to_text);
			return;
		}
	}
	
	
	for (int i = 0; i < SCHEDULE_SIZE; i++) {
		short si = schedule[i];
		if (si >= min) {
			snprintf(countdown_text, sizeof(countdown_text), "%d", -(si - min));
			snprintf(to_text, sizeof(to_text), "to %d:%02d", si/60, si%60);
			text_layer_set_text(countdown_layer, countdown_text);
			text_layer_set_text(to_layer, to_text);
			return;
		}
	}
	
	snprintf(countdown_text, sizeof(countdown_text), "End");
	snprintf(to_text, sizeof(to_text), " ");
	text_layer_set_text(countdown_layer, countdown_text);
	text_layer_set_text(to_layer, to_text);
	return;
}

static void handle_minute_tick(struct tm* tick_time, TimeUnits units_changed) {
	
	write_countdown(tick_time);
	
	static char clock_text[] = "00:00";
	static char date_text[] = "xxx 00/00/0000";
	
	strftime(clock_text, sizeof(clock_text), "%H:%M", tick_time);
	text_layer_set_text(clock_layer, clock_text);
	
	
	strftime(date_text, sizeof(date_text), "%a %d/%m/%Y", tick_time);
	text_layer_set_text(date_layer, date_text);
}





static void handle_bluetooth(bool connected) {
	static char stat[] = "B";
	
	if (connected) {
		stat[0] = 'B';
	} else {
		stat[0] = ' ';
	}
	
	text_layer_set_text(bt_layer, stat);
}





static void handle_battery(BatteryChargeState s) {
	static char stat[] = "100%";
	
	snprintf(stat, sizeof(stat), "%d%%", s.charge_percent);
	
	text_layer_set_text(battery_layer, stat);
}





static TextLayer* create_text_layer_f(GRect rect, GFont font, GTextAlignment align, GColor text, GColor back) {
	TextLayer* l = text_layer_create(rect);
	
	text_layer_set_text_color      (l, text);
	text_layer_set_background_color(l, back);
	text_layer_set_font            (l, fonts_get_system_font(font));
	text_layer_set_text_alignment  (l, align);
	
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(l));
	
	return l;
}

static TextLayer* create_text_layer_a(GRect rect, GFont font, GTextAlignment align) {
	return create_text_layer_f(rect, font, align, GColorWhite, GColorBlack);
}

static TextLayer* create_text_layer(GRect rect, GFont font) {
	return create_text_layer_f(rect, font, GTextAlignmentCenter, GColorWhite, GColorBlack);
}


static void init_ui() {
	window = window_create();
	window_stack_push(window, true);
	window_set_background_color(window, GColorBlack);
	
	bt_layer = create_text_layer_a(GRect(5, 5, 20, 18), FONT_KEY_GOTHIC_18, GTextAlignmentLeft);
	battery_layer = create_text_layer_a(GRect(144-45, 5, 40, 18), FONT_KEY_GOTHIC_18, GTextAlignmentRight);
	countdown_layer = create_text_layer(GRect(10, 28, 144-20, 45), FONT_KEY_BITHAM_42_BOLD);
	to_layer = create_text_layer(GRect(2, 74, 144-4, 20), FONT_KEY_GOTHIC_18);
	clock_layer = create_text_layer(GRect(2, 106, 144-4, 30), FONT_KEY_GOTHIC_28_BOLD);
	date_layer = create_text_layer(GRect(2, 140, 144-4, 30), FONT_KEY_GOTHIC_18);
	
	time_t now = time(NULL);
	struct tm *current_time = localtime(&now);
	handle_minute_tick(current_time, MINUTE_UNIT);
	tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
	
	handle_bluetooth(bluetooth_connection_service_peek());
	bluetooth_connection_service_subscribe(&handle_bluetooth);
	
	handle_battery(battery_state_service_peek());
	battery_state_service_subscribe(&handle_battery);
}

static void deinit_ui() {
	text_layer_destroy(countdown_layer);
	text_layer_destroy(to_layer);
	text_layer_destroy(clock_layer);
	text_layer_destroy(date_layer);
	text_layer_destroy(bt_layer);
	text_layer_destroy(battery_layer);
	window_destroy(window);
}





void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple* tp = dict_read_first(received);
	
	do {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received message key: %d length: %d type: %d", (int) tp->key, tp->length, tp->type);
		
		if (tp->key <= CODE_SUN && tp->type == TUPLE_BYTE_ARRAY) {
			// schedule for a day
			
			short sched[SCHEDULE_SIZE] = {};
			
			for (int i = 0; i < tp->length; i+=2) {
				short x = (256 * ((short) tp->value->data[i]) + ((short) tp->value->data[i+1]));
				APP_LOG(APP_LOG_LEVEL_DEBUG, "\tNum: %d", x);
				sched[i/2] = x;
			}
			
			int res = persist_write_data(tp->key, sched, sizeof(sched));
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Written %d bytes", res);
			
		} else if (tp->key == CODE_OVERRIDE && tp->type == TUPLE_INT && tp->length == 4) {
			override = tp->value->int32;
			persist_write_int(CODE_OVERRIDE, override);
			
			// this is the last message, so we reset the UI
			schedule_day = -1;
			time_t now = time(NULL);
			struct tm *current_time = localtime(&now);
			handle_minute_tick(current_time, MINUTE_UNIT);
			
			// saving the time of the override countdown
			if (override >= 0) {
				int deadline = now + (- 3600*current_time->tm_hour
									  -   60*current_time->tm_min
									  -      current_time->tm_sec
									  +   60*override);
				
				persist_write_int(CODE_OVERRIDE_FULL, deadline);
			} else {
				persist_delete(CODE_OVERRIDE_FULL);
			}
			
		} else {
			APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown message");
		}
		
	} while ( (tp = dict_read_next(received)) );
}




static void init_mess() {
	app_message_register_inbox_received(&in_received_handler);
	
	app_message_open(124, 64);
}




static void init_pers() {
	if (persist_exists(CODE_OVERRIDE)) {
		override = persist_read_int(CODE_OVERRIDE);
	}
	if (persist_exists(CODE_OVERRIDE_FULL)) {
		int override_full = persist_read_int(CODE_OVERRIDE_FULL);
		int now = time(NULL);
		if (now > override_full) {
			override = -1;
			persist_delete(CODE_OVERRIDE_FULL);
		}
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Deleting old override");
	}
}





















static void do_init() {
	init_pers();
	init_mess();
	init_ui();
}

static void do_deinit() {
	deinit_ui();
}

// The main event/run loop for our app
int main(void) {
	do_init();
	app_event_loop();
	do_deinit();
}
