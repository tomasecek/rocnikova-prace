#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

const int PIN_SDA = 23;
const int PIN_SCL = 22;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Test RTC DS3231");

  Wire.begin(PIN_SDA, PIN_SCL);

  if (!rtc.begin()) {
    Serial.println("RTC modul nebyl nalezen!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC ztratilo napajeni, nastavuji cas podle casu prekladu...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("RTC je pripraveno.");
}

void loop() {
  DateTime now = rtc.now();

  Serial.print("Datum: ");
  Serial.print(now.day());
  Serial.print(".");
  Serial.print(now.month());
  Serial.print(".");
  Serial.print(now.year());

  Serial.print("  Cas: ");
  if (now.hour() < 10) Serial.print("0");
  Serial.print(now.hour());
  Serial.print(":");
  if (now.minute() < 10) Serial.print("0");
  Serial.print(now.minute());
  Serial.print(":");
  if (now.second() < 10) Serial.print("0");
  Serial.println(now.second());

  delay(1000);
}
