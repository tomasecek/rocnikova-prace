#include <WiFi.h>
#include <ESP32Servo.h>

// =========================
//  WIFI AP (bez routeru)
// =========================
const char* AP_SSID = "ESP32-DVIRKA";
const char* AP_PASS = "123456789";   // min. 8 znaků

WiFiServer server(80);

// =========================
//  SERVO NASTAVENÍ
// =========================
Servo servo1;
Servo servo2;

// Nastav piny, kam vede SIGNÁL serv
const int SERVO1_PIN = 18;
const int SERVO2_PIN = 19;

// Pokud máš jen 1 servo, nech USE_SERVO2 = false
const bool USE_SERVO2 = true;

// Úhly pro otevřeno/zavřeno (upravit podle mechaniky)
int ANGLE_OPEN  = 20;
int ANGLE_CLOSE = 120;

// Stav dvířek
enum DoorState { UNKNOWN, OPEN, CLOSED, OPENING, CLOSING, STOPPED };
DoorState doorState = UNKNOWN;

bool autoMode = false;

// =========================
//  "ČAS" bez internetu/RTC
//  (telefon ho nastaví přes web)
// =========================
// Udržíme si "sekundy dne" (0..86399) a posun vůči millis()
bool timeSet = false;
uint32_t baseMillis = 0;
uint32_t baseSecondsOfDay = 0;

// =========================
//  ROZVRHY (jednoduše)
// =========================
struct Schedule {
  uint16_t openMin;   // minuty dne 0..1439
  uint16_t closeMin;  // minuty dne 0..1439
  uint32_t id;
  bool used;
};

const int MAX_SCHEDULES = 6;
Schedule schedules[MAX_SCHEDULES];

// aby se stejné akce nespouštěly pořád dokola
int lastActionMinuteOpen[MAX_SCHEDULES];
int lastActionMinuteClose[MAX_SCHEDULES];

// =========================
//  HTML (UI)
// =========================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="cs">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Dvířka – ovládání</title>
  <style>
    :root{
      --bg:#0b1020; --card:#121a33; --card2:#0f1730; --text:#e8ecff; --muted:#aab3de;
      --ok:#35d07f; --bad:#ff4d5d; --warn:#ffcc66; --line:rgba(255,255,255,.08);
      --shadow:0 12px 30px rgba(0,0,0,.35); --r:18px;
    }
    *{box-sizing:border-box}
    body{
      margin:0;
      font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;
      background:
        radial-gradient(1200px 700px at 20% -10%, rgba(106,168,255,.25), transparent 60%),
        radial-gradient(900px 600px at 100% 0%, rgba(53,208,127,.18), transparent 55%),
        radial-gradient(900px 700px at 0% 100%, rgba(255,77,93,.12), transparent 55%),
        var(--bg);
      color:var(--text);
      min-height:100vh;
    }
    .wrap{max-width:980px;margin:0 auto;padding:18px 14px 28px}
    .top{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:14px}
    .title{display:flex;flex-direction:column;gap:4px}
    .title h1{margin:0;font-size:20px;letter-spacing:.2px}
    .title p{margin:0;color:var(--muted);font-size:13px}
    .chip{
      display:inline-flex;align-items:center;gap:8px;padding:10px 12px;border:1px solid var(--line);
      background:rgba(18,26,51,.6);border-radius:999px;box-shadow:var(--shadow);font-size:13px;color:var(--muted)
    }
    .dot{width:10px;height:10px;border-radius:999px;background:var(--warn)}
    .grid{display:grid;grid-template-columns:1.2fr .8fr;gap:14px}
    @media (max-width: 860px){.grid{grid-template-columns:1fr}}
    .card{
      background:linear-gradient(180deg, rgba(18,26,51,.9), rgba(15,23,48,.9));
      border:1px solid var(--line);border-radius:var(--r);box-shadow:var(--shadow);overflow:hidden
    }
    .hd{padding:14px 14px 10px;border-bottom:1px solid var(--line);display:flex;justify-content:space-between;gap:10px}
    .hd h2{margin:0;font-size:15px}
    .hd small{color:var(--muted)}
    .bd{padding:14px}
    .row{display:flex;gap:10px;flex-wrap:wrap}
    .btn{
      flex:1;min-width:140px;border:0;border-radius:14px;padding:14px 14px;font-size:16px;font-weight:800;
      color:#0b1020;cursor:pointer;transition:transform .05s ease, filter .15s ease;
      box-shadow:0 10px 18px rgba(0,0,0,.25)
    }
    .btn:active{transform:translateY(1px)}
    .btnOpen{background:linear-gradient(135deg,#35d07f,#6ff3b0)}
    .btnClose{background:linear-gradient(135deg,#ff4d5d,#ff9aa3)}
    .btnGhost{background:transparent;border:1px solid var(--line);color:var(--text);box-shadow:none;font-weight:700}
    .pill{
      display:inline-flex;align-items:center;gap:8px;padding:8px 10px;border-radius:999px;border:1px solid var(--line);
      color:var(--muted);background:rgba(255,255,255,.04);font-size:12px;user-select:none
    }
    .statusBox{
      display:flex;flex-direction:column;gap:10px;padding:12px;border:1px solid var(--line);
      border-radius:14px;background:rgba(0,0,0,.18)
    }
    .statusTop{display:flex;align-items:center;justify-content:space-between;gap:10px}
    .big{font-size:18px;font-weight:900;display:flex;align-items:center;gap:10px}
    .toggle{
      display:flex;align-items:center;gap:10px;padding:10px 12px;border:1px solid var(--line);border-radius:14px;
      background:rgba(255,255,255,.03)
    }
    .switch{
      width:50px;height:28px;border-radius:999px;background:rgba(255,255,255,.08);border:1px solid var(--line);
      position:relative;cursor:pointer;flex:0 0 auto
    }
    .knob{position:absolute;top:3px;left:3px;width:22px;height:22px;border-radius:999px;background:#e8ecff;
      transition:left .15s ease, background .15s ease}
    .switch.on{background:rgba(53,208,127,.18)}
    .switch.on .knob{left:25px;background:#6ff3b0}
    label{font-size:13px;color:var(--muted)}
    .form{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:10px}
    @media (max-width: 480px){.form{grid-template-columns:1fr}}
    .field{display:flex;flex-direction:column;gap:6px}
    input[type="time"]{
      width:100%;padding:12px 12px;border-radius:14px;border:1px solid var(--line);
      background:rgba(0,0,0,.2);color:var(--text);font-size:15px;outline:none
    }
    .list{display:flex;flex-direction:column;gap:10px}
    .item{
      display:flex;align-items:center;justify-content:space-between;gap:10px;padding:12px;border-radius:14px;
      border:1px solid var(--line);background:rgba(0,0,0,.18)
    }
    .item b{font-size:14px}
    .item small{color:var(--muted)}
    .miniBtn{
      border:1px solid var(--line);background:rgba(255,255,255,.03);color:var(--text);
      border-radius:12px;padding:10px 12px;cursor:pointer;font-weight:900
    }
    .hint{margin-top:10px;color:var(--muted);font-size:12px;line-height:1.35}
    .toast{
      position:fixed;left:50%;bottom:16px;transform:translateX(-50%);
      background:rgba(18,26,51,.92);border:1px solid var(--line);border-radius:14px;padding:10px 12px;
      box-shadow:var(--shadow);color:var(--text);font-size:13px;display:none;max-width:92vw
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="top">
      <div class="title">
        <h1>Automatická dvířka</h1>
        <p>ESP32 Access Point</p>
      </div>
      <div class="chip">
        <span class="dot" id="dot"></span>
        <span id="conn">Neověřeno</span>
      </div>
    </div>

    <div class="grid">
      <div class="card">
        <div class="hd">
          <div>
            <h2>Ovládání</h2>
            <small>Manuál / Auto + stav</small>
          </div>
          <span class="pill">IP: 192.168.4.1</span>
        </div>
        <div class="bd">
          <div class="statusBox">
            <div class="statusTop">
              <div class="big">
                <span id="stateDot" class="dot"></span>
                <span id="stateText">Neznámý stav</span>
              </div>
              <span class="pill" id="timeNow">čas: --:--</span>
            </div>

            <div class="toggle">
              <div class="switch" id="sw"><div class="knob"></div></div>
              <div>
                <div style="font-weight:900" id="modeText">MANUÁL</div>
                <label>V AUTO se řídí rozvrhem</label>
              </div>
            </div>
          </div>

          <div class="row" style="margin-top:12px">
            <button class="btn btnOpen" id="btnOpen">OTEVŘÍT</button>
            <button class="btn btnClose" id="btnClose">ZAVŘÍT</button>
          </div>

          <div class="row" style="margin-top:10px">
            <button class="btn btnGhost" id="btnRefresh">Obnovit stav</button>
            <button class="btn btnGhost" id="btnSetTime">Nastavit čas z telefonu</button>
          </div>

          <div class="hint">
            Pozn.: Telefon někdy hlásí „bez internetu“ – to je normální.
          </div>
        </div>
      </div>

      <div class="card">
        <div class="hd">
          <div>
            <h2>Rozvrh</h2>
            <small>Např. otevřít 08:00, zavřít 20:00</small>
          </div>
          <span class="pill" id="schedCount">0 rozvrhů</span>
        </div>
        <div class="bd">
          <div class="form">
            <div class="field">
              <label for="tOpen">Otevřít v</label>
              <input id="tOpen" type="time" value="08:00" />
            </div>
            <div class="field">
              <label for="tClose">Zavřít v</label>
              <input id="tClose" type="time" value="20:00" />
            </div>
          </div>

          <div class="row" style="margin-top:10px">
            <button class="btn btnGhost" id="btnAdd">Přidat rozvrh</button>
            <button class="btn btnGhost" id="btnLoad">Načíst rozvrhy</button>
          </div>

          <div class="list" id="list" style="margin-top:12px"></div>
        </div>
      </div>
    </div>
  </div>

  <div class="toast" id="toast"></div>

<script>
  const el=(id)=>document.getElementById(id);
  const toast=(msg)=>{
    const t=el("toast"); t.textContent=msg; t.style.display="block";
    clearTimeout(window.__t); window.__t=setTimeout(()=>t.style.display="none",2200);
  };
  const setConn=(ok,text)=>{ el("conn").textContent=text; el("dot").style.background=ok?"var(--ok)":"var(--bad)"; };
  const setState=(s)=>{
    el("stateText").textContent=s;
    const d=el("stateDot");
    if(s.includes("OPEN")) d.style.background="var(--ok)";
    else if(s.includes("CLOSE")) d.style.background="var(--bad)";
    else d.style.background="var(--warn)";
  };
  const setMode=(auto)=>{
    el("modeText").textContent=auto?"AUTO":"MANUÁL";
    el("sw").classList.toggle("on", auto);
  };

  async function apiText(path){
    const url=path+(path.includes("?")?"&":"?")+"t="+Date.now();
    const r=await fetch(url,{cache:"no-store"});
    if(!r.ok) throw new Error("HTTP "+r.status);
    return await r.text();
  }

  // Schedule UI
  let schedules=[];
  function renderSchedules(){
    el("schedCount").textContent = schedules.length + (schedules.length===1 ? " rozvrh" : " rozvrhů");
    const list=el("list"); list.innerHTML="";
    schedules.forEach(s=>{
      const div=document.createElement("div");
      div.className="item";
      div.innerHTML=`
        <div>
          <b>Otevřít ${s.open}</b><br/>
          <small>Zavřít ${s.close}</small>
        </div>
        <button class="miniBtn" data-id="${s.id}">Smazat</button>`;
      list.appendChild(div);
    });
    list.querySelectorAll("button[data-id]").forEach(btn=>{
      btn.addEventListener("click", async ()=>{
        const id=btn.getAttribute("data-id");
        try{
          await apiText("/schedule/del?id="+encodeURIComponent(id));
          toast("Smazáno"); setConn(true,"Připojeno"); await loadSchedules();
        }catch(e){ toast("Nelze smazat"); setConn(false,"Chyba"); }
      });
    });
  }

  async function loadSchedules(){
    const txt = await apiText("/schedule/list"); // JSON
    schedules = JSON.parse(txt);
    renderSchedules();
  }

  // Buttons
  el("btnOpen").addEventListener("click", async ()=>{
    try{ await apiText("/open"); setConn(true,"Připojeno"); toast("OPEN"); await refreshStatus(); }
    catch(e){ setConn(false,"Chyba"); toast("Nepovedlo se /open"); }
  });
  el("btnClose").addEventListener("click", async ()=>{
    try{ await apiText("/close"); setConn(true,"Připojeno"); toast("CLOSE"); await refreshStatus(); }
    catch(e){ setConn(false,"Chyba"); toast("Nepovedlo se /close"); }
  });
  el("btnRefresh").addEventListener("click", async ()=>{
    await refreshStatus();
  });

  async function refreshStatus(){
    try{
      const txt = await apiText("/status"); // text lines
      setConn(true,"Připojeno");
      // očekávej "STATE=OPEN\nMODE=AUTO\nTIME=HH:MM"
      const lines = txt.split("\n").map(x=>x.trim()).filter(Boolean);
      let st="UNKNOWN", md="MANUAL", tm="--:--";
      lines.forEach(l=>{
        if(l.startsWith("STATE=")) st=l.substring(6);
        if(l.startsWith("MODE=")) md=l.substring(5);
        if(l.startsWith("TIME=")) tm=l.substring(5);
      });
      setState(st);
      setMode(md==="AUTO");
      el("timeNow").textContent="čas: "+tm;
    }catch(e){
      setConn(false,"Offline");
      toast("Status nejde načíst");
    }
  }

  el("sw").addEventListener("click", async ()=>{
    const wantAuto = !el("sw").classList.contains("on");
    try{
      await apiText("/mode?auto="+(wantAuto?"1":"0"));
      setConn(true,"Připojeno");
      toast(wantAuto?"AUTO":"MANUÁL");
      await refreshStatus();
    }catch(e){
      setConn(false,"Chyba");
      toast("Nejde změnit režim");
    }
  });

  el("btnAdd").addEventListener("click", async ()=>{
    const open = el("tOpen").value || "08:00";
    const close = el("tClose").value || "20:00";
    try{
      await apiText(`/schedule/add?open=${encodeURIComponent(open)}&close=${encodeURIComponent(close)}`);
      setConn(true,"Připojeno");
      toast("Rozvrh přidán");
      await loadSchedules();
    }catch(e){
      setConn(false,"Chyba");
      toast("Nepovedlo se přidat");
    }
  });

  el("btnLoad").addEventListener("click", async ()=>{
    try{
      await loadSchedules();
      setConn(true,"Připojeno");
      toast("Načteno");
    }catch(e){
      setConn(false,"Chyba");
      toast("Nejde načíst");
    }
  });

  el("btnSetTime").addEventListener("click", async ()=>{
    const d=new Date();
    const hh=String(d.getHours()).padStart(2,"0");
    const mm=String(d.getMinutes()).padStart(2,"0");
    try{
      await apiText(`/time/set?hh=${hh}&mm=${mm}`);
      setConn(true,"Připojeno");
      toast("Čas nastaven "+hh+":"+mm);
      await refreshStatus();
    }catch(e){
      setConn(false,"Chyba");
      toast("Nejde nastavit čas");
    }
  });

  // init
  setConn(false,"Neověřeno");
  setState("UNKNOWN");
  setMode(false);
  (async ()=>{
    try{
      await refreshStatus();
      await loadSchedules();
    }catch(e){}
  })();
</script>
</body>
</html>
)rawliteral";

// =========================
//  POMOCNÉ FUNKCE
// =========================
static inline uint16_t clampU16(int v, int lo, int hi){
  if(v<lo) return lo;
  if(v>hi) return hi;
  return (uint16_t)v;
}

bool parseTimeHHMM(const String& s, uint16_t &outMin){
  // očekává "08:00"
  if(s.length() != 5 || s.charAt(2) != ':') return false;
  int hh = s.substring(0,2).toInt();
  int mm = s.substring(3,5).toInt();
  if(hh<0 || hh>23 || mm<0 || mm>59) return false;
  outMin = (uint16_t)(hh*60 + mm);
  return true;
}

String minToHHMM(uint16_t m){
  uint16_t hh = m / 60;
  uint16_t mm = m % 60;
  char buf[6];
  snprintf(buf, sizeof(buf), "%02u:%02u", hh, mm);
  return String(buf);
}

uint32_t nowSecondsOfDay(){
  if(!timeSet) return 0;
  uint32_t elapsed = (millis() - baseMillis) / 1000UL;
  return (baseSecondsOfDay + elapsed) % 86400UL;
}

uint16_t nowMinutesOfDay(){
  return (uint16_t)(nowSecondsOfDay() / 60UL);
}

String nowHHMM(){
  if(!timeSet) return "--:--";
  uint32_t s = nowSecondsOfDay();
  uint32_t hh = s / 3600UL;
  uint32_t mm = (s % 3600UL) / 60UL;
  char buf[6];
  snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)hh, (unsigned long)mm);
  return String(buf);
}

// Servo akce
void doOpen(){
  doorState = OPENING;
  servo1.write(ANGLE_OPEN);
  if(USE_SERVO2) servo2.write(ANGLE_OPEN);
  // v jednoduché verzi hned přepneme stav (bez koncáků)
  doorState = OPEN;
}

void doClose(){
  doorState = CLOSING;
  servo1.write(ANGLE_CLOSE);
  if(USE_SERVO2) servo2.write(ANGLE_CLOSE);
  doorState = CLOSED;
}

String stateToString(){
  switch(doorState){
    case OPEN: return "OPEN";
    case CLOSED: return "CLOSED";
    case OPENING: return "OPENING";
    case CLOSING: return "CLOSING";
    case STOPPED: return "STOPPED";
    default: return "UNKNOWN";
  }
}

String modeToString(){
  return autoMode ? "AUTO" : "MANUAL";
}

// =========================
//  SCHEDULE API
// =========================
void initSchedules(){
  for(int i=0;i<MAX_SCHEDULES;i++){
    schedules[i].used=false;
    schedules[i].id=0;
    schedules[i].openMin=0;
    schedules[i].closeMin=0;
    lastActionMinuteOpen[i] = -1;
    lastActionMinuteClose[i] = -1;
  }
}

int findFreeScheduleSlot(){
  for(int i=0;i<MAX_SCHEDULES;i++) if(!schedules[i].used) return i;
  return -1;
}

int findScheduleById(uint32_t id){
  for(int i=0;i<MAX_SCHEDULES;i++){
    if(schedules[i].used && schedules[i].id==id) return i;
  }
  return -1;
}

String schedulesToJson(){
  String json = "[";
  bool first=true;
  for(int i=0;i<MAX_SCHEDULES;i++){
    if(!schedules[i].used) continue;
    if(!first) json += ",";
    first=false;
    json += "{";
    json += "\"id\":\"" + String(schedules[i].id) + "\",";
    json += "\"open\":\"" + minToHHMM(schedules[i].openMin) + "\",";
    json += "\"close\":\"" + minToHHMM(schedules[i].closeMin) + "\"";
    json += "}";
  }
  json += "]";
  return json;
}

// v AUTO režimu: kontrola každou smyčku
void scheduleTick(){
  if(!autoMode) return;
  if(!timeSet) return;
  uint16_t nowMin = nowMinutesOfDay();

  for(int i=0;i<MAX_SCHEDULES;i++){
    if(!schedules[i].used) continue;

    // otevřít
    if(nowMin == schedules[i].openMin && lastActionMinuteOpen[i] != (int)nowMin){
      doOpen();
      lastActionMinuteOpen[i] = nowMin;
    }

    // zavřít
    if(nowMin == schedules[i].closeMin && lastActionMinuteClose[i] != (int)nowMin){
      doClose();
      lastActionMinuteClose[i] = nowMin;
    }

    // reset ochrany po změně minuty (aby to fungovalo další den)
    // (tady stačí jednoduché – při rozdílné minutě už se to spustí znovu až když se to rovná)
  }
}

// =========================
//  HTTP HELPERS (synchronní)
// =========================
String getQueryParam(const String& reqLine, const String& key){
  // reqLine = "GET /path?x=1&y=2 HTTP/1.1"
  int q = reqLine.indexOf('?');
  if(q < 0) return "";
  int sp = reqLine.indexOf(' ', q);
  String qs = reqLine.substring(q+1, sp); // "x=1&y=2"
  String find = key + "=";
  int p = qs.indexOf(find);
  if(p < 0) return "";
  int start = p + find.length();
  int amp = qs.indexOf('&', start);
  if(amp < 0) amp = qs.length();
  String val = qs.substring(start, amp);
  val.replace("%3A", ":"); // minimální decode pro HH:MM
  val.replace("%2F", "/");
  val.replace("%20", " ");
  return val;
}

String getPath(const String& reqLine){
  // "GET /open HTTP/1.1" -> "/open"
  int s1 = reqLine.indexOf(' ');
  if(s1 < 0) return "/";
  int s2 = reqLine.indexOf(' ', s1+1);
  if(s2 < 0) return "/";
  String full = reqLine.substring(s1+1, s2);
  int q = full.indexOf('?');
  if(q >= 0) return full.substring(0, q);
  return full;
}

void sendText(WiFiClient &client, const String& body, const String& type="text/plain; charset=utf-8"){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:" + type);
  client.println("Connection: close");
  client.println();
  client.print(body);
}

void sendHtml(WiFiClient &client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.print(INDEX_HTML);
}

void sendNotFound(WiFiClient &client){
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-type:text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.print("404");
}

// iPhone captive portal endpointy – ať se chytne stránka
bool handleCaptivePortal(const String& path, WiFiClient &client){
  if(path == "/generate_204" || path == "/hotspot-detect.html" || path == "/fwlink" || path == "/") {
    sendHtml(client);
    return true;
  }
  return false;
}

// =========================
//  SETUP / LOOP
// =========================
void setup() {
  Serial.begin(115200);
  delay(200);

  initSchedules();

  // Servo init
  servo1.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, 500, 2400);
  if(USE_SERVO2){
    servo2.setPeriodHertz(50);
    servo2.attach(SERVO2_PIN, 500, 2400);
  }

  // Default state
  doClose();

  // Wi-Fi AP start
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP spuštěn. SSID: ");
  Serial.println(AP_SSID);
  Serial.print("IP: ");
  Serial.println(IP);

  server.begin();
}

void loop() {
  scheduleTick();

  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("New Client connected");

  // přečteme jen první řádek requestu
  String reqLine = client.readStringUntil('\n');
  reqLine.trim(); // remove \r
  // "GET /path HTTP/1.1"
  // pro jistotu dočteme zbytek hlaviček, ale rychle:
  while (client.connected() && client.available()) {
    String line = client.readStringUntil('\n');
    if(line == "\r" || line.length() == 1) break;
  }

  String path = getPath(reqLine);

  // captive portal support
  if(handleCaptivePortal(path, client)){
    client.stop();
    return;
  }

  // ROUTES
  if(path == "/open"){
    doOpen();
    sendText(client, "OK");
  }
  else if(path == "/close"){
    doClose();
    sendText(client, "OK");
  }
  else if(path == "/mode"){
    String a = getQueryParam(reqLine, "auto");
    autoMode = (a == "1");
    sendText(client, "OK");
  }
  else if(path == "/status"){
    String body;
    body += "STATE=" + stateToString() + "\n";
    body += "MODE=" + modeToString() + "\n";
    body += "TIME=" + nowHHMM() + "\n";
    sendText(client, body);
  }
  else if(path == "/time/set"){
    String hhS = getQueryParam(reqLine, "hh");
    String mmS = getQueryParam(reqLine, "mm");
    int hh = hhS.toInt();
    int mm = mmS.toInt();
    hh = (int)clampU16(hh, 0, 23);
    mm = (int)clampU16(mm, 0, 59);
    baseSecondsOfDay = (uint32_t)(hh*3600 + mm*60);
    baseMillis = millis();
    timeSet = true;
    sendText(client, "OK");
  }
  else if(path == "/schedule/add"){
    String openS  = getQueryParam(reqLine, "open");  // "08:00"
    String closeS = getQueryParam(reqLine, "close"); // "20:00"
    uint16_t oMin, cMin;
    if(!parseTimeHHMM(openS, oMin) || !parseTimeHHMM(closeS, cMin)){
      sendText(client, "BAD_TIME", "text/plain; charset=utf-8");
    }else{
      int idx = findFreeScheduleSlot();
      if(idx < 0){
        sendText(client, "FULL", "text/plain; charset=utf-8");
      }else{
        schedules[idx].used = true;
        schedules[idx].openMin = oMin;
        schedules[idx].closeMin = cMin;
        schedules[idx].id = (uint32_t)esp_random(); // id pro mazání
        lastActionMinuteOpen[idx] = -1;
        lastActionMinuteClose[idx] = -1;
        sendText(client, "OK");
      }
    }
  }
  else if(path == "/schedule/list"){
    String json = schedulesToJson();
    sendText(client, json, "application/json; charset=utf-8");
  }
  else if(path == "/schedule/del"){
    String idS = getQueryParam(reqLine, "id");
    uint32_t id = (uint32_t)idS.toInt();
    int idx = findScheduleById(id);
    if(idx >= 0){
      schedules[idx].used = false;
      sendText(client, "OK");
    }else{
      sendText(client, "NOT_FOUND");
    }
  }
  else if(path == "/"){
    sendHtml(client);
  }
  else {
    sendNotFound(client);
  }

  client.stop();
}
