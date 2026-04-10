#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "KURNIK_ESP32";
const char* password = "12345678";

WebServer server(80);

String doorState = "Neznamy stav";
String openTime = "07:00";
String closeTime = "20:00";

String htmlPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="cs">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Ovládání dvířek</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #f4f4f4;
      text-align: center;
      padding: 20px;
    }
    .box {
      max-width: 420px;
      margin: auto;
      background: white;
      padding: 20px;
      border-radius: 12px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
    }
    h1 {
      margin-bottom: 10px;
    }
    .state {
      font-size: 18px;
      margin: 15px 0;
      font-weight: bold;
    }
    button {
      width: 140px;
      padding: 12px;
      margin: 8px;
      font-size: 16px;
      border: none;
      border-radius: 8px;
      cursor: pointer;
    }
    .openBtn {
      background-color: #4CAF50;
      color: white;
    }
    .closeBtn {
      background-color: #f44336;
      color: white;
    }
    .saveBtn {
      background-color: #2196F3;
      color: white;
      width: 200px;
    }
    input[type="time"] {
      padding: 10px;
      font-size: 16px;
      margin: 8px;
      width: 140px;
    }
    .section {
      margin-top: 20px;
    }
  </style>
</head>
<body>
  <div class="box">
    <h1>Ovládání dvířek</h1>
    <p class="state">Aktuální stav: %STATE%</p>

    <div class="section">
      <form action="/open" method="get">
        <button class="openBtn" type="submit">Otevřít</button>
      </form>
      <form action="/close" method="get">
        <button class="closeBtn" type="submit">Zavřít</button>
      </form>
    </div>

    <div class="section">
      <h3>Nastavení rozvrhu</h3>
      <form action="/save" method="get">
        <p>Čas otevření:</p>
        <input type="time" name="openTime" value="%OPENTIME%" required>

        <p>Čas zavření:</p>
        <input type="time" name="closeTime" value="%CLOSETIME%" required>

        <br><br>
        <button class="saveBtn" type="submit">Uložit rozvrh</button>
      </form>
    </div>

    <div class="section">
      <p><strong>Uložené otevření:</strong> %OPENTIME%</p>
      <p><strong>Uložené zavření:</strong> %CLOSETIME%</p>
    </div>
  </div>
</body>
</html>
)rawliteral";

  html.replace("%STATE%", doorState);
  html.replace("%OPENTIME%", openTime);
  html.replace("%CLOSETIME%", closeTime);

  return html;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", htmlPage());
}

void handleOpen() {
  doorState = "Dvirka otevrena";
  Serial.println("Prikaz: OTEVRIT");
  server.send(200, "text/html; charset=utf-8", htmlPage());
}

void handleClose() {
  doorState = "Dvirka zavrena";
  Serial.println("Prikaz: ZAVRIT");
  server.send(200, "text/html; charset=utf-8", htmlPage());
}

void handleSave() {
  if (server.hasArg("openTime")) {
    openTime = server.arg("openTime");
  }

  if (server.hasArg("closeTime")) {
    closeTime = server.arg("closeTime");
  }

  Serial.println("Novy rozvrh:");
  Serial.print("Otevrit v: ");
  Serial.println(openTime);
  Serial.print("Zavrit v: ");
  Serial.println(closeTime);

  server.send(200, "text/html; charset=utf-8", htmlPage());
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("WiFi AP spustena");
  Serial.print("Nazev site: ");
  Serial.println(ssid);
  Serial.print("Heslo: ");
  Serial.println(password);
  Serial.print("IP adresa: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/open", handleOpen);
  server.on("/close", handleClose);
  server.on("/save", handleSave);

  server.begin();
  Serial.println("Web server spusten");
}

void loop() {
  server.handleClient();
}
