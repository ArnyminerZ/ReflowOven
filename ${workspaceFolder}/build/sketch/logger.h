#line 1 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\logger.h"
#ifndef __LOGGER_h
#define __LOGGER_h

void logInit() {
  #ifdef LOGGER
  Serial.begin(SERIAL_SPEED);
  Serial.println("=============================");
  Serial.println("  REFLOW OVEN by Arnau Mora");
  Serial.println("=============================");
  #endif
}

void logln(String tag, String message) {
  #ifdef LOGGER
  Serial.println(tag + " > " + message);
  #endif
}

#endif
