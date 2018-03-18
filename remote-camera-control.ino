#include <LiquidCrystal.h>

#define MODE_PROGRAMMING 0
#define MODE_RUNNING 1
#define PROGRAMMING_INTERVAL 0
#define PROGRAMMING_COUNT 1
#define PROGRAMMING_START_WAIT 2

#define RUN_LED 5
#define SHUTTER_LED 4

#define FOCUS_RELAY 2
#define SHUTTER_RELAY 3

#define PROGRAMMING_LINE_1_A "INTERVAL SEC    "
#define PROGRAMMING_LINE_1_B "PHOTO COUNT     "
#define PROGRAMMING_LINE_1_C "START DELAY SEC "
#define PROGRAMMING_LINE_2   ">               "

#define JOYSTICK_X 0
#define JOYSTICK_Y 1
#define BTN_1 (3 + 14)
#define BTN_2 (4 + 14)

int previousJoystickButtonState = LOW;
int previousButton1State = LOW;
int previousButton2State = LOW;

const int rs = 12, en = 11, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

long intervalSec = 30;
long picturesRemaining = 5;
long startDelaySec = 10;

long nextMillis = 0;

int operationMode = MODE_PROGRAMMING;
int programmingMode = PROGRAMMING_INTERVAL;

void setup() {
  // put your setup code here, to run once:
  lcd.begin(16, 2);

  pinMode(JOYSTICK_X, INPUT);
  pinMode(JOYSTICK_Y, INPUT);
  pinMode(BTN_1, INPUT);
  pinMode(BTN_2, INPUT);
  
  pinMode(RUN_LED, OUTPUT);
  pinMode(SHUTTER_LED, OUTPUT);
  pinMode(FOCUS_RELAY, OUTPUT);
  pinMode(SHUTTER_RELAY, OUTPUT);
}

void loop() {
  if (operationMode == MODE_PROGRAMMING) {
    runLedBlinking();
    programmingLoop();
    if (button2Pressed()) {
      nextMillis = millis() + (startDelaySec * 1000);
      operationMode = MODE_RUNNING;
    }
  } else {
    runLedOn();
    runningModeLoop();
    if (button2Pressed() || picturesRemaining <= 0) {
      nextMillis = 0;
      operationMode = MODE_PROGRAMMING;
      if (picturesRemaining <= 0) {
        picturesRemaining = 0;
      }
    } 
  }
}

void runningModeLoop() {
  long timeRemaining = nextMillis - millis();
  lcd.setCursor(0, 0);
  lcd.print("NEXT:        SEC");
  lcd.setCursor(6, 0);
  lcd.print(timeRemaining / 1000);
  lcd.setCursor(0, 1);
  lcd.print("LEFT:           ");
  lcd.setCursor(6, 1);
  lcd.print(picturesRemaining);
  if (timeRemaining <= 4000) {
    takePicture(); // Takes 4000 mS
  } else if (timeRemaining < 7000) {
    blinkShutterLedQuickly();
    delay(50);
  } else if (timeRemaining < 10000) {
    blinkShutterLedSlowly();
    delay(50);
  } else {
    delay(200);
  }
}

void programmingLoop() {
  if (programmingMode == PROGRAMMING_INTERVAL) {
    lcd.setCursor(0, 0);
    lcd.print(PROGRAMMING_LINE_1_A);
    lcd.setCursor(0, 1);
    lcd.print(PROGRAMMING_LINE_2);
    lcd.setCursor(2, 1);
    lcd.print(intervalSec);
    intervalSec += translateJoystickInput(analogRead(JOYSTICK_X));
    if (intervalSec < 5) {
      intervalSec = 5;
    }
    if (button1Pressed() == HIGH) {
      programmingMode = PROGRAMMING_COUNT;
    }
  } else if (programmingMode == PROGRAMMING_COUNT){
    lcd.setCursor(0, 0);
    lcd.print(PROGRAMMING_LINE_1_B);
    lcd.setCursor(0, 1);
    lcd.print(PROGRAMMING_LINE_2);
    lcd.setCursor(2, 1);
    lcd.print(picturesRemaining);
    picturesRemaining += translateJoystickInput(analogRead(JOYSTICK_X));
    if (picturesRemaining < 0) {
      picturesRemaining = 5;
    }
    if (button1Pressed() == HIGH) {
      programmingMode = PROGRAMMING_START_WAIT;
    }
  } else if (programmingMode == PROGRAMMING_START_WAIT){
    lcd.setCursor(0, 0);
    lcd.print(PROGRAMMING_LINE_1_C);
    lcd.setCursor(0, 1);
    lcd.print(PROGRAMMING_LINE_2);
    lcd.setCursor(2, 1);
    lcd.print(startDelaySec);
    startDelaySec += translateJoystickInput(analogRead(JOYSTICK_X));
    if (startDelaySec < 0) {
      startDelaySec = 0;
    }
    if (button1Pressed() == HIGH) {
      programmingMode = PROGRAMMING_INTERVAL;
    }
  }

  delay(100);
}

/*
 * 4000 MS 
 */
void takePicture() {
  lcd.setCursor(0, 0);
  lcd.print("FOCUS ON 4000 MS");
  focusRelayOn();
  shutterLedOn();
  delay(4000);
  lcd.setCursor(0, 0);
  lcd.print("SHUTT ON 2000 MS");
  shutterLedOff();
  runLedOff();
  shutterRelayOn();
  delay(2000);
  focusRelayOff();
  shutterRelayOff();
  runLedOn();

  nextMillis = millis() + (intervalSec * 1000);
  picturesRemaining--;
}

void shutterRelayOn() {
  digitalWrite(SHUTTER_RELAY, HIGH);
}

void shutterRelayOff() {
  digitalWrite(SHUTTER_RELAY, LOW);
}

void focusRelayOn() {
  digitalWrite(FOCUS_RELAY, HIGH);
}

void focusRelayOff() {
  digitalWrite(FOCUS_RELAY, LOW);
}

void shutterLedOn() {
  digitalWrite(SHUTTER_LED, HIGH);
}

void shutterLedOff() {
  digitalWrite(SHUTTER_LED, LOW);
}

void runLedOn() {
  digitalWrite(RUN_LED, HIGH);
}

void runLedOff() {
  digitalWrite(RUN_LED, LOW);
}

void blinkShutterLedSlowly() {
  blinkLed(SHUTTER_LED, 1000);
}

void blinkShutterLedQuickly() {
  blinkLed(SHUTTER_LED, 500);
}

void runLedBlinking() {
  blinkLed(RUN_LED, 2000); 
}

void blinkLed(int ledPin, long fullOnOffCycleDuration) {
  long remaining = millis() % fullOnOffCycleDuration;
  Serial.println(remaining);
  if (remaining < (fullOnOffCycleDuration / 2)) {
    digitalWrite(ledPin, LOW);
  } else {
    digitalWrite(ledPin, HIGH);
  }
}

int translateJoystickInput(int val) { 
  const int baseline = 512;
  const int idle = 24;
  int diff = (baseline - val) * -1;
  if (abs(diff) < idle) {
    return 0;
  }
  return diff / 200.0;
}

int button1Pressed() {
  int buttonVal = digitalRead(BTN_1);
  if (buttonVal == HIGH && previousButton1State == LOW) {
    previousButton1State = buttonVal;
    return HIGH;
  }
  previousButton1State = buttonVal;
  return LOW;
}

int button2Pressed() {
  int buttonVal = digitalRead(BTN_2);
  if (buttonVal == HIGH && previousButton2State == LOW) {
    previousButton2State = buttonVal;
    return HIGH;
  }
  previousButton2State = buttonVal;
  return LOW;
}

