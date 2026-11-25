// esp32-input - MSP-DASH input board.
// Reads the buttons (Buttons page) and a knob (brightness) and sends them to every screen over ESP-NOW.
// Also on the home network for WiFi updates: same curl command as the screens, at its own IP.
//
// Bench board: ESP32-D (classic ESP32, fqbn esp32:esp32:esp32:FlashFreq=40,FlashMode=dio: at the default 80 MHz this board
// boot-looped reading garbage from flash). Later: the S2 Mini (change the pins below).
// Car inputs (see the web page at this board's IP for settings): lights-on wire -> 47k -> GPIO 35, and 10k from GPIO 35 to GND;
//   dimmer wire the same into GPIO 32 (add 10 uF from GPIO 32 to GND if the dimmer is PWM).
// Wiring: buttons to GND on any free pin (internal pull-ups), or several on one analog pin through resistors (Buttons page),
//         10k knob: outer legs to 3.3 V and GND, middle leg to POT_PIN (ADC1 only: ADC2 doesn't work with the radio on).
// Copy wifi_config.h.example to wifi_config.h and fill in the network, the same one the screens use.

#include "hub_pins.h"         // every GPIO the hub uses: roles, defaults, what each GPIO can do (docs/HUB_PINS.md)
#include "hub_inputs.h"       // configurable inputs: switches, analog sensors (Inputs page)
#include "settings_backup.h"   // export / restore / factory reset (Backup page)
#include "cpu_load.h"         // busy % per core (Board status page)
#include "hub_wifi.h"         // the hub's own Wi-Fi network for the car (Setup page)
#include "shift_light.h"      // staged shift light across the screens (Setup page)
#include "hub_buttons.h"      // every button: digital or on a resistor ladder, one action each (Buttons page)
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "wifi_config.h"
#include <Preferences.h>
#include "driver/twai.h"
#include "esp_heap_caps.h"
#include "gauge_config.h"   // copied in at compile time (see PREREQUISITES.md)
#include "gauge_app.h"
#include "link_packets.h"   // copied in at compile time: same packets as the screens
#include "web_monitor.h"   // raw CAN tracker, used by the /can page
#include "gps_time.h"       // copied in: GPS date/time, the MST1 packet
#include "speed_sensors.h" // the GPS reader (host-tested)
#include "custom_slots.h"    // copied in: the 16 custom sensor slots (the /custom page)
#include "color_presets.h"   // copied in: the 10 colour schemes
#include "button_actions.h"  // copied in: the actions the buttons can run (/buttons page)
#include "btn_logic.h"       // screen order for "Next gauge"; the old button table (read once, host-tested)
// Runtime serial messages are off unless SERIAL_STREAM is 1; boot messages always print (same macro as can_parsing.h).
#ifndef RT_LOG
#if defined(SERIAL_STREAM) && SERIAL_STREAM
#define RT_LOG(...) Serial.printf(__VA_ARGS__)
#else
#define RT_LOG(...) do { } while (0)
#endif
#endif
#include "rf_cc1101.h"      // CC1101 433 MHz listen mode (tire sensors), the /rf page
#include "oled_panel.h"      // warning-light panel on the SSD1306 OLED (host-tested)
#include "oled_pages.h"      // its pages: lamps, big number, values, G-meter, clock (host-tested)
#include "oled_ssd1306.h"    // its I2C driver (GPIO 5 / 15)
#include "imu_math.h"         // accelerometer: mounting axes, zero, smoothing (host-tested)
#include "imu_mpu6050.h"      // MPU6050 on the same I2C wires
#include "mcp23017.h"         // MCP23017 16-pin expanders on the same wires (inputs, outputs)
#include "ads1115.h"          // ADS1115 analog channels on the same wires (pins 140..155)
#include "hub_pages.h"     // the web pages (kept out of the .ino on purpose, see the file)
#include "simulator.h"     // copied in at compile time with simulator.cpp: the bench simulator runs here (build 15 on)

#define INPUT_BUILD 69
#define BOARD_NAME "Input"

// ---- pins (ESP32-D bench) ----
#define DIVIDER (57.0f / 10.0f)

// ---- network: this board's fixed address ----
static const IPAddress MY_IP(192, 168, 55, 204), MY_GW(192, 168, 55, 1), MY_MASK(255, 255, 255, 0);
#define CAR_CHANNEL 1   // the screens park here when the home network isn't there; must match ESPNOW_CAR_CHANNEL


static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint16_t s_seq = 0;
static volatile uint32_t s_dataSent = 0;   // data packets sent to the screens

// ---- the hub: CAN values (written by the decoder from can_parsing.h) and the data packet the screens read ----
volatile float g_rpm, g_clt_f, g_boost_psi, g_afr, g_oilp_psi, g_oilt_f, g_fuelp_psi, g_flex_pct, g_tps, g_battv, g_egt_f, g_iat_f, g_ambt_f, g_hum_pct, g_fuel_pct, g_vss_mph, g_gps_mph, g_gps_sats;
volatile bool g_rpm_has_data, g_clt_has_data, g_boost_has_data, g_afr_has_data, g_oilp_has_data, g_oilt_has_data,
              g_fuelp_has_data, g_flex_has_data, g_tps_has_data, g_battv_has_data, g_egt_has_data, g_iat_has_data, g_ambt_has_data, g_hum_has_data, g_fuel_has_data, g_vss_has_data, g_gps_has_data, g_sats_has_data;
volatile uint16_t g_flag_bits, g_flag_seen;
static volatile uint32_t s_seenMs[SIG_COUNT];
#define STALE_MS 2000   // same as the screens: a signal not heard for 2 s shows "--"

// Where each signal last came from, for the /sensors page: a CAN ID, or one of the hub's own sensors.
#define SRC_HUB_EGT  0xFFFF0001u   // MAX6675
#define SRC_HUB_DHT  0xFFFF0002u   // DHT22
#define SRC_HUB_FUEL 0xFFFF0003u   // fuel pot / sender
#define SRC_HUB_SIM  0xFFFF0004u   // the bench simulator
#define SRC_HUB_GPS  0xFFFF0006u   // GPS
#define SRC_HUB_IMU  0xFFFF0007u   // MPU6050 accelerometer
#define SRC_HUB_INPUT 0xFFFF0008u  // a configurable input (Inputs page)
static volatile uint32_t s_srcId[SIG_COUNT];
// Called by the CAN decoder (can_parsing.h, can_flags.h) from the CAN task: the source is the frame being decoded.
// The simulator calls it too (from loop()); its values are tagged as coming from the simulator.
extern "C" void gauge_config_mark_signal_active(SignalId sig) { if (sig > 0 && sig < SIG_COUNT) { s_seenMs[sig] = millis() | 1; s_srcId[sig] = sim_is_active() ? SRC_HUB_SIM : s_canCurId; } }
// Called by the hub's own sensor tasks. They do not write their values while the simulator runs (see simOwns).
static void markLocal(SignalId sig, uint32_t src) { if (sig > 0 && sig < SIG_COUNT) { s_seenMs[sig] = millis() | 1; s_srcId[sig] = src; } }
// True while the simulator feeds the values: the hub's own sensors then leave the shared values alone, so a real
// reading never flickers in between fake ones (the raw readings still show on the home page).
static inline bool simOwns(void) { return sim_is_active(); }
extern "C" bool gauge_config_signal_has_data(SignalId sig) {
  return sig > 0 && sig < SIG_COUNT && s_seenMs[sig] && millis() - s_seenMs[sig] < STALE_MS;
}
// ---- custom sensor slots (custom_slots.h): set up on /custom, saved in NVS, names sent to the screens ----
volatile float g_imu_val[4];   // SIG_GLAT .. SIG_YAW (imu_math.h)
volatile bool g_imu_has[4];
// ---- the pin map (hub_pins.h): defaults, or the moves saved on the Inputs page (NVS "pins"), applied at start-up ----
uint8_t g_pins[PR_COUNT];
static bool s_pinsMoved = false;
static uint8_t s_oldBtnNext = PIN_BTN_NEXT_DEF, s_oldBtnPrev = PIN_BTN_PREV_DEF;   // page buttons from a map saved before hub 53
static void pinsLoad() {
  pin_map_defaults(g_pins);
  uint8_t m[PR_COUNT];
  Preferences p; p.begin("pins", false);
  size_t len = p.getBytesLength("map");
  bool got = false;
  if (len == PIN_MAP_OLD_COUNT) {   // saved before hub 53: the two page buttons were in it. Take them out (Buttons page).
    uint8_t old[PIN_MAP_OLD_COUNT];
    if (p.getBytes("map", old, sizeof old) == sizeof old) {
      pin_map_upgrade(old, m, &s_oldBtnNext, &s_oldBtnPrev);
      got = true;
      if (pin_map_valid(m)) p.putBytes("map", m, sizeof m); else p.remove("map");
      Serial.printf("Pin map upgraded: page buttons on %d / %d go to the Buttons page\n", s_oldBtnPrev, s_oldBtnNext);
    }
  } else if (len == sizeof m) got = p.getBytes("map", m, sizeof m) == sizeof m;
  p.end();
  if (got && pin_map_valid(m)) memcpy(g_pins, m, sizeof m);   // a saved map that does not check out: defaults
  for (int r = 0; r < PR_COUNT; r++) if (g_pins[r] != HUB_PIN_ROLES[r].def) s_pinsMoved = true;
}
volatile float g_custom_val[CUSTOM_SLOTS];
volatile bool g_custom_has[CUSTOM_SLOTS];
static CustomSlotConfig s_slots[CUSTOM_SLOTS];
static volatile bool s_slotsChanged = true;   // send the names now (also every SLOTS_MS)
static CustomSlotInfo s_noSlot;   // returned for a slot number out of range
// extern "C", several lines and no "static" inside the body on purpose: the Arduino builder's prototype scan read
// the one-line version as a static function ("declared 'extern' and later 'static'", hub build 22).
extern "C" const CustomSlotInfo *custom_slot_info(int slot) {
  if (slot < 0 || slot >= CUSTOM_SLOTS) return &s_noSlot;
  return &s_slots[slot].info;
}
static void slotsLoad() {
  Preferences p; p.begin("slots", true);
  if (p.getBytes("cfg", s_slots, sizeof s_slots) != sizeof s_slots) memset(s_slots, 0, sizeof s_slots);
  p.end();
  for (int i = 0; i < CUSTOM_SLOTS; i++) { s_slots[i].info.name[CUSTOM_NAME_LEN - 1] = 0; s_slots[i].info.unit[CUSTOM_UNIT_LEN - 1] = 0; }
}
static void slotsSave() { Preferences p; p.begin("slots", false); p.putBytes("cfg", s_slots, sizeof s_slots); p.end(); s_slotsChanged = true; }
// Every received CAN frame (from the tracker): decode every slot that reads this ID.
void hub_custom_can_frame(uint32_t id, const uint8_t *d, uint8_t len) {
  for (int i = 0; i < CUSTOM_SLOTS; i++) {
    const CustomSlotConfig &c = s_slots[i];
    if (c.src != CUSTOM_SRC_CAN || c.can.id != id || !c.info.name[0]) continue;   // (input-fed slots are skipped here)
    float v;
    if (!custom_can_decode(&c.can, d, len, &v)) continue;
    g_custom_val[i] = v; g_custom_has[i] = true;
    s_seenMs[SIG_C1 + i] = millis() | 1; s_srcId[SIG_C1 + i] = id;
  }
}
static void slotsSendNames() {
  for (int first = 0; first < CUSTOM_SLOTS; first += SLOTS_PER_PACKET) {
    SlotsPacket p; memset(&p, 0, sizeof p);
    p.magic = SLOTS_MAGIC; p.first = first; p.n = SLOTS_PER_PACKET;
    for (int k = 0; k < SLOTS_PER_PACKET; k++) p.info[k] = s_slots[first + k].info;
    esp_now_send(BCAST, (const uint8_t *)&p, sizeof p);
    delay(4);
  }
}

extern "C" float gauge_config_signal_value(SignalId sig) {
  if (gauge_signal_is_custom(sig)) return g_custom_val[sig - SIG_C1];
  switch (sig) {
    case SIG_RPM: return g_rpm;       case SIG_CLT: return g_clt_f;     case SIG_MAP: return g_boost_psi;
    case SIG_AFR: return g_afr;       case SIG_OILP: return g_oilp_psi; case SIG_OILT: return g_oilt_f;
    case SIG_FUELP: return g_fuelp_psi; case SIG_FLEX: return g_flex_pct; case SIG_TPS: return g_tps;
    case SIG_BATTV: return g_battv;   case SIG_EGT: return g_egt_f;     case SIG_IAT: return g_iat_f;
    case SIG_AMBT: return g_ambt_f;  case SIG_HUM: return g_hum_pct;
    case SIG_FUEL: return g_fuel_pct;  case SIG_VSS: return g_vss_mph;
    case SIG_GPS_SPD: return g_gps_mph; case SIG_GPS_SATS: return g_gps_sats;
    case SIG_GLAT: case SIG_GLONG: case SIG_GVERT: case SIG_YAW: return g_imu_val[sig - SIG_GLAT];
    default: return gauge_signal_is_flag(sig) ? (((g_flag_bits >> (sig - SIG_FIRST_FLAG)) & 1u) ? 1.0f : 0.0f) : 0.0f;
  }
}

// ---- screen status: every remote screen sends a hello every 2 s; no hello for 6 s = offline (red) ----
#define SCREEN_MAX 6
#define SCREEN_OFFLINE_MS 6000
typedef struct { uint8_t mac[6]; uint32_t ip; uint16_t build; char name[LINK_NAME_LEN + 1]; uint32_t lastMs; uint32_t hellos; uint32_t uptime; HelloStats st; bool used; } ScreenSlot;
static ScreenSlot s_screens[SCREEN_MAX];
static portMUX_TYPE s_scrMux = portMUX_INITIALIZER_UNLOCKED;
// ---- the stock tach / speedo board (aircore/): its status, for the Stock gauges page ----
#define AC_SLOTS 4
typedef struct { bool used; uint8_t mac[6]; uint8_t gauge; uint32_t lastMs; AcStatus st; } AcSlot;
static AcSlot s_ac[AC_SLOTS];
static portMUX_TYPE s_acMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t s_acRx = 0, s_acShort = 0;   // gauge-board status packets heard; too short to use
static void acStatusIn(const uint8_t *mac, const uint8_t *data, int len) {
  AcStatus tmp; memset(&tmp, 0, sizeof tmp); memcpy(&tmp, data, len < (int)sizeof tmp ? len : sizeof tmp);   // older boards send less
  const AcStatus *st = &tmp; s_acRx++;
  portENTER_CRITICAL(&s_acMux);
  int slot = -1, oldest = 0;
  for (int i = 0; i < AC_SLOTS; i++) {
    if (s_ac[i].used && s_ac[i].gauge == st->gauge && !memcmp(s_ac[i].mac, mac, 6)) { slot = i; break; }
    if (slot < 0 && !s_ac[i].used) slot = i;
  }
  if (slot < 0) { for (int i = 1; i < AC_SLOTS; i++) if (s_ac[i].lastMs < s_ac[oldest].lastMs) oldest = i; slot = oldest; }
  s_ac[slot].used = true; memcpy(s_ac[slot].mac, mac, 6); s_ac[slot].gauge = st->gauge;
  memcpy(&s_ac[slot].st, st, sizeof(AcStatus)); s_ac[slot].lastMs = millis();
  portEXIT_CRITICAL(&s_acMux);
}

static void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len >= 4 && ((const AcStatus *)data)->magic == AC_STATUS_MAGIC) {   // the gauge board (aircore 7 sends 4 bytes less than 8)
    if (len >= (int)offsetof(AcStatus, upS)) acStatusIn(info->src_addr, data, len); else s_acShort++;
    return;
  }
  if (len < (int)offsetof(HelloPacket, name) || ((const HelloPacket *)data)->magic != HELLO_MAGIC) return;
  HelloPacket h; memset(&h, 0, sizeof h); memcpy(&h, data, len < (int)sizeof h ? len : sizeof h);
  portENTER_CRITICAL(&s_scrMux);
  int slot = -1, oldest = 0;
  for (int i = 0; i < SCREEN_MAX; i++) {
    if (s_screens[i].used && !memcmp(s_screens[i].mac, info->src_addr, 6)) { slot = i; break; }
    if (slot < 0 && !s_screens[i].used) slot = i;
  }
  if (slot < 0) { for (int i = 1; i < SCREEN_MAX; i++) if (s_screens[i].lastMs < s_screens[oldest].lastMs) oldest = i; slot = oldest; }
  ScreenSlot *c = &s_screens[slot];
  if (!c->used || memcmp(c->mac, info->src_addr, 6)) { memset(c, 0, sizeof *c); memcpy(c->mac, info->src_addr, 6); }
  c->ip = h.ip; c->build = h.build; memcpy(c->name, h.name, LINK_NAME_LEN); c->name[LINK_NAME_LEN] = 0;
  c->uptime = h.uptime_s;   // 0 = an older screen that does not send it
  c->st = h.st;             // stats_ver 0 = an older screen without stats
  c->lastMs = millis(); c->hellos++; c->used = true;
  portEXIT_CRITICAL(&s_scrMux);
}

static WebServer server(80);
// ---- the hub's own numbers for the Board status page, worked out once a second ----
static uint32_t s_loopN = 0;
static struct { uint32_t loops, canFps, sentPs; uint32_t canTotal, sent, loopsAt, ms; } s_rate = {};
static void rateStep() {
  s_loopN++;
  uint32_t now = millis();
  if (now - s_rate.ms < 1000) return;
  uint32_t dt = now - s_rate.ms; s_rate.ms = now;
  uint32_t ct = s_canTotal, sn = s_dataSent;
  s_rate.loops = (s_loopN - s_rate.loopsAt) * 1000 / dt; s_rate.loopsAt = s_loopN;
  s_rate.canFps = (ct - s_rate.canTotal) * 1000 / dt; s_rate.canTotal = ct;
  s_rate.sentPs = (sn - s_rate.sent) * 1000 / dt; s_rate.sent = sn;
}

// ---- settings backup and factory reset (settings_backup.h) ----
static const char *const HUB_BK_NS[] = { "ap", "bri", "btncfg", "buttons", "imu", "inputs", "oled", "opages", "outputs", "pins", "rf", "shift", "slots", "time" };
static const char *const HUB_BK_SECRET[] = { "ap" };   // its Wi-Fi password: only with "include passwords"
static void hubFactoryReset() { bk_factory_wipe(); Serial.println("Factory reset done, restarting"); delay(200); ESP.restart(); }

// ---- the hub's own Wi-Fi network (hub_wifi.h, Setup page) ----
static ApCfg s_ap;
static bool s_apOn = false;
static void apLoad() {
  Preferences p; p.begin("ap", true);
  bool got = p.getBytes("cfg", &s_ap, sizeof s_ap) == sizeof s_ap;
  p.end();
  if (!got || !ap_valid(&s_ap)) ap_defaults(&s_ap);
}
static void apSave() { Preferences p; p.begin("ap", false); p.putBytes("cfg", &s_ap, sizeof s_ap); p.end(); }
static char s_last[48] = "none yet";
static int s_briPct = -1;
static bool s_wifiUp = false;

// Each command goes out 3 times (broadcast has no acknowledgement); the screens act on the first copy only.
static void sendCmd(uint8_t cmd, int16_t value, const char *target) {
  LinkCmdPacket p = {};
  p.magic = LINK_CMD_MAGIC; p.seq = ++s_seq; p.cmd = cmd; p.value = value;
  strncpy(p.target, target, sizeof(p.target) - 1);
  for (int i = 0; i < 3; i++) { esp_now_send(BCAST, (const uint8_t *)&p, sizeof(p)); delay(4); }
  const char *what = cmd == LINK_CMD_PAGE_NEXT ? "next page" : cmd == LINK_CMD_PAGE_PREV ? "previous page" : cmd == LINK_CMD_SELECT ? "buttons control"
                   : cmd == LINK_CMD_ALERT_ACK ? "clear warning" : cmd == LINK_CMD_LOG_MARK ? "log marker" : cmd == LINK_CMD_GOTO_PAGE ? "go to page"
                   : cmd == LINK_CMD_COLOR ? "color scheme" : cmd == LINK_CMD_RACE ? "race mode" : cmd == LINK_CMD_HINT ? "hint"
                   : cmd == LINK_CMD_SHOW_IP ? "show IP" : cmd == LINK_CMD_POPUP ? "pop-up" : "brightness";
  if (cmd == LINK_CMD_SELECT) snprintf(s_last, sizeof(s_last), "%s %s", what, target[0] ? target : "all screens");
  else if (cmd == LINK_CMD_ALERT_ACK) snprintf(s_last, sizeof(s_last), "%s", what);
  else snprintf(s_last, sizeof(s_last), "%s %d%s%s", what, value, target[0] ? " to " : "", target);
  RT_LOG("sent: %s\n", s_last);   // runtime: off unless SERIAL_STREAM (can_parsing.h defines RT_LOG)
}


// ---- shift light (shift_light.h, Setup page): the screens light up left to right with RPM ----
static ShiftCfg s_shift;
static int s_shiftLevel = 0;
static char s_shiftOrder[LINK_NAME_LEN + 1] = "";   // online screens, left to right: "Left,Center,Right"
static uint32_t s_shiftSentMs = 0, s_shiftTestMs = 0;
#define SHIFT_TEST_MS 4000
static void shiftLoad() {
  Preferences p; p.begin("shift", true);
  bool got = p.getBytes("cfg", &s_shift, sizeof s_shift) == sizeof s_shift;
  p.end();
  if (!got || !shift_valid(&s_shift)) shift_defaults(&s_shift);
}
static void shiftSave() { Preferences p; p.begin("shift", false); p.putBytes("cfg", &s_shift, sizeof s_shift); p.end(); }
// The online screens in order (Left, Center, Right by name), as one comma list. Returns how many.
static int shiftOrder(char *out) {
  char names[SCREEN_MAX][LINK_NAME_LEN + 1]; int n = 0;
  portENTER_CRITICAL(&s_scrMux);
  for (int i = 0; i < SCREEN_MAX; i++)
    if (s_screens[i].used && millis() - s_screens[i].lastMs < SCREEN_OFFLINE_MS && s_screens[i].name[0]) strcpy(names[n++], s_screens[i].name);
  portEXIT_CRITICAL(&s_scrMux);
  for (int i = 1; i < n; i++) for (int k = i; k > 0 && btn_name_cmp(names[k - 1], names[k]) > 0; k--) { char t[LINK_NAME_LEN + 1]; strcpy(t, names[k]); strcpy(names[k], names[k - 1]); strcpy(names[k - 1], t); }
  out[0] = 0; int used = 0;
  for (int i = 0; i < n; i++) {
    size_t need = strlen(out) + strlen(names[i]) + (used ? 1 : 0);
    if (need > LINK_NAME_LEN) break;   // the list must fit the packet's 20 characters
    if (used++) strcat(out, ",");
    strcat(out, names[i]);
  }
  return used;
}
static void sendShift(int level, int copies) {
  LinkCmdPacket p = {};
  p.magic = LINK_CMD_MAGIC; p.cmd = LINK_CMD_SHIFT; p.value = level;
  strncpy(p.target, s_shiftOrder, sizeof(p.target) - 1);
  for (int i = 0; i < copies; i++) { p.seq = ++s_seq; esp_now_send(BCAST, (const uint8_t *)&p, sizeof(p)); }
  s_shiftSentMs = millis();
}
// Every 20 ms: the level from RPM (or the test sweep); sent at once when it changes, repeated while lit.
static void shiftStep() {
  uint32_t now = millis();
  char order[LINK_NAME_LEN + 1]; int n = shiftOrder(order);
  float rpm = g_rpm;
  if (s_shiftTestMs) {   // test: a sweep from below the start to past the top
    uint32_t t = now - s_shiftTestMs;
    if (t > SHIFT_TEST_MS) s_shiftTestMs = 0;
    float top = s_shift.over ? s_shift.over + 200 : s_shift.shift + 300, lo = s_shift.start - 300;
    rpm = lo + (top - lo) * (t < SHIFT_TEST_MS / 2 ? t : SHIFT_TEST_MS - t) / (SHIFT_TEST_MS / 2.0f);
  }
  ShiftCfg c = s_shift; if (s_shiftTestMs) c.on = 1;
  int lvl = shift_level(&c, n, rpm, s_shiftLevel);
  bool changed = lvl != s_shiftLevel || strcmp(order, s_shiftOrder);
  s_shiftLevel = lvl; strcpy(s_shiftOrder, order);
  if (changed) sendShift(lvl, 2);
  else if (lvl > 0 && now - s_shiftSentMs >= SHIFT_KEEPALIVE_MS) sendShift(lvl, 1);
}

// ---- brightness settings (saved; edited on this board's web page) ----
// Mode auto: lights off = day %. Lights on = night min % at the dimmer's lowest, rising to night max % at its highest.
// Mode knob: the potentiometer sets it directly (bench / manual).
struct BriCfg { int mode; int dayPct; int nightMin; int nightMax; float dimLoV; float dimHiV; float lightsOnV; };
static BriCfg s_cfg = { 0, 100, 20, 60, 2.0f, 12.0f, 6.0f };   // mode 0 = auto, 1 = knob
static void cfgLoad() {
  Preferences p; p.begin("bri", true);
  s_cfg.mode = p.getInt("mode", 0); s_cfg.dayPct = p.getInt("day", 100); s_cfg.nightMin = p.getInt("nmin", 20);
  s_cfg.nightMax = p.getInt("nmax", 60); s_cfg.dimLoV = p.getFloat("dlo", 2.0f); s_cfg.dimHiV = p.getFloat("dhi", 12.0f);
  s_cfg.lightsOnV = p.getFloat("lon", 6.0f);
  p.end();
}
static void cfgSave() {
  Preferences p; p.begin("bri", false);
  p.putInt("mode", s_cfg.mode); p.putInt("day", s_cfg.dayPct); p.putInt("nmin", s_cfg.nightMin); p.putInt("nmax", s_cfg.nightMax);
  p.putFloat("dlo", s_cfg.dimLoV); p.putFloat("dhi", s_cfg.dimHiV); p.putFloat("lon", s_cfg.lightsOnV);
  p.end();
}

// ---- readings: smoothed volts at the car side of each divider, and the knob ----
static float s_lightsV = 0, s_dimV = 0, s_potAvg = -1;
static uint32_t s_lastBriMs = 0;
// The ADC is shared by two tasks (fuel, and the main loop: knob, lights, dimmer). Two reads at the same moment fail
// ("adc oneshot read fail") and return 0, so every analog read goes through this lock.
static SemaphoreHandle_t s_adcLock = NULL;
static int adcRaw(int pin) {
  if (pin_is_ads(pin)) return (int)(s_adsV[pin - PIN_ADS_BASE] / 3.3f * 4095.0f + 0.5f);   // ADS1115 channel, on the same 0-4095 scale
  if (s_adcLock) xSemaphoreTake(s_adcLock, portMAX_DELAY);
  int v = analogRead(pin);
  if (s_adcLock) xSemaphoreGive(s_adcLock);
  return v;
}
static uint32_t adcMilliVolts(int pin) {
  if (pin_is_ads(pin)) { float v = s_adsV[pin - PIN_ADS_BASE]; return v > 0 ? (uint32_t)(v * 1000.0f + 0.5f) : 0; }   // ADS1115 channel
  if (s_adcLock) xSemaphoreTake(s_adcLock, portMAX_DELAY);
  uint32_t v = analogReadMilliVolts(pin);
  if (s_adcLock) xSemaphoreGive(s_adcLock);
  return v;
}
static float carVolts(int pin) { return adcMilliVolts(pin) / 1000.0f * DIVIDER; }
static int clampPct(int v) { return v < 5 ? 5 : (v > 100 ? 100 : v); }

static int wantedPct() {
  if (s_cfg.mode == 1) return clampPct(5 + (int)lroundf(s_potAvg * 95.0f / 4095.0f));
  if (s_lightsV < s_cfg.lightsOnV) return clampPct(s_cfg.dayPct);
  float span = s_cfg.dimHiV - s_cfg.dimLoV;
  float f = span > 0.1f ? (s_dimV - s_cfg.dimLoV) / span : 0.0f;
  f = f < 0 ? 0 : (f > 1 ? 1 : f);
  return clampPct(s_cfg.nightMin + (int)lroundf(f * (s_cfg.nightMax - s_cfg.nightMin)));
}

// Brightness from the buttons (Dim all / Brighten all): an offset on top of the automatic or knob level. It drops
// back to 0 when the base level moves by 5 % or more (lights switched, dimmer or knob turned). Race mode: 100 %.
static int s_briOffset = 0, s_briOffsetBase = 0;
static bool s_raceOn = false;
static int finalPct() {
  int base = wantedPct();
  if (s_raceOn) return 100;
  if (s_briOffset && abs(base - s_briOffsetBase) >= 5) s_briOffset = 0;
  return clampPct(base + s_briOffset);
}

#include "can_parsing.h"   // copied in at compile time; uses the values and functions above

// ---- EGT: MAX6675 + K-type thermocouple, read by hand (bit-banged SPI) ----
// Wiring: SCK GPIO18, SO GPIO19, CS GPIO23, VCC 3V3. The chip needs ~220 ms per reading, so read every 250 ms.
// Each good reading goes on the CAN bus as ID 0x7F0 (CAN_ID_EGT_PLACEHOLDER in can_parsing.h) for rusEFI:
//   bytes 0-1 = EGT deg C, unsigned little endian; byte 2 = 1 if the thermocouple is open (not connected).
#define EGT_PERIOD_MS 250
#define EGT_HOLD_MS 1000   // send EGT only if the ECU was heard in the last second (an unanswered frame pushes the hub to bus-off)
static volatile float s_egtC = 0; static volatile bool s_egtOpen = true; static volatile uint32_t s_egtSent = 0; static volatile bool s_egtHeld = true;
static uint16_t max6675Read() {
  uint16_t v = 0;
  digitalWrite(EGT_CS, LOW); delayMicroseconds(2);
  for (int i = 15; i >= 0; i--) {
    digitalWrite(EGT_SCK, HIGH); delayMicroseconds(1);
    if (digitalRead(EGT_SO)) v |= 1u << i;
    digitalWrite(EGT_SCK, LOW); delayMicroseconds(1);
  }
  digitalWrite(EGT_CS, HIGH);
  return v;
}
static void egtTask(void *) {
  pinMode(EGT_CS, OUTPUT); digitalWrite(EGT_CS, HIGH);
  pinMode(EGT_SCK, OUTPUT); digitalWrite(EGT_SCK, LOW);
  pinMode(EGT_SO, INPUT);
  vTaskDelay(pdMS_TO_TICKS(500));                   // first conversion after power-up
  for (;;) {
    uint16_t raw = max6675Read();
    bool open = (raw & 0x0004) || raw == 0xFFFF;    // bit 2 = no thermocouple; all ones = chip missing
    float c = (raw >> 3) * 0.25f;
    s_egtOpen = open;
    if (!open) s_egtC = c;
    if (!open && !simOwns()) {
      g_egt_f = c * 9.0f / 5.0f + 32.0f;            // the hub does not hear its own frame, so set the value here too
      g_egt_has_data = true;
      markLocal(SIG_EGT, SRC_HUB_EGT);
    }
    vTaskDelay(pdMS_TO_TICKS(EGT_PERIOD_MS));
  }
}

// ---- GPS: GT-U7 (NEO-6M compatible) on Serial2: module TX -> GPIO 16, module RX <- GPIO 17, VCC 5 V ----
static GpsParser s_gps;
static volatile uint32_t s_gpsUtcMs = 0;   // millis() when s_gps.utc was last set
// Time zone for the screens' log names and timestamps (saved): standard offset in minutes, US daylight saving on/off.
static int s_tzStd = -300; static bool s_tzDst = true;
static void tzLoad() { Preferences p; p.begin("time", true); s_tzStd = p.getInt("std", -300); s_tzDst = p.getUChar("dst", 1); p.end(); }
static void tzSave() { Preferences p; p.begin("time", false); p.putInt("std", s_tzStd); p.putUChar("dst", s_tzDst); p.end(); }
static uint32_t gpsUtcNow() {   // 0 = the GPS has not given the time in the last minute
  uint32_t u = s_gps.utc, at = s_gpsUtcMs;
  if (!u || !at || millis() - at > 60000) return 0;
  return u + (millis() - at) / 1000;
}
static int tzOffsetNow(uint32_t utc) { return s_tzStd + ((s_tzDst && gt_us_dst(utc, s_tzStd)) ? 60 : 0); }
static volatile uint32_t s_gpsBytes = 0, s_gpsLastByteMs = 0;
static void gpsTask(void *) {
  gps_init(&s_gps);
  Serial2.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  vTaskDelay(pdMS_TO_TICKS(1000));
  Serial2.write(GPS_CFG_RATE_5HZ, sizeof GPS_CFG_RATE_5HZ);      // 5 fixes a second
  for (;;) {
    while (Serial2.available()) {
      char c = (char)Serial2.read(); s_gpsBytes++; s_gpsLastByteMs = millis();
      if (!gps_feed(&s_gps, c) || simOwns()) continue;
      if (s_gps.newRmc && s_gps.utc) s_gpsUtcMs = millis();
      if (s_gps.newRmc) { s_gps.newRmc = false; if (s_gps.fix) { g_gps_mph = s_gps.mph; g_gps_has_data = true; markLocal(SIG_GPS_SPD, SRC_HUB_GPS); } }
      if (s_gps.newGga) { s_gps.newGga = false; if (s_gps.fix) { g_gps_sats = s_gps.sats; g_sats_has_data = true; markLocal(SIG_GPS_SATS, SRC_HUB_GPS); } }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ---- CAN frames to rusEFI, 4 times a second: 0x7F0 EGT, and the ECU map frame ----
//   bytes 0-1 EGT deg C (LE), byte 2 = 1 if the thermocouple is open. (Speed: the hall sensor is wired to rusEFI.)
// Sent only while the ECU was heard in the last second (an unanswered frame pushes the hub to bus-off).
static EcuMapCfg s_ecuMap;                         // the ECU maps (Buttons page), the one in use
static volatile bool s_ecuMapUsed = false;         // a button runs "ECU map (next)": the map frame goes out
static volatile bool s_ecuMapSendNow = false;      // just changed: send it now
// ECU map frame (Buttons page, "ECU map (next)"): its id, byte 0 = map number (1-based), byte 1 = how many maps.
// Sent with the EGT frame (same rule: only while the ECU is heard), and at once after a press. Only if a button uses it.
static void canSendEcuMap() {
  if (!s_ecuMapUsed || s_egtHeld) return;
  s_ecuMapSendNow = false;
  twai_message_t m = {};
  m.identifier = s_ecuMap.id; m.data_length_code = 2; m.data[0] = s_ecuMap.cur + 1; m.data[1] = s_ecuMap.count;
  twai_transmit(&m, pdMS_TO_TICKS(5));
}
static void canTxTask(void *) {
  for (;;) {
    for (int k = 0; k < EGT_PERIOD_MS / 10; k++) { vTaskDelay(pdMS_TO_TICKS(10)); if (s_ecuMapSendNow) canSendEcuMap(); }
    twai_message_t m = {};
    m.identifier = CAN_ID_EGT_PLACEHOLDER; m.data_length_code = 3;
    bool open = s_egtOpen;
    uint16_t ci = open ? 0 : (uint16_t)lroundf(s_egtC);
    m.data[0] = ci & 0xFF; m.data[1] = ci >> 8; m.data[2] = open ? 1 : 0;
    s_egtHeld = !s_canLastRxMs || millis() - s_canLastRxMs > EGT_HOLD_MS;
    if (!s_egtHeld && twai_transmit(&m, pdMS_TO_TICKS(5)) == ESP_OK) s_egtSent++;
    canSendEcuMap();
  }
}

// ---- DHT22 ambient temp + humidity on GPIO4 (3-pin module, pull-up built in), read every 2.5 s ----
#define DHT_PERIOD_MS 2500
static volatile float s_dhtC = 0, s_dhtH = 0; static volatile bool s_dhtOk = false; static volatile uint32_t s_dhtFail = 0;
static portMUX_TYPE s_dhtMux = portMUX_INITIALIZER_UNLOCKED;
static int dhtWait(int level, int maxUs) {           // us spent while the line stays at 'level', -1 on timeout
  uint32_t t0 = micros();
  while (digitalRead(DHT_PIN) == level) if ((int)(micros() - t0) > maxUs) return -1;
  return (int)(micros() - t0);
}
static bool dhtRead(float *c, float *h) {
  uint8_t d[5] = {0};
  pinMode(DHT_PIN, OUTPUT); digitalWrite(DHT_PIN, LOW); delay(2);   // start signal: low for at least 1 ms
  bool ok = true;
  portENTER_CRITICAL(&s_dhtMux);                     // ~5 ms with interrupts off so the bit timing is exact
  pinMode(DHT_PIN, INPUT_PULLUP);                    // let go: the sensor answers 20-40 us after release
  if (dhtWait(HIGH, 100) < 0 || dhtWait(LOW, 100) < 0 || dhtWait(HIGH, 100) < 0) ok = false;   // sensor answers low 80 / high 80
  for (int i = 0; ok && i < 40; i++) {
    if (dhtWait(LOW, 80) < 0) { ok = false; break; }
    int hi = dhtWait(HIGH, 100); if (hi < 0) { ok = false; break; }
    if (hi > 40) d[i / 8] |= 0x80 >> (i % 8);        // ~27 us high = 0, ~70 us high = 1
  }
  portEXIT_CRITICAL(&s_dhtMux);
  if (!ok || (uint8_t)(d[0] + d[1] + d[2] + d[3]) != d[4]) return false;
  *h = ((d[0] << 8) | d[1]) * 0.1f;
  int16_t t = ((d[2] & 0x7F) << 8) | d[3];
  *c = (d[2] & 0x80 ? -t : t) * 0.1f;
  return *h <= 100.0f;
}
static void dhtTask(void *) {
  vTaskDelay(pdMS_TO_TICKS(2000));                   // the sensor needs ~1 s after power-up
  for (;;) {
    float c, h;
    if (dhtRead(&c, &h)) {
      s_dhtC = c; s_dhtH = h; s_dhtOk = true;
      if (!simOwns()) {
      g_ambt_f = c * 9.0f / 5.0f + 32.0f; g_ambt_has_data = true; markLocal(SIG_AMBT, SRC_HUB_DHT);
      g_hum_pct = h;                      g_hum_has_data = true;  markLocal(SIG_HUM, SRC_HUB_DHT);
      }
    } else { s_dhtFail++; }
    vTaskDelay(pdMS_TO_TICKS(DHT_PERIOD_MS));
  }
}

// ---- Fuel level: pot (mock sender) on GPIO36, 0-3.3 V = 0-100 %, smoothed, read every 100 ms ----
static volatile float s_fuelPct = 0;   // the pot's own reading, shown on the home page even while the simulator runs
static void fuelTask(void *) {
  float avg = adcRaw(FUEL_PIN);
  for (;;) {
    avg += (adcRaw(FUEL_PIN) - avg) * 0.1f;      // slow filter: fuel sloshes, the gauge should not
    s_fuelPct = avg * 100.0f / 4095.0f;
    if (simOwns()) { vTaskDelay(pdMS_TO_TICKS(100)); continue; }
    g_fuel_pct = avg * 100.0f / 4095.0f;
    g_fuel_has_data = true;
    markLocal(SIG_FUEL, SRC_HUB_FUEL);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// every 50 ms: smooth the inputs; send when the result moves 2 % or more, and every 2 s so a screen that reboots catches up
static void readKnob() {
  int raw = adcRaw(POT_PIN);
  s_potAvg = s_potAvg < 0 ? raw : s_potAvg * 0.85f + raw * 0.15f;
  s_lightsV = s_lightsV * 0.8f + carVolts(LIGHTS_PIN) * 0.2f;
  s_dimV = s_dimV * 0.8f + carVolts(DIMMER_PIN) * 0.2f;
  int pct = finalPct();
  if (s_briPct < 0 || abs(pct - s_briPct) >= 2 || millis() - s_lastBriMs > 2000) {
    s_briPct = pct; s_lastBriMs = millis();
    sendCmd(LINK_CMD_BRIGHTNESS, (int16_t)pct, "");
  }
}

// ---- web: status page and WiFi updates (curl -F "firmware=@...bin" http://192.168.55.204/update) ----
static void oledNextPage();   // OLED panel: next page (defined with the panel, below)
static void outputToggle(int n);   // outputs: toggle output n (1-based), for the "Toggle output" action (below)
// LOW on a pin? Hub GPIOs are read directly, expander pins from the expander's last read (50 times a second).
static bool anyPinLow(int pin) {
  if (pin_is_mcp(pin)) { int n = pin - PIN_MCP_BASE, e = n / 16, b = n % 16; return s_mcpAddr[e] && !((s_mcpBits[e] >> b) & 1); }
  return digitalRead(pin) == LOW;
}
// ---- buttons (hub_buttons.h, Buttons page): digital or on a resistor ladder, one action each on a press ----
static HubButton s_btns[HUB_BUTTONS];
static HbDebounce s_btnDeb[HUB_BUTTONS];          // digital: its own; ladder: in the slot of the pin's first button
static volatile int16_t s_btnMv[HUB_BUTTONS];     // ladder: the pin's last reading, millivolts (for the page)
static volatile bool s_btnDown[HUB_BUTTONS];
static char s_target[LINK_NAME_LEN + 1] = "";     // "Chosen screen": picked with "Next gauge"; "" = all
static uint32_t s_lastBtnMs = 0;
static int s_markN = 0;
#define BTN_TARGET_IDLE_MS 60000                  // the chosen screen goes back to all after a minute without a press
static void buttonsSave() { Preferences p; p.begin("buttons", false); p.putUChar("ver", HUB_BUTTONS_VERSION); p.putBytes("cfg", s_btns, sizeof s_btns); p.end(); }
static void ecuMapSave() { Preferences p; p.begin("buttons", false); p.putBytes("ecu", &s_ecuMap, sizeof s_ecuMap); p.end(); }
static void buttonsReset() {   // after a change: fresh debounce, pins set up, is the ECU map in use
  bool ecu = false;
  for (int i = 0; i < HUB_BUTTONS; i++) {
    hb_deb_init(&s_btnDeb[i]); s_btnDown[i] = false;
    const HubButton &b = s_btns[i];
    if (b.kind == HB_DIGITAL && !pin_is_mcp(b.pin)) pinMode(b.pin, (gpio_caps(b.pin) & CAP_PULLUP) ? INPUT_PULLUP : INPUT);
    if (b.kind != HB_NONE && b.action == BA_ECU_MAP) ecu = true;
  }
  s_ecuMapUsed = ecu;
}
static void chooseNextScreen() {
  const char *names[SCREEN_MAX]; static char copy[SCREEN_MAX][LINK_NAME_LEN + 1];
  portENTER_CRITICAL(&s_scrMux);
  for (int i = 0; i < SCREEN_MAX; i++) {
    bool on = s_screens[i].used && millis() - s_screens[i].lastMs < SCREEN_OFFLINE_MS;
    strcpy(copy[i], on ? s_screens[i].name : "");
  }
  portEXIT_CRITICAL(&s_scrMux);
  for (int i = 0; i < SCREEN_MAX; i++) names[i] = copy[i];
  int k = btn_next_target(s_target, names, SCREEN_MAX);
  strcpy(s_target, k < 0 ? "" : copy[k]);
  sendCmd(LINK_CMD_SELECT, 0, s_target);   // only that screen (or all) shows the pop-up
}
// The screen name for a target: Left / Center / Right by the names the screens report (any case, "centre" too).
static void targetName(int target, char *out) {
  out[0] = 0;
  if (target == HBT_ALL) return;
  if (target == HBT_CHOSEN) { strcpy(out, s_target); return; }
  portENTER_CRITICAL(&s_scrMux);
  for (int i = 0; i < SCREEN_MAX && !out[0]; i++) if (s_screens[i].used && hb_target_is(target, s_screens[i].name)) strcpy(out, s_screens[i].name);
  portEXIT_CRITICAL(&s_scrMux);
  if (!out[0]) strcpy(out, HBT_NAMES[target]);   // not heard yet: the usual name
}
// Page actions (next, previous, go to page) go to the button's screen; the rest are for every screen.
static void runAction(int action, int param, int target) {
  char t[LINK_NAME_LEN + 1]; targetName(target, t);
  switch (action) {
    case BA_NEXT_PAGE: sendCmd(LINK_CMD_PAGE_NEXT, 0, t); break;
    case BA_PREV_PAGE: sendCmd(LINK_CMD_PAGE_PREV, 0, t); break;
    case BA_NEXT_GAUGE: chooseNextScreen(); break;
    case BA_GOTO_PAGE: sendCmd(LINK_CMD_GOTO_PAGE, param ? param : 1, t); break;
    case BA_DIM:    s_briOffset = s_briOffset - 20 < -95 ? -95 : s_briOffset - 20; s_briOffsetBase = wantedPct(); s_briPct = -1; break;
    case BA_BRIGHT: s_briOffset = s_briOffset + 20 > 95 ? 95 : s_briOffset + 20; s_briOffsetBase = wantedPct(); s_briPct = -1; break;
    case BA_COLOR: sendCmd(LINK_CMD_COLOR, param, ""); break;
    case BA_RACE:
      s_raceOn = !s_raceOn; s_briPct = -1;
      sendCmd(LINK_CMD_RACE, s_raceOn ? 1 : 0, "");
      if (s_raceOn) sendCmd(LINK_CMD_GOTO_PAGE, param ? param : 1, "");
      break;
    case BA_CLEAR_WARNING: sendCmd(LINK_CMD_ALERT_ACK, 0, ""); break;
    case BA_LOG_MARK: sendCmd(LINK_CMD_LOG_MARK, ++s_markN, ""); break;
    case BA_SHOW_IP: sendCmd(LINK_CMD_SHOW_IP, WiFi.localIP()[3], ""); break;   // every screen: its name and IP
    case BA_OLED_NEXT: oledNextPage(); break;
    case BA_OUT_TOGGLE: outputToggle(param); break;
    case BA_ECU_MAP: {   // next map: saved, sent to the ECU now, its name on every screen
      ecu_map_next(&s_ecuMap); ecuMapSave(); s_ecuMapSendNow = true;
      char lb[LINK_NAME_LEN + 1]; ecu_map_label(&s_ecuMap, lb, sizeof lb);
      sendCmd(LINK_CMD_POPUP, s_ecuMap.cur + 1, lb);
      break;
    }
  }
}
// Every 10 ms: read each button (a ladder pin once), run the action of a new press.
static void handleButtons() {
  uint32_t now = millis();
  for (int i = 0; i < HUB_BUTTONS; i++) {
    const HubButton &b = s_btns[i];
    if (b.kind == HB_DIGITAL) {
      bool low = anyPinLow(b.pin), act = b.active_high ? !low : low;
      s_btnDown[i] = act;
      if (hb_deb_step(&s_btnDeb[i], now, act ? i : -1, HB_DEBOUNCE_MS) == i) { s_lastBtnMs = now; runAction(b.action, b.param, b.target); }
    } else if (b.kind == HB_LADDER) {
      bool first = true;
      for (int k = 0; k < i && first; k++) if (s_btns[k].kind == HB_LADDER && s_btns[k].pin == b.pin) first = false;
      if (!first) continue;   // one read per ladder pin
      int mv = (int)adcMilliVolts(b.pin);
      int hit = hb_ladder_match(s_btns, HUB_BUTTONS, b.pin, mv);
      for (int k = i; k < HUB_BUTTONS; k++) if (s_btns[k].kind == HB_LADDER && s_btns[k].pin == b.pin) { s_btnMv[k] = mv; s_btnDown[k] = k == hit; }
      int pr = hb_deb_step(&s_btnDeb[i], now, hit, HB_LADDER_DEBOUNCE_MS);
      if (pr >= 0) { s_lastBtnMs = now; runAction(s_btns[pr].action, s_btns[pr].param, s_btns[pr].target); }
    }
  }
  if (s_target[0] && now - s_lastBtnMs > BTN_TARGET_IDLE_MS) { s_target[0] = 0; sendCmd(LINK_CMD_SELECT, 0, ""); }
}

// ---- OLED warning panel (oled_panel.h): 0.96" SSD1306 on GPIO 5 (SDA, R15) / 15 (SCL, R21). Lamp inputs: GPIO 2 (R20),
// GPIO 12 (L18), GPIO 0 (R19, BOOT), and up to two MCP23017 expanders (16 inputs each) on the same two wires ----
static OledConfig s_oled;
static uint8_t s_oledBuf[1024];            // what the display shows (also sent to the /oled page as a live picture)
static volatile uint8_t s_oledState[OLED_LAMPS];
static bool s_oledOk = false;
static void oledLoad() {
  Preferences p; p.begin("oled", true);
  if (p.getBytes("cfg", &s_oled, sizeof s_oled) != sizeof s_oled || s_oled.version != OLED_CFG_VERSION) oled_defaults(&s_oled);
  p.end();
  for (int i = 0; i < OLED_LAMPS; i++) {
    OledLamp &l = s_oled.lamp[i];
    if (l.op == OLED_INPUT) l.input = (uint8_t)oled_input_upgrade(l.input);   // older codes 1-3 (GPIO 2 / 12 / 0)
    if (l.icon > OLED_ICON_COUNT || l.op > OLED_INPUT || (l.op == OLED_INPUT && !oled_input_valid(l.input))) l.icon = 0;
  }
}
// Pages (oled_pages.h): which one is chosen, the timer, the biggest combined G seen.
static OledPages s_pages;
static volatile int s_oledPage = 0, s_oledShown = 0;
static uint32_t s_oledPageMs = 0;
static float s_gPeak = 0;
static void pagesLoad() {
  Preferences p; p.begin("opages", true);
  if (p.getBytes("cfg", &s_pages, sizeof s_pages) != sizeof s_pages || s_pages.version != OLED_PAGES_VERSION || s_pages.count < 1 || s_pages.count > OLED_MAX_PAGES) oled_pages_defaults(&s_pages);
  p.end();
  for (int i = 0; i < OLED_MAX_PAGES; i++) { OledPage &g = s_pages.page[i]; if (g.type >= OPG_TYPES) g.type = OPG_LAMPS; for (int k = 0; k < 4; k++) { if (g.sig[k] >= SIG_COUNT) g.sig[k] = 0; g.label[k][OLED_LABEL_LEN - 1] = 0; } }
}
static void pagesSave() { s_pages.version = OLED_PAGES_VERSION; Preferences p; p.begin("opages", false); p.putBytes("cfg", &s_pages, sizeof s_pages); p.end(); }
static void oledNextPage() { s_oledPage = (s_oledPage + 1) % (s_pages.count ? s_pages.count : 1); s_oledPageMs = millis(); }
static void oledSave() { s_oled.version = OLED_CFG_VERSION; Preferences p; p.begin("oled", false); p.putBytes("cfg", &s_oled, sizeof s_oled); p.end(); }
static uint8_t s_inPulled = 0;   // hub input pins already set to INPUT_PULLUP
static int inputPin(int in) { return in >= OLED_IN_GPIO_BASE && in < OLED_IN_GPIO_BASE + 40 ? in - OLED_IN_GPIO_BASE : -1; }
// Reads an input (true = LOW). Hub pins get their pull-up the first time; expander pins are pulled up by the chip.
static bool inputLow(int in) {
  int pin = inputPin(in);
  if (pin >= 0) {
    uint8_t bit = 1 << in;
    if (!(s_inPulled & bit)) { pinMode(pin, INPUT_PULLUP); s_inPulled |= bit; delay(1); }
    return digitalRead(pin) == LOW;
  }
  if (in >= OLED_IN_MCP0 && in < OLED_IN_MCP1 + 16) { int e = (in - OLED_IN_MCP0) / 16, b = (in - OLED_IN_MCP0) % 16; return s_mcpAddr[e] && !((s_mcpBits[e] >> b) & 1); }
  return false;
}
// ---- accelerometer (MPU6050) on the same wires: 50 Hz, car axes from the mounting setup on the home page ----
static ImuConfig s_imuCfg;
static bool s_imuOk = false;
static volatile float s_imuRaw[4];        // car frame before the zero (shown on the page, used for zeroing)
static volatile int s_imuZeroLeft = 0;    // samples still to average for a zero (0 = not zeroing)
static float s_imuSum[4];
static void imuLoad() {
  Preferences p; p.begin("imu", true);
  if (p.getBytes("cfg", &s_imuCfg, sizeof s_imuCfg) != sizeof s_imuCfg || s_imuCfg.version != IMU_CFG_VERSION || !imu_axes_valid(s_imuCfg.fwd, s_imuCfg.left)) imu_defaults(&s_imuCfg);
  p.end();
}
static void imuSave() { s_imuCfg.version = IMU_CFG_VERSION; Preferences p; p.begin("imu", false); p.putBytes("cfg", &s_imuCfg, sizeof s_imuCfg); p.end(); }
static void imuStep() {
  float acc[3], gyro[3], raw[4], out[4];
  if (!imuRead(acc, gyro)) return;
  imu_car_frame(&s_imuCfg, acc, gyro, raw);
  for (int i = 0; i < 4; i++) s_imuRaw[i] = raw[i];
  if (s_imuZeroLeft > 0) {   // zeroing: average one second of readings with the car still
    for (int i = 0; i < 4; i++) s_imuSum[i] += raw[i];
    if (--s_imuZeroLeft == 0) { for (int i = 0; i < 4; i++) s_imuCfg.zero[i] = s_imuSum[i] / 50.0f; s_imuCfg.zeroed = 1; imuSave(); }
  }
  imu_apply(&s_imuCfg, raw, out);
  if (simOwns()) return;   // the simulator feeds these while it runs
  for (int i = 0; i < 4; i++) { g_imu_val[i] = imu_smooth(g_imu_val[i], out[i], 0.17f); markLocal((SignalId)(SIG_GLAT + i), SRC_HUB_IMU); }
}

// Every address that answered on the I2C bus at boot, for the home page (0 = end of list).
static uint8_t s_i2cFound[16];
static void i2cScan() {
  int n = 0;
  for (uint8_t a = 0x08; a < 0x78 && n < 15; a++) { Wire.beginTransmission(a); if (Wire.endTransmission() == 0) s_i2cFound[n++] = a; }
  s_i2cFound[n] = 0;
  Serial.print("I2C bus:");
  for (int i = 0; i < n; i++) Serial.printf(" 0x%02X", s_i2cFound[i]);
  Serial.println(n ? "" : " nothing found");
}

static void oledTask(void *) {
  s_oledOk = oledBegin();   // starts Wire on GPIO 5 / 15
  i2cScan();
  s_imuOk = imuBegin();
  Serial.printf("Accelerometer: %s\n", s_imuOk ? "MPU6050 found" : "not found (SDA GPIO 5, SCL GPIO 15, 3.3 V)");
  int mcpN = mcpBegin();
  int adsN = adsBegin();
  Serial.printf("ADS1115: %d board(s)", adsN);
  for (int b = 0; b < ADS_BOARDS; b++) if (s_adsAddr[b]) Serial.printf(" (0x%02X)", s_adsAddr[b]);
  Serial.println(adsN ? "" : " - none found (ADDR to GND = 0x48)");
  Serial.printf("OLED: %s | MCP23017 expanders: %d", s_oledOk ? "SSD1306 found" : "not found (VCC 3.3 V, SDA GPIO 5, SCL GPIO 15)", mcpN);
  for (int e = 0; e < mcpN; e++) Serial.printf(" (0x%02X)", s_mcpAddr[e]);
  Serial.println();
  int lastContrast = -1;
  uint32_t tick = 0;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(20));   // 50 Hz: the accelerometer every pass, the panel every 10th
    if (s_imuOk) imuStep();
    mcpRead();   // expander pins every pass: buttons on them need it
    mcpApplyOutputs();   // and outputs on them, when they change
    adsStep();           // ADS1115 channels: the next conversion on each board
    if (++tick % 10) continue;
    uint8_t st[OLED_LAMPS];
    bool anyLit = false;
    for (int i = 0; i < OLED_LAMPS; i++) {
      const OledLamp *l = &s_oled.lamp[i];
      SignalId sg = (SignalId)(l->sig < SIG_COUNT ? l->sig : 0);
      st[i] = (uint8_t)oled_lamp_state(l, gauge_config_signal_value(sg), gauge_config_signal_has_data(sg), l->op == OLED_INPUT && inputLow(l->input));
      s_oledState[i] = st[i];
      if (st[i]) anyLit = true;
    }
    { float m = sqrtf(g_imu_val[0] * g_imu_val[0] + g_imu_val[1] * g_imu_val[1]); if (gauge_config_signal_has_data(SIG_GLAT) && m > s_gPeak) s_gPeak = m; }
    if (s_pages.cycle_s && millis() - s_oledPageMs > s_pages.cycle_s * 1000u) oledNextPage();   // page timer
    char idle[16] = "ALL OK", sub[20] = "";
    uint32_t u = gpsUtcNow();
    if (u) {   // the GPS clock, 12-hour
      int32_t t = (int32_t)(((int64_t)u + tzOffsetNow(u) * 60) % 86400); if (t < 0) t += 86400;
      int h = t / 3600, m = t / 60 % 60;
      snprintf(idle, sizeof idle, "%d:%02d %s", h % 12 ? h % 12 : 12, m, h < 12 ? "AM" : "PM");
      strcpy(sub, "ALL OK");
    }
    if (!s_canLastRxMs || millis() - s_canLastRxMs > 2000) strcpy(sub, "NO ECU DATA");
    int pg = oled_page_to_show(&s_pages, s_oledPage, anyLit);
    s_oledShown = pg;
    if (pg < 0 || s_pages.page[pg].type == OPG_LAMPS) oled_render(s_oledBuf, &s_oled, st, (millis() / 400) % 2 == 0, idle, sub);
    else {
      const OledPage *p = &s_pages.page[pg];
      OledPageData d = {};
      for (int k = 0; k < 4; k++) { SignalId sg = (SignalId)p->sig[k]; d.val[k] = gauge_config_signal_value(sg); d.has[k] = sg && gauge_config_signal_has_data(sg); }
      d.ghas = gauge_config_signal_has_data(SIG_GLAT); d.glat = g_imu_val[0]; d.glon = g_imu_val[1]; d.gpeak = s_gPeak;
      char date[24] = "";
      if (u) { gt_format(u, tzOffsetNow(u), 0, date, sizeof date); date[10] = 0; }
      d.time = u ? idle : ""; d.date = date;
      oled_render_page(s_oledBuf, p, &d);
    }
    if (s_oledOk) {
      int pct = s_briPct < 0 ? 100 : s_briPct, contrast = 8 + pct * 247 / 100;   // follows the screens' brightness (night = dimmer)
      if (abs(contrast - lastContrast) > 4) { oledContrast((uint8_t)contrast); lastContrast = contrast; }
      oledShow(s_oledBuf);
    }
  }
}

// ---- configurable inputs (hub_inputs.h, Inputs page) ----
static HubInput s_inputs[HUB_INPUTS];
static volatile float s_inValue[HUB_INPUTS];     // last reading (sensors) or 1 / 0 (switches), for the page
static volatile bool s_inActive[HUB_INPUTS];
static void inputsLoad() {
  Preferences p; p.begin("inputs", true);
  uint8_t ver = p.getUChar("ver", 0);
  if (ver != HUB_INPUTS_VERSION || p.getBytes("cfg", s_inputs, sizeof s_inputs) != sizeof s_inputs) for (int i = 0; i < HUB_INPUTS; i++) input_defaults(&s_inputs[i]);
  p.end();
  for (int i = 0; i < HUB_INPUTS; i++) { s_inputs[i].name[15] = 0; if (s_inputs[i].type > IN_ANALOG || s_inputs[i].slot >= CUSTOM_SLOTS) s_inputs[i].type = IN_NONE; }
}
static void inputsSave() { Preferences p; p.begin("inputs", false); p.putUChar("ver", HUB_INPUTS_VERSION); p.putBytes("cfg", s_inputs, sizeof s_inputs); p.end(); }
// After inputsLoad(): the first start on hub 53 builds the list from the old page buttons and any button inputs.
static void buttonsLoad() {
  Preferences p; p.begin("buttons", true);
  bool got = p.getUChar("ver", 0) == HUB_BUTTONS_VERSION && p.getBytes("cfg", s_btns, sizeof s_btns) == sizeof s_btns;
  if (p.getBytes("ecu", &s_ecuMap, sizeof s_ecuMap) != sizeof s_ecuMap) ecu_map_defaults(&s_ecuMap);
  p.end();
  ecu_map_fix(&s_ecuMap);
  if (!got) {
    for (int i = 0; i < HUB_BUTTONS; i++) hb_defaults(&s_btns[i]);
    BtnBinding bind[BTN_INPUTS][BTN_LEVELS];
    Preferences q; q.begin("btncfg", true);
    if (q.getBytes("b", bind, sizeof bind) != sizeof bind) btn_defaults(bind);
    q.end();
    int n = hb_from_old(s_btns, s_oldBtnPrev, s_oldBtnNext, bind);
    bool moved = false;
    for (int i = 0; i < HUB_INPUTS && n < HUB_BUTTONS; i++) {
      HubInput &in = s_inputs[i];
      if (in.type != IN_BUTTON) continue;
      HubButton &b = s_btns[n++]; hb_defaults(&b);
      b.kind = HB_DIGITAL; b.pin = in.pin; b.active_high = in.contact == CONTACT_NC; b.action = in.act_press < BA_COUNT ? in.act_press : BA_NONE;
      b.param = in.prm_press; b.target = HBT_ALL; memcpy(b.name, in.name, sizeof b.name); b.name[15] = 0;
      input_defaults(&in); moved = true;
    }
    buttonsSave(); if (moved) inputsSave();
    Serial.printf("Buttons: set up from the old settings (%d buttons)\n", n);
  }
  for (int i = 0; i < HUB_BUTTONS; i++) {
    HubButton &b = s_btns[i];
    b.name[15] = 0;
    if (b.kind > HB_LADDER || !hb_pin_fits(b.kind, b.pin)) b.kind = HB_NONE;
    if (b.action >= BA_COUNT) b.action = BA_NONE;
    if (b.target >= HBT_COUNT) b.target = HBT_ALL;
  }
  buttonsReset();
}
static void inputPinSetup(const HubInput &in) {
  if (in.type == IN_NONE || input_is_mcp(in.pin)) return;
  if (in.type == IN_ANALOG) return;   // the ADC sets the pin up itself
  pinMode(in.pin, (gpio_caps(in.pin) & CAP_PULLUP) ? INPUT_PULLUP : INPUT);
}
// LOW on the input's pin? (MCP23017 pins: from the expander's last read.)
static bool inputPinLow(int pin) { return anyPinLow(pin); }
// Who uses a hub GPIO, an expander pin or an ADS1115 channel? "" = free. One user per pin; the skip arguments leave out
// the thing being edited. ladderShare: a ladder button asking, so other ladder buttons on that pin do not count.
static HubOutput s_outputs[HUB_OUTPUTS];
static int lampPin(int in) { return in >= OLED_IN_GPIO_BASE ? in - OLED_IN_GPIO_BASE : INPUT_MCP_BASE + (in - OLED_IN_MCP0); }
static String pinUser(int pin, int skipInput, int skipOutput = -1, int skipButton = -1, bool ladderShare = false, int skipLamp = -1) {
  { int r = pin_map_role_at(g_pins, pin); if (r >= 0) return HUB_PIN_ROLES[r].name; }
  for (int i = 0; i < HUB_OUTPUTS; i++) if (i != skipOutput && s_outputs[i].type != OUT_NONE && s_outputs[i].pin == pin)
    return String("Output ") + (i + 1) + (s_outputs[i].name[0] ? String(" (") + s_outputs[i].name + ")" : String(""));
  for (int i = 0; i < HUB_INPUTS; i++) if (i != skipInput && s_inputs[i].type != IN_NONE && s_inputs[i].pin == pin)
    return String("Input ") + (i + 1) + (s_inputs[i].name[0] ? String(" (") + s_inputs[i].name + ")" : String(""));
  for (int i = 0; i < HUB_BUTTONS; i++) {
    const HubButton &b = s_btns[i];
    if (i == skipButton || b.kind == HB_NONE || b.pin != pin) continue;
    if (b.kind == HB_LADDER) {
      if (ladderShare) continue;
      String names; int n = 0;
      for (int k = 0; k < HUB_BUTTONS; k++) if (k != skipButton && s_btns[k].kind == HB_LADDER && s_btns[k].pin == pin) { names += n++ ? ", " : ""; names += s_btns[k].name; }
      return "Button ladder (" + names + ")";
    }
    return String("Button ") + (i + 1) + (b.name[0] ? String(" (") + b.name + ")" : String(""));
  }
  for (int i = 0; i < OLED_LAMPS; i++) {
    const OledLamp &l = s_oled.lamp[i];
    if (i == skipLamp || !l.icon || l.op != OLED_INPUT) continue;
    if (lampPin(l.input) == pin) return String("OLED lamp ") + (i + 1);
  }
  return "";
}
// ---- outputs ----
static volatile bool s_outOn[HUB_OUTPUTS], s_outManual[HUB_OUTPUTS];
static void outputsLoad() {
  Preferences p; p.begin("outputs", true);
  uint8_t ver = p.getUChar("ver", 0);
  if (ver != HUB_OUTPUTS_VERSION || p.getBytes("cfg", s_outputs, sizeof s_outputs) != sizeof s_outputs) for (int i = 0; i < HUB_OUTPUTS; i++) output_defaults(&s_outputs[i]);
  p.end();
  for (int i = 0; i < HUB_OUTPUTS; i++) { s_outputs[i].name[15] = 0; if (s_outputs[i].type > OUT_MANUAL || !output_pin_fits(s_outputs[i].pin)) s_outputs[i].type = OUT_NONE; }
}
static void outputsSave() { Preferences p; p.begin("outputs", false); p.putUChar("ver", HUB_OUTPUTS_VERSION); p.putBytes("cfg", s_outputs, sizeof s_outputs); p.end(); }
static void outputPinSetup(const HubOutput &o) {
  if (o.type == OUT_NONE || pin_is_mcp(o.pin)) return;
  digitalWrite(o.pin, output_pin_high(&o, false, 0) ? HIGH : LOW);   // off before it becomes an output
  pinMode(o.pin, OUTPUT);
}
static void outputToggle(int n) { if (n >= 1 && n <= HUB_OUTPUTS && s_outputs[n - 1].type == OUT_MANUAL) s_outManual[n - 1] = !s_outManual[n - 1]; }
// Every 20 ms: each output's state, written to its GPIO or queued for the expander.
static void outputsStep() {
  uint32_t now = millis();
  uint16_t mask[2] = { 0, 0 }, bits[2] = { 0, 0 };
  for (int i = 0; i < HUB_OUTPUTS; i++) {
    const HubOutput &o = s_outputs[i];
    if (o.type == OUT_NONE) continue;
    SignalId sg = (SignalId)(o.sig < SIG_COUNT ? o.sig : 0);
    bool on = output_want(&o, gauge_config_signal_value(sg), sg && gauge_config_signal_has_data(sg),
                          o.input < HUB_INPUTS && s_inActive[o.input], s_outManual[i], s_outOn[i]);
    s_outOn[i] = on;
    bool high = output_pin_high(&o, on, now);
    if (pin_is_mcp(o.pin)) { int n = o.pin - PIN_MCP_BASE, e = n / 16, b = n % 16; mask[e] |= 1u << b; if (high) bits[e] |= 1u << b; }
    else digitalWrite(o.pin, high ? HIGH : LOW);
  }
  for (int e = 0; e < 2; e++) { s_mcpOutMask[e] = mask[e]; s_mcpOutBits[e] = bits[e]; }
}

// Every 20 ms: read each input, fill the switch / sensor slots.
static void inputsStep() {
  for (int i = 0; i < HUB_INPUTS; i++) {
    const HubInput &in = s_inputs[i];
    if (in.type == IN_NONE) continue;
    if (in.type == IN_ANALOG) {
      float v = input_convert(&in, adcMilliVolts(in.pin) / 1000.0f);
      if (!isfinite(v)) { s_inActive[i] = false; continue; }
      float sm = s_inValue[i] + (v - s_inValue[i]) * 0.2f;
      s_inValue[i] = sm; s_inActive[i] = true;
      if (!simOwns()) { g_custom_val[in.slot] = sm; g_custom_has[in.slot] = true; markLocal((SignalId)(SIG_C1 + in.slot), SRC_HUB_INPUT); }
      continue;
    }
    if (in.type != IN_SWITCH) continue;   // (an old button input moved to the Buttons page)
    bool act = input_active(&in, inputPinLow(in.pin));
    s_inActive[i] = act; s_inValue[i] = act ? 1.0f : 0.0f;
    if (!simOwns()) { g_custom_val[in.slot] = act ? 1.0f : 0.0f; g_custom_has[in.slot] = true; markLocal((SignalId)(SIG_C1 + in.slot), SRC_HUB_INPUT); }
  }
}

static void setupWeb() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", HOME_HTML, strlen_P(HOME_HTML));
  });
  // ---- raw CAN monitor: every ID seen, latest bytes, count, age ----
  server.on("/can.json", HTTP_GET, []() {
    CanMonEntry snap[CANMON_MAX]; int n; uint32_t tot;
    portENTER_CRITICAL(&s_canMux); n = s_canMonN; tot = s_canTotal; memcpy(snap, s_canMon, sizeof(CanMonEntry) * n); portEXIT_CRITICAL(&s_canMux);
    twai_status_info_t st; bool ok = twai_get_status_info(&st) == ESP_OK;
    String j = "{\"total\":" + String(tot) + ",\"state\":" + String(ok ? (int)st.state : -1) +
               ",\"rxerr\":" + String(ok ? st.rx_error_counter : 0) + ",\"missed\":" + String(ok ? st.rx_missed_count : 0) + ",\"ids\":[";
    uint32_t now = millis();
    for (int i = 0; i < n; i++) {
      char b[96]; int k = snprintf(b, sizeof b, "%s{\"id\":%u,\"n\":%u,\"age\":%u,\"d\":\"", i ? "," : "", (unsigned)snap[i].id, (unsigned)snap[i].count, (unsigned)(now - snap[i].lastMs));
      for (int x = 0; x < snap[i].len; x++) k += snprintf(b + k, sizeof b - k, "%02X ", snap[i].d[x]);
      snprintf(b + k, sizeof b - k, "\"}");
      j += b;
    }
    j += "]}";
    server.send(200, "application/json", j);
  });
  server.on("/can", HTTP_GET, []() { server.send_P(200, "text/html", CAN_HTML, strlen_P(CAN_HTML)); });
  server.on("/screen", HTTP_GET, []() { server.send_P(200, "text/html", SCREEN_HTML, strlen_P(SCREEN_HTML)); });
  server.on("/setup", HTTP_GET, []() { server.send_P(200, "text/html", SETUP_HTML, strlen_P(SETUP_HTML)); });
  server.on("/firmware", HTTP_GET, []() { server.send_P(200, "text/html", FIRMWARE_HTML, strlen_P(FIRMWARE_HTML)); });
  // ---- sensors: every signal that has ever had data, its value, age and where it comes from ----
  server.on("/sensors.json", HTTP_GET, []() {
    uint32_t now = millis();
    String j = "[";
    bool first = true;
    for (int i = 1; i < SIG_COUNT; i++) {
      uint32_t seen = s_seenMs[i]; if (!seen) continue;
      float v = gauge_config_signal_value((SignalId)i);
      char b[120];
      snprintf(b, sizeof b, "%s{\"sig\":%d,\"v\":%.2f,\"age\":%u,\"src\":%u}", first ? "" : ",", i, (v == v && v > -1e30f && v < 1e30f) ? (double)v : 0.0,
               (unsigned)(now - seen), (unsigned)s_srcId[i]);
      j += b; first = false;
    }
    j += "]";
    server.send(200, "application/json", j);
  });
  server.on("/sensors", HTTP_GET, []() {
    server.send_P(200, "text/html", SENSORS_HTML, strlen_P(SENSORS_HTML));
  });
  // ---- bench simulator (moved here from the screens in build 15; safety rules in simulator.h) ----
  server.on("/sim.json", HTTP_GET, []() {
    String j = "{\"active\":" + String(sim_is_active() ? "true" : "false") + ",\"manual\":" + String(sim_is_manual() ? "true" : "false") +
               ",\"seconds_left\":" + String(sim_seconds_left()) + ",\"stop_reason\":\"" + String(sim_last_stop_reason()) + "\",\"signals\":[";
    bool first = true;
    for (int sig = 1; sig < SIG_COUNT; sig++) {
      if (sig == SIG_CLOCK || (gauge_signal_is_custom(sig) && !custom_slot_info(sig - SIG_C1)->name[0])) continue;   // not simulated / not set up
      SimRange r = sim_range(sig);
      char b[120];
      snprintf(b, sizeof b, "%s{\"id\":%d,\"min\":%g,\"max\":%g,\"step\":%g,\"value\":%g}", first ? "" : ",", sig, (double)r.min, (double)r.max, (double)r.step, (double)sim_value(sig));
      j += b; first = false;
    }
    j += "]}";
    server.send(200, "application/json", j);
  });
  server.on("/sim", HTTP_POST, []() {
    String cmd = server.arg("cmd");
    if (cmd == "start") {
      uint32_t last = s_canLastRxMs;
      if (last && millis() - last < 1000) { server.send(409, "text/plain", "Real CAN frames are arriving, so the simulator will not start. It only runs while the bus is silent."); return; }
      sim_start(server.arg("mode") == "manual");
      server.send(200, "text/plain", "OK");
    } else if (cmd == "stop") {
      sim_stop("stopped from the web page");
      server.send(200, "text/plain", "OK");
    } else if (cmd == "set") {
      float val = server.hasArg("val") ? server.arg("val").toFloat() : NAN;
      if (sim_set_manual(server.arg("sig").toInt(), val)) server.send(200, "text/plain", "OK");
      else server.send(409, "text/plain", "The simulator is not running in manual mode");
    } else server.send(400, "text/plain", "Unknown command");
  });
  server.on("/simulator", HTTP_GET, []() {
    server.send_P(200, "text/html", SIM_HTML, strlen_P(SIM_HTML));
  });
  // ---- custom sensor slots (/custom) ----
  server.on("/custom", HTTP_GET, []() { server.send_P(200, "text/html", CUSTOM_HTML, strlen_P(CUSTOM_HTML)); });
  server.on("/custom.json", HTTP_GET, []() {
    String j = "[";
    for (int i = 0; i < CUSTOM_SLOTS; i++) {
      const CustomSlotConfig &c = s_slots[i];
      String nm = c.info.name, un = c.info.unit;
      nm.replace("\\", "\\\\"); nm.replace("\"", "\\\""); un.replace("\\", "\\\\"); un.replace("\"", "\\\"");
      char b[300];
      snprintf(b, sizeof b, "%s{\"i\":%d,\"name\":\"%s\",\"unit\":\"%s\",\"dec\":%u,\"min\":%g,\"max\":%g,\"src\":%u,\"id\":%lu,\"start\":%u,\"bits\":%u,\"bit\":%u,\"be\":%u,\"sgn\":%u,\"scale\":%g,\"offset\":%g,\"has\":%s,\"value\":%g}",
               i ? "," : "", i, nm.c_str(), un.c_str(), c.info.decimals, (double)c.info.min, (double)c.info.max, c.src, (unsigned long)c.can.id, c.can.start, c.can.bits, c.can.bit,
               c.can.big_endian, c.can.is_signed, (double)c.can.scale, (double)c.can.offset, gauge_config_signal_has_data((SignalId)(SIG_C1 + i)) ? "true" : "false", (double)g_custom_val[i]);
      j += b;
    }
    server.send(200, "application/json", j + "]");
  });
  server.on("/custom/set", HTTP_GET, []() {
    int i = server.arg("i").toInt();
    if (i < 0 || i >= CUSTOM_SLOTS) { server.send(400, "text/plain", "No such slot"); return; }
    if (s_slots[i].src == CUSTOM_SRC_INPUT) { server.send(400, "text/plain", "This slot is filled by a hub input: change it on the Inputs page"); return; }
    CustomSlotConfig c; memset(&c, 0, sizeof c);
    String name = server.arg("name"); name.trim();
    if (!name.length()) { server.send(400, "text/plain", "Give the sensor a name"); return; }
    custom_copy_text(c.info.name, CUSTOM_NAME_LEN, name.c_str());
    custom_copy_text(c.info.unit, CUSTOM_UNIT_LEN, server.arg("unit").c_str());
    int dec = server.arg("dec").toInt(); c.info.decimals = dec < 0 ? 0 : dec > 3 ? 3 : dec;
    c.info.min = server.arg("min").toFloat(); c.info.max = server.arg("max").toFloat();
    if (!(c.info.max > c.info.min)) { server.send(400, "text/plain", "Gauge max must be above gauge min"); return; }
    c.src = CUSTOM_SRC_CAN;
    c.can.id = strtoul(server.arg("id").c_str(), NULL, 0);
    c.can.start = server.arg("start").toInt(); c.can.bits = server.arg("bits").toInt(); c.can.bit = server.arg("bit").toInt();
    c.can.big_endian = server.arg("be") == "1"; c.can.is_signed = server.arg("sgn") == "1";
    c.can.scale = server.hasArg("scale") ? server.arg("scale").toFloat() : 1.0f; c.can.offset = server.arg("offset").toFloat();
    if (!custom_can_valid(&c.can)) { server.send(400, "text/plain", "That CAN field does not fit: check the ID (up to 0x7FF), start byte, size and scale (not 0)"); return; }
    for (int k = 0; k < CUSTOM_SLOTS; k++)
      if (k != i && s_slots[k].info.name[0] && !strcasecmp(s_slots[k].info.name, c.info.name)) { server.send(400, "text/plain", "Another slot already has that name"); return; }
    s_slots[i] = c; g_custom_has[i] = false; s_seenMs[SIG_C1 + i] = 0;
    slotsSave();
    server.send(200, "text/plain", "Saved. The screens get the name within a few seconds.");
  });
  server.on("/custom/clear", HTTP_GET, []() {
    int i = server.arg("i").toInt();
    if (i < 0 || i >= CUSTOM_SLOTS) { server.send(400, "text/plain", "No such slot"); return; }
    if (s_slots[i].src == CUSTOM_SRC_INPUT) { server.send(400, "text/plain", "This slot is filled by a hub input: remove it on the Inputs page"); return; }
    memset(&s_slots[i], 0, sizeof s_slots[i]); g_custom_has[i] = false; s_seenMs[SIG_C1 + i] = 0;
    slotsSave();
    server.send(200, "text/plain", "Cleared");
  });
  // ---- 433 MHz listen mode (/rf) ----
  server.on("/rf", HTTP_GET, []() { server.send_P(200, "text/html", RF_HTML, strlen_P(RF_HTML)); });
  server.on("/rf.json", HTTP_GET, []() {
    String j;
    j.reserve(12000);
    char b[320];
    snprintf(b, sizeof b, "{\"ok\":%s,\"part\":%u,\"version\":%u,\"ook\":%s,\"thr\":%d,\"auto\":%s,\"margin\":%d,\"floor\":%d,\"active\":%d,\"rssi\":%d,\"total\":%lu,\"hist\":[",
             s_rfOk ? "true" : "false", s_rfPart, s_rfVersion, s_rfOok ? "true" : "false", s_rfThr, s_rfAuto ? "true" : "false", s_rfMargin,
             (int)s_rfFloor, (int)s_rfActiveThr, (int)s_rfRssi, (unsigned long)s_rfBurstTotal);
    j += b;
    for (int k = s_rfHistCount - 1; k >= 0; k--) {   // oldest first: [peak, background] per second
      int i = (s_rfHistHead - 1 - k + RF_HIST) % RF_HIST;
      j += "["; j += String((int)s_rfHistPeak[i]); j += ","; j += String((int)s_rfHistFloor[i]); j += k ? "]," : "]";
    }
    j += "],\"bursts\":[";
    // newest bursts, then the strongest ones; only the first 48 pulses here (/rf/burst gives all of them)
    static RfBurst *copy = (RfBurst *)calloc(RF_BURSTS + RF_TOP, sizeof(RfBurst));   // heap: static RAM is full
    if (!copy || !s_rfBursts) { server.send(200, "application/json", j + "],\"top\":[]}"); return; }
    portENTER_CRITICAL(&s_rfMux);
    int count = s_rfBurstCount, head = s_rfBurstHead, top = s_rfTopCount;
    for (int k = 0; k < count; k++) copy[k] = s_rfBursts[(head - 1 - k + RF_BURSTS) % RF_BURSTS];
    for (int k = 0; k < top; k++) copy[count + k] = s_rfTop[k];
    portEXIT_CRITICAL(&s_rfMux);
    auto one = [&](const RfBurst &r, bool first) {
      snprintf(b, sizeof b, "%s{\"ms\":%lu,\"age\":%lu,\"rssi\":%d,\"pulses\":%u,\"unit\":%u,\"nbits\":%u,\"inv\":%s,\"first\":%u,\"bits\":\"",
               first ? "" : ",", (unsigned long)r.ms, (unsigned long)(millis() - r.ms), r.rssi, r.pulses, r.unit, r.nbits, r.inverted ? "true" : "false", r.firstLevel);
      j += b; j += r.bits; j += "\",\"hex\":\""; j += r.hex; j += "\",\"w\":[";
      for (int i = 0; i < 48 && i < r.nw; i++) { if (i) j += ","; j += String(r.w[i]); }
      j += "]}";
    };
    for (int k = 0; k < count; k++) one(copy[k], k == 0);
    j += "],\"top\":[";
    for (int k = 0; k < top; k++) one(copy[count + k], k == 0);
    server.send(200, "application/json", j + "]}");
  });
  // all pulses of one of the strongest bursts (?ms= its time), as plain text for copying
  server.on("/rf/burst", HTTP_GET, []() {
    uint32_t ms = strtoul(server.arg("ms").c_str(), NULL, 10);
    static RfBurst r; bool found = false;
    portENTER_CRITICAL(&s_rfMux);
    for (int k = 0; k < s_rfTopCount && !found; k++) if (s_rfTop[k].ms == ms) { r = s_rfTop[k]; found = true; }
    for (int k = 0; k < s_rfBurstCount && !found; k++) if (s_rfBursts[k].ms == ms) { r = s_rfBursts[k]; found = true; }
    portEXIT_CRITICAL(&s_rfMux);
    if (!found) { server.send(404, "text/plain", "That burst is no longer kept"); return; }
    String t; t.reserve(r.nw * 5 + 100);
    t += "rssi "; t += String((int)r.rssi); t += " dBm, "; t += String((int)r.pulses); t += " pulses, first level "; t += String((int)r.firstLevel); t += ", ";   // String(int): += on an int8_t would add a character t += s_rfOok ? "OOK" : "2-FSK"; t += "\n";
    for (int i = 0; i < r.nw; i++) { t += String(r.w[i]); t += (i % 20 == 19) ? "\n" : " "; }
    server.send(200, "text/plain", t);
  });
  server.on("/rf/clear", HTTP_GET, []() {
    portENTER_CRITICAL(&s_rfMux); s_rfTopCount = 0; s_rfBurstCount = 0; portEXIT_CRITICAL(&s_rfMux);
    server.send(200, "text/plain", "Cleared");
  });
  server.on("/rf/set", HTTP_GET, []() {
    if (server.hasArg("mod")) s_rfOok = server.arg("mod") == "ook";
    if (server.hasArg("auto")) s_rfAuto = server.arg("auto") == "1";
    if (server.hasArg("margin")) { int m = server.arg("margin").toInt(); if (m < 3 || m > 40) { server.send(400, "text/plain", "Margin must be 3 to 40 dB"); return; } s_rfMargin = m; }
    if (server.hasArg("thr")) { int t = server.arg("thr").toInt(); if (t < -110 || t > -30) { server.send(400, "text/plain", "Threshold must be -110 to -30 dBm"); return; } s_rfThr = t; }
    rfSaveSettings(); s_rfReconfig = true;
    server.send(200, "text/plain", "Saved");
  });
  // ---- buttons (/buttons) ----
  server.on("/buttons", HTTP_GET, []() { server.send_P(200, "text/html", BUTTONS_HTML, strlen_P(BUTTONS_HTML)); });
  server.on("/buttons.json", HTTP_GET, []() {
    String j = "{\"actions\":[";
    for (int a = 0; a < BA_COUNT; a++) { if (a) j += ","; j += "\""; j += BA_NAMES[a]; j += "\""; }
    j += "],\"colors\":[";
    for (int c = 0; c < COLOR_PRESET_COUNT; c++) { if (c) j += ","; j += "\""; j += COLOR_PRESETS_C[c].name; j += "\""; }
    j += "],\"targets\":[";
    for (int t = 0; t < HBT_COUNT; t++) { if (t) j += ","; j += "\""; j += HBT_NAMES[t]; j += "\""; }
    j += "],\"race\":"; j += s_raceOn ? "true" : "false";
    j += ",\"chosen\":\""; j += s_target; j += "\",\"buttons\":[";
    for (int i = 0; i < HUB_BUTTONS; i++) {
      const HubButton &b = s_btns[i]; char t[200];
      snprintf(t, sizeof t, "%s{\"kind\":%u,\"pin\":%u,\"high\":%u,\"a\":%u,\"p\":%u,\"t\":%u,\"mv\":%u,\"name\":\"%s\",\"down\":%s,\"now\":%d}",
               i ? "," : "", b.kind, b.pin, b.active_high, b.action, b.param, b.target, b.mv, b.name, s_btnDown[i] ? "true" : "false", (int)s_btnMv[i]);
      j += t;
    }
    // every pin a button could use, with what uses it now: [pin, caps, user]
    j += "],\"pins\":[";
    bool first = true;
    auto add = [&](int pin, int caps) { if (!first) j += ","; first = false; j += "["; j += String(pin); j += ","; j += String(caps); j += ",\""; j += pinUser(pin, -1); j += "\"]"; };
    { const int list[] = GPIO_LIST; for (int g : list) add(g, gpio_caps(g)); }
    for (int n = 0; n < 32; n++) if (s_mcpAddr[n / 16]) add(PIN_MCP_BASE + n, CAP_IN | CAP_OUT | CAP_PULLUP);
    for (int n = 0; n < 16; n++) if (s_adsAddr[n / 4]) add(PIN_ADS_BASE + n, CAP_ADC);
    j += "],\"ecu\":{\"id\":"; j += String((int)s_ecuMap.id); j += ",\"count\":"; j += String((int)s_ecuMap.count); j += ",\"cur\":"; j += String((int)s_ecuMap.cur);
    j += ",\"used\":"; j += s_ecuMapUsed ? "true" : "false"; j += ",\"names\":[";
    for (int m = 0; m < ECU_MAPS; m++) { if (m) j += ","; j += "\""; j += s_ecuMap.names[m]; j += "\""; }
    server.send(200, "application/json", j + "]}}");
  });
  server.on("/buttons/set", HTTP_GET, []() {
    int i = server.arg("i").toInt(), kind = server.arg("kind").toInt(), pin = server.arg("pin").toInt();
    int a = server.arg("a").toInt(), prm = server.arg("p").toInt(), tg = server.arg("t").toInt(), mv = server.arg("mv").toInt();
    if (i < 0 || i >= HUB_BUTTONS || kind < HB_DIGITAL || kind > HB_LADDER || a < 0 || a >= BA_COUNT || prm < 0 || prm > 255 || tg < 0 || tg >= HBT_COUNT) { server.send(400, "text/plain", "Bad setting"); return; }
    if (!hb_pin_fits(kind, pin)) { server.send(400, "text/plain", kind == HB_LADDER ? "A button ladder needs an analog pin: GPIO 32-36, 39 or an ADS1115 channel" : "That pin cannot read a button"); return; }
    if (pin_is_mcp(pin) && !s_mcpAddr[(pin - PIN_MCP_BASE) / 16]) { server.send(400, "text/plain", "That expander was not found"); return; }
    if (pin_is_ads(pin) && !adsFound(pin)) { server.send(400, "text/plain", "That ADS1115 was not found"); return; }
    String user = pinUser(pin, -1, -1, i, kind == HB_LADDER);
    if (user.length()) { server.send(400, "text/plain", "That pin is already used by " + user); return; }
    if (kind == HB_LADDER) {
      if (mv < 1 || mv > 3300) { server.send(400, "text/plain", "Learn the button's voltage first (hold it and press Learn)"); return; }
      int c = hb_ladder_clash(s_btns, HUB_BUTTONS, pin, mv, i);
      if (c >= 0) { server.send(400, "text/plain", String("Too close to button ") + (c + 1) + " (" + s_btns[c].name + ", " + s_btns[c].mv + " mV): the two must differ by " + LADDER_MIN_GAP_MV + " mV or more. Use another resistor."); return; }
    }
    if ((a == BA_GOTO_PAGE || a == BA_RACE) && (prm < 1 || prm > 8)) { server.send(400, "text/plain", "Page must be 1 to 8"); return; }
    if (a == BA_COLOR && prm >= COLOR_PRESET_COUNT && prm != COLOR_PRESET_CYCLE) { server.send(400, "text/plain", "Pick a color scheme"); return; }
    if (a == BA_OUT_TOGGLE && (prm < 1 || prm > HUB_OUTPUTS)) { server.send(400, "text/plain", "Output number must be 1 to 16"); return; }
    String nm = server.arg("name"); nm.trim();
    if (!nm.length()) { server.send(400, "text/plain", "Give the button a name"); return; }
    HubButton b; hb_defaults(&b);
    b.kind = kind; b.pin = pin; b.active_high = server.arg("high") == "1"; b.action = a; b.param = prm; b.target = tg; b.mv = kind == HB_LADDER ? mv : 0;
    int w = 0; for (unsigned c = 0; c < nm.length() && w < 15; c++) { char ch = nm[c]; if (ch >= 32 && ch < 127 && ch != '"' && ch != '\\') b.name[w++] = ch; } b.name[w] = 0;
    s_btns[i] = b; buttonsSave(); buttonsReset();
    server.send(200, "text/plain", String("Button ") + (i + 1) + " saved");
  });
  server.on("/action", HTTP_GET, []() {   // run a button action from a page (the Dashboard's quick controls)
    int a = server.arg("a").toInt(), prm = server.arg("p").toInt(), tg = server.arg("t").toInt();
    if (a < 1 || a >= BA_COUNT || tg < 0 || tg >= HBT_COUNT) { server.send(400, "text/plain", "Bad action"); return; }
    if ((a == BA_GOTO_PAGE || a == BA_RACE) && (prm < 1 || prm > 8)) prm = 1;
    if (a == BA_OUT_TOGGLE && (prm < 1 || prm > HUB_OUTPUTS)) { server.send(400, "text/plain", "Output number must be 1 to 16"); return; }
    runAction(a, prm, tg);
    String r = BA_NAMES[a];
    if (a == BA_ECU_MAP) { char lb[LINK_NAME_LEN + 1]; ecu_map_label(&s_ecuMap, lb, sizeof lb); r = lb; }
    if (a == BA_RACE) r = s_raceOn ? "Race mode on" : "Race mode off";
    server.send(200, "text/plain", r);
  });
  // ---- settings backup and factory reset (factory setup) ----
  // ---- Stock gauges page (aircore board) ----
  server.on("/aircore", HTTP_GET, []() { server.send_P(200, "text/html", AIRCORE_HTML, strlen_P(AIRCORE_HTML)); });
  server.on("/aircore.json", HTTP_GET, []() {
    AcSlot c[AC_SLOTS];
    portENTER_CRITICAL(&s_acMux); memcpy(c, s_ac, sizeof c); portEXIT_CRITICAL(&s_acMux);
    String j = "{\"gauges\":[";
    bool first = true; uint32_t now = millis();
    for (int i = 0; i < AC_SLOTS; i++) {
      if (!c[i].used || now - c[i].lastMs > 60000) continue;
      const AcStatus &t = c[i].st; char b[460];
      snprintf(b, sizeof b, "%s{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"g\":%u,\"age\":%lu,\"build\":%u,\"gauges\":%u,\"name\":\"%s\",\"mode\":%u,"
               "\"slow\":%u,\"hub\":%u,\"has\":%u,\"pwm\":%lu,\"needle\":%.1f,\"value\":%.1f,\"shown\":%.1f,\"sweepS\":%.1f,\"strength\":%.2f,"
               "\"slew\":%.0f,\"min\":%g,\"max\":%g,\"zero\":%.1f,\"span\":%.1f,\"ch\":%u,\"seq\":%u,\"up\":%lu,\"pts\":[",
               first ? "" : ",", c[i].mac[0], c[i].mac[1], c[i].mac[2], c[i].mac[3], c[i].mac[4], c[i].mac[5], t.gauge, (unsigned long)(now - c[i].lastMs),
               t.build, t.gauges, t.sig == SIG_RPM ? "tach" : t.sig == SIG_VSS ? "speedo" : "gauge", t.mode, t.slow, t.hubHeard, t.has, (unsigned long)t.pwmHz,
               t.needle, t.value, t.target, t.sweepS, t.strength, t.slew, t.minV, t.maxV, t.zero, t.span, t.ch, t.lastSeq, (unsigned long)t.upS);
      j += b; first = false;
      for (int k = 0; k < t.npts && k < AC_POINTS; k++) { snprintf(b, sizeof b, "%s[%g,%.1f]", k ? "," : "", t.pv[k], t.pa[k]); j += b; }
      j += "]}";
    }
    j += "],\"rx\":" + String(s_acRx) + ",\"short\":" + String(s_acShort) + "}";
    server.send(200, "application/json", j);
  });
  server.on("/aircore/cmd", HTTP_GET, []() {   // mac=AA:BB:..&g=0&op=N&a=..&b=..
    AcCmd c; memset(&c, 0, sizeof c); c.magic = AC_CMD_MAGIC;
    unsigned m[6];
    if (sscanf(server.arg("mac").c_str(), "%x:%x:%x:%x:%x:%x", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) != 6) { server.send(400, "text/plain", "Which board?"); return; }
    for (int i = 0; i < 6; i++) c.mac[i] = (uint8_t)m[i];
    c.gauge = (uint8_t)server.arg("g").toInt(); c.op = (uint8_t)server.arg("op").toInt();
    if (c.op < AC_OP_LIVE || c.op > AC_OP_SPAN) { server.send(400, "text/plain", "Unknown command"); return; }
    c.a = server.arg("a").toFloat(); c.b = server.arg("b").toFloat();
    static uint16_t seq = 0; if (++seq == 0) seq = 1;
    c.seq = seq;
    for (int i = 0; i < 3; i++) { esp_now_send(BCAST, (const uint8_t *)&c, sizeof c); if (i < 2) delay(8); }   // broadcast has no delivery check
    server.send(200, "text/plain", String("Sent ") + seq);
  });
  server.on("/backup", HTTP_GET, []() { server.send_P(200, "text/html", BACKUP_HTML, strlen_P(BACKUP_HTML)); });
  server.on("/backup.txt", HTTP_GET, []() {
    String out = String("# msp-dash hub build ") + INPUT_BUILD + "\n";
    out += bk_export(HUB_BK_NS, sizeof HUB_BK_NS / sizeof *HUB_BK_NS, HUB_BK_SECRET, 1, server.arg("pw") == "1");
    server.sendHeader("Content-Disposition", "attachment; filename=\"msp-dash-hub.txt\"");
    server.send(200, "text/plain", out);
  });
  server.on("/restore", HTTP_POST, []() {
    String body = server.arg("plain");
    if (!body.length()) { server.send(400, "text/plain", "Nothing received"); return; }
    int bad = bk_check(body.c_str(), HUB_BK_NS, sizeof HUB_BK_NS / sizeof *HUB_BK_NS, NULL, 0);
    if (bad) { server.send(400, "text/plain", "Not restored: line " + String(bad) + " is not a setting this hub knows. Nothing was changed."); return; }
    char *b = strdup(body.c_str());
    bool ok = b && bk_apply(b, HUB_BK_NS, sizeof HUB_BK_NS / sizeof *HUB_BK_NS, NULL, 0, NULL);
    free(b);
    server.send(ok ? 200 : 500, "text/plain", ok ? "Restored. The hub restarts." : "Restored with errors; the hub restarts.");
    delay(500); ESP.restart();
  });
  server.on("/factory", HTTP_POST, []() {
    if (server.arg("confirm") != "yes") { server.send(400, "text/plain", "Not confirmed"); return; }
    server.send(200, "text/plain", "Factory reset: every hub setting is erased and the hub restarts.");
    delay(500); hubFactoryReset();
  });
  server.on("/boards", HTTP_GET, []() { server.send_P(200, "text/html", BOARDS_HTML, strlen_P(BOARDS_HTML)); });
  server.on("/boards.json", HTTP_GET, []() {   // every board's numbers: the hub's own, then each screen from its hello
    uint8_t cpu[2]; cpu_load_pct(cpu);
    twai_status_info_t ts; bool canOk = twai_get_status_info(&ts) == ESP_OK;
    const char *canState = !canOk ? "off" : ts.state == TWAI_STATE_RUNNING ? "running" : ts.state == TWAI_STATE_BUS_OFF ? "bus off" : ts.state == TWAI_STATE_RECOVERING ? "recovering" : "stopped";
    char t[600];
    snprintf(t, sizeof t, "{\"hub\":{\"name\":\"Hub\",\"build\":%d,\"ip\":\"%s\",\"uptime\":%lu,\"mhz\":%lu,\"cpu0\":%u,\"cpu1\":%u,\"heap\":%u,\"heapMin\":%u,\"heapBig\":%u,"
             "\"temp\":%d,\"rssi\":%d,\"loops\":%lu,\"canFps\":%lu,\"canState\":\"%s\",\"canRxMissed\":%lu,\"canBusErr\":%lu,\"canTxFail\":%lu,\"sentPs\":%lu,\"apClients\":%d},\"screens\":[",
             INPUT_BUILD, (s_wifiUp ? WiFi.localIP() : WiFi.softAPIP()).toString().c_str(), (unsigned long)(millis() / 1000), (unsigned long)getCpuFrequencyMhz(), cpu[0], cpu[1],
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024), (unsigned)(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) / 1024),
             (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024), (int)temperatureRead(), s_wifiUp ? (int)WiFi.RSSI() : 0,
             (unsigned long)s_rate.loops, (unsigned long)s_rate.canFps, canState, canOk ? (unsigned long)ts.rx_missed_count : 0ul, canOk ? (unsigned long)ts.bus_error_count : 0ul,
             canOk ? (unsigned long)ts.tx_failed_count : 0ul, (unsigned long)s_rate.sentPs, s_apOn ? WiFi.softAPgetStationNum() : 0);
    String j = t;
    ScreenSlot copy[SCREEN_MAX];
    portENTER_CRITICAL(&s_scrMux); memcpy(copy, s_screens, sizeof copy); portEXIT_CRITICAL(&s_scrMux);
    bool first = true;
    for (int i = 0; i < SCREEN_MAX; i++) {
      const ScreenSlot &c = copy[i];
      if (!c.used) continue;
      const HelloStats &st = c.st;
      IPAddress ip(c.ip);
      snprintf(t, sizeof t, "%s{\"name\":\"%s\",\"build\":%u,\"ip\":\"%s\",\"age\":%lu,\"online\":%s,\"uptime\":%lu,\"stats\":%s,\"fps\":%u,\"draw\":%u,\"cpu0\":%u,\"cpu1\":%u,"
               "\"heap\":%u,\"heapMin\":%u,\"heapBig\":%u,\"psram\":%u,\"temp\":%d,\"rssi\":%d,\"rxPs\":%u,\"rxAge\":%d,\"logging\":%s,\"mhz\":%u}",
               first ? "" : ",", c.name, c.build, ip.toString().c_str(), (unsigned long)(millis() - c.lastMs), millis() - c.lastMs < SCREEN_OFFLINE_MS ? "true" : "false",
               (unsigned long)c.uptime, st.stats_ver ? "true" : "false", st.fps, st.draw_pct, st.cpu0, st.cpu1, st.heap_kb, st.heap_min_kb, st.heap_big_kb, st.psram_kb,
               st.temp_c, st.rssi, st.rx_pps, st.rx_age_ds == 0xFFFF ? -1 : (int)st.rx_age_ds, st.logging ? "true" : "false", st.cpu_mhz_4 * 4);
      j += t; first = false;
    }
    server.send(200, "application/json", j + "]}");
  });
  server.on("/wifi.json", HTTP_GET, []() {   // never sends the password
    String j = "{\"home\":"; j += s_wifiUp ? "true" : "false";
    j += ",\"homeSsid\":\""; j += WIFI_SSID; j += "\",\"ip\":\""; j += s_wifiUp ? WiFi.localIP().toString() : String("");
    j += "\",\"mode\":"; j += String((int)s_ap.mode); j += ",\"ssid\":\""; j += s_ap.ssid;
    j += "\",\"defaultPass\":"; j += strcmp(s_ap.pass, AP_PASS_DEFAULT) ? "false" : "true";
    j += ",\"apOn\":"; j += s_apOn ? "true" : "false"; j += ",\"apIp\":\""; j += s_apOn ? WiFi.softAPIP().toString() : String("");
    j += "\",\"clients\":"; j += String(s_apOn ? WiFi.softAPgetStationNum() : 0); j += ",\"ch\":"; j += String(WiFi.channel());
    j += ",\"modes\":[";
    for (int m = 0; m < AP_MODES; m++) { if (m) j += ","; j += "\""; j += AP_MODE_NAMES[m]; j += "\""; }
    server.send(200, "application/json", j + "]}");
  });
  server.on("/wifi/set", HTTP_GET, []() {   // used from the next start (or Save and restart)
    ApCfg c = s_ap;
    c.mode = server.arg("mode").toInt();
    String ss = server.arg("ssid"), pw = server.arg("pass"); ss.trim();
    if (ss.length() > 32) { server.send(400, "text/plain", "Network name: 1 to 32 characters"); return; }
    strncpy(c.ssid, ss.c_str(), sizeof c.ssid - 1); c.ssid[sizeof c.ssid - 1] = 0;
    if (pw.length()) { if (pw.length() > 63) { server.send(400, "text/plain", "Password: 8 to 63 characters"); return; } strncpy(c.pass, pw.c_str(), sizeof c.pass - 1); c.pass[sizeof c.pass - 1] = 0; }
    if (c.mode >= AP_MODES) { server.send(400, "text/plain", "Pick when it is on"); return; }
    if (!ap_text_ok(c.ssid, 1, 32)) { server.send(400, "text/plain", "Network name: 1 to 32 plain characters"); return; }
    if (!ap_text_ok(c.pass, 8, 63)) { server.send(400, "text/plain", "Password: 8 to 63 plain characters"); return; }
    s_ap = c; apSave();
    bool restart = server.arg("restart") == "1";
    server.send(200, "text/plain", restart ? "Saved. The hub restarts now; reconnect in about 20 s." : "Saved. Used from the next start.");
    if (restart) { delay(300); ESP.restart(); }
  });
  server.on("/shift.json", HTTP_GET, []() {
    char t[200];
    snprintf(t, sizeof t, "{\"on\":%s,\"start\":%u,\"shift\":%u,\"over\":%u,\"order\":\"%s\",\"level\":%d,\"rpm\":%d,\"test\":%s}", s_shift.on ? "true" : "false",
             s_shift.start, s_shift.shift, s_shift.over, s_shiftOrder, s_shiftLevel, (int)g_rpm, s_shiftTestMs ? "true" : "false");
    server.send(200, "application/json", t);
  });
  server.on("/shift/set", HTTP_GET, []() {
    ShiftCfg c; shift_defaults(&c);
    c.on = server.arg("on") == "1"; c.start = server.arg("start").toInt(); c.shift = server.arg("shift").toInt(); c.over = server.arg("over").toInt();
    if (!shift_valid(&c)) { server.send(400, "text/plain", "Start must be below shift (500 to 12000 RPM), and over-rev above shift or 0 for off"); return; }
    s_shift = c; shiftSave();
    server.send(200, "text/plain", c.on ? "Shift light saved" : "Saved, shift light off");
  });
  server.on("/shift/test", HTTP_GET, []() { s_shiftTestMs = millis(); server.send(200, "text/plain", "Testing: watch the screens for 4 s"); });
  server.on("/buttons/clear", HTTP_GET, []() {
    int i = server.arg("i").toInt();
    if (i < 0 || i >= HUB_BUTTONS) { server.send(400, "text/plain", "Bad button"); return; }
    hb_defaults(&s_btns[i]); buttonsSave(); buttonsReset();
    server.send(200, "text/plain", String("Button ") + (i + 1) + " removed");
  });
  server.on("/buttons/read", HTTP_GET, []() {   // a ladder pin's voltage now (for Learn): 8 reads averaged
    int pin = server.arg("pin").toInt();
    if (!hb_pin_fits(HB_LADDER, pin) || (pin_is_ads(pin) && !adsFound(pin))) { server.send(400, "text/plain", "Not an analog pin"); return; }
    uint32_t sum = 0;
    for (int k = 0; k < 8; k++) { sum += adcMilliVolts(pin); delay(pin_is_ads(pin) ? 25 : 2); }
    server.send(200, "text/plain", String((sum + 4) / 8));
  });
  server.on("/buttons/ecu", HTTP_GET, []() {   // the ECU maps: CAN id, how many, names
    int id = (int)strtol(server.arg("id").c_str(), NULL, 0), n = server.arg("count").toInt();
    if (id < 1 || id > 0x7FF) { server.send(400, "text/plain", "CAN id must be 0x001 to 0x7FF"); return; }
    if (n < 1 || n > ECU_MAPS) { server.send(400, "text/plain", "1 to 8 maps"); return; }
    s_ecuMap.id = id; s_ecuMap.count = n; if (s_ecuMap.cur >= n) s_ecuMap.cur = 0;
    for (int m = 0; m < ECU_MAPS; m++) {
      String nm = server.arg(String("n") + m); nm.trim(); int w = 0;
      for (unsigned c = 0; c < nm.length() && w < ECU_MAP_NAME - 1; c++) { char ch = nm[c]; if (ch >= 32 && ch < 127 && ch != '"' && ch != '\\') s_ecuMap.names[m][w++] = ch; }
      s_ecuMap.names[m][w] = 0;
    }
    ecuMapSave(); s_ecuMapSendNow = true;
    server.send(200, "text/plain", "ECU maps saved");
  });
  // ---- clock from the GPS (/time.json, /time/set) ----
  server.on("/time.json", HTTP_GET, []() {
    uint32_t u = gpsUtcNow(); char t[24] = "";
    if (u) gt_format(u, tzOffsetNow(u), 0, t, sizeof t);
    char b[140];
    snprintf(b, sizeof b, "{\"known\":%s,\"local\":\"%s\",\"std\":%d,\"dst\":%s}", u ? "true" : "false", t, s_tzStd, s_tzDst ? "true" : "false");
    server.send(200, "application/json", b);
  });
  server.on("/time/set", HTTP_GET, []() {
    int std = server.arg("std").toInt();
    if (std < -720 || std > 840) { server.send(400, "text/plain", "Offset must be -12:00 to +14:00"); return; }
    s_tzStd = std; s_tzDst = server.arg("dst") == "1"; tzSave();
    server.send(200, "text/plain", "Saved");
  });
  // ---- OLED warning panel (/oled) ----
  server.on("/oled", HTTP_GET, []() { server.send_P(200, "text/html", OLED_HTML, strlen_P(OLED_HTML)); });   // ~30 KB with the icon list: sent straight from flash, not copied into one big String first
  server.on("/oled.bin", HTTP_GET, []() { server.send_P(200, "application/octet-stream", (const char *)s_oledBuf, sizeof s_oledBuf); });
  server.on("/oled.json", HTTP_GET, []() {
    String j = "{\"found\":"; j += s_oledOk ? "true" : "false";
    j += ",\"mcp\":["; j += String((int)s_mcpAddr[0]); j += ","; j += String((int)s_mcpAddr[1]); j += "]";   // addresses, 0 = none
    j += ",\"layout\":"; j += String((int)s_oled.layout);
    j += ",\"inusers\":{";   // what uses each input-wire pin (the shared pin list), only the used ones
    { bool first = true;
      auto add = [&](int code) { String u = pinUser(lampPin(code), -1); if (!u.length()) return; if (!first) j += ","; first = false; j += "\""; j += String(code); j += "\":\""; j += u; j += "\""; };
      const int list[] = GPIO_LIST; for (int g : list) add(OLED_IN_GPIO_BASE + g);
      for (int n = 0; n < 32; n++) add(OLED_IN_MCP0 + n); }
    j += "},\"lamps\":[";
    for (int i = 0; i < OLED_LAMPS; i++) {
      const OledLamp &l = s_oled.lamp[i];
      char b[220]; snprintf(b, sizeof b, "%s{\"icon\":%u,\"sig\":%u,\"op\":%u,\"blink\":%u,\"level\":%g,\"danger\":%g,\"has_danger\":%u,\"input\":%u,\"input_low\":%u,\"state\":%u,\"in_low\":%s}",
                            i ? "," : "", l.icon, l.sig, l.op, l.blink, (double)l.level, (double)l.danger, l.has_danger, l.input, l.input_low,
                            s_oledState[i], (l.op == OLED_INPUT && inputLow(l.input)) ? "true" : "false");
      j += b;
    }
    server.send(200, "application/json", j + "]}");
  });
  server.on("/oled/set", HTTP_GET, []() {
    int i = server.arg("i").toInt(), icon = server.arg("icon").toInt(), sig = server.arg("sig").toInt(), op = server.arg("op").toInt(), in = server.arg("input").toInt();
    if (i < 0 || i >= OLED_LAMPS || icon < 0 || icon > OLED_ICON_COUNT || sig < 0 || sig >= SIG_COUNT || op < 0 || op > OLED_INPUT) { server.send(400, "text/plain", "Bad setting"); return; }
    if (op == OLED_INPUT && !oled_input_valid(in)) { server.send(400, "text/plain", "Pick an input"); return; }
    if (op == OLED_INPUT && icon) { String u = pinUser(lampPin(in), -1, -1, -1, false, i); if (u.length()) { server.send(400, "text/plain", "That pin is already used by " + u); return; } }
    OledLamp &l = s_oled.lamp[i];
    l.icon = icon; l.sig = sig; l.op = op; l.blink = server.arg("blink") == "1"; l.level = server.arg("level").toFloat();
    l.danger = server.arg("danger").toFloat(); l.has_danger = server.arg("has_danger") == "1"; l.input = op == OLED_INPUT ? in : 0; l.input_low = server.arg("input_low") != "0";
    oledSave(); server.send(200, "text/plain", "Saved");
  });
  server.on("/oled/opts", HTTP_GET, []() {
    if (server.hasArg("layout")) s_oled.layout = server.arg("layout") == "1" ? OLED_COMPACT : OLED_FIXED;
    oledSave(); server.send(200, "text/plain", "Saved");
  });
  // ---- OLED pages ----
  server.on("/oled/pages.json", HTTP_GET, []() {
    String j = "{\"count\":"; j += String((int)s_pages.count); j += ",\"cycle\":"; j += String((int)s_pages.cycle_s);
    j += ",\"takeover\":"; j += s_pages.takeover ? "true" : "false"; j += ",\"chosen\":"; j += String((int)s_oledPage);
    j += ",\"shown\":"; j += String((int)s_oledShown); j += ",\"pages\":[";
    for (int i = 0; i < s_pages.count; i++) {
      const OledPage &g = s_pages.page[i];
      char b[220];
      snprintf(b, sizeof b, "%s{\"type\":%u,\"count\":%u,\"sig\":[%u,%u,%u,%u],\"dec\":[%u,%u,%u,%u],\"label\":[\"%s\",\"%s\",\"%s\",\"%s\"]}", i ? "," : "",
               g.type, g.count, g.sig[0], g.sig[1], g.sig[2], g.sig[3], g.dec[0], g.dec[1], g.dec[2], g.dec[3], g.label[0], g.label[1], g.label[2], g.label[3]);
      j += b;
    }
    server.send(200, "application/json", j + "]}");
  });
  server.on("/oled/page/set", HTTP_GET, []() {
    int i = server.arg("i").toInt(), type = server.arg("type").toInt(), n = server.arg("count").toInt();
    if (i < 0 || i >= s_pages.count || type < 0 || type >= OPG_TYPES) { server.send(400, "text/plain", "Bad page"); return; }
    OledPage &g = s_pages.page[i];
    g.type = type; g.count = n < 1 ? 1 : n > 4 ? 4 : n;
    for (int k = 0; k < 4; k++) {
      String ks = String(k), lab = server.arg("l" + ks);
      int sg = server.arg("s" + ks).toInt(), dc = server.arg("d" + ks).toInt();
      g.sig[k] = sg >= 0 && sg < SIG_COUNT ? sg : 0; g.dec[k] = dc < 0 ? 0 : dc > 2 ? 2 : dc;
      int w = 0;   // printable, no quotes or backslashes (they go into JSON), 9 characters
      for (unsigned c = 0; c < lab.length() && w < OLED_LABEL_LEN - 1; c++) { char ch = lab[c]; if (ch >= 32 && ch < 127 && ch != '"' && ch != '\\') g.label[k][w++] = ch; }
      g.label[k][w] = 0;
    }
    pagesSave(); server.send(200, "text/plain", "Saved");
  });
  server.on("/oled/page/add", HTTP_GET, []() {
    if (s_pages.count >= OLED_MAX_PAGES) { server.send(400, "text/plain", "6 pages at most"); return; }
    oled_page_set(&s_pages.page[s_pages.count], OPG_BIG, 1); oled_page_item(&s_pages.page[s_pages.count], 0, 1, 0, "RPM");
    s_pages.count++; pagesSave(); server.send(200, "text/plain", "Page added");
  });
  server.on("/oled/page/del", HTTP_GET, []() {
    int i = server.arg("i").toInt();
    if (i < 0 || i >= s_pages.count || s_pages.count <= 1) { server.send(400, "text/plain", "Keep at least one page"); return; }
    for (int k = i; k < s_pages.count - 1; k++) s_pages.page[k] = s_pages.page[k + 1];
    s_pages.count--; if (s_oledPage >= s_pages.count) s_oledPage = 0;
    pagesSave(); server.send(200, "text/plain", "Page removed");
  });
  server.on("/oled/page/move", HTTP_GET, []() {
    int i = server.arg("i").toInt(), j = i + (server.arg("d").toInt() < 0 ? -1 : 1);
    if (i < 0 || j < 0 || i >= s_pages.count || j >= s_pages.count) { server.send(400, "text/plain", "Cannot move there"); return; }
    OledPage t = s_pages.page[i]; s_pages.page[i] = s_pages.page[j]; s_pages.page[j] = t;
    pagesSave(); server.send(200, "text/plain", "Moved");
  });
  server.on("/oled/page/show", HTTP_GET, []() {
    int i = server.arg("i").toInt();
    if (i < 0 || i >= s_pages.count) { server.send(400, "text/plain", "Bad page"); return; }
    s_oledPage = i; s_oledPageMs = millis(); server.send(200, "text/plain", "Showing it");
  });
  server.on("/oled/pageopts", HTTP_GET, []() {
    int cy = server.arg("cycle").toInt();
    s_pages.cycle_s = cy < 0 ? 0 : cy > 60 ? 60 : cy; s_pages.takeover = server.arg("takeover") != "0";
    pagesSave(); server.send(200, "text/plain", "Saved");
  });
  server.on("/oled/pages/defaults", HTTP_GET, []() { oled_pages_defaults(&s_pages); s_oledPage = 0; pagesSave(); server.send(200, "text/plain", "Pages back to the defaults"); });
  server.on("/oled/peak/reset", HTTP_GET, []() { s_gPeak = 0; server.send(200, "text/plain", "Peak G cleared"); });
  server.on("/oled/defaults", HTTP_GET, []() { oled_defaults(&s_oled); oledSave(); server.send(200, "text/plain", "Back to the defaults"); });
  // ---- accelerometer (/imu.json, /imu/set, /imu/zero) ----
  server.on("/imu.json", HTTP_GET, []() {
    char b[360];
    snprintf(b, sizeof b, "{\"found\":%s,\"fwd\":%d,\"left\":%d,\"zeroed\":%s,\"zeroing\":%s,\"lat\":%.2f,\"lon\":%.2f,\"vert\":%.2f,\"yaw\":%.1f,\"raw\":[%.3f,%.3f,%.3f,%.2f]}",
             s_imuOk ? "true" : "false", s_imuCfg.fwd, s_imuCfg.left, s_imuCfg.zeroed ? "true" : "false", s_imuZeroLeft > 0 ? "true" : "false",
             (double)g_imu_val[0], (double)g_imu_val[1], (double)g_imu_val[2], (double)g_imu_val[3], (double)s_imuRaw[0], (double)s_imuRaw[1], (double)s_imuRaw[2], (double)s_imuRaw[3]);
    server.send(200, "application/json", b);
  });
  server.on("/imu/set", HTTP_GET, []() {
    int f = server.arg("fwd").toInt(), l = server.arg("left").toInt();
    if (!imu_axes_valid(f, l)) { server.send(400, "text/plain", "Forward and left must be two different axes"); return; }
    s_imuCfg.fwd = f; s_imuCfg.left = l; s_imuCfg.zeroed = 0; imuSave();   // a new mounting needs a new zero
    server.send(200, "text/plain", "Saved - now park on level ground and press Zero");
  });
  server.on("/imu/zero", HTTP_GET, []() {
    if (!s_imuOk) { server.send(400, "text/plain", "No accelerometer found"); return; }
    for (int i = 0; i < 4; i++) s_imuSum[i] = 0;
    s_imuZeroLeft = 50;
    server.send(200, "text/plain", "Zeroing: keep the car still for a second");
  });
  server.on("/i2c.json", HTTP_GET, []() {   // what the boot scan found
    String j = "{\"found\":[";
    for (int i = 0; s_i2cFound[i]; i++) { if (i) j += ","; j += String((int)s_i2cFound[i]); }
    server.send(200, "application/json", j + "]}");
  });
  // ---- Inputs page: every pin and what uses it; move a fixed function; set up inputs ----
  server.on("/inputs", HTTP_GET, []() { server.send_P(200, "text/html", INPUTS_HTML, strlen_P(INPUTS_HTML)); });
  server.on("/inputs.json", HTTP_GET, []() {
    String j = "{\"roles\":[";
    for (int r = 0; r < PR_COUNT; r++) {
      if (r) j += ",";
      j += "{\"key\":\""; j += HUB_PIN_ROLES[r].key; j += "\",\"name\":\""; j += HUB_PIN_ROLES[r].name;
      j += "\",\"gpio\":"; j += String((int)g_pins[r]); j += ",\"def\":"; j += String((int)HUB_PIN_ROLES[r].def); j += ",\"need\":"; j += String((int)HUB_PIN_ROLES[r].need);
      j += ",\"mcp\":"; j += pin_role_mcp_ok(r) ? "true" : "false"; j += "}";
    }
    j += "],\"gpios\":[";
    const int list[] = GPIO_LIST; bool first = true;
    for (int g : list) {
      if (!first) j += ","; first = false;
      j += "{\"g\":"; j += String(g); j += ",\"caps\":"; j += String((int)gpio_caps(g)); j += ",\"note\":\""; j += gpio_note(g);
      j += "\",\"user\":\""; j += pinUser(g, -1); j += "\",\"low\":"; j += (pin_map_role_at(g_pins, g) < 0 && digitalRead(g) == LOW) ? "true" : "false"; j += "}";
    }
    j += "],\"mcp\":["; j += String((int)s_mcpAddr[0]); j += ","; j += String((int)s_mcpAddr[1]); j += "],\"mcpbits\":["; j += String((int)s_mcpBits[0]); j += ","; j += String((int)s_mcpBits[1]);
    j += "],\"mcpusers\":[";
    for (int n = 0; n < 32; n++) { if (n) j += ","; j += "\""; j += pinUser(INPUT_MCP_BASE + n, -1); j += "\""; }
    j += "],\"ads\":[";
    for (int b = 0; b < ADS_BOARDS; b++) { if (b) j += ","; j += String((int)s_adsAddr[b]); }
    j += "],\"adsv\":[";
    for (int n = 0; n < 16; n++) { char v[16]; snprintf(v, sizeof v, "%s%.3f", n ? "," : "", (double)s_adsV[n]); j += v; }
    j += "],\"adsusers\":[";
    for (int n = 0; n < 16; n++) { if (n) j += ","; j += "\""; j += pinUser(PIN_ADS_BASE + n, -1); j += "\""; }
    j += "],\"inputs\":[";
    for (int i = 0; i < HUB_INPUTS; i++) {
      const HubInput &in = s_inputs[i]; char b[420];
      snprintf(b, sizeof b, "%s{\"type\":%u,\"pin\":%u,\"contact\":%u,\"slot\":%u,\"ap\":%u,\"pp\":%u,\"ah\":%u,\"ph\":%u,\"preset\":%u,"
               "\"mult\":%g,\"v1\":%g,\"val1\":%g,\"v2\":%g,\"val2\":%g,\"pullup\":%g,\"name\":\"%s\",\"active\":%s,\"value\":%.3f}",
               i ? "," : "", in.type, in.pin, in.contact, in.slot, in.act_press, in.prm_press, in.act_hold, in.prm_hold, in.preset,
               (double)in.mult, (double)in.v1, (double)in.val1, (double)in.v2, (double)in.val2, (double)in.pullup, in.name,
               s_inActive[i] ? "true" : "false", (double)s_inValue[i]);
      j += b;
    }
    j += "],\"outputs\":[";
    for (int i = 0; i < HUB_OUTPUTS; i++) {
      const HubOutput &o = s_outputs[i]; char b[260];
      snprintf(b, sizeof b, "%s{\"type\":%u,\"pin\":%u,\"low\":%u,\"blink\":%u,\"sig\":%u,\"cond\":%u,\"input\":%u,\"level\":%g,\"hyst\":%g,\"name\":\"%s\",\"on\":%s}",
               i ? "," : "", o.type, o.pin, o.active_low, o.blink, o.sig, o.cond, o.input, (double)o.level, (double)o.hyst, o.name, s_outOn[i] ? "true" : "false");
      j += b;
    }
    j += "],\"slots\":[";
    for (int k = 0; k < CUSTOM_SLOTS; k++) { if (k) j += ","; j += "{\"name\":\""; j += s_slots[k].info.name; j += "\",\"src\":"; j += String((int)s_slots[k].src); j += "}"; }
    j += "],\"presets\":[";
    for (int k = 0; k < PRE_COUNT; k++) { if (k) j += ","; j += "{\"name\":\""; j += INPUT_PRESETS[k].name; j += "\",\"unit\":\""; j += INPUT_PRESETS[k].unit; j += "\"}"; }
    j += "],\"actions\":[";
    for (int a = 0; a < BA_COUNT; a++) { if (a) j += ","; j += "\""; j += BA_NAMES[a]; j += "\""; }
    server.send(200, "application/json", j + "]}");
  });
  server.on("/inputs/set", HTTP_GET, []() {
    int i = server.arg("i").toInt(), type = server.arg("type").toInt(), pin = server.arg("pin").toInt(), slot = server.arg("slot").toInt();
    if (i < 0 || i >= HUB_INPUTS || type < IN_SWITCH || type > IN_ANALOG) { server.send(400, "text/plain", type == IN_BUTTON ? "Buttons are set up on the Buttons page" : "Bad input"); return; }
    if (!input_pin_fits(type, pin)) { server.send(400, "text/plain", type == IN_ANALOG ? "An analog sensor needs an analog pin: GPIO 32-36, 39 or an ADS1115 channel" : "That pin cannot be an input"); return; }
    if (pin_is_ads(pin) && !adsFound(pin)) { server.send(400, "text/plain", "That ADS1115 was not found"); return; }
    String user = pinUser(pin, i);
    if (user.length()) { server.send(400, "text/plain", "That pin is already used by " + user); return; }
    if (slot < 0 || slot >= CUSTOM_SLOTS) { server.send(400, "text/plain", "Pick a sensor slot"); return; }
    bool mine = s_inputs[i].type != IN_NONE && s_inputs[i].type != IN_BUTTON && s_inputs[i].slot == slot;
    if (!mine && s_slots[slot].info.name[0]) { server.send(400, "text/plain", String("Sensor slot ") + (slot + 1) + " is in use (" + s_slots[slot].info.name + ")"); return; }
    String nm = server.arg("name"); nm.trim();
    if (!nm.length()) { server.send(400, "text/plain", "Give the input a name"); return; }
    HubInput in; input_defaults(&in);
    in.type = type; in.pin = pin; in.contact = server.arg("contact") == "1" ? CONTACT_NC : CONTACT_NO; in.slot = slot;
    int pre = server.arg("preset").toInt(); in.preset = pre >= 0 && pre < PRE_COUNT ? pre : 0;
    if (server.hasArg("mult")) in.mult = server.arg("mult").toFloat();
    if (server.hasArg("v1")) { in.v1 = server.arg("v1").toFloat(); in.val1 = server.arg("val1").toFloat(); in.v2 = server.arg("v2").toFloat(); in.val2 = server.arg("val2").toFloat(); }
    if (server.hasArg("pullup")) in.pullup = server.arg("pullup").toFloat();
    if (type == IN_ANALOG && in.preset == PRE_CUSTOM && (in.v1 == in.v2 || !(in.mult > 0))) { server.send(400, "text/plain", "Custom sensor: the two voltages must differ and the multiplier must be above 0"); return; }
    int w = 0; for (unsigned c = 0; c < nm.length() && w < 15; c++) { char ch = nm[c]; if (ch >= 32 && ch < 127 && ch != '"' && ch != '\\') in.name[w++] = ch; } in.name[w] = 0;
    // the old slot (if it moved) goes back to free; the new one is named after the input
    const HubInput &old = s_inputs[i];
    if (old.type == IN_SWITCH || old.type == IN_ANALOG) if (old.slot != in.slot) { memset(&s_slots[old.slot], 0, sizeof s_slots[old.slot]); g_custom_has[old.slot] = false; }
    {
      CustomSlotConfig &c = s_slots[in.slot]; memset(&c, 0, sizeof c);
      custom_copy_text(c.info.name, CUSTOM_NAME_LEN, in.name);
      if (type == IN_SWITCH) { c.info.decimals = 0; c.info.min = 0; c.info.max = 1; }
      else { const InputPreset &p = INPUT_PRESETS[in.preset];
        custom_copy_text(c.info.unit, CUSTOM_UNIT_LEN, server.hasArg("unit") && in.preset == PRE_CUSTOM ? server.arg("unit").c_str() : p.unit);
        c.info.decimals = p.dec; c.info.min = in.preset == PRE_CUSTOM ? fminf(in.val1, in.val2) : p.min; c.info.max = in.preset == PRE_CUSTOM ? fmaxf(in.val1, in.val2) : p.max; }
      c.src = CUSTOM_SRC_INPUT; g_custom_has[in.slot] = false;
    }
    s_inputs[i] = in; s_inValue[i] = 0;
    inputPinSetup(in); inputsSave(); slotsSave();
    server.send(200, "text/plain", String("Input ") + (i + 1) + " saved");
  });
  server.on("/inputs/clear", HTTP_GET, []() {
    int i = server.arg("i").toInt();
    if (i < 0 || i >= HUB_INPUTS) { server.send(400, "text/plain", "Bad input"); return; }
    const HubInput &old = s_inputs[i];
    if (old.type == IN_SWITCH || old.type == IN_ANALOG) { memset(&s_slots[old.slot], 0, sizeof s_slots[old.slot]); g_custom_has[old.slot] = false; slotsSave(); }
    input_defaults(&s_inputs[i]); inputsSave();
    server.send(200, "text/plain", String("Input ") + (i + 1) + " removed");
  });
  server.on("/outputs/set", HTTP_GET, []() {
    int i = server.arg("i").toInt(), type = server.arg("type").toInt(), pin = server.arg("pin").toInt();
    if (i < 0 || i >= HUB_OUTPUTS || type < OUT_SIGNAL || type > OUT_MANUAL) { server.send(400, "text/plain", "Bad output"); return; }
    if (!output_pin_fits(pin)) { server.send(400, "text/plain", "That pin cannot drive an output (GPIO 34-39 are input only)"); return; }
    String user = pinUser(pin, -1, i);
    if (user.length()) { server.send(400, "text/plain", "That pin is already used by " + user); return; }
    if (pin_is_mcp(pin) && !s_mcpAddr[(pin - PIN_MCP_BASE) / 16]) { server.send(400, "text/plain", "That expander was not found"); return; }
    String nm = server.arg("name"); nm.trim();
    if (!nm.length()) { server.send(400, "text/plain", "Give the output a name"); return; }
    HubOutput o; output_defaults(&o);
    o.type = type; o.pin = pin; o.active_low = server.arg("low") == "1"; o.blink = server.arg("blink") == "1";
    int sg = server.arg("sig").toInt(), cd = server.arg("cond").toInt(), in = server.arg("input").toInt();
    if (type == OUT_SIGNAL && (sg < 1 || sg >= SIG_COUNT || cd < OUTC_ON || cd > OUTC_BELOW)) { server.send(400, "text/plain", "Pick a signal and when it switches on"); return; }
    if (type == OUT_INPUT && (in < 0 || in >= HUB_INPUTS || s_inputs[in].type == IN_NONE)) { server.send(400, "text/plain", "Pick an input that is set up"); return; }
    o.sig = sg; o.cond = cd; o.input = in; o.level = server.arg("level").toFloat(); o.hyst = fabsf(server.arg("hyst").toFloat());
    int w = 0; for (unsigned c = 0; c < nm.length() && w < 15; c++) { char ch = nm[c]; if (ch >= 32 && ch < 127 && ch != '"' && ch != '\\') o.name[w++] = ch; } o.name[w] = 0;
    const HubOutput &old = s_outputs[i];
    if (old.type != OUT_NONE && old.pin != pin && !pin_is_mcp(old.pin)) pinMode(old.pin, INPUT);   // let the old pin go
    s_outputs[i] = o; s_outOn[i] = false; s_outManual[i] = false;
    outputPinSetup(o); outputsSave();
    server.send(200, "text/plain", String("Output ") + (i + 1) + " saved");
  });
  server.on("/outputs/clear", HTTP_GET, []() {
    int i = server.arg("i").toInt();
    if (i < 0 || i >= HUB_OUTPUTS) { server.send(400, "text/plain", "Bad output"); return; }
    const HubOutput &old = s_outputs[i];
    if (old.type != OUT_NONE && !pin_is_mcp(old.pin)) pinMode(old.pin, INPUT);
    output_defaults(&s_outputs[i]); s_outOn[i] = false; outputsSave();
    server.send(200, "text/plain", String("Output ") + (i + 1) + " removed");
  });
  server.on("/outputs/toggle", HTTP_GET, []() {   // the page's on / off button for a "button or page" output
    int i = server.arg("i").toInt();
    if (i < 0 || i >= HUB_OUTPUTS || s_outputs[i].type != OUT_MANUAL) { server.send(400, "text/plain", "Not a switchable output"); return; }
    outputToggle(i + 1); server.send(200, "text/plain", s_outManual[i] ? "On" : "Off");
  });
  server.on("/pins/set", HTTP_GET, []() {   // move a fixed function to another GPIO: saved, applied after a restart
    String key = server.arg("role"); int g = server.arg("gpio").toInt(), r = -1;
    for (int k = 0; k < PR_COUNT; k++) if (key == HUB_PIN_ROLES[k].key) r = k;
    if (r < 0) { server.send(400, "text/plain", "Unknown function"); return; }
    if (!pin_role_fits(r, g)) { server.send(400, "text/plain", String("GPIO ") + g + " cannot do that (" + HUB_PIN_ROLES[r].name + ")"); return; }
    if (pin_is_ads(g) && !adsFound(g)) { server.send(400, "text/plain", "That ADS1115 was not found"); return; }
    String user = pinUser(g, -1);
    if (user.length() && pin_map_role_at(g_pins, g) != r) { server.send(400, "text/plain", "GPIO " + String(g) + " is already used by " + user); return; }
    uint8_t m[PR_COUNT]; memcpy(m, g_pins, sizeof m); m[r] = g;
    if (!pin_map_valid(m)) { server.send(400, "text/plain", "That map does not check out"); return; }
    Preferences p; p.begin("pins", false); p.putBytes("map", m, sizeof m); p.end();
    server.send(200, "text/plain", String(HUB_PIN_ROLES[r].name) + " moves to GPIO " + g + ". Move the wire, then the hub restarts with it.");
    delay(300); ESP.restart();
  });
  server.on("/pins/defaults", HTTP_GET, []() {
    Preferences p; p.begin("pins", false); p.clear(); p.end();
    server.send(200, "text/plain", "Every function back on its default GPIO. Restarting.");
    delay(300); ESP.restart();
  });
  server.on("/screens.json", HTTP_GET, []() {
    ScreenSlot snap[SCREEN_MAX];
    portENTER_CRITICAL(&s_scrMux); memcpy(snap, s_screens, sizeof snap); portEXIT_CRITICAL(&s_scrMux);
    uint32_t now = millis();
    String j = "[";
    bool first = true;
    for (int i = 0; i < SCREEN_MAX; i++) {
      if (!snap[i].used) continue;
      char nm[LINK_NAME_LEN + 1]; int k = 0;   // keep the name JSON-safe
      for (int x = 0; snap[i].name[x] && k < LINK_NAME_LEN; x++) { char ch = snap[i].name[x]; if (ch >= 32 && ch != '"' && ch != '\\') nm[k++] = ch; }
      nm[k] = 0;
      uint32_t age = now - snap[i].lastMs;
      char b[200];
      snprintf(b, sizeof b, "%s{\"name\":\"%s\",\"ip\":\"%u.%u.%u.%u\",\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"build\":%u,\"age\":%u,\"online\":%s,\"hellos\":%u,\"up\":%u}",
               first ? "" : ",", nm, (unsigned)(snap[i].ip & 255), (unsigned)(snap[i].ip >> 8 & 255), (unsigned)(snap[i].ip >> 16 & 255), (unsigned)(snap[i].ip >> 24),
               snap[i].mac[0], snap[i].mac[1], snap[i].mac[2], snap[i].mac[3], snap[i].mac[4], snap[i].mac[5],
               (unsigned)snap[i].build, (unsigned)age, age < SCREEN_OFFLINE_MS ? "true" : "false", (unsigned)snap[i].hellos,
               (unsigned)(snap[i].uptime ? snap[i].uptime + age / 1000 : 0));
      j += b; first = false;
    }
    j += "]";
    server.send(200, "application/json", j);
  });
  server.on("/state", HTTP_GET, []() {
    char egtTxt[56]; if (s_egtOpen) strcpy(egtTxt, "open / not connected"); else snprintf(egtTxt, sizeof egtTxt, "%.0f C / %.0f F", s_egtC, s_egtC * 1.8f + 32);
    if (s_egtHeld) strcat(egtTxt, " (not sent: ECU not heard)");
    char simTxt[80];
    if (sim_is_active()) snprintf(simTxt, sizeof simTxt, "<b style='color:#f5a623'>ON (%s), %d s left</b>", sim_is_manual() ? "manual" : "demo", sim_seconds_left());
    else strcpy(simTxt, "off");
    char dhtTxt[40]; if (!s_dhtOk) snprintf(dhtTxt, sizeof dhtTxt, "no reading (fails %u)", (unsigned)s_dhtFail); else snprintf(dhtTxt, sizeof dhtTxt, "%.1f F, %.0f%% humidity", s_dhtC * 1.8f + 32, s_dhtH);
    auto liveCount = []() { int n = 0; for (int i = 1; i < SIG_COUNT; i++) if (gauge_config_signal_has_data((SignalId)i)) n++; return n; };
    char b[900];
    snprintf(b, sizeof(b), "{\"lightsV\":%.2f,\"lightsOn\":%s,\"dimV\":%.2f,\"bri\":%d,\"build\":%d,\"ip\":\"%s\",\"ch\":%d,\"last\":\"%s\","
             "\"egt\":\"%s\",\"sim\":\"%s\",\"dht\":\"%s\",\"fuel\":%.0f,\"egtsent\":%u,\"ecu\":\"%s\",\"live\":%d,\"sent\":%u,\"cfg\":{\"mode\":%d,\"day\":%d,\"nmin\":%d,\"nmax\":%d,\"dlo\":%.1f,\"dhi\":%.1f,\"lon\":%.1f}}",
             s_lightsV, s_lightsV >= s_cfg.lightsOnV ? "true" : "false", s_dimV, s_briPct, INPUT_BUILD, WiFi.localIP().toString().c_str(),
             WiFi.channel(), s_last, egtTxt, simTxt, dhtTxt, (double)s_fuelPct, (unsigned)s_egtSent, hub_ecu_detected(), liveCount(), (unsigned)s_dataSent, s_cfg.mode, s_cfg.dayPct, s_cfg.nightMin, s_cfg.nightMax, s_cfg.dimLoV, s_cfg.dimHiV, s_cfg.lightsOnV);
    server.send(200, "application/json", b);
  });
  // ---- speed: rusEFI (hall sensor on the ECU) next to the GPS ----
  server.on("/speed.json", HTTP_GET, []() {
    char b[220];
    snprintf(b, sizeof b, "{\"ecuok\":%s,\"ecumph\":%.2f,\"gpsok\":%s,\"fix\":%s,\"gpsmph\":%.2f,\"sats\":%d,\"rmc\":%u,\"bad\":%u}",
             gauge_config_signal_has_data(SIG_VSS) ? "true" : "false", (double)g_vss_mph,
             (s_gpsLastByteMs && millis() - s_gpsLastByteMs < 2000) ? "true" : "false", s_gps.fix ? "true" : "false",
             (double)s_gps.mph, s_gps.sats, (unsigned)s_gps.rmcCount, (unsigned)s_gps.badCount);
    server.send(200, "application/json", b);
  });
  server.on("/save", HTTP_GET, []() {
    auto I = [](const char *k, int d) { return server.hasArg(k) ? server.arg(k).toInt() : d; };
    auto rdF = [](const char *k, float d) { return server.hasArg(k) ? server.arg(k).toFloat() : d; };
    s_cfg.mode = I("mode", s_cfg.mode) == 1 ? 1 : 0;
    s_cfg.dayPct = clampPct(I("day", s_cfg.dayPct));
    s_cfg.nightMin = clampPct(I("nmin", s_cfg.nightMin)); s_cfg.nightMax = clampPct(I("nmax", s_cfg.nightMax));
    s_cfg.dimLoV = rdF("dlo", s_cfg.dimLoV); s_cfg.dimHiV = rdF("dhi", s_cfg.dimHiV); s_cfg.lightsOnV = rdF("lon", s_cfg.lightsOnV);
    cfgSave();
    server.send(200, "text/plain", "Saved.");
  });
  server.on("/update", HTTP_GET, []() {
    server.send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='firmware'><input type='submit' value='Update'></form>");
  });
  server.on("/update", HTTP_POST, []() {
    server.send(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
    delay(300); ESP.restart();
  }, []() {
    HTTPUpload &u = server.upload();
    if (u.status == UPLOAD_FILE_START) Update.begin(UPDATE_SIZE_UNKNOWN);
    else if (u.status == UPLOAD_FILE_WRITE) Update.write(u.buf, u.currentSize);
    else if (u.status == UPLOAD_FILE_END) Update.end(true);
  });
  server.begin();
}


// Same packet as the screens' espnow_link.cpp ("MSD1"): every signal plus a has-data mask, 20 times a second
static void dataTask(void *) {
  uint16_t seq = 0;
  for (;;) {
    LinkPacket p; p.magic = PKT_MAGIC; p.seq = seq++; p.count = SIG_COUNT | LINK_COUNT_FROM_HUB; p.has_mask = 0;   // "from a hub" flag (factory setup)
    for (int i = 0; i < SIG_COUNT; i++) {
      bool has = i != SIG_NONE && gauge_config_signal_has_data((SignalId)i);
      p.v[i] = has ? gauge_config_signal_value((SignalId)i) : 0.0f;
      if (has) p.has_mask |= 1ull << i;
    }
    if (sim_is_active()) p.has_mask |= LINK_MASK_SIMULATED;   // the screens show SIMULATED and the main screen does not log
    if (esp_now_send(BCAST, (const uint8_t *)&p, sizeof(p)) == ESP_OK) s_dataSent++;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void setup() {
  Serial.begin(115200);
  pinsLoad();   // first: every pin below comes from the map
  s_adcLock = xSemaphoreCreateMutex();   // before any task reads the ADC
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);   // about 0 - 3.1 V at the pins
  cfgLoad();

  // Same rule as the screens: join the home network if it's there (ESP-NOW then shares its channel),
  // otherwise skip it and park on the car channel.
  WiFi.mode(WIFI_STA);
  WiFi.config(MY_IP, MY_GW, MY_MASK, MY_GW);
  bool homeSeen = false;
  int n = WiFi.scanNetworks(false, false, false, 100);
  for (int i = 0; i < n && !homeSeen; i++) if (WiFi.SSID(i) == WIFI_SSID) homeSeen = true;
  WiFi.scanDelete();
  if (homeSeen) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 15000) delay(200);
  }
  s_wifiUp = WiFi.status() == WL_CONNECTED;
  if (!s_wifiUp) {
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false, false);
  }
  // Its own network: on the channel the screens use (the home network's at home, CAR_CHANNEL away), so ESP-NOW is unchanged.
  apLoad();
  if (ap_should_start(&s_ap, s_wifiUp)) {
    WiFi.mode(WIFI_AP_STA);
    s_apOn = WiFi.softAP(s_ap.ssid, s_ap.pass, s_wifiUp ? WiFi.channel() : CAR_CHANNEL, 0, 8);   // 3 screens + phones and a laptop
    Serial.printf("Own Wi-Fi \"%s\": %s, %s\n", s_ap.ssid, s_apOn ? "on" : "FAILED", WiFi.softAPIP().toString().c_str());
  }
  if (!s_wifiUp && !s_apOn) {   // nothing up: park on the car channel by hand
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(CAR_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
  }
  Serial.printf("MSP-DASH input board, build %d: %s, channel %d\n", INPUT_BUILD, s_wifiUp ? WiFi.localIP().toString().c_str() : "no WiFi", WiFi.channel());

  if (esp_now_init() != ESP_OK) Serial.println("ESP-NOW init FAILED");
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BCAST, 6);
  peer.channel = 0; peer.ifidx = WIFI_IF_STA; peer.encrypt = false;
  esp_now_add_peer(&peer);
  esp_now_register_recv_cb(onEspNowRecv);   // screen hellos, for the status lights

  slotsLoad();   // custom sensor slots, before CAN starts decoding them
  tzLoad();      // time zone for the GPS clock
  oledLoad();    // OLED warning panel
  inputsLoad();  // configurable inputs
  cpu_load_begin();   // idle counters per core (Board status page)
  shiftLoad();   // shift light
  buttonsLoad(); // buttons (after the inputs: the first start on hub 53 moves button inputs over)
  outputsLoad(); // configurable outputs
  for (int i = 0; i < HUB_OUTPUTS; i++) outputPinSetup(s_outputs[i]);
  for (int i = 0; i < HUB_INPUTS; i++) inputPinSetup(s_inputs[i]);
  imuLoad();     // accelerometer mounting and zero
  pagesLoad();   // OLED pages
  // CAN: the hub is now the only CAN reader; the screens are all remotes
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  g.rx_queue_len = 64;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK || twai_start() != ESP_OK) Serial.println("CAN start FAILED");
  else { can_start_task(); Serial.println("CAN running on GPIO 21 (TX) / 22 (RX)"); }
  xTaskCreatePinnedToCore(egtTask, "egt", 3072, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(dhtTask, "dht", 3072, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(fuelTask, "fuel", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(gpsTask, "gps", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(canTxTask, "can_tx", 3072, NULL, 2, NULL, 1);
  rfBegin();   // CC1101 on GPIO 13/14/27/33/39; carries on without it if not wired
  xTaskCreatePinnedToCore(oledTask, "oled", 4096, NULL, 1, NULL, 1);   // OLED panel; carries on without it if not wired
  xTaskCreatePinnedToCore(dataTask, "espnow_tx", 4096, NULL, 2, NULL, 0);

  if (s_wifiUp) setupWeb();
}

// Factory reset from the hub's BOOT button (GPIO 0): in the first 15 s after power-up hold it 5 s, then press again
// within 10 s (the screens and the serial port ask). Skipped if GPIO 0 is given to an input or button.
static int s_facState = 0; static uint32_t s_facT = 0;
static void hubFactoryButton() {
  int &state = s_facState; uint32_t &t = s_facT;
  if (state == 9) return;
  uint32_t now = millis();
  if (state == 0 && (now > 15000 || pinUser(PIN_BOOT_BTN, -1).length())) { state = 9; return; }
  if (state == 0) pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
  bool down = digitalRead(PIN_BOOT_BTN) == LOW;
  switch (state) {
    case 0: if (down) { state = 1; t = now; } break;
    case 1: if (!down) { state = 0; break; }
            if (now - t >= 5000) { state = 2; Serial.println("Factory reset? Let go of BOOT, then press it again within 10 s"); sendCmd(LINK_CMD_POPUP, 0, "Hub reset? Press BOOT"); } break;
    case 2: if (!down) { state = 3; t = now; } break;
    case 3: if (now - t > 10000) { Serial.println("Factory reset cancelled"); sendCmd(LINK_CMD_POPUP, 0, "Hub reset cancelled"); state = 9; }
            else if (down && now - t > 300) { sendCmd(LINK_CMD_POPUP, 0, "Hub factory reset"); hubFactoryReset(); }
            break;
  }
}
static char s_serLine[32]; static uint8_t s_serN = 0;
static void hubSerial() {   // USB serial: "factory reset"
  char *line = s_serLine; uint8_t &n = s_serN;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { line[n] = 0; n = 0; if (!strcmp(line, "factory reset")) { Serial.println("Factory reset..."); hubFactoryReset(); } }
    else if (n < sizeof s_serLine - 1) line[n++] = c;
  }
}

void loop() {
  rateStep();
  hubFactoryButton();
  hubSerial();
  { static uint32_t lastBtn = 0; if (millis() - lastBtn >= 10) { lastBtn = millis(); handleButtons(); } }
  { static uint32_t lastNet = 0;   // own network on: tell the screens its name and password, so they join it in the car
    if (s_apOn && millis() - lastNet >= HUBNET_MS) {
      lastNet = millis();
      HubNetPacket h = {}; h.magic = HUBNET_MAGIC;
      strncpy(h.ssid, s_ap.ssid, sizeof h.ssid - 1); strncpy(h.pass, s_ap.pass, sizeof h.pass - 1);
      esp_now_send(BCAST, (const uint8_t *)&h, sizeof h);
    } }
  { static uint8_t told = 0;   // own network on: say so on the screens, once they are up
    if (s_apOn && told < 2 && millis() > 15000u + told * 2500u) {
      char t[LINK_NAME_LEN + 1];
      if (told == 0) snprintf(t, sizeof t, "Wi-Fi: %s", s_ap.ssid); else snprintf(t, sizeof t, "Hub %s", WiFi.softAPIP().toString().c_str());
      sendCmd(LINK_CMD_POPUP, 0, t); told++;
    } }
  { static uint32_t lastShift = 0; if (millis() - lastShift >= 20) { lastShift = millis(); shiftStep(); } }
  { static uint32_t lastIn = 0; if (millis() - lastIn >= 20) { lastIn = millis(); inputsStep(); outputsStep(); } }
  static uint32_t lastKnob = 0;
  if (millis() - lastKnob >= 50) { lastKnob = millis(); readKnob(); }
  sim_loop();
  { static uint32_t lastTime = 0;   // GPS date/time to the screens (log names and timestamps), every 10 s
    uint32_t u = gpsUtcNow();
    if (s_wifiUp && u && millis() - lastTime > 10000) {
      lastTime = millis();
      TimePacket tp; tp.magic = TIME_MAGIC; tp.utc = u; tp.offset_min = (int16_t)tzOffsetNow(u);
      esp_now_send(BCAST, (const uint8_t *)&tp, sizeof tp);
    } }
  { static uint32_t lastNames = 0;   // custom slot names to the screens: on a change and every 10 s
    if (s_wifiUp && (s_slotsChanged || millis() - lastNames > SLOTS_MS)) { s_slotsChanged = false; lastNames = millis(); slotsSendNames(); } }
  if (s_wifiUp) server.handleClient();
  delay(2);
}
