// aircore.ino - drives the stock NB Miata tach and speedo (air-core gauges) from the MSP-DASH hub's data.
// Boards:
//   Wemos D1 Mini (ESP8266), fqbn esp8266:esp8266:d1_mini: ONE gauge per board (type "role tach" or "role speedo"
//     once), one DRV8833 each, on D1 / D2 / D5 / D6.
//   Lolin ESP32-S2 Mini, fqbn esp32:esp32:lolin_s2_mini: BOTH gauges on one board, two DRV8833.
//   ESP32 DevKitC (WROOM), fqbn esp32:esp32:esp32: BOTH gauges, two DRV8833, on GPIO 16-19 and 21-23, 25
//     (hardware PWM, steady whatever Wi-Fi does; no boot or flash pins).
// It only listens: it picks up the hub's ESP-NOW data packet (RPM and speed) on whatever channel the hub is on, and
// needs no Wi-Fi network. Setup and calibration over the USB serial port (115200): type "help", or on the hub's
// "Stock gauges" page (build 7 on: modes, PWM, calibration over ESP-NOW). See README.md.
#include <Arduino.h>
#include "aircore.h"
#include "aircore_link.h"

#define AIRCORE_BUILD 11
#define PWM_HZ_DEFAULT 20000  // above hearing, so the coils do not whine (the D1 Mini may need lower: "pwm" command)
static uint32_t s_pwmHz = PWM_HZ_DEFAULT;
static uint8_t s_slow = 1;    // 1 = slow decay (brake between pulses, the default), 0 = fast decay: "decay" command
#define PWM_FULL 1023
#define STEP_MS 5             // needle update
#define LOST_MS 1500          // no data for this long: needles go to the start of the scale
#define HUNT_MS 5000          // no data for this long: hunt the channels for the hub (not sooner: a short gap is just a
                              // missed packet, and while hunting the board misses the hub's commands too)
#define HOP_MS 300            // channel scan: time on each channel
#define RATE_MIN 8            // the hub sends ~20 packets a second: fewer than this over RATE_MS = wrong channel, scan
#define RATE_MS 4000
#define HOLD_MS 60000         // "angle" holds the needle this long (calibration)

#if defined(ESP8266)
  // ---- ESP8266 (D1 Mini): one gauge, settings in the EEPROM area ----
  #include <ESP8266WiFi.h>
  #include <espnow.h>
  #include <EEPROM.h>
  extern "C" {
  #include <user_interface.h>
  }
  #define GAUGES 1
  #define LOCK()   noInterrupts()
  #define UNLOCK() interrupts()
#else
  // ---- ESP32-S2 Mini: both gauges, settings in NVS ----
  #include <WiFi.h>
  #include <esp_now.h>
  #include <esp_wifi.h>
  #include <Preferences.h>
  #define GAUGES 2
  static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
  #define LOCK()   portENTER_CRITICAL(&s_mux)
  #define UNLOCK() portEXIT_CRITICAL(&s_mux)
#endif

typedef struct {
  const char *name;
  uint8_t pins[4];            // DRV8833 IN1, IN2 (sine coil) and IN3, IN4 (cosine coil)
  int sig;
  AirCoreCfg cfg;
  AirCorePoints pts;          // multi-point calibration (2+ points: used instead of zero / span)
  float needle;               // where it is now, degrees
  float holdDeg; uint32_t holdUntil;   // AC_MODE_ANGLE: the angle, and until when (0 = until told otherwise)
  uint8_t mode;               // AC_MODE_LIVE / SWEEP / TARGET / ANGLE (not saved: every start is live)
  float target, sweepS, shown;   // TARGET: the value; SWEEP: seconds end to end and back; what it shows now
  uint32_t sweepT0;
} Gauge;
#if defined(ESP8266)
// D1 = GPIO 5, D2 = GPIO 4, D5 = GPIO 14, D6 = GPIO 12 (none of them a boot pin). Its role picks tach or speedo.
static Gauge G[GAUGES] = { { "tach", { 5, 4, 14, 12 }, LINK_SIG_RPM, {}, {}, 0, 0, 0, AC_MODE_LIVE, 0, 4, 0, 0 } };
#elif defined(CONFIG_IDF_TARGET_ESP32)
// ESP32 DevKitC: tach IN1-IN4 on GPIO 16, 17, 18, 19; speedo on 21, 22, 23, 25 (none a boot, flash or input-only pin).
static Gauge G[GAUGES] = {
  { "tach",   { 16, 17, 18, 19 }, LINK_SIG_RPM, {}, {}, 0, 0, 0, AC_MODE_LIVE, 0, 4, 0, 0 },
  { "speedo", { 21, 22, 23, 25 }, LINK_SIG_VSS, {}, {}, 0, 0, 0, AC_MODE_LIVE, 0, 4, 0, 0 },
};
#else
static Gauge G[GAUGES] = {
  { "tach",   { 3, 5, 7, 9 },     LINK_SIG_RPM, {}, {}, 0, 0, 0, AC_MODE_LIVE, 0, 4, 0, 0 },
  { "speedo", { 11, 12, 16, 18 }, LINK_SIG_VSS, {}, {}, 0, 0, 0, AC_MODE_LIVE, 0, 4, 0, 0 },
};
#endif

static volatile float s_val[GAUGES];
static volatile bool s_has[GAUGES], s_sim = false;
static volatile uint32_t s_lastRx = 0, s_rx = 0;
static uint8_t s_ch = 1;
static uint32_t s_hopMs = 0;
// Channel scan (build 11): every channel is listened to for HOP_MS and the one with the MOST of the hub's packets wins.
// Stopping at the first channel with any packet was wrong on the bench: right next to the hub, a few of its packets
// leak onto far-away channels (the board sat on 1 while the hub was on 10, hearing a trickle and missing the rest).
static uint8_t s_scanCh = 0, s_bestCh = 1;     // s_scanCh 0 = not scanning
static uint32_t s_scanRx0 = 0, s_bestN = 0, s_rateT0 = 0, s_rateRx0 = 0, s_scans = 0;
static float s_rate = 0;

static void defaultsFor(Gauge &g) { if (g.sig == LINK_SIG_RPM) aircore_defaults_tach(&g.cfg); else aircore_defaults_speedo(&g.cfg); }

// ---- settings ----
#if defined(ESP8266)
// EEPROM layout: [0] version, [1] role (0 tach, 1 speedo), [4..] AirCoreCfg
#define EE_SIZE 256   // [64..] the calibration points
static void setRole(int speedo) { G[0].name = speedo ? "speedo" : "tach"; G[0].sig = speedo ? LINK_SIG_VSS : LINK_SIG_RPM; }
static void cfgSave(int) {
  EEPROM.write(0, AIRCORE_VERSION); EEPROM.write(1, G[0].sig == LINK_SIG_VSS ? 1 : 0);
  EEPROM.put(4, G[0].cfg); EEPROM.put(40, s_pwmHz); EEPROM.write(44, s_slow); EEPROM.put(64, G[0].pts); EEPROM.commit();
}
static void cfgLoad() {
  EEPROM.begin(EE_SIZE);
  bool ok = EEPROM.read(0) == AIRCORE_VERSION;
  setRole(ok && EEPROM.read(1) == 1);
  if (ok) { EEPROM.get(4, G[0].cfg); EEPROM.get(40, s_pwmHz); s_slow = EEPROM.read(44) != 0; EEPROM.get(64, G[0].pts); }
  if (!ok || !aircore_points_valid(&G[0].pts)) aircore_points_clear(&G[0].pts);
  if (s_pwmHz < 100 || s_pwmHz > 40000) s_pwmHz = PWM_HZ_DEFAULT;
  if (!ok || !aircore_valid(&G[0].cfg)) defaultsFor(G[0]);
}
#else
static void cfgSave(int g) { Preferences p; p.begin("aircore", false); p.putBytes(G[g].name, &G[g].cfg, sizeof(AirCoreCfg)); p.putUChar("ver", AIRCORE_VERSION); p.putULong("pwm", s_pwmHz); p.putUChar("slow", s_slow);
  p.putBytes((String(G[g].name) + "p").c_str(), &G[g].pts, sizeof(AirCorePoints)); p.end(); }
static void cfgLoad() {
  Preferences p; p.begin("aircore", true);
  bool ok = p.getUChar("ver", 0) == AIRCORE_VERSION;
  s_slow = p.getUChar("slow", 1) != 0;
  s_pwmHz = p.getULong("pwm", PWM_HZ_DEFAULT); if (s_pwmHz < 100 || s_pwmHz > 40000) s_pwmHz = PWM_HZ_DEFAULT;
  for (int g = 0; g < GAUGES; g++) {
    if (!ok || p.getBytes(G[g].name, &G[g].cfg, sizeof(AirCoreCfg)) != sizeof(AirCoreCfg) || !aircore_valid(&G[g].cfg)) defaultsFor(G[g]);
    if (p.getBytes((String(G[g].name) + "p").c_str(), &G[g].pts, sizeof(AirCorePoints)) != sizeof(AirCorePoints) || !aircore_points_valid(&G[g].pts)) aircore_points_clear(&G[g].pts);
  }
  p.end();
}
#endif

// ---- the hub's data packet: RPM and speed out of it, if it has them ----
static volatile bool s_cmdNew = false;
static AcCmd s_cmdRx;
static uint8_t s_myMac[6];
static void handlePacket(const uint8_t *d, int len) {
  if (len >= (int)sizeof(AcCmd) && ((const AcCmd *)d)->magic == AC_CMD_MAGIC) {   // from the hub's Stock gauges page
    const AcCmd *c = (const AcCmd *)d;
    static const uint8_t all[6] = { 255, 255, 255, 255, 255, 255 };
    if (memcmp(c->mac, s_myMac, 6) && memcmp(c->mac, all, 6)) return;
    LOCK(); memcpy(&s_cmdRx, d, sizeof s_cmdRx); s_cmdNew = true; UNLOCK();
    return;
  }
  if (len < LINK_PKT_V_OFFSET + 4) return;
  uint32_t magic; memcpy(&magic, d, 4);
  if (magic != LINK_PKT_MAGIC) return;
  uint64_t has; memcpy(&has, d + 8, 8);
  LOCK();
  for (int g = 0; g < GAUGES; g++) {
    int at = LINK_PKT_V_OFFSET + 4 * G[g].sig;
    if (at + 4 <= len && ((has >> G[g].sig) & 1)) { float v; memcpy(&v, d + at, 4); s_val[g] = v; s_has[g] = true; }
    else s_has[g] = false;
  }
  s_sim = has & AC_MASK_SIMULATED;
  s_lastRx = millis(); s_rx++;
  UNLOCK();
}
#if defined(ESP8266)
static void onRecv(uint8_t *mac, uint8_t *d, uint8_t len) { handlePacket(d, len); }
static void setChannel(uint8_t ch) { wifi_set_channel(ch); }
#else
static void onRecv(const esp_now_recv_info_t *info, const uint8_t *d, int len) { handlePacket(d, len); }
static void setChannel(uint8_t ch) { esp_wifi_set_promiscuous(true); esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE); esp_wifi_set_promiscuous(false); }
#endif

// ---- coils ----
static void pwmApply() {   // the PWM frequency (the D1 Mini makes PWM in software: lower is steadier there)
#if defined(ESP8266)
  analogWriteFreq(s_pwmHz); analogWriteRange(PWM_FULL);
#else
  for (int g = 0; g < GAUGES; g++) for (int k = 0; k < 4; k++) { ledcDetach(G[g].pins[k]); ledcAttach(G[g].pins[k], s_pwmHz, 10); }
#endif
}
static void pwmWrite(uint8_t pin, uint32_t duty) {
#if defined(ESP8266)
  analogWrite(pin, duty);
#else
  ledcWrite(pin, duty);
#endif
}
static void drive(int g, float deg) {
  float s, c; aircore_coils(deg, G[g].cfg.strength, &s, &c);
  uint32_t a, b;
  void (*bridge)(float, uint32_t, uint32_t *, uint32_t *) = s_slow ? aircore_bridge_slow : aircore_bridge;
  bridge(s, PWM_FULL, &a, &b); pwmWrite(G[g].pins[0], a); pwmWrite(G[g].pins[1], b);
  bridge(c, PWM_FULL, &a, &b); pwmWrite(G[g].pins[2], a); pwmWrite(G[g].pins[3], b);
}

// Start-up sweep: the needle(s) to the end of the scale and back, like the stock cluster.
static void sweep() {
  for (int pass = 0; pass < 2; pass++)
    for (int k = 0; k <= 100; k++) {
      float f = pass ? 1.0f - k / 100.0f : k / 100.0f;
      for (int g = 0; g < GAUGES; g++) {
        float lo = G[g].pts.n >= 2 ? G[g].pts.v[0] : G[g].cfg.minVal, hi = G[g].pts.n >= 2 ? G[g].pts.v[G[g].pts.n - 1] : G[g].cfg.maxVal;
        G[g].needle = aircore_angle_pts(&G[g].cfg, &G[g].pts, lo + f * (hi - lo)); drive(g, G[g].needle);
      }
      delay(7);
    }
}

// ---- the hub's Stock gauges page ----
static uint32_t s_stSent = 0, s_stFail = 0;   // status packets to the hub (show prints them)
static void sendRaw(const uint8_t *d, int len) {
  static uint8_t bc[6] = { 255, 255, 255, 255, 255, 255 };
#if defined(ESP8266)
  int r = esp_now_send(bc, (uint8_t *)d, len);
#else
  esp_err_t r = esp_now_send(bc, d, len);
  if (r == ESP_ERR_ESPNOW_NOT_FOUND) {   // the broadcast peer went missing: add it again and retry once
    esp_now_peer_info_t pi = {}; memset(pi.peer_addr, 0xFF, 6); pi.channel = 0; pi.encrypt = false;
    esp_now_add_peer(&pi); r = esp_now_send(bc, d, len);
  }
#endif
  if (r == 0) s_stSent++; else s_stFail++;
}
static uint16_t s_lastSeq = 0;   // the last command applied (so the hub page can tell it arrived)
static void sendStatus(int g) {
  AcStatus st; memset(&st, 0, sizeof st);
  st.magic = AC_STATUS_MAGIC; st.build = AIRCORE_BUILD; st.gauge = g; st.gauges = GAUGES; st.sig = G[g].sig;
  st.mode = G[g].mode; st.slow = s_slow; st.hubHeard = s_lastRx && millis() - s_lastRx < LOST_MS; st.has = s_has[g];
  st.pwmHz = s_pwmHz; st.needle = G[g].needle; st.value = s_val[g]; st.target = G[g].shown; st.sweepS = G[g].sweepS;
  st.strength = G[g].cfg.strength; st.slew = G[g].cfg.slewDegS; st.minV = G[g].cfg.minVal; st.maxV = G[g].cfg.maxVal;
  st.zero = G[g].cfg.zeroDeg; st.span = G[g].cfg.spanDeg; st.npts = G[g].pts.n; st.ch = s_ch;
  st.lastSeq = s_lastSeq; st.upS = millis() / 1000;
  for (int i = 0; i < G[g].pts.n && i < AC_POINTS; i++) { st.pv[i] = G[g].pts.v[i]; st.pa[i] = G[g].pts.a[i]; }
  sendRaw((const uint8_t *)&st, sizeof st);
}
static void setMode(int g, uint8_t m) { G[g].mode = m; G[g].holdUntil = 0; G[g].sweepT0 = millis(); }
static const char *const AC_OP_NAMES[] = { "?", "live", "sweep", "target", "angle", "pwm", "decay", "strength", "slew",
  "range", "mark", "mark here", "remove mark", "clear marks", "factory settings", "zero", "span" };
static void applyCmd(const AcCmd &c) {
  if (c.seq && c.seq == s_lastSeq) return;   // the same command again (the hub sends each one 3 times)
  s_lastSeq = c.seq;
  int g = c.gauge < GAUGES ? c.gauge : 0;
  // Every command from the hub's page, on the serial port (build 10): what arrived and for which gauge.
  Serial.printf("hub page, command %u: %s %s", c.seq, G[g].name, c.op <= AC_OP_SPAN ? AC_OP_NAMES[c.op] : "?");
  if (c.op == AC_OP_RANGE || c.op == AC_OP_MARK) Serial.printf(" %g %g", c.a, c.b);
  else if (c.op != AC_OP_LIVE && c.op != AC_OP_CLEARMARKS && c.op != AC_OP_DEFAULTS) Serial.printf(" %g", c.a);
  Serial.println();
  AirCoreCfg cfg = G[g].cfg;
  switch (c.op) {
    case AC_OP_LIVE: setMode(g, AC_MODE_LIVE); return;
    case AC_OP_SWEEP: setMode(g, AC_MODE_SWEEP); if (c.a >= 1 && c.a <= 60) G[g].sweepS = c.a; return;
    case AC_OP_TARGET: setMode(g, AC_MODE_TARGET); G[g].target = c.a; return;
    case AC_OP_ANGLE: setMode(g, AC_MODE_ANGLE); G[g].holdDeg = c.a; return;
    case AC_OP_PWM: if (c.a >= 100 && c.a <= 40000) { s_pwmHz = (uint32_t)c.a; pwmApply(); for (int k = 0; k < GAUGES; k++) cfgSave(k); } return;
    case AC_OP_DECAY: s_slow = c.a != 0; for (int k = 0; k < GAUGES; k++) cfgSave(k); return;
    case AC_OP_STRENGTH: cfg.strength = c.a; break;
    case AC_OP_SLEW: cfg.slewDegS = c.a; break;
    case AC_OP_RANGE: cfg.minVal = c.a; cfg.maxVal = c.b; break;
    case AC_OP_ZERO: cfg.zeroDeg = c.a; break;
    case AC_OP_SPAN: cfg.spanDeg = c.a; break;
    case AC_OP_MARK: if (aircore_points_set(&G[g].pts, c.a, c.b)) cfgSave(g); return;
    case AC_OP_MARK_HERE: if (aircore_points_set(&G[g].pts, c.a, G[g].needle)) cfgSave(g); return;
    case AC_OP_UNMARK: if (aircore_points_remove(&G[g].pts, c.a)) cfgSave(g); return;
    case AC_OP_CLEARMARKS: aircore_points_clear(&G[g].pts); cfgSave(g); return;
    case AC_OP_DEFAULTS: defaultsFor(G[g]); aircore_points_clear(&G[g].pts); cfgSave(g); return;
    default: return;
  }
  if (aircore_valid(&cfg)) { G[g].cfg = cfg; cfgSave(g); }
}
// The needle angle wanted now, from the gauge's mode.
static float wantedAngle(int g, uint32_t now, bool heard) {
  Gauge &G1 = G[g];
  float lo = G1.pts.n >= 2 ? G1.pts.v[0] : G1.cfg.minVal, hi = G1.pts.n >= 2 ? G1.pts.v[G1.pts.n - 1] : G1.cfg.maxVal;
  if (G1.mode == AC_MODE_ANGLE && G1.holdUntil && (int32_t)(now - G1.holdUntil) >= 0) setMode(g, AC_MODE_LIVE);   // a serial hold ran out
  switch (G1.mode) {
    case AC_MODE_ANGLE: return G1.holdDeg;
    case AC_MODE_SWEEP: {
      float ph = fmodf((now - G1.sweepT0) / 1000.0f, G1.sweepS) / G1.sweepS;
      G1.shown = lo + (ph < 0.5f ? ph * 2 : 2 - ph * 2) * (hi - lo);
      return aircore_angle_pts(&G1.cfg, &G1.pts, G1.shown);
    }
    case AC_MODE_TARGET: G1.shown = G1.target; return aircore_angle_pts(&G1.cfg, &G1.pts, G1.target);
    default: {
      float v; bool has;
      LOCK(); v = s_val[g]; has = s_has[g] && heard; UNLOCK();
      G1.shown = has ? v : lo;
      return aircore_angle_pts(&G1.cfg, &G1.pts, G1.shown);
    }
  }
}

static void show() {
  Serial.printf("status to the hub's page: %lu sent, %lu failed | hub packets %.1f a second on channel %d%s | %lu channel scans\n",
                (unsigned long)s_stSent, (unsigned long)s_stFail, s_rate, s_ch, s_scanCh ? " (scanning)" : "", (unsigned long)s_scans);
  Serial.printf("aircore build %d | %s decay | pwm %lu Hz | channel %d | packets %lu | %s\n", AIRCORE_BUILD, s_slow ? "slow" : "fast", (unsigned long)s_pwmHz, s_ch, (unsigned long)s_rx,
                millis() - s_lastRx < LOST_MS ? (s_sim ? "hub heard (SIMULATED data)" : "hub heard") : "no hub: hunting channels");
  for (int g = 0; g < GAUGES; g++) {
    const AirCoreCfg &c = G[g].cfg;
    char v[16]; if (s_has[g]) snprintf(v, sizeof v, "%.0f", (float)s_val[g]); else strcpy(v, "--");
    Serial.printf("  %-6s value %s | needle %.1f deg | min %g max %g zero %g span %g strength %g slew %g\n", G[g].name, v, G[g].needle,
                  c.minVal, c.maxVal, c.zeroDeg, c.spanDeg, c.strength, c.slewDegS);
    if (G[g].pts.n) {
      Serial.printf("         points (%s):", G[g].pts.n >= 2 ? "in use" : "need 2 or more");
      for (int i = 0; i < G[g].pts.n; i++) Serial.printf(" %g=%g", G[g].pts.v[i], G[g].pts.a[i]);
      Serial.println();
    }
  }
}
static void help() {
  Serial.println("Commands:");
  Serial.println("  show                          values, settings, channel");
#if defined(ESP8266)
  Serial.println("  role tach | role speedo       which gauge this board drives (factory settings for it)");
#endif
  Serial.println("  tach angle 30                 hold the tach needle at 30 deg for a minute (find zero and span)");
  Serial.println("  tach release                  back to live data");
  Serial.println("  tach zero 12 | span 245       coil angle at the start of the scale | degrees to the end");
  Serial.println("  tach min 0 | max 8000         values at the start and end of the dial");
  Serial.println("  tach strength 0.8 | slew 720  drive (0.1-1) | fastest needle speed, deg/s");
  Serial.println("  tach target 3000 | sweeping 4   show a fixed value | sweep end to end and back every 4 s (release: live)");
  Serial.println("  tach mark 2000                 the angle the needle is held at now (tach angle ...) becomes the 2000 mark");
  Serial.println("  tach point 2000 118           the 2000 mark at coil angle 118, typed in");
  Serial.println("  tach unpoint 2000 | clearpoints   remove one mark | all of them (back to zero / span)");
  Serial.println("  (same for speedo)   sweep     needle(s) end to end   defaults   factory settings");
  Serial.println("  pwm 2000                      coil PWM frequency, 100-40000 Hz (lower = steadier on the D1 Mini, may hum)");
  Serial.println("  decay slow | decay fast       how the coil current is driven between pulses (slow: stronger, steadier)");
}
static void command(String line) {
  line.trim(); line.toLowerCase();
  if (!line.length()) return;
  Serial.printf("> %s\n", line.c_str());   // echo: every typed command gets a line back
  if (line == "help") { help(); return; }
  if (line == "show") { show(); return; }
  if (line == "sweep") { sweep(); Serial.println("Sweep done"); return; }
  if (line == "defaults") { for (int g = 0; g < GAUGES; g++) { defaultsFor(G[g]); cfgSave(g); } Serial.println("Factory settings back"); return; }
  if (line == "decay slow" || line == "decay fast") {
    s_slow = line == "decay slow"; for (int g = 0; g < GAUGES; g++) cfgSave(g);
    Serial.printf("%s decay, saved\n", s_slow ? "slow" : "fast"); return;
  }
  if (line.startsWith("pwm ")) {
    long hz = line.substring(4).toInt();
    if (hz < 100 || hz > 40000) { Serial.println("100 to 40000 Hz"); return; }
    s_pwmHz = hz; pwmApply(); for (int g = 0; g < GAUGES; g++) cfgSave(g);
    Serial.printf("pwm %ld Hz, saved\n", hz); return;
  }
#if defined(ESP8266)
  if (line == "role tach" || line == "role speedo") {
    setRole(line == "role speedo"); defaultsFor(G[0]); cfgSave(0);
    Serial.printf("This board drives the %s (factory settings for it, saved)\n", G[0].name); return;
  }
#endif
  int sp1 = line.indexOf(' '), sp2 = line.indexOf(' ', sp1 + 1);
  String who = line.substring(0, sp1), what = sp1 < 0 ? "" : line.substring(sp1 + 1, sp2 < 0 ? line.length() : sp2);
  float v = sp2 < 0 ? NAN : line.substring(sp2 + 1).toFloat();
  int g = -1;
  for (int k = 0; k < GAUGES; k++) if (who == G[k].name) g = k;
  if (g < 0) { Serial.printf("? type help%s\n", GAUGES == 1 ? (String(" (this board drives the ") + G[0].name + ")").c_str() : ""); return; }
  if (what == "release") { setMode(g, AC_MODE_LIVE); Serial.println("Live data"); return; }
  if (what == "clearpoints") { aircore_points_clear(&G[g].pts); cfgSave(g); Serial.printf("%s: marks cleared (zero / span again), saved\n", G[g].name); return; }
  if (isnan(v)) { Serial.println("? a number is missing"); return; }
  if (what == "angle") { setMode(g, AC_MODE_ANGLE); G[g].holdDeg = v; G[g].holdUntil = millis() + HOLD_MS; Serial.printf("%s held at %.1f deg\n", G[g].name, v); return; }
  if (what == "target") { setMode(g, AC_MODE_TARGET); G[g].target = v; Serial.printf("%s showing %g (release: live)\n", G[g].name, v); return; }
  if (what == "sweeping") { setMode(g, AC_MODE_SWEEP); if (v >= 1 && v <= 60) G[g].sweepS = v; Serial.printf("%s sweeping, %g s end to end and back (release: live)\n", G[g].name, G[g].sweepS); return; }
  if (what == "mark" || what == "point" || what == "unpoint") {
    float val = v, ang = G[g].holdDeg;
    if (what == "point") {   // "tach point 2000 118": value and angle typed in
      String rest = line.substring(sp2 + 1); rest.trim(); int sp3 = rest.indexOf(' ');
      if (sp3 < 0) { Serial.println("? tach point <value> <angle>"); return; }
      val = rest.substring(0, sp3).toFloat(); ang = rest.substring(sp3 + 1).toFloat();
    } else if (what == "mark" && G[g].mode != AC_MODE_ANGLE) { Serial.println("Hold the needle on the mark first (tach angle ...), then mark it"); return; }
    if (what == "unpoint") { Serial.println(aircore_points_remove(&G[g].pts, val) ? "Mark removed, saved" : "No mark at that value"); cfgSave(g); return; }
    if (!aircore_points_set(&G[g].pts, val, ang)) { Serial.println("12 marks already: remove one first"); return; }
    cfgSave(g); Serial.printf("%s: %g at %.1f deg (%d marks), saved\n", G[g].name, val, ang, G[g].pts.n); return;
  }
  AirCoreCfg c = G[g].cfg;
  if (what == "zero") c.zeroDeg = v; else if (what == "span") c.spanDeg = v; else if (what == "min") c.minVal = v; else if (what == "max") c.maxVal = v;
  else if (what == "strength") c.strength = v; else if (what == "slew") c.slewDegS = v;
  else { Serial.println("? type help"); return; }
  if (!aircore_valid(&c)) { Serial.println("Out of range, not saved"); return; }
  G[g].cfg = c; cfgSave(g); Serial.printf("%s %s = %g, saved\n", G[g].name, what.c_str(), v);
}

void setup() {
  Serial.begin(115200);
  cfgLoad();
#if defined(ESP8266)
  analogWriteFreq(s_pwmHz); analogWriteRange(PWM_FULL);
  for (int k = 0; k < 4; k++) { pinMode(G[0].pins[k], OUTPUT); analogWrite(G[0].pins[k], 0); }
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  setChannel(s_ch);
  if (esp_now_init() == 0) {
    static uint8_t bc[6] = { 255, 255, 255, 255, 255, 255 };
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO); esp_now_register_recv_cb(onRecv);
    esp_now_add_peer(bc, ESP_NOW_ROLE_COMBO, 0, NULL, 0);   // status to the hub
  } else Serial.println("ESP-NOW start FAILED");
  WiFi.macAddress(s_myMac);
#else
  for (int g = 0; g < GAUGES; g++) for (int k = 0; k < 4; k++) ledcAttach(G[g].pins[k], s_pwmHz, 10);   // 8 pins: the S2's 8 PWM channels (the ESP32 has 16)
  WiFi.mode(WIFI_STA); WiFi.disconnect(false, false);
  setChannel(s_ch);
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(onRecv);
    esp_now_peer_info_t pi = {}; memset(pi.peer_addr, 0xFF, 6); pi.channel = 0; pi.encrypt = false;
    esp_now_add_peer(&pi);   // status to the hub (channel 0 = whatever channel it is on)
  } else Serial.println("ESP-NOW start FAILED");
  WiFi.macAddress(s_myMac);
#endif
  sweep();
  Serial.printf("\naircore build %d ready (%s). Type help.\n", AIRCORE_BUILD, GAUGES == 1 ? G[0].name : "tach + speedo");
}

void loop() {
  static uint32_t last = 0; static String line;
  while (Serial.available()) { char ch = Serial.read(); if (ch == '\n' || ch == '\r') { command(line); line = ""; } else if (line.length() < 60) line += ch; }
  uint32_t now = millis();
  bool heard = s_lastRx && now - s_lastRx < LOST_MS;
  static uint32_t lastSt = 0; static uint8_t stG = 0; bool stNow = false;
  if (s_cmdNew) { AcCmd c; LOCK(); c = s_cmdRx; s_cmdNew = false; UNLOCK(); applyCmd(c); stNow = true; stG = c.gauge < GAUGES ? c.gauge : 0; }
  if (heard && (stNow || now - lastSt >= 500 / GAUGES)) { lastSt = now; sendStatus(stG); stG = (stG + 1) % GAUGES; }   // half a second per gauge
  bool lost = !s_lastRx || now - s_lastRx >= HUNT_MS;
  if (s_scanCh) {   // scanning: count the hub's packets on each channel, keep the best
    if (now - s_hopMs >= HOP_MS) {
      uint32_t n = s_rx - s_scanRx0;
      if (n > s_bestN) { s_bestN = n; s_bestCh = s_scanCh; }
      if (s_scanCh >= 13) {
        s_ch = s_bestN ? s_bestCh : 1; setChannel(s_ch); s_scanCh = 0;
        s_rateT0 = now; s_rateRx0 = s_rx;
        Serial.printf("Channel scan: the hub is loudest on channel %d (%lu packets in %d ms)\n", s_ch, (unsigned long)s_bestN, HOP_MS);
      } else { s_scanCh++; setChannel(s_scanCh); s_hopMs = now; s_scanRx0 = s_rx; }
    }
  } else {
    if (!s_rateT0) { s_rateT0 = now; s_rateRx0 = s_rx; }
    if (now - s_rateT0 >= RATE_MS) {   // how many of the hub's packets arrive here
      s_rate = (s_rx - s_rateRx0) * 1000.0f / (now - s_rateT0);
      s_rateT0 = now; s_rateRx0 = s_rx;
      if (s_rate < RATE_MIN) lost = true;   // a trickle: most likely the wrong channel
    }
    if (lost && (s_scans == 0 || now - s_hopMs >= HUNT_MS)) {   // start a scan: at once after start-up, then at most every HUNT_MS
      s_scans++; s_scanCh = 1; s_bestN = 0; s_bestCh = s_ch; setChannel(1); s_hopMs = now; s_scanRx0 = s_rx;
    }
  }
  if (now - last < STEP_MS) return;
  float dt = (now - last) / 1000.0f; last = now;
  for (int g = 0; g < GAUGES; g++) {
    float target = wantedAngle(g, now, heard);
    G[g].needle = aircore_slew(G[g].needle, target, G[g].cfg.slewDegS, dt);
    drive(g, G[g].needle);
  }
}
