#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>

// === Pin Definitions ===
#define BTN_NEXT_PIN 3
#define BTN_INC_PIN 4
#define BUZZER_PIN 2

// LCD pins (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(5, 6, 7, 8, 9, 10);
RTC_DS3231 rtc;

enum Mode {
  MODE_TIME_DISPLAY,
  MODE_SET_TIME,
  MODE_SET_ALARM
};

Mode currentMode = MODE_TIME_DISPLAY;
uint8_t editField = 0;

DateTime currentTime;
uint8_t alarmHour = 7;
uint8_t alarmMinute = 0;
bool alarmTriggered = false;

unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 200;

void setup() {
  pinMode(BTN_NEXT_PIN, INPUT_PULLUP);
  pinMode(BTN_INC_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.begin(16, 2);
  Wire.begin();  // Automatically uses SDA/SCL for Mega (pins 20, 21)

  rtc.begin();
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.clear();
  lcd.print("Alarm Clock Ready");
  delay(2000);
}

void loop() {
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();

  if (now - lastUpdate >= 500) {
    currentTime = rtc.now();
    updateDisplay();
    lastUpdate = now;
  }

  handleButtons();
  checkAlarm();
}

void updateDisplay() {
  lcd.setCursor(0, 0);
  char buf[17];
  sprintf(buf, "Time: %02d:%02d:%02d", currentTime.hour(), currentTime.minute(), currentTime.second());
  lcd.print(buf);

  lcd.setCursor(0, 1);
  if (currentMode == MODE_SET_ALARM) {
    sprintf(buf, "Set Alarm: %02d:%02d ", alarmHour, alarmMinute);
  } else if (currentMode == MODE_SET_TIME) {
    sprintf(buf, "Set Time:  %02d:%02d ", currentTime.hour(), currentTime.minute());
  } else {
    sprintf(buf, "Alarm:     %02d:%02d ", alarmHour, alarmMinute);
  }
  lcd.print(buf);
}

void handleButtons() {
  if ((millis() - lastDebounce) < debounceDelay) return;

  if (digitalRead(BTN_NEXT_PIN) == LOW) {
    lastDebounce = millis();

    if (alarmTriggered) {
      digitalWrite(BUZZER_PIN, LOW);
      alarmTriggered = false;
      return;
    }

    if (currentMode == MODE_TIME_DISPLAY) {
      currentMode = MODE_SET_TIME;
      editField = 0;
    } else if (currentMode == MODE_SET_TIME) {
      if (++editField > 1) {
        currentMode = MODE_SET_ALARM;
        editField = 0;
      }
    } else if (currentMode == MODE_SET_ALARM) {
      if (++editField > 1) {
        currentMode = MODE_TIME_DISPLAY;
      }
    }
  }

  if (digitalRead(BTN_INC_PIN) == LOW) {
    lastDebounce = millis();

    if (currentMode == MODE_SET_TIME) {
      if (editField == 0) {
        rtc.adjust(DateTime(currentTime.year(), currentTime.month(), currentTime.day(), (currentTime.hour() + 1) % 24, currentTime.minute(), 0));
      } else if (editField == 1) {
        rtc.adjust(DateTime(currentTime.year(), currentTime.month(), currentTime.day(), currentTime.hour(), (currentTime.minute() + 1) % 60, 0));
      }
    }

    if (currentMode == MODE_SET_ALARM) {
      if (editField == 0) {
        alarmHour = (alarmHour + 1) % 24;
      } else if (editField == 1) {
        alarmMinute = (alarmMinute + 1) % 60;
      }
    }
  }
}

void checkAlarm() {
  if (currentTime.hour() == alarmHour &&
      currentTime.minute() == alarmMinute &&
      currentTime.second() == 0 &&
      !alarmTriggered) {
    digitalWrite(BUZZER_PIN, HIGH);
    alarmTriggered = true;
  }
}
