#include <Arduino.h>
#line 1 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino"

/**
 * Made by Arnau Mora
 * 
 * Screen controller by http://www.multicopterox.es/display-ili9341-spi-y-esp8266-nodemcu/
 * Reflowing algorithm by https://github.com/thedalles77/Reflow_Oven
 */

#include <SPI.h>
#include <TFT_eSPI.h>      // Hardware-specific library

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// Comment to disable logging to Serial port
#define LOGGER

#include "consts.h"
#include "logger.h"

// Timing storage
unsigned long loopTimer;
unsigned long dispTempMillis;  // The millis at last screen temperature update
unsigned long dispStatMillis;  // The millis at last screen status update
unsigned long beepMillis;      // The millis at last beep cycle
unsigned long buttonMillis;    // The millis the button has been pressed

// Notifications
bool displayReset = true;
bool displayedNotification = false;
bool displayingAnyNotification = false;
bool showOpenDoorAlarm = true; // If the alarm for opening the door should be shown

// Sensor storage
double temperature;
short temperatureCounter;
double temperatureAccum;

// Reflow storage
int topHeater = 0;
int botHeater = 0;
short reflowStage;
short reflowSubstage;
double reflowMaxTemp;
unsigned long reflowStart;   // The millis at the starting of reflowing
unsigned long reflowEl50;    // Elapsed seconds at 50ºC
unsigned long reflowEl150;   // Elapsed seconds at 150ºC
unsigned long reflowEl180up; // Elapsed seconds at 180ºC when heating
unsigned long reflowEl180dw; // Elapsed seconds at 180ºC when cooling
unsigned long reflowElPeak;  // Elapsed seconds at peak temperature
unsigned long reflowDoorOpen;    // Elapsed seconds when the door gets open

// Function definitions
void measureTemp();
void resetDisplay();
void displayTemp();
void displayStatus();
void displayAlarms();
void displayOpenDoorAlarm();
void pwmController();
void reflowController();

#line 62 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino"
void setup();
#line 116 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino"
void loop();
#line 62 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino"
void setup() {
  tft.init();
  tft.setRotation(0);
  
  logInit();

  logln(TAG_SCREEN, "Showing boot screen...");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Reflow Oven", 120, 70, 4); // Draw x=120, y=70, size=4
  tft.drawCentreString("by Arnau Mora", 120, 90, 2);
  tft.drawString("Booting...", 10, HEIGHT - 20, 2);

  logln(TAG_CORE, "Initializing timing variables...");
  dispTempMillis = millis();
  dispStatMillis = millis();
  loopTimer = millis();
  beepMillis = millis();
  buttonMillis = millis();

  logln(TAG_IO, "Configuring ports...");
  pinMode(RELAY_TOP_PIN, OUTPUT); // Define the top relay pin as output
  pinMode(RELAY_BOT_PIN, OUTPUT); // Define the bottom relay pin as output
  topHeater = 0;                  // Turn off the relays
  botHeater = 0;

  pinMode(BUZZER_PIN, OUTPUT); // Define the beeper output

  pinMode(DOOR_SWITCH, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN_16);

  // Initialize reflow variables
  logln(TAG_CORE, "Initializing reflow variables...");
  reflowStage = REFLOW_STAGE_OFF;
  reflowSubstage = 0;
  reflowMaxTemp = 0;

  // Initialize sensor variables
  logln(TAG_CORE, "Initializing sensor variables...");
  temperature = 0.0;
  temperatureAccum = 0.0;
  temperatureCounter = 0;

  // Show boot screen for at least 3 seconds
  logln(TAG_SCREEN, "Waiting for 3 seconds...");
  while(millis() - loopTimer < 3000) {
    delay(10);
  }
  loopTimer = millis();

  // Initialize the home screen
  resetDisplay();
}

void loop() {
  // Perform sensor readings
  measureTemp();
  
  
  // Reset screen when no alarm displayed
  if (displayedNotification && !displayingAnyNotification) {
    resetDisplay();
    displayedNotification = false;
  }


  // Play beep if any alarm playing
  if (showOpenDoorAlarm && !digitalRead(DOOR_SWITCH)) {
    unsigned long elapsedBeep = millis() - beepMillis;

    logln(TAG_CORE, "Beep = " + String(elapsedBeep < ALARM_CYCLE));
    digitalWrite(BUZZER_PIN, elapsedBeep < ALARM_CYCLE);
    
    if (elapsedBeep >= ALARM_CYCLE * 2) {
      beepMillis = millis();
    }
  } else
    digitalWrite(BUZZER_PIN, LOW);


  if (digitalRead(BUTTON_PIN)) {
    logln(TAG_CORE, "Hold.");
    if(reflowStage < 0)
      reflowStage = REFLOW_STAGE_HOLD;
  } else {
    buttonMillis = millis();
    if(reflowStage < 0)
      reflowStage = REFLOW_STAGE_OFF;
  }
  if (millis() - buttonMillis > BUTTON_TIME && reflowStage == REFLOW_STAGE_HOLD) {
    logln(TAG_CORE, "Starting reflow!");
    reflowStage = REFLOW_STAGE_PREHEAT;
  }

    
  bool updatedDisplay = false;
  if(displayReset || millis() - dispTempMillis >= DISP_TEMP) {
    displayTemp();
    dispTempMillis = millis();
    updatedDisplay = true;
  }
  if(displayReset || millis() - dispStatMillis >= DISP_STAT) {
    displayStatus();
    dispStatMillis = millis();
    updatedDisplay = true;
  }
  if (updatedDisplay) {
    displayAlarms();
  }
  // Restore the display reset flag
  displayReset = false;

  reflowController();

  pwmController();
}

void resetDisplay() {
  logln(TAG_SCREEN, "Reset!");
  tft.fillScreen(COLOR_BACKGROUND);
  
  tft.setTextColor(TFT_WHITE, COLOR_BACKGROUND);
  tft.drawString("Reflow oven by Arnau Mora", 5, HEIGHT - 20, 2);

  displayReset = true;
}

void displayTemp() {
  double temp = temperature - 273.15;
  
  tft.fillRect(10, 10, WIDTH-20, 50, COLOR_TEMP_BG);
  tft.drawRect(10, 10, WIDTH-20, 50, COLOR_TEMP_BD);
  if (reflowStage == REFLOW_STAGE_OFF)
    tft.setTextColor(COLOR_TEMP_FF, COLOR_TEMP_BG);
  else if (topHeater <= 0 && topHeater <= 0)
    tft.setTextColor(COLOR_TEMP_FO, COLOR_TEMP_BG);
  else
    tft.setTextColor(COLOR_TEMP_FI, COLOR_TEMP_BG);
  tft.drawCentreString(String(temp) + " C", WIDTH/2, 25, 4);

  if (reflowStage == REFLOW_STAGE_OFF) {
    tft.fillCircle(35, 35, 10, COLOR_TEMP_FF);
    tft.fillCircle(WIDTH - 35, 35, 10, COLOR_TEMP_FF);
  } else if (topHeater <= 0 && topHeater <= 0) {
    tft.fillCircle(35, 35, 10, COLOR_TEMP_FO);
    tft.fillCircle(WIDTH - 35, 35, 10, COLOR_TEMP_FO);
  } else {
    tft.fillCircle(35, 35, 10, COLOR_TEMP_FI);
    tft.fillCircle(WIDTH - 35, 35, 10, COLOR_TEMP_FI);
  }
}

void displayStatus() {
  tft.fillRect(10, 70, WIDTH-20, 50, COLOR_STAT_BG);
  tft.drawRect(10, 70, WIDTH-20, 50, COLOR_STAT_BD);
  tft.setTextColor(COLOR_STAT_FG, COLOR_STAT_BG);
  
  String status;
  if (reflowStage == REFLOW_STAGE_OFF)
    status = "Off";
  else if (reflowStage == REFLOW_STAGE_HOLD)
    status = "Hold";
  else if (reflowStage == REFLOW_STAGE_PREHEAT)
    status = "Preheat " + String(reflowSubstage);
  else if (reflowStage == REFLOW_STAGE_SOAK)
    status = "Soak " + String(reflowSubstage);
  else if (reflowStage == REFLOW_STAGE_REFLOW)
    status = "Reflow" + String(reflowSubstage);
  else if (reflowStage == REFLOW_STAGE_COOL)
    status = "Cooldown " + String(reflowSubstage);
  else
    status = "Unknown (" + String(reflowStage) + ")";
  
  tft.drawCentreString(status, WIDTH/2, 85, 4);
}

void displayAlarms() {
  displayingAnyNotification = false;
  if (showOpenDoorAlarm) {
    bool doorClosed = digitalRead(DOOR_SWITCH);
    if (!doorClosed)
      displayOpenDoorAlarm();
  }
}

void displayOpenDoorAlarm() {
  displayingAnyNotification = true;

  tft.fillRect(10, 50, WIDTH-20, HEIGHT-100, COLOR_ALARM_DOOR_BG);
  tft.setTextColor(COLOR_ALARM_DOOR_FG, COLOR_ALARM_DOOR_BG);
  tft.drawCentreString("OPEN DOOR!", WIDTH/2, (HEIGHT/2)-20, 4);

  displayedNotification = true;
}

void reflowController() {
  if (reflowStage == REFLOW_STAGE_OFF) {
    topHeater = 0;              // Turn off the relays
    botHeater = 0;
  } else {
    double temp = temperature - 273.15;
    
    switch(reflowStage) {
      case REFLOW_STAGE_PREHEAT:
        // Save time when reached 50ºC
        if (temp <= 50)
          reflowEl50 = (millis() - reflowStart) / 1000;
          
        // Preheat stage goes up to 140ºC with 3 sub-stages
        if (temp < 40) { // Heat slowly
          reflowSubstage = 1;
          topHeater = 0;
          botHeater = 5;
        } else if (temp < 100) { // Heat a bit quicker
          reflowSubstage = 2;
          topHeater = 1;
          botHeater = 5;
        } else if (temp < 140) { // Heat quicker
          reflowSubstage = 3;
          topHeater = 3;
          botHeater = 6;
        } else // Temperature higher than 140ºC, switch stage
          reflowStage++;
        break;
      case REFLOW_STAGE_SOAK:
        // Save time when reached 150ºC
        if (temp <= 150)
          reflowEl150 = (millis() - reflowStart) / 1000;
        // Save time when reached 180ºC
        if (temp <= 180)
          reflowEl180up = (millis() - reflowStart) / 1000;
          
        // Preheat stage goes from 140ºC to 180ºC with 3 sub-stages
        if (temp < 150) { // Reduce heating rate between 140 to 150ºC
          // Soak 1
          reflowSubstage = 1;
          topHeater = 1;
          botHeater = 4;
        } else if (temp < 170) { // Heat from 150ºC to 170ºC slowly
          // Soak 2
          reflowSubstage = 2;
          topHeater = 2;
          botHeater = 5;
        } else if (temp < 180) { // Heat from 170ºC to 180ºC a bit faster
          // Soak 3
          reflowSubstage = 3;
          topHeater = 2;
          botHeater = 6;
        } else // Temperature higher than 180ºC, switch stage
          reflowStage++;
        break;
      case REFLOW_STAGE_REFLOW:
        // Reflow stage goes from 180ºC to 212ºC with 3 sub-stages at full speed
        if (temp < 190) {
          // Reflow 1
          reflowSubstage = 1;
          topHeater = 3;
          botHeater = 6;
        } else if (temp < 190) {
          // Reflow 2
          reflowSubstage = 2;
          topHeater = 3;
          botHeater = 6;
        } else if (temp < 212) {
          // Reflow 3
          reflowSubstage = 3;
          topHeater = 3;
          botHeater = 6;
        } else { // Temperature higher than 212ºC, switch stage
          // Save time when the door shall be open
          reflowDoorOpen = (millis() - reflowStart) / 1000;
          // Notify to open door
          showOpenDoorAlarm = true;
          // Switch to the next stage
          reflowStage++;
        }
        break;
      case REFLOW_STAGE_COOL:
        // Cooldown stage goes from max temp to 50ºC
        // Turn off heaters
        topHeater = 0;
        botHeater = 0;
        if (temp > 50) {
          reflowSubstage = 1;
          // Save time when reached 180ºC
          if (temp >= 180)
            reflowEl180dw = (millis() - reflowStart) / 1000;
          // Store maximum temperature and the time at that moment
          if (temp > reflowMaxTemp) {
            reflowMaxTemp = temp;
            reflowElPeak = (millis() - reflowStart) / 1000;
          }
          // 8 seconds after opening the door, notify to open 1/2 inch
          unsigned long openDoorTime = ((millis() - reflowStart) / 1000) - reflowDoorOpen;
          if (openDoorTime >= 8) {
            showOpenDoorAlarm = false;
          } else {
            showOpenDoorAlarm = true;
          }
        } else {
          // The reflowing has finished
          reflowStage = REFLOW_STAGE_OFF;
          reflowSubstage = 0;
          showOpenDoorAlarm = false;
        }
        break;
    }
  }
}

void pwmController() {
  unsigned long elapsed = millis() - loopTimer;
  
  // Time slice 1 (0ms-166ms)
  if (elapsed < 166) {
    digitalWrite(RELAY_TOP_PIN, topHeater >= 1);
    digitalWrite(RELAY_BOT_PIN, botHeater >= 1);
  }

  // Time slice 2 (166ms-333ms)
  if (elapsed >= 166 && elapsed < 333) {
    digitalWrite(RELAY_TOP_PIN, topHeater >= 2);
    digitalWrite(RELAY_BOT_PIN, botHeater >= 2);
  }

  // Time slice 3 (333ms-500ms)
  if (elapsed >= 333 && elapsed < 500) {
    digitalWrite(RELAY_TOP_PIN, topHeater >= 3);
    digitalWrite(RELAY_BOT_PIN, botHeater >= 3);
  }

  // Time slice 4 (500ms-666ms)
  if (elapsed >= 500 && elapsed < 666) {
    digitalWrite(RELAY_TOP_PIN, topHeater >= 4);
    digitalWrite(RELAY_BOT_PIN, botHeater >= 4);
  }

  // Time slice 5 (666ms-833ms)
  if (elapsed >= 666 && elapsed < 833) {
    digitalWrite(RELAY_TOP_PIN, topHeater >= 5);
    digitalWrite(RELAY_BOT_PIN, botHeater >= 5);
  }

  // Time slice 6 (833ms-1000ms)
  if (elapsed >= 833 && elapsed < 1000) {
    digitalWrite(RELAY_TOP_PIN, topHeater >= 5);
    digitalWrite(RELAY_BOT_PIN, botHeater >= 5);
  }

  // Reset timer
  if (elapsed >= 1000)
    loopTimer = millis();
}

void measureTemp() {
  double Vout, Rth, adc_value;
  
  adc_value = analogRead(THERM_PIN);
  Vout = (adc_value * THERM_VCC) / THERM_RES;
  Rth = (THERM_VCC * THERM_R / Vout) - THERM_R;

  /*  Steinhart-Hart Thermistor Equation:
   *  Temperature in Kelvin = 1 / (A + B[ln(R)] + C[ln(R)]^3)
   *  where A = 0.001129148, B = 0.000234125 and C = 8.76741*10^-8  */
  temperatureAccum += (1 / (THERM_A + (THERM_B * log(Rth)) + (THERM_C * pow((log(Rth)),3))));   // Temperature in kelvin
  temperatureCounter++;

  if (temperatureCounter >= SENSOR_TEMP_CYCLES) {
    temperature = temperatureAccum / temperatureCounter;
    logln(TAG_IO, "Temperature: " + String(temperature) + "K. Cycles: " + String(temperatureCounter));
    temperatureCounter = 0;
    temperatureAccum = 0;
  }
}

