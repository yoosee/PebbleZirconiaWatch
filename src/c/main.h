#pragma once

// Message is automatically assigned. Now it's used only for persist storage

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

#define KEY_IS_FAHRENHEIT 2
#define KEY_IS_STEPS_ENABLED 3

#define KEY_COLOR_BACKGROUND 10
#define KEY_COLOR_CLOCK 11
#define KEY_COLOR_STEPS 12
#define KEY_COLOR_WEATHER 13
#define KEY_COLOR_DATE 14

#define KEY_WEATHER_TEMP 20
#define KEY_WEATHER_CONDITION 21
#define KEY_WEATHER_LAST_UPDATED 22

// Watch Configuration resources

#define NUM_CLOCK_TICKS 8

#define TICK_UPDATE_SECONDS 30
#define WEATHER_UPDATE_INTERVAL_MINUTES 30

#define COLOR_DEFAULT_BACKGROUND 0x001e41
#define COLOR_DEFAULT_CLOCK 0xffffff
#define COLOR_DEFAULT_STEPS 0x4cb4db
#define COLOR_DEFAULT_WEATHER 0x4cb4db
#define COLOR_DEFAULT_DATE 0xffffff

