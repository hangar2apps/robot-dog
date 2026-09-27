/*
 * RoboDog — Phase 1 firmware
 * -------------------------------------------------------------
 * ESP32 "brain transplant" for a 4-DC-motor toy robot dog.
 *
 * What it does:
 *   - Joins your home WiFi; if that fails, falls back to its own
 *     access point ("RoboDog-AP", password "robodog123").
 *   - Serves a phone-friendly web control pad at the ESP32's IP (or 192.168.4.1 in AP mode).
 *   - Exposes a small JSON API so a program (e.g. rodrigo) can drive
 *     the SAME endpoints the web pad uses. One API, two clients.
 *   - Drives 4 DC motors (2 per side) as tank/skid steer through two DRV8833 boards.
 *
 * Wiring: see wiring section in README. Each motor = 2 ESP32 GPIOs -> one DRV8833 channel.
 *
 * Safety: a watchdog stops the motors if no command arrives for CMD_TIMEOUT_MS.
 *         So if WiFi drops or the browser closes mid-drive, the dog halts instead of running off.
 * -------------------------------------------------------------
 */

#include <WiFi.h>
#include <WebServer.h>

// ---------------- USER CONFIG ----------------
const char* WIFI_SSID = "YOUR_WIFI_NAME";       // <-- put your home WiFi here
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";   // <-- and password here

const char* AP_SSID   = "RoboDog-AP";           // fallback network name
const char* AP_PASS   = "robodog123";           // >= 8 chars

const uint32_t WIFI_JOIN_TIMEOUT_MS = 10000;    // how long to try home WiFi before AP fallback
const uint32_t CMD_TIMEOUT_MS       = 600;      // stop motors if no command within this window
// ---------------------------------------------

// ---- Motor pin map (ESP32 GPIO -> DRV8833 inputs) ----
// DRV8833 #1 = LEFT side, DRV8833 #2 = RIGHT side.
// Each motor uses one channel (IN1/IN2). PWM on one pin = speed; which pin = direction.
struct Motor { uint8_t in1; uint8_t in2; };

Motor LF = {25, 26};   // Left-Front   -> DRV8833 #1 AIN1/AIN2
Motor LR = {27, 14};   // Left-Rear    -> DRV8833 #1 BIN1/BIN2
Motor RF = {32, 33};   // Right-Front  -> DRV8833 #2 AIN1/AIN2
Motor RR = {13, 23};   // Right-Rear   -> DRV8833 #2 BIN1/BIN2

// PWM config
const int PWM_FREQ = 20000;   // 20 kHz = silent (above hearing), good for small motors
const int PWM_RES  = 8;       // 8-bit -> duty 0..255

WebServer server(80);
uint32_t lastCmdMs = 0;

// Attach both pins of a motor to LEDC PWM channels
void setupMotor(const Motor& m) {
  ledcAttach(m.in1, PWM_FREQ, PWM_RES);
  ledcAttach(m.in2, PWM_FREQ, PWM_RES);
  ledcWrite(m.in1, 0);
  ledcWrite(m.in2, 0);
}

// speed: -255..255. Positive = forward, negative = reverse, 0 = coast.
void driveMotor(const Motor& m, int speed) {
  speed = constrain(speed, -255, 255);
  if (speed >= 0) {
    ledcWrite(m.in1, speed);
    ledcWrite(m.in2, 0);
  } else {
    ledcWrite(m.in1, 0);
    ledcWrite(m.in2, -speed);
  }
}

// Tank drive: left value -> both left motors, right value -> both right motors.
int lastLeft = 0, lastRight = 0;
void drive(int left, int right) {
  lastLeft = constrain(left, -255, 255);
  lastRight = constrain(right, -255, 255);
  driveMotor(LF, lastLeft);
  driveMotor(LR, lastLeft);
  driveMotor(RF, lastRight);
  driveMotor(RR, lastRight);
  lastCmdMs = millis();
}

void stopAll() {
  driveMotor(LF, 0); driveMotor(LR, 0);
  driveMotor(RF, 0); driveMotor(RR, 0);
  lastLeft = lastRight = 0;
}

// ---------------- Web control pad ----------------
const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>RoboDog</title>
<style>
  :root{--bg:#0f1115;--fg:#e8eaed;--accent:#4f8cff;--btn:#1c1f26;}
  *{box-sizing:border-box;-webkit-tap-highlight-color:transparent;}
  body{margin:0;background:var(--bg);color:var(--fg);font-family:system-ui,sans-serif;
       height:100dvh;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:18px;}
  h1{font-size:18px;font-weight:600;margin:0;opacity:.8;}
  #pad{display:grid;grid-template-columns:repeat(3,90px);grid-template-rows:repeat(3,90px);gap:10px;}
  button{background:var(--btn);color:var(--fg);border:1px solid #2a2e37;border-radius:16px;
         font-size:30px;touch-action:none;user-select:none;}
  button:active{background:var(--accent);}
  .stop{grid-column:2;grid-row:2;background:#3a1c1c;}
  #fwd{grid-column:2;grid-row:1;} #back{grid-column:2;grid-row:3;}
  #left{grid-column:1;grid-row:2;} #right{grid-column:3;grid-row:2;}
  #speed{width:270px;} label{font-size:13px;opacity:.7;}
  #status{font-size:12px;opacity:.5;height:16px;}
</style></head><body>
<h1>RoboDog Control</h1>
<div id="pad">
  <button id="fwd">&#9650;</button>
  <button id="left">&#9664;</button>
  <button class="stop" id="stop">&#9632;</button>
  <button id="right">&#9654;</button>
  <button id="back">&#9660;</button>
</div>
<label>Speed <input type="range" id="speed" min="80" max="255" value="200"></label>
<div id="status">ready</div>
<script>
let spd=200;
const s=document.getElementById('speed'), st=document.getElementById('status');
s.oninput=()=>spd=+s.value;
function send(l,r){fetch('/drive',{method:'POST',headers:{'Content-Type':'application/json'},
  body:JSON.stringify({left:l,right:r})}).then(_=>st.textContent='L:'+l+' R:'+r).catch(_=>st.textContent='err');}
function stop(){fetch('/stop',{method:'POST'}).then(_=>st.textContent='stop');}
// one binding path per button; DIR gives the tank mix for each
const DIR={fwd:[1,1],back:[-1,-1],left:[-1,1],right:[1,-1]};
Object.keys(DIR).forEach(id=>{
  const b=document.getElementById(id), m=DIR[id];
  const on=e=>{e.preventDefault();send(m[0]*spd,m[1]*spd);};
  const off=e=>{e.preventDefault();stop();};
  b.addEventListener('touchstart',on,{passive:false});
  b.addEventListener('touchend',off);
  b.addEventListener('mousedown',on);
  b.addEventListener('mouseup',off);
  b.addEventListener('mouseleave',off);
});
document.getElementById('stop').addEventListener('click',stop);
</script></body></html>
)HTML";

// Tiny JSON int extractor (no library needed): finds "key": <int>
int jsonInt(const String& body, const char* key, int dflt) {
  int k = body.indexOf(String("\"") + key + "\"");
  if (k < 0) return dflt;
  int c = body.indexOf(':', k);
  if (c < 0) return dflt;
  return body.substring(c + 1).toInt();  // toInt handles leading spaces and sign
}

void handleRoot()  { server.send_P(200, "text/html", INDEX_HTML); }
void handleDrive() {
  String b = server.arg("plain");
  int l = jsonInt(b, "left", 0);
  int r = jsonInt(b, "right", 0);
  drive(l, r);
  server.send(200, "application/json", "{\"ok\":true}");
}
void handleStop()  { stopAll(); server.send(200, "application/json", "{\"ok\":true}"); }
void handleState() {
  char buf[96];
  snprintf(buf, sizeof(buf), "{\"left\":%d,\"right\":%d,\"uptime\":%lu}", lastLeft, lastRight, millis()/1000);
  server.send(200, "application/json", buf);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  setupMotor(LF); setupMotor(LR); setupMotor(RF); setupMotor(RR);
  stopAll();

  // Try home WiFi, then fall back to AP
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Joining WiFi");
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_JOIN_TIMEOUT_MS) {
    delay(300); Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConnected. Open http://%s\n", WiFi.localIP().toString().c_str());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("\nHome WiFi failed. AP up: SSID '%s' pass '%s' -> http://%s\n",
                  AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str());
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/drive", HTTP_POST, handleDrive);
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/state", HTTP_GET, handleState);
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
  // Watchdog: if no command recently and motors are running, stop.
  if ((lastLeft || lastRight) && millis() - lastCmdMs > CMD_TIMEOUT_MS) {
    stopAll();
  }
}
