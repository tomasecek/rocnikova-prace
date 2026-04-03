#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

// =========================
// PINY
// =========================
const int PIN_STEP          = 18;
const int PIN_DIR           = 19;
const int PIN_EN            = 21;

const int PIN_RTC_SDA       = 23;
const int PIN_RTC_SCL       = 22;

const int PIN_ENDSTOP_OPEN  = 15;  // koncak otevreno
const int PIN_ENDSTOP_CLOSE = 4;   // koncak zavreno

// =========================
// NASTAVENI
// =========================
const bool ENABLE_ACTIVE_LOW = true;   // u vetsiny driveru LOW = aktivni
const int STEP_DELAY_US = 1200;        // rychlost motoru
const int TEST_STEPS = 400;            // max kroků na jeden test
const int RTC_TEST_INTERVAL_SEC = 10;  // každých 10 sekund zkus pohyb

// Směry - když budou obráceně, jen prohoď true/false
const bool DIR_OPEN  = true;
const bool DIR_CLOSE = false;

// =========================
// PROMENNE
// =========================
int lastTriggeredSecond = -1;
bool alternateDirection = true;

// =========================
// POMOCNE FUNKCE
// =========================
void enableDriver() {
  digitalWrite(PIN_EN, ENABLE_ACTIVE_LOW ? LOW : HIGH);
}

void disableDriver() {
  digitalWrite(PIN_EN, ENABLE_ACTIVE_LOW ? HIGH : LOW);
}

bool isOpenEndstopPressed() {
  return digitalRead(PIN_ENDSTOP_OPEN) == LOW;
}

bool isCloseEndstopPressed() {
  return digitalRead(PIN_ENDSTOP_CLOSE) == LOW;
}

void printDateTime(const DateTime& now) {
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
}

void printEndstopStates() {
  Serial.print("OPEN koncak: ");
  Serial.print(isOpenEndstopPressed() ? "SEPNUTY" : "volny");

  Serial.print(" | CLOSE koncak: ");
  Serial.println(isCloseEndstopPressed() ? "SEPNUTY" : "volny");
}

// Udela jeden krok
void oneStep() {
  digitalWrite(PIN_STEP, HIGH);
  delayMicroseconds(STEP_DELAY_US);
  digitalWrite(PIN_STEP, LOW);
  delayMicroseconds(STEP_DELAY_US);
}

// Pohyb s hlidanim koncaku
void moveMotorSteps(int steps, bool direction) {
  // Bezpecnostni kontrola pred startem
  if (direction == DIR_OPEN && isOpenEndstopPressed()) {
    Serial.println("Pohyb OPEN zrusen - OPEN koncak uz je sepnuty.");
    return;
  }

  if (direction == DIR_CLOSE && isCloseEndstopPressed()) {
    Serial.println("Pohyb CLOSE zrusen - CLOSE koncak uz je sepnuty.");
    return;
  }

  digitalWrite(PIN_DIR, direction ? HIGH : LOW);
  enableDriver();
  delay(20);

  for (int i = 0; i < steps; i++) {
    // Kontrola behem pohybu
    if (direction == DIR_OPEN && isOpenEndstopPressed()) {
      Serial.print("Zastavuji - OPEN koncak sepnut po ");
      Serial.print(i);
      Serial.println(" krocich.");
      break;
    }

    if (direction == DIR_CLOSE && isCloseEndstopPressed()) {
      Serial.print("Zastavuji - CLOSE koncak sepnut po ");
      Serial.print(i);
      Serial.println(" krocich.");
      break;
    }

    oneStep();
  }

  delay(20);
  disableDriver();
}

void printHelp() {
  Serial.println();
  Serial.println("===== OVLADANI PRES SERIAL =====");
  Serial.println("o = kratky pohyb OPEN");
  Serial.println("z = kratky pohyb CLOSE");
  Serial.println("s = stav RTC + koncaku");
  Serial.println("t = automaticky test podle RTC zustava aktivni");
  Serial.println("================================");
  Serial.println();
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_EN, OUTPUT);

  pinMode(PIN_ENDSTOP_OPEN, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_CLOSE, INPUT_PULLUP);

  digitalWrite(PIN_STEP, LOW);
  digitalWrite(PIN_DIR, LOW);
  disableDriver();

  Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL);

  Serial.println();
  Serial.println("Start testu: RTC + motor + koncove spinace");

  if (!rtc.begin()) {
    Serial.println("RTC modul nebyl nalezen!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC ztratilo napajeni, nastavuji cas podle casu prekladu...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("RTC je pripraveno.");
  printHelp();
}

// =========================
// LOOP
// =========================
void loop() {
  DateTime now = rtc.now();

  // kazdych 10 sekund vypis stavu + kratky test
  if ((now.second() % RTC_TEST_INTERVAL_SEC == 0) && (now.second() != lastTriggeredSecond)) {
    lastTriggeredSecond = now.second();

    printDateTime(now);
    printEndstopStates();

    if (alternateDirection) {
      Serial.println("RTC test: pohyb OPEN");
      moveMotorSteps(TEST_STEPS, DIR_OPEN);
    } else {
      Serial.println("RTC test: pohyb CLOSE");
      moveMotorSteps(TEST_STEPS, DIR_CLOSE);
    }

    alternateDirection = !alternateDirection;
    Serial.println("RTC test hotov.");
    Serial.println();
  }

  // Manualni test pres Serial
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == 'o' || cmd == 'O') {
      Serial.println("Manualni test: OPEN");
      moveMotorSteps(TEST_STEPS, DIR_OPEN);
    }
    else if (cmd == 'z' || cmd == 'Z') {
      Serial.println("Manualni test: CLOSE");
      moveMotorSteps(TEST_STEPS, DIR_CLOSE);
    }
    else if (cmd == 's' || cmd == 'S') {
      printDateTime(now);
      printEndstopStates();
    }
    else if (cmd == 't' || cmd == 'T') {
      Serial.println("Automaticky RTC test je aktivni.");
    }
  }

  delay(50);
}
