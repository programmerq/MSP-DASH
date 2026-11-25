// The hub's Stock gauges page (AIRCORE_HTML) with a fake gauge board: cards per board and gauge, live values,
// the marks table, and each control sending the right command.
const vm = require('vm'), { hubScript, hubPage } = require('./hub_page.js');
let pass = 0, fail = 0; const check = (c, m) => { if (c) pass++; else { fail++; console.log("  FAIL:", m); } };
const settle = async () => { for (let i = 0; i < 20; i++) await new Promise(r => setImmediate(r)); };
const els = {}; const el = id => els[id] || (els[id] = { id, innerHTML: "", textContent: "", value: "", style: {} });
const calls = []; let data = { gauges: [] };
const ctx = { document: { getElementById: el }, console, URLSearchParams, setInterval() {}, setTimeout: (f, ms) => (ms > 1000 ? 0 : f()), clearTimeout() {}, confirm: () => true,
  fetch: async (u) => { calls.push(u); if (u === "/aircore.json") return { ok: true, json: async () => data }; return { ok: true, text: async () => "Sent 7" }; } };
vm.createContext(ctx); vm.runInContext(hubScript('AIRCORE_HTML'), ctx);
const R = c => vm.runInContext(c, ctx);
const mac = "AA:BB:CC:DD:EE:01";
const g = (n, name, extra) => Object.assign({ mac, g: n, age: 100, build: 7, gauges: 2, name, mode: 0, slow: 1, hub: 1, has: 0, pwm: 20000, needle: 175, value: 0, shown: 0, sweepS: 4, strength: 1, slew: 720,
  min: 0, max: name === "tach" ? 8000 : 150, zero: 175, span: -260, ch: 6, up: 0, seq: 0, pts: [] }, extra);
(async () => {
  console.log("1. no board");
  check(/<a href="\/aircore">Stock gauges<\/a>/.test(hubPage('HOME_HTML')), "in the menu");
  await settle(); check(/No gauge board heard/.test(el("gauges").innerHTML), "says when nothing is heard");
  console.log("2. a board with both gauges");
  data = { gauges: [g(0, "tach", { mode: 1, shown: 4321, pts: [[0, 175], [8000, -84]] }), g(1, "speedo", { pwm: 7000 })] };
  await R("poll()"); await settle();
  const h = el("gauges").innerHTML;
  check(/Board AA:BB:CC:DD:EE:01/.test(h) && /<h3>Tach<\/h3>/.test(h) && /<h3>Speedo<\/h3>/.test(h), "a board card and a card per gauge");
  check(/<option value="40000"/.test(h) && /<option value="100"/.test(h), "PWM list from 100 Hz to 40 kHz");
  const id = "AABBCCDDEE010";
  check(el("v" + id).textContent === "4321 RPM" && el("m" + id).textContent === "Sweep" && el("n" + id).textContent === "175.0\u00b0", "live: showing, mode, needle");
  check(/<td>8000<\/td><td>-84\u00b0<\/td>/.test(el("pt" + id).innerHTML), "the marks table");
  console.log("3. commands");
  const sent = () => calls.filter(u => u.startsWith("/aircore/cmd")).map(u => Object.fromEntries(new URLSearchParams(u.split("?")[1])));
  calls.length = 0; el("sw" + id).value = "6"; R(`C("${mac}/0",2,$("sw${id}").value)`); await settle();
  let c = sent()[0]; check(c && c.mac === mac && c.g === "0" && c.op === "2" && c.a === "6", "sweep, 6 s");
  calls.length = 0; R(`C("${mac}/1",3,95)`); await settle(); c = sent()[0]; check(c && c.g === "1" && c.op === "3" && c.a === "95", "speedo target 95 mph");
  calls.length = 0; R(`C("${mac}/0",11,3000)`); await settle(); c = sent()[0]; check(c && c.op === "11" && c.a === "3000", "save the needle's angle as the 3000 mark");
  calls.length = 0; R(`cmd({mac:"${mac}",g:0},5,2000)`); await settle(); c = sent()[0]; check(c && c.op === "5" && c.a === "2000", "PWM 2 kHz for the board");
  calls.length = 0; R(`C("${mac}/0",9,0,7000)`); await settle(); c = sent()[0]; check(c && c.op === "9" && c.a === "0" && c.b === "7000", "dial range 0 to 7000");
  calls.length = 0; R(`C("${mac}/0",2,4)`); await settle();
  data.gauges[0].seq = 7; await R("poll()"); await settle();
  check(/applied command 7/.test(el("msg").textContent), "the board's confirmation shown");
  console.log("4. the board's health");
  data.gauges[0].up = 600; data.gauges[0].seq = 7; await R("poll()"); await settle();
  check(/^Up 10 min 0 s$/.test(el("bsAABBCCDDEE01").textContent), "uptime shown");
  data.gauges[1].up = 599; await R("poll()"); await settle();
  check(!/RESTARTED/.test(el("bsAABBCCDDEE01").textContent), "the two gauges' uptimes a second apart: not a restart");
  data.gauges[0].up = 5; data.gauges[1].up = 5; await R("poll()"); await settle();
  check(/RESTARTED/.test(el("bsAABBCCDDEE01").textContent), "a restart is flagged");
  data.gauges[0].up = 600; data.gauges[1].up = 601; await R("poll()"); await settle(); data.gauges[0].up = 601; data.gauges[1].up = 600; await R("poll()"); await settle();
  const t = el("bsAABBCCDDEE01").textContent;
  data.gauges[0].age = 2000; await R("poll()"); await settle();
  check(!/not heard/.test(el("bsAABBCCDDEE01").textContent), "a 2 s gap (a lost packet or two) is not flagged");
  data.gauges[0].age = 6000; await R("poll()"); await settle();
  check(/not heard for 6 s/.test(el("bsAABBCCDDEE01").textContent), "a quiet board is flagged");
  console.log(`\n${pass} passed, ${fail} failed`); process.exit(fail ? 1 : 0);
})();
