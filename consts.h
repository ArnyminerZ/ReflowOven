#ifndef __CONSTS_H
#define __CONSTS_H

#define WIDTH  240
#define HEIGHT 320

// The baud rate for the Serial Port
#define SERIAL_SPEED 115200

// Whether or not to invert the relay
#define RELAY_INVERT true

#define BUTTON_PIN 16
#define RELAY_TOP_PIN 5 // D2
#define RELAY_BOT_PIN 4 // D1
#define BUZZER_PIN 2
#define DOOR_SWITCH PIN_D4
#define THERM_PIN A0

// Constants for Thermistor
#define THERM_VCC 3.3   // Connected to board's 3.3V pin
#define THERM_RES 1023  // The resolution of the ADC
#define THERM_R   10000 // Resistor of 10k
#define THERM_A   -0.001409530395     // 0.001129148
#define THERM_B    0.0005396718651    // 0.000234125
#define THERM_C   -0.0000009499650276 // 0.0000000876741

// Color definitions
#define COLOR_BACKGROUND 0x31A6
#define COLOR_TEMP_BG 0xC638 // Background color of the temperature display
#define COLOR_TEMP_BD 0xC945 // Border color of the temperature display
#define COLOR_TEMP_FF 0xC945 // Text color of the temperature display when turned off
#define COLOR_TEMP_FO 0x1E03 // Text color of the temperature display when reflowing
#define COLOR_TEMP_FI 0x2BF9 // Text color of the temperature display when idle
#define COLOR_ALARM_DOOR_BG 0xF481           // Background color of the open door alarm
#define COLOR_ALARM_DOOR_FG COLOR_BACKGROUND // Text color color of the open door alarm
#define COLOR_STAT_BG 0xC638 // Background color of the status display
#define COLOR_STAT_BD 0xC945 // Border color of the status display
#define COLOR_STAT_FG 0xC945 // Text color of the status display

// Timing constants
#define DISP_TEMP 1000   // The amount of time between screen temperature updates
#define DISP_STAT 1000   // The amount of time between screen status updates
#define DISP_OTA 1000    // The amount of time between OTA status updates
#define ALARM_CYCLE 1000 // The cycle time of the alarm for beeping (half time beep on, half time beep off)
#define BUTTON_TIME 3000 // The amount of time the button must be pressed for the process to start
#define PWM_CYCLE 1000   // The time that it takes for the pwm controller to make a cycle

// Reflow constants
#define REFLOW_STAGE_OFF -2
#define REFLOW_STAGE_HOLD -1
#define REFLOW_STAGE_PREHEAT 0
#define REFLOW_STAGE_SOAK 1
#define REFLOW_STAGE_REFLOW 2
#define REFLOW_STAGE_COOL 3

#define POWER_PREHEAT_1_TOP 0
#define POWER_PREHEAT_1_BOT 3
#define POWER_PREHEAT_2_TOP 1
#define POWER_PREHEAT_2_BOT 4
#define POWER_PREHEAT_3_TOP 3
#define POWER_PREHEAT_3_BOT 4

#define POWER_SOAK_1_TOP 2
#define POWER_SOAK_1_BOT 3
#define POWER_SOAK_2_TOP 2
#define POWER_SOAK_2_BOT 4
#define POWER_SOAK_3_TOP 3
#define POWER_SOAK_3_BOT 4

#define POWER_REFLOW_1_TOP 3
#define POWER_REFLOW_1_BOT 4
#define POWER_REFLOW_2_TOP 3
#define POWER_REFLOW_2_BOT 4
#define POWER_REFLOW_3_TOP 3
#define POWER_REFLOW_3_BOT 4

// Sensor constants
#define SENSOR_TEMP_CYCLES 100 // The amount of measures to make for the temperature sensor

// Web server constants
#define MDNS_HOST "reflow-oven"
#define OTA_PORT 8266

// Log constants
#define TAG_SCREEN "Screen"
#define TAG_CORE "Core"
#define TAG_IO "I/O"

#endif
