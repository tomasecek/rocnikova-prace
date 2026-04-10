const int PIN_ENDSTOP_OPEN  = 5;
const int PIN_ENDSTOP_CLOSE = 4;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_ENDSTOP_OPEN, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_CLOSE, INPUT_PULLUP);

  Serial.println();
  Serial.println("Test koncovych spinacu");
  Serial.println("D5 = OPEN endstop");
  Serial.println("D4 = CLOSE endstop");
}

void loop() {
  bool openPressed  = (digitalRead(PIN_ENDSTOP_OPEN) == LOW);
  bool closePressed = (digitalRead(PIN_ENDSTOP_CLOSE) == LOW);

  Serial.print("OPEN koncak: ");
  Serial.print(openPressed ? "SEPNUTY" : "volny");

  Serial.print(" | CLOSE koncak: ");
  Serial.println(closePressed ? "SEPNUTY" : "volny");

  delay(300);
}
