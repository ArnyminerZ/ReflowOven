# 1 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino"

/**

 * Made by Arnau Mora

 * 

 * Screen controller by http://www.multicopterox.es/display-ili9341-spi-y-esp8266-nodemcu/

 * Reflowing algorithm by https://github.com/thedalles77/Reflow_Oven

 */
# 9 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino"
# 10 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino" 2
# 11 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino" 2

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// Comment to disable logging to Serial port


# 18 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino" 2
# 19 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino" 2

// Timing storage
unsigned long loopTimer;
unsigned long dispTempMillis; // The millis at last screen temperature update
unsigned long dispStatMillis; // The millis at last screen status update
unsigned long beepMillis; // The millis at last beep cycle
unsigned long buttonMillis; // The millis the button has been pressed

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
unsigned long reflowStart; // The millis at the starting of reflowing
unsigned long reflowEl50; // Elapsed seconds at 50ºC
unsigned long reflowEl150; // Elapsed seconds at 150ºC
unsigned long reflowEl180up; // Elapsed seconds at 180ºC when heating
unsigned long reflowEl180dw; // Elapsed seconds at 180ºC when cooling
unsigned long reflowElPeak; // Elapsed seconds at peak temperature
unsigned long reflowDoorOpen; // Elapsed seconds when the door gets open

// Function definitions
void measureTemp();
void resetDisplay();
void displayTemp();
void displayStatus();
void displayAlarms();
void displayOpenDoorAlarm();
void pwmController();
void reflowController();

void setup() {
  tft.init();
  tft.setRotation(0);

  logInit();

  logln("Screen", "Showing boot screen...");
  tft.fillScreen(0x0000 /*   0,   0,   0 */);
  tft.setTextColor(0xFFFF /* 255, 255, 255 */, 0x0000 /*   0,   0,   0 */);
  tft.drawCentreString("Reflow Oven", 120, 70, 4); // Draw x=120, y=70, size=4
  tft.drawCentreString("by Arnau Mora", 120, 90, 2);
  tft.drawString("Booting...", 10, 320 - 20, 2);

  logln("Core", "Initializing timing variables...");
  dispTempMillis = millis();
  dispStatMillis = millis();
  loopTimer = millis();
  beepMillis = millis();
  buttonMillis = millis();

  logln("I/O", "Configuring ports...");
  pinMode(4 /* GPIO4        User purpose*/, 0x01); // Define the top relay pin as output
  pinMode(4 /* GPIO4        User purpose*/, 0x01); // Define the bottom relay pin as output
  topHeater = 0; // Turn off the relays
  botHeater = 0;

  pinMode(2, 0x01); // Define the beeper output

  pinMode(2 /* GPIO2        TXD1 (must be high on boot to go to UART0 FLASH mode)*/, 0x00);
  pinMode(16, 0x04 /* PULLDOWN only possible for pin16*/);

  // Initialize reflow variables
  logln("Core", "Initializing reflow variables...");
  reflowStage = -2;
  reflowSubstage = 0;
  reflowMaxTemp = 0;

  // Initialize sensor variables
  logln("Core", "Initializing sensor variables...");
  temperature = 0.0;
  temperatureAccum = 0.0;
  temperatureCounter = 0;

  // Show boot screen for at least 3 seconds
  logln("Screen", "Waiting for 3 seconds...");
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
  if (showOpenDoorAlarm && !digitalRead(2 /* GPIO2        TXD1 (must be high on boot to go to UART0 FLASH mode)*/)) {
    unsigned long elapsedBeep = millis() - beepMillis;

    logln("Core", "Beep = " + String(elapsedBeep < 1000 /* The cycle time of the alarm for beeping (half time beep on, half time beep off)*/));
    digitalWrite(2, elapsedBeep < 1000 /* The cycle time of the alarm for beeping (half time beep on, half time beep off)*/);

    if (elapsedBeep >= 1000 /* The cycle time of the alarm for beeping (half time beep on, half time beep off)*/ * 2) {
      beepMillis = millis();
    }
  } else
    digitalWrite(2, 0x0);


  if (digitalRead(16)) {
    logln("Core", "Hold.");
    if(reflowStage < 0)
      reflowStage = -1;
  } else {
    buttonMillis = millis();
    if(reflowStage < 0)
      reflowStage = -2;
  }
  if (millis() - buttonMillis > 3000 /* The amount of time the button must be pressed for the process to start*/ && reflowStage == -1) {
    logln("Core", "Starting reflow!");
    reflowStage = 0;
  }


  bool updatedDisplay = false;
  if(displayReset || millis() - dispTempMillis >= 1000 /* The amount of time between screen temperature updates*/) {
    displayTemp();
    dispTempMillis = millis();
    updatedDisplay = true;
  }
  if(displayReset || millis() - dispStatMillis >= 1000 /* The amount of time between screen status updates*/) {
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
  logln("Screen", "Reset!");
  tft.fillScreen(0x31A6);

  tft.setTextColor(0xFFFF /* 255, 255, 255 */, 0x31A6);
  tft.drawString("Reflow oven by Arnau Mora", 5, 320 - 20, 2);

  displayReset = true;
}

void displayTemp() {
  double temp = temperature - 273.15;

  tft.fillRect(10, 10, 240 -20, 50, 0xC638 /* Background color of the temperature display*/);
  tft.drawRect(10, 10, 240 -20, 50, 0xC945 /* Border color of the temperature display*/);
  if (reflowStage == -2)
    tft.setTextColor(0xC945 /* Text color of the temperature display when turned off*/, 0xC638 /* Background color of the temperature display*/);
  else if (topHeater <= 0 && topHeater <= 0)
    tft.setTextColor(0x1E03 /* Text color of the temperature display when reflowing*/, 0xC638 /* Background color of the temperature display*/);
  else
    tft.setTextColor(0x2BF9 /* Text color of the temperature display when idle*/, 0xC638 /* Background color of the temperature display*/);
  tft.drawCentreString(String(temp) + " C", 240/2, 25, 4);

  if (reflowStage == -2) {
    tft.fillCircle(35, 35, 10, 0xC945 /* Text color of the temperature display when turned off*/);
    tft.fillCircle(240 - 35, 35, 10, 0xC945 /* Text color of the temperature display when turned off*/);
  } else if (topHeater <= 0 && topHeater <= 0) {
    tft.fillCircle(35, 35, 10, 0x1E03 /* Text color of the temperature display when reflowing*/);
    tft.fillCircle(240 - 35, 35, 10, 0x1E03 /* Text color of the temperature display when reflowing*/);
  } else {
    tft.fillCircle(35, 35, 10, 0x2BF9 /* Text color of the temperature display when idle*/);
    tft.fillCircle(240 - 35, 35, 10, 0x2BF9 /* Text color of the temperature display when idle*/);
  }
}

void displayStatus() {
  tft.fillRect(10, 70, 240 -20, 50, 0xC638 /* Background color of the status display*/);
  tft.drawRect(10, 70, 240 -20, 50, 0xC945 /* Border color of the status display*/);
  tft.setTextColor(0xC945 /* Text color of the status display*/, 0xC638 /* Background color of the status display*/);

  String status;
  if (reflowStage == -2)
    status = "Off";
  else if (reflowStage == -1)
    status = "Hold";
  else if (reflowStage == 0)
    status = "Preheat " + String(reflowSubstage);
  else if (reflowStage == 1)
    status = "Soak " + String(reflowSubstage);
  else if (reflowStage == 2)
    status = "Reflow" + String(reflowSubstage);
  else if (reflowStage == 3)
    status = "Cooldown " + String(reflowSubstage);
  else
    status = "Unknown (" + String(reflowStage) + ")";

  tft.drawCentreString(status, 240/2, 85, 4);
}

void displayAlarms() {
  displayingAnyNotification = false;
  if (showOpenDoorAlarm) {
    bool doorClosed = digitalRead(2 /* GPIO2        TXD1 (must be high on boot to go to UART0 FLASH mode)*/);
    if (!doorClosed)
      displayOpenDoorAlarm();
  }
}

void displayOpenDoorAlarm() {
  displayingAnyNotification = true;

  tft.fillRect(10, 50, 240 -20, 320 -100, 0xF481 /* Background color of the open door alarm*/);
  tft.setTextColor(0x31A6 /* Text color color of the open door alarm*/, 0xF481 /* Background color of the open door alarm*/);
  tft.drawCentreString("OPEN DOOR!", 240/2, (320/2)-20, 4);

  displayedNotification = true;
}

void reflowController() {
  if (reflowStage == -2) {
    topHeater = 0; // Turn off the relays
    botHeater = 0;
  } else {
    double temp = temperature - 273.15;

    switch(reflowStage) {
      case 0:
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
      case 1:
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
      case 2:
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
      case 3:
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
          reflowStage = -2;
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
    digitalWrite(4 /* GPIO4        User purpose*/, topHeater >= 1);
    digitalWrite(4 /* GPIO4        User purpose*/, botHeater >= 1);
  }

  // Time slice 2 (166ms-333ms)
  if (elapsed >= 166 && elapsed < 333) {
    digitalWrite(4 /* GPIO4        User purpose*/, topHeater >= 2);
    digitalWrite(4 /* GPIO4        User purpose*/, botHeater >= 2);
  }

  // Time slice 3 (333ms-500ms)
  if (elapsed >= 333 && elapsed < 500) {
    digitalWrite(4 /* GPIO4        User purpose*/, topHeater >= 3);
    digitalWrite(4 /* GPIO4        User purpose*/, botHeater >= 3);
  }

  // Time slice 4 (500ms-666ms)
  if (elapsed >= 500 && elapsed < 666) {
    digitalWrite(4 /* GPIO4        User purpose*/, topHeater >= 4);
    digitalWrite(4 /* GPIO4        User purpose*/, botHeater >= 4);
  }

  // Time slice 5 (666ms-833ms)
  if (elapsed >= 666 && elapsed < 833) {
    digitalWrite(4 /* GPIO4        User purpose*/, topHeater >= 5);
    digitalWrite(4 /* GPIO4        User purpose*/, botHeater >= 5);
  }

  // Time slice 6 (833ms-1000ms)
  if (elapsed >= 833 && elapsed < 1000) {
    digitalWrite(4 /* GPIO4        User purpose*/, topHeater >= 5);
    digitalWrite(4 /* GPIO4        User purpose*/, botHeater >= 5);
  }

  // Reset timer
  if (elapsed >= 1000)
    loopTimer = millis();
}

void measureTemp() {
  double Vout, Rth, adc_value;

  adc_value = analogRead(A0);
  Vout = (adc_value * 3.3 /* Connected to board's 3.3V pin*/) / 1023 /* The resolution of the ADC*/;
  Rth = (3.3 /* Connected to board's 3.3V pin*/ * 10000 /* Resistor of 10k*/ / Vout) - 10000 /* Resistor of 10k*/;

  /*  Steinhart-Hart Thermistor Equation:

   *  Temperature in Kelvin = 1 / (A + B[ln(R)] + C[ln(R)]^3)

   *  where A = 0.001129148, B = 0.000234125 and C = 8.76741*10^-8  */
# 426 "c:\\Users\\Arnym\\Documents\\Arduino\\reflow_oven\\reflow_oven.ino"
  temperatureAccum += (1 / (-0.001409530395 /* 0.001129148*/ + (0.0005396718651 /* 0.000234125*/ * log(Rth)) + (-0.0000009499650276 /* 0.0000000876741*/ * pow((log(Rth)),3)))); // Temperature in kelvin
  temperatureCounter++;

  if (temperatureCounter >= 100 /* The amount of measures to make for the temperature sensor*/) {
    temperature = temperatureAccum / temperatureCounter;
    logln("I/O", "Temperature: " + String(temperature) + "K. Cycles: " + String(temperatureCounter));
    temperatureCounter = 0;
    temperatureAccum = 0;
  }
}
