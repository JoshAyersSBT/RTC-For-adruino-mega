// Real time clock and calendar with set buttons using DS1307 and Arduino Mega
// Mega I2C pins: SDA = 20, SCL = 21

#include <LiquidCrystal.h>
#include <Wire.h>

// ---- Pin assignments ----
const uint8_t BTN_NEXT_PIN = 3;   // Next/confirm button (to GND, uses INPUT_PULLUP)
const uint8_t BTN_INC_PIN  = 4;   // Increment button (to GND, uses INPUT_PULLUP)

// LCD module connections (RS, E, D4, D5, D6, D7)
// NOTE: Tie LCD RW pin to GND on the hardware.
LiquidCrystal lcd(30, 31, 32, 33, 34, 35);

// ---- Globals ----
char Time[]     = "TIME:  :  :  ";
char Calendar[] = "DATE:  /  /20  ";
byte i, second_, minute_, hour_, date_, month_, year_;

void setup() {
  pinMode(BTN_NEXT_PIN, INPUT_PULLUP);
  pinMode(BTN_INC_PIN,  INPUT_PULLUP);

  delay(50);               // allow LCD power/contrast to settle
  lcd.begin(16, 2);
  Wire.begin();            // On Mega this uses SDA(20), SCL(21)

  Serial.begin(9600);      // Serial Monitor
  while (!Serial) { ; }    // harmless on Mega
}

void DS1307_display() {
  // Convert BCD to decimal (work on local copies)
  byte s  = (second_ >> 4) * 10 + (second_ & 0x0F);
  byte m  = (minute_ >> 4) * 10 + (minute_ & 0x0F);
  byte h  = (hour_   >> 4) * 10 + (hour_   & 0x0F);
  byte d  = (date_   >> 4) * 10 + (date_   & 0x0F);
  byte mo = (month_  >> 4) * 10 + (month_  & 0x0F);
  byte y  = (year_   >> 4) * 10 + (year_   & 0x0F);

  // --- LCD update ---
  Time[12]     = (s % 10) + '0';
  Time[11]     = (s / 10) + '0';
  Time[9]      = (m % 10) + '0';
  Time[8]      = (m / 10) + '0';
  Time[6]      = (h % 10) + '0';
  Time[5]      = (h / 10) + '0';

  Calendar[14] = (y % 10) + '0';
  Calendar[13] = (y / 10) + '0';
  Calendar[9]  = (mo % 10) + '0';
  Calendar[8]  = (mo / 10) + '0';
  Calendar[6]  = (d % 10) + '0';
  Calendar[5]  = (d / 10) + '0';

  lcd.setCursor(0, 0); lcd.print(Time);
  lcd.setCursor(0, 1); lcd.print(Calendar);

  // --- Serial print once per second ---
  static uint8_t lastS = 255;
  if (s != lastS) {
    lastS = s;
    char buf[32]; // HH:MM:SS  DD/MM/20YY
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u  %02u/%02u/20%02u", h, m, s, d, mo, y);
    Serial.println(buf);
  }
}

void blink_parameter() {
  byte j = 0;
  while (j < 10 && digitalRead(BTN_NEXT_PIN) && digitalRead(BTN_INC_PIN)) {
    j++;
    delay(25);
  }
}

byte edit(byte x, byte y, byte parameter) {
  char text[3];
  while (!digitalRead(BTN_NEXT_PIN)); // wait until NEXT released

  while (true) {
    while (!digitalRead(BTN_INC_PIN)) { // increment while held
      parameter++;
      if (i == 0 && parameter > 23) parameter = 0;  // hours
      if (i == 1 && parameter > 59) parameter = 0;  // minutes
      if (i == 2 && parameter > 31) parameter = 1;  // date
      if (i == 3 && parameter > 12) parameter = 1;  // month
      if (i == 4 && parameter > 99) parameter = 0;  // year

      sprintf(text, "%02u", parameter);
      lcd.setCursor(x, y); lcd.print(text);
      delay(200);
    }

    lcd.setCursor(x, y); lcd.print("  ");
    blink_parameter();
    sprintf(text, "%02u", parameter);
    lcd.setCursor(x, y); lcd.print(text);
    blink_parameter();

    if (!digitalRead(BTN_NEXT_PIN)) {
      i++;
      return parameter;
    }
  }
}

void loop() {
  if (!digitalRead(BTN_NEXT_PIN)) {
    i = 0;

    // read current values (in decimal context) and edit
    hour_   = edit(5, 0, ((hour_   >> 4) * 10 + (hour_   & 0x0F)));
    minute_ = edit(8, 0, ((minute_ >> 4) * 10 + (minute_ & 0x0F)));
    date_   = edit(5, 1, ((date_   >> 4) * 10 + (date_   & 0x0F)));
    month_  = edit(8, 1, ((month_  >> 4) * 10 + (month_  & 0x0F)));
    year_   = edit(13, 1, ((year_   >> 4) * 10 + (year_   & 0x0F)));

    // Convert decimal to BCD
    byte m_bcd  = ((minute_ / 10) << 4) | (minute_ % 10);
    byte h_bcd  = ((hour_   / 10) << 4) | (hour_   % 10);
    byte d_bcd  = ((date_   / 10) << 4) | (date_   % 10);
    byte mo_bcd = ((month_  / 10) << 4) | (month_  % 10);
    byte y_bcd  = ((year_   / 10) << 4) | (year_   % 10);

    // Write data to DS1307 (reset seconds and start oscillator)
    Wire.beginTransmission(0x68);
    Wire.write(0x00);         // start at register 0
    Wire.write(0x00);         // seconds = 0, CH=0
    Wire.write(m_bcd);        // minutes
    Wire.write(h_bcd);        // hours (24h mode assumed)
    Wire.write(0x01);         // day of week (not used)
    Wire.write(d_bcd);        // date
    Wire.write(mo_bcd);       // month
    Wire.write(y_bcd);        // year
    Wire.endTransmission();
    delay(200);
  }

  // Read time from DS1307
  Wire.beginTransmission(0x68);
  Wire.write(0x00);            // seconds register
  Wire.endTransmission(false); // repeated start
  Wire.requestFrom(0x68, 7);

  second_ = Wire.read();   // seconds (bit7 is CH)
  minute_ = Wire.read();   // minutes
  hour_   = Wire.read();   // hours
  Wire.read();             // day (ignored)
  date_   = Wire.read();   // date
  month_  = Wire.read();   // month
  year_   = Wire.read();   // year

  DS1307_display();
  delay(50);
}
