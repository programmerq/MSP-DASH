// hub_pages.h - the hub's web pages.
// They live in this header, NOT in the .ino: the Arduino builder scans the .ino to add function prototypes and
// can mangle long raw strings full of JavaScript ("'asyncfunction' does not name a type"). Headers are left alone.
#pragma once
#include <Arduino.h>
#include "oled_icons.h"   // OLED_ICONS_JS for the /oled page

// The side menu on every hub page (HUB_NAV, styled by HUB_NAV_STYLE inside each page's <style>): grouped by task, a
// drawer behind the menu button on a phone. Also the shared look (chips, toast). The current page is highlighted by the
// onerror of a 1-pixel image, so the menu adds no <script> to the page.
#define HUB_NAV_STYLE "*{box-sizing:border-box}\nhtml{-webkit-text-size-adjust:100%}\nbody{background:#000;color:#ddd;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;max-width:1000px;margin:0 auto;padding:12px 16px calc(24px + env(safe-area-inset-bottom))}body.wide{max-width:none}\ninput,select,textarea{font-size:16px}\n.nav{position:fixed;left:0;top:0;bottom:0;width:200px;background:#0b1a1d;padding:12px 8px;overflow-y:auto;z-index:50}\n.nav .brand{color:#4dd0e1;font-weight:bold;font-size:18px;margin:2px 10px 8px}\n.nav .grp{color:#6b8a90;font-size:11px;text-transform:uppercase;letter-spacing:.08em;margin:14px 10px 4px}\n.nav a{display:block;color:#cfe8ec;text-decoration:none;padding:7px 10px;border-radius:6px;font-size:14px}\n.nav a:hover{background:#13292d}.nav a.on{background:#1d3b40;color:#4dd0e1;font-weight:bold}\n.navbtn{display:none;position:fixed;top:calc(8px + env(safe-area-inset-top));left:8px;z-index:60;background:#0b1a1d;color:#4dd0e1;border:1px solid #1d3b40;border-radius:6px;font-size:20px;padding:4px 10px}\n@media(min-width:800px){body{margin-left:216px!important}}\n@media(max-width:799px){.navbtn{display:block}.nav{transform:translateX(-100%);transition:transform .2s}.nav.open{transform:none;box-shadow:4px 0 20px #000}body{padding-top:calc(52px + env(safe-area-inset-top))!important}}\n.chip{display:inline-block;font-size:12px;padding:2px 8px;border-radius:10px;background:#222;color:#bbb}.chip.ok{background:#16351c;color:#7bd88f}.chip.warn{background:#3d3000;color:#ffd54f}.chip.bad{background:#3d0b0b;color:#ff6b6b}\n.toast{position:fixed;right:16px;bottom:16px;background:#1d3b40;color:#e0f7fa;padding:10px 14px;border-radius:8px;z-index:70;box-shadow:0 4px 16px #000}"
#define HUB_NAV "<button class=\"navbtn\" onclick=\"document.querySelector('.nav').classList.toggle('open')\">&#9776;</button><div class=\"nav\"><div class=\"brand\">MSP-DASH</div><div class=\"grp\">Overview</div><a href=\"/\">Dashboard</a><a href=\"/boards\">Board status</a><div class=\"grp\">Displays</div><a href=\"/screen\">Screens</a><a href=\"/oled\">OLED</a><a href=\"/aircore\">Stock gauges</a><div class=\"grp\">Sensors</div><a href=\"/sensors\">Live sensors</a><a href=\"/custom\">Custom sensors</a><div class=\"grp\">Controls</div><a href=\"/inputs\">Inputs and pins</a><a href=\"/buttons\">Buttons</a><div class=\"grp\">Logging</div><a href=\"/screen?s=Center&p=log-channels\">Logging</a><a href=\"/screen?s=Center&p=sd\">SD card</a><div class=\"grp\">Setup</div><a href=\"/setup\">Setup</a><a href=\"/firmware\">Firmware</a><a href=\"/backup\">Backup</a><div class=\"grp\">Tools</div><a href=\"/simulator\">Simulator</a><a href=\"/can\">Raw CAN</a><a href=\"/rf\">433 MHz</a></div><img src=\"data:,\" alt=\"\" style=\"display:none\" onerror=\"var h=location.pathname+location.search;document.querySelectorAll('.nav a').forEach(function(a){var t=a.getAttribute('href');if(t===h||(t===location.pathname&&t.indexOf('?')<0&&h.indexOf('?')<0))a.classList.add('on')})\">"

static const char HOME_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH</title>
<meta name="apple-mobile-web-app-capable" content="yes"><meta name="mobile-web-app-capable" content="yes"><meta name="apple-mobile-web-app-title" content="MSP-DASH">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent"><meta name="theme-color" content="#000000">
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:900px;margin:auto;padding:12px}h2{color:#4dd0e1;margin:6px 0 10px}
h3{color:#4dd0e1;margin:0 0 8px;font-size:16px}.card{background:#111;border-radius:8px;padding:12px;margin:0 0 12px}.note{color:#888;font-size:13px}
.tiles{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:10px;margin:0 0 12px}
.tile{background:#111;border-radius:8px;padding:10px 12px;border-left:5px solid #444;text-decoration:none;color:#ddd;display:block}
.tile b{display:block;color:#888;font-weight:normal;font-size:12px;margin-bottom:4px}.tile span{font-size:18px}
.tile.ok{border-color:#2e7d32}.tile.warn{border-color:#ffb300}.tile.bad{border-color:#e53935}
.two{display:grid;grid-template-columns:1fr 1fr;gap:12px}@media(max-width:700px){.two{grid-template-columns:1fr}}
canvas{image-rendering:pixelated;width:100%;max-width:384px;border:1px solid #333;border-radius:4px;background:#000}
.read{display:grid;grid-template-columns:repeat(auto-fill,minmax(120px,1fr));gap:8px}.read div{background:#000;border-radius:6px;padding:8px}
.read b{display:block;color:#888;font-weight:normal;font-size:12px}.read span{font-size:22px}
.scr{display:flex;flex-wrap:wrap;align-items:center;gap:6px 10px;padding:6px 0;border-top:1px solid #222}.scr:first-child{border-top:0}.scr .who{flex:1 1 180px}
.scr a{background:#1d2b2e;color:#4dd0e1;text-decoration:none;padding:5px 10px;border-radius:5px;font-size:13px}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:6px}.dot.on{background:#2e7d32}.dot.off{background:#e53935}
.qhead{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:8px;margin:0 0 8px}.qhead h3{margin:0}
.qhead select{background:#000;color:#ddd;border:1px solid #333;border-radius:6px;padding:6px}
.btns{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:8px}
.btns button{padding:16px 8px;font-size:16px;border:0;border-radius:10px;background:#1d3b40;color:#4dd0e1;-webkit-tap-highlight-color:transparent}
.btns button.big{background:#4dd0e1;color:#000;font-weight:bold}.btns button:active{filter:brightness(1.4)}
#qmsg{min-height:1.3em;margin-top:8px;color:#4dd0e1}
@media(max-width:600px){.tiles{grid-template-columns:repeat(2,1fr)}.read span{font-size:20px}}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Dashboard</h2>
<div class="card"><div class="qhead"><h3>Quick controls</h3><label class="note">Pages on <select id="qt"><option value="0">All screens</option><option value="1">Left</option><option value="2">Center</option><option value="3">Right</option></select></label></div>
<div class="btns"><button class="big" onclick="act(2)">&#9664; Previous page</button><button class="big" onclick="act(1)">Next page &#9654;</button>
<button id="qecu" onclick="act(14)" style="display:none">ECU map</button><button onclick="act(8,1)">Race mode</button><button onclick="act(10)">Log marker</button>
<button onclick="act(9)">Clear warning</button><button onclick="act(11)">Show IPs</button><button onclick="act(12)">OLED next page</button></div>
<div class="note" id="qmsg"></div></div>
<div class="tiles" id="tiles"></div>
<div class="two">
<div class="card"><h3>Warning panel</h3><canvas id="cv" width="128" height="64"></canvas><div class="note" id="lit"></div></div>
<div class="card"><h3>Readings</h3><div class="read" id="read"></div></div>
</div>
<div class="card"><h3>Screens</h3><div id="scr">Waiting for screens...</div></div>
<script>
const NAME={1:"RPM",2:"Coolant Temp",3:"MAP/Boost",4:"AFR",5:"Oil Pressure",6:"Oil Temp",7:"Fuel Pressure",8:"Ethanol",9:"Throttle Position",10:"Battery Voltage",
11:"Rev Limiter",12:"Main Relay",13:"Fuel Pump",14:"Check Engine",15:"O2 Heater",16:"Lambda Protection",17:"Fan 1",18:"Fan 2",19:"Brake Pedal",
20:"EGT",21:"Intake Air Temp",22:"Ambient Temp",23:"Humidity",24:"Fuel Level",25:"Speed",26:"GPS Speed",27:"GPS Satellites",45:"Lateral G",46:"Accel/Brake G",47:"Vertical G",48:"Yaw Rate"};
const $=id=>document.getElementById(id),j=async u=>(await fetch(u)).json(),order=n=>({left:0,center:1,centre:1,middle:1,right:2})[(n||"").toLowerCase()]??3;
const tile=(k,v,cls,href)=>'<a class="tile '+cls+'" href="'+href+'"><b>'+k+'</b><span>'+v+'</span></a>';
// The center screen's logging status (screens from build 204 allow this cross-origin read). Asked every 5 s.
let logText="on Center",logCls="",logAt=0;
async function logStatus(scr){
 if(Date.now()-logAt<5000)return;logAt=Date.now();
 const c=(scr||[]).find(s=>/^cent(er|re)$/i.test(s.name||""))||(scr||[])[0];if(!c||!c.online){logText="Center offline";logCls="warn";return;}
 try{const ctl=new AbortController();setTimeout(()=>ctl.abort(),2500);const st=await (await fetch("http://"+c.ip+"/status.json",{signal:ctl.signal})).json();
  const t=st.logging||"";logText=t.length>28?t.slice(0,27)+"...":(t||"no status");logCls=/no sd/i.test(t)?"bad":/logging to|started/i.test(t)?"ok":"";}
 catch(e){logText="on Center";logCls="";}}
const KEY=[[1,"RPM",0],[3,"Boost psi",1],[2,"Coolant F",0],[5,"Oil psi",0],[6,"Oil F",0],[10,"Battery V",1],[25,"Speed mph",0],[45,"Lateral G",2]];
async function tick(){
 let st={},can={},spd={},sim={},scr=[],ol={lamps:[]},tm={},sens=[];
 try{[st,can,spd,sim,scr,ol,tm,sens]=await Promise.all(["/state","/can.json","/speed.json","/sim.json","/screens.json","/oled.json","/time.json","/sensors.json"].map(u=>j(u).catch(()=>({}))));}catch(e){}
 if(!Array.isArray(scr))scr=[];if(!Array.isArray(sens))sens=[];   // a failed request gives {}: treat as an empty list
 logStatus(scr);
 const on=(scr||[]).filter(s=>s.online).length,tot=(scr||[]).length;
 const w=(ol.lamps||[]).filter(l=>l.state===1).length,d=(ol.lamps||[]).filter(l=>l.state===2).length;
 const canOk=can.state===1&&can.total>0,ecu=st.ecu||"--";
 $("tiles").innerHTML=
  tile("ECU / CAN",(ecu==="none"||!canOk?"no data":ecu),canOk&&ecu!=="none"?"ok":"bad","/can")+
  tile("GPS",!spd.gpsok?"not connected":spd.fix?"fix, "+spd.sats+" sats":"no fix ("+(spd.sats||0)+")",!spd.gpsok?"bad":spd.fix?"ok":"warn","/setup")+
  tile("Screens",on+" of "+tot+" online",tot&&on===tot?"ok":on?"warn":"bad","/screen")+
  tile("Warnings",d?d+" danger"+(w?", "+w+" warning":""):w?w+" warning":"none",d?"bad":w?"warn":"ok","/oled")+
  tile("Simulator",sim.active?"ON, "+sim.seconds_left+" s left":"off",sim.active?"warn":"ok","/simulator")+
  tile("Clock",tm.known?tm.local.slice(11,16):"no GPS time",tm.known?"ok":"warn","/setup")+
  tile("Brightness",(st.bri!=null?st.bri:"--")+" %",st.bri>0?"ok":"warn","/setup")+
  tile("Logging",logText,logCls,"/screen?s=Center&p=log-channels");
 $("lit").textContent=(ol.lamps||[]).map((l,i)=>l.state?"spot "+(i+1)+" "+(l.state===2?"DANGER":"warning")+(l.op===3?" (input)":" ("+(NAME[l.sig]||"signal "+l.sig)+")"):"").filter(x=>x).join(" | ")||"Nothing lit";
 const by={};(sens||[]).forEach(x=>by[x.sig]=x);
 $("read").innerHTML=KEY.map(k=>{const x=by[k[0]],fresh=x&&x.age<3000;return '<div><b>'+k[1]+'</b><span>'+(fresh?x.v.toFixed(k[2]):"--")+'</span></div>';}).join("");
 (scr||[]).sort((a,b)=>order(a.name)-order(b.name));
 $("scr").innerHTML=tot?scr.map(s=>{const q="/screen?s="+encodeURIComponent(s.name||s.ip)+"&p=";return '<div class="scr"><span class="who"><span class="dot '+(s.online?"on":"off")+'"></span><b>'+(s.name||"(no name)")+'</b> <span class="note">'+s.ip+' | build '+s.build+'</span></span>'+
  '<a href="'+q+'">Live</a><a href="'+q+'gauges">Gauges</a><a href="'+q+'settings">Settings</a></div>';}).join(""):"No screens heard yet.";
}
async function pic(){try{const b=new Uint8Array(await (await fetch("/oled.bin")).arrayBuffer());const cx=$("cv").getContext("2d"),img=cx.createImageData(128,64);
 for(let y=0;y<64;y++)for(let x=0;x<128;x++){const v=b[(y>>3)*128+x]>>(y&7)&1,p=(y*128+x)*4;img.data[p]=img.data[p+1]=img.data[p+2]=v?255:0;img.data[p+3]=255;}cx.putImageData(img,0,0);}catch(e){}}
// Quick controls: run a button action from the phone (the same actions as the Buttons page; page actions go to the
// screen picked above). The ECU map button shows only when the maps are in use, named after the map in use.
async function act(a,p){try{const r=await fetch("/action?a="+a+"&p="+(p||0)+"&t="+$("qt").value);const t=await r.text();$("qmsg").textContent=t;
 if(a===14&&r.ok)$("qecu").textContent="ECU map: "+t;}catch(e){$("qmsg").textContent="Could not reach the hub";}}
async function ecu(){try{const b=await j("/buttons.json");if(b.ecu&&b.ecu.used){const q=$("qecu");q.style.display="";q.textContent="ECU map: "+(b.ecu.names[b.ecu.cur]||("Map "+(b.ecu.cur+1)));}}catch(e){}}
tick();pic();ecu();setInterval(tick,1000);setInterval(pic,700);
</script></body></html>)HTML";

// /setup: brightness, clock, accelerometer, I2C devices, hub sensors and the speed check, hub details.
static const char SETUP_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Setup</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:720px;margin:auto;padding:12px}h2{color:#4dd0e1;margin:6px 0 10px}
h3{color:#4dd0e1;margin:0 0 8px;font-size:16px}.card{background:#111;border-radius:8px;padding:12px;margin:0 0 12px}
label{display:block;margin:10px 0 4px}input,select{width:100%;padding:6px;box-sizing:border-box;background:#000;color:#ddd;border:1px solid #333;border-radius:4px}
button{margin-top:10px;padding:8px 12px;background:#4dd0e1;border:0;color:#000;border-radius:6px}.row{display:flex;gap:8px}.row>*{flex:1}.note{color:#999;font-size:13px}
.dot{display:inline-block;width:12px;height:12px;border-radius:50%;margin-right:8px;vertical-align:middle}.on{background:#2e7d32}.off{background:#e53935}
.scr{display:flex;flex-wrap:wrap;align-items:center;gap:6px 10px;padding:8px 0;border-top:1px solid #222}.scr:first-child{border-top:0}
.scr .who{flex:1 1 200px}.scr a.btn{background:#1d2b2e;color:#4dd0e1;text-decoration:none;padding:5px 10px;border-radius:5px;font-size:13px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:8px}.kv{background:#000;border-radius:6px;padding:8px}
.kv b{display:block;color:#888;font-weight:normal;font-size:12px;margin-bottom:2px}details summary{cursor:pointer;color:#4dd0e1}
.jump{display:flex;flex-wrap:wrap;gap:6px;margin:0 0 12px}.jump a{background:#1d2b2e;color:#4dd0e1;text-decoration:none;padding:5px 10px;border-radius:5px;font-size:13px}
.card{scroll-margin-top:56px}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Setup</h2>
<div class="jump"><a href="#brightness">Brightness</a><a href="#clock">Clock</a><a href="#accel">Accelerometer</a><a href="#i2c">I2C bus</a><a href="#shift">Shift light</a><a href="#speed">Speed check</a><a href="#wifi">Wi-Fi</a><a href="#hub">Hub</a></div>
<div class="card" id="brightness"><details open><summary><b>Brightness</b> <span class="note" id="brisum"></span></summary>
<label>Brightness mode<select id="mode"><option value="0">Auto: car lights and dimmer</option><option value="1">Knob</option></select></label>
<label>Day brightness, lights off (%)<input type="number" id="day" min="5" max="100"></label>
<div class="row"><label>Night, dimmer lowest (%)<input type="number" id="nmin" min="5" max="100"></label>
<label>Night, dimmer highest (%)<input type="number" id="nmax" min="5" max="100"></label></div>
<div class="row"><label>Dimmer lowest (V)<input type="number" id="dlo" step="0.1"></label>
<label>Dimmer highest (V)<input type="number" id="dhi" step="0.1"></label></div>
<div class="row"><button type="button" onclick="grab('dlo')">Use now as lowest</button><button type="button" onclick="grab('dhi')">Use now as highest</button></div>
<label>Lights count as on above (V)<input type="number" id="lon" step="0.1"></label>
<button type="button" onclick="save()">Save</button> <span id="msg" class="note"></span>
<p class="note">Turn the lights on, roll the dimmer all the way down and press "Use now as lowest", all the way up and press "Use now as highest", then Save.</p></details></div>
<div class="card" id="clock"><details><summary><b>Clock</b> <span class="note" id="clk"></span></summary>
<p class="note">From the GPS. The screens name their log files by it and stamp each row with it.</p>
<label>Time zone<select id="tzstd"><option value="-300">Eastern</option><option value="-360">Central</option><option value="-420">Mountain</option><option value="-480">Pacific</option><option value="-540">Alaska</option><option value="-600">Hawaii</option><option value="0">UTC</option></select></label>
<label><input type="checkbox" id="tzdst" style="width:auto"> US daylight saving time</label>
<button type="button" onclick="saveTz()">Save</button> <span id="tzmsg" class="note"></span></details></div>
<div class="card" id="accel"><h3>Accelerometer</h3><div class="grid" id="imu"></div>
<details><summary class="note">Mounting and zero</summary>
<p class="note">MPU6050 on the OLED wires: VCC 3.3 V (L6), GND, SDA GPIO 5 (R15), SCL GPIO 15 (R21). Mount it solid to the chassis. Say which arrow printed on the module points to the front and which to the left, then park on level ground, engine running, and press Zero.</p>
<div class="row"><label>Points to the front<select id="ifwd"></select></label><label>Points to the left<select id="ileft"></select></label></div>
<div class="row"><button type="button" onclick="imuSet()">Save mounting</button><button type="button" onclick="imuZero()">Zero (car still and level)</button></div>
<span id="imsg" class="note"></span></details></div>
<div class="card" id="i2c"><h3>I2C bus</h3><div class="grid" id="i2c"></div>
<p class="note">Found at power-up on SDA GPIO 5 (R15) / SCL GPIO 15 (R21). Plug a device in with the hub powered off, then restart it.</p></div>
<div class="card" id="shift"><details open><summary><b>Shift light</b> <span class="note" id="shsum"></span></summary>
<p class="note">A ring round each screen: they light yellow one at a time from left to right as RPM climbs from the start, all turn green at the shift point, and flash red past the over-rev point.</p>
<label><input type="checkbox" id="shon" style="width:auto"> On</label>
<div class="row"><label>Start (RPM)<input type="number" id="shstart" min="500" max="12000" step="100"></label>
<label>Shift (RPM)<input type="number" id="shshift" min="500" max="12000" step="100"></label>
<label>Over-rev, 0 = off (RPM)<input type="number" id="shover" min="0" max="13000" step="100"></label></div>
<div class="row"><button type="button" onclick="shSave()">Save</button><button type="button" onclick="shTest()">Test (4 s sweep)</button></div>
<span id="shmsg" class="note"></span><p class="note" id="shorder"></p></details></div>
<div class="card" id="speed"><h3>Hub sensors and speed check</h3><div class="grid" id="sens"></div>
<p class="note">Speed comes from rusEFI (hall sensor on the ECU). If it disagrees with the GPS at a steady speed, multiply rusEFI's speed calibration by the factor shown.</p></div>
<div class="card" id="wifi"><details open><summary><b>Hub Wi-Fi</b> <span class="note" id="wfsum"></span></summary>
<p class="note">Its own network for the car, where your home Wi-Fi is not around: join it from the phone and open the address shown here. The screens keep getting data either way, and join it too (as 192.168.4.201, .202, .203), so their pages load in the car. The hub sends them the name and password over ESP-NOW, which is not encrypted.</p>
<div class="grid" id="wfstat"></div>
<label>Its own network is on<select id="wfmode"></select></label>
<div class="row"><label>Network name<input id="wfssid" maxlength="32"></label>
<label>Password (8 or more, blank = keep)<input id="wfpass" type="password" maxlength="63" autocomplete="new-password"></label></div>
<div class="row"><button type="button" onclick="wfSave(0)">Save</button><button type="button" onclick="wfSave(1)">Save and restart</button></div>
<span id="wfmsg" class="note"></span></details></div>
<div class="card" id="hub"><h3>Hub</h3><div class="grid" id="stat"></div>
<p class="note">Factory reset erases every hub setting (buttons, inputs, outputs, pin moves, OLED, shift light, ECU maps, Wi-Fi, brightness, clock, custom sensors) and restarts it. Export a backup first (Backup page).
Without this page: in the first 15 s after power-up hold the hub's BOOT button for 5 s, then press it again when the screens ask; or type <b>factory reset</b> in the USB serial monitor.</p>
<button type="button" style="background:#e53935;color:#fff" onclick="facReset()">Factory reset the hub</button> <span id="facmsg" class="note"></span></div>
<script>
let st={};const ids=["mode","day","nmin","nmax","dlo","dhi","lon"];const $=id=>document.getElementById(id);
const kv=(k,v)=>'<div class="kv"><b>'+k+'</b>'+v+'</div>';
function grab(id){$(id).value=st.dimV.toFixed(1);}
async function poll(){try{st=await (await fetch("/state")).json();
$("stat").innerHTML=kv("ECU",st.ecu)+kv("Signals live",st.live)+kv("Simulator",st.sim)+kv("Last button / command",st.last)+kv("Data packets sent",st.sent)+kv("Hub","build "+st.build+"<br>"+st.ip+", channel "+st.ch);
$("brisum").textContent="| "+st.bri+" % now | lights "+st.lightsV.toFixed(1)+" V ("+(st.lightsOn?"on":"off")+") | dimmer "+st.dimV.toFixed(1)+" V";
if(!poll.done){ids.forEach(i=>$(i).value=st.cfg[i]);poll.done=1;}}catch(e){}}
async function save(){const q=ids.map(i=>i+"="+encodeURIComponent($(i).value)).join("&");$("msg").textContent=await (await fetch("/save?"+q)).text();}
function ago(ms){return ms<60000?Math.round(ms/1000)+" s ago":Math.round(ms/60000)+" min ago";}
async function spd(){try{const g=await (await fetch("/speed.json")).json();
const gps=!g.gpsok?"not connected":!g.fix?"no fix ("+g.sats+" sats)":g.gpsmph.toFixed(1)+" mph, "+g.sats+" sats";
const ecu=g.ecuok?g.ecumph.toFixed(1)+" mph":"no data";
const cmp=g.ecuok&&g.fix&&g.gpsmph>10&&g.ecumph>1?((g.ecumph-g.gpsmph)/g.gpsmph*100).toFixed(1)+" % (factor "+(g.gpsmph/g.ecumph).toFixed(3)+")":"--";
$("sens").innerHTML=kv("Ambient",st.dht||"--")+kv("Fuel",(st.fuel!=null?st.fuel+" %":"--"))+kv("EGT",(st.egt||"--")+"<br><span class=note>"+(st.egtsent||0)+" frames to CAN</span>")+
kv("Speed (rusEFI)",ecu)+kv("GPS",gps)+kv("Speed difference",cmp);}catch(e){}}
// shift light
async function sh(){try{const c=await (await fetch("/shift.json")).json();
 $("shsum").textContent="| "+(c.on?"on":"off")+(c.test?", testing":"")+" | "+c.rpm+" RPM";
 $("shorder").textContent=c.order?"Order, left to right: "+c.order.split(",").join(", ")+".":"No screens online.";
 if(!sh.done){$("shon").checked=c.on;$("shstart").value=c.start;$("shshift").value=c.shift;$("shover").value=c.over;sh.done=1;}}catch(e){}}
async function shSave(){const q="on="+($("shon").checked?1:0)+"&start="+$("shstart").value+"&shift="+$("shshift").value+"&over="+$("shover").value;
 const r=await fetch("/shift/set?"+q);$("shmsg").textContent=await r.text();}
async function shTest(){$("shmsg").textContent=await (await fetch("/shift/test")).text();}
// hub Wi-Fi
async function wf(){try{const w=await (await fetch("/wifi.json")).json();
 $("wfsum").textContent="| "+(w.home?"home network":"no home network")+(w.apOn?", own network on ("+w.clients+" joined)":"");
 $("wfstat").innerHTML=kv("Home network",w.home?w.homeSsid+"<br>"+w.ip:"not found")+kv("Own network",w.apOn?w.ssid+"<br>"+w.apIp:"off")+kv("Channel",w.ch);
 if(!wf.done){$("wfmode").innerHTML=w.modes.map((m,k)=>'<option value="'+k+'"'+(k===w.mode?" selected":"")+'>'+m+'</option>').join("");$("wfssid").value=w.ssid;wf.done=1;
  if(w.defaultPass)$("wfmsg").textContent="Still the default password (mspdash123): set your own.";}}catch(e){}}
async function wfSave(rs){if(rs&&!confirm("Restart the hub now? The screens lose data for about 20 s."))return;
 const q="mode="+$("wfmode").value+"&ssid="+encodeURIComponent($("wfssid").value)+"&pass="+encodeURIComponent($("wfpass").value)+"&restart="+rs;
 const r=await fetch("/wifi/set?"+q);$("wfmsg").textContent=await r.text();if(r.ok)$("wfpass").value="";}
async function facReset(){if(!confirm("Factory reset the hub? Every hub setting is erased."))return;if(!confirm("Really? This cannot be undone."))return;
 const r=await fetch("/factory",{method:"POST",body:"confirm=yes",headers:{"Content-Type":"application/x-www-form-urlencoded"}});$("facmsg").textContent=await r.text();}
async function clk(){try{const t=await (await fetch("/time.json")).json();$("clk").textContent="| "+(t.known?t.local:"no time from the GPS yet");
if(!clk.done){$("tzstd").value=String(t.std);$("tzdst").checked=t.dst;clk.done=1;}}catch(e){}}
async function saveTz(){$("tzmsg").textContent=await (await fetch("/time/set?std="+$("tzstd").value+"&dst="+($("tzdst").checked?1:0))).text();}
const AX=[[1,"+X"],[-1,"-X"],[2,"+Y"],[-2,"-Y"],[3,"+Z"],[-3,"-Z"]];
["ifwd","ileft"].forEach(id=>{const e=$(id);if(e)e.innerHTML=AX.map(a=>'<option value="'+a[0]+'">'+a[1]+'</option>').join("");});
async function imu(){try{const m=await (await fetch("/imu.json")).json();
if(!m.found){$("imu").innerHTML=kv("Status","not found");return;}
$("imu").innerHTML=kv("Lateral",m.lat.toFixed(2)+" G")+kv("Accel / brake",m.lon.toFixed(2)+" G")+kv("Vertical",m.vert.toFixed(2)+" G")+kv("Yaw rate",m.yaw.toFixed(1)+" deg/s")+
 kv("Zero",m.zeroing?"zeroing...":m.zeroed?"done":"<span style=color:#ffb300>not yet</span>");
if(!imu.done){$("ifwd").value=String(m.fwd);$("ileft").value=String(m.left);imu.done=1;}}catch(e){}}
async function imuSet(){$("imsg").textContent=await (await fetch("/imu/set?fwd="+$("ifwd").value+"&left="+$("ileft").value)).text();}
async function imuZero(){$("imsg").textContent=await (await fetch("/imu/zero")).text();}
const I2CNAME={0x3C:"OLED",0x3D:"OLED",0x68:"MPU6050",0x69:"MPU6050",0x48:"ADS1115",0x49:"ADS1115",0x4A:"ADS1115",0x4B:"ADS1115",0x20:"MCP23017",0x21:"MCP23017",0x22:"MCP23017",0x23:"MCP23017",0x24:"MCP23017",0x25:"MCP23017",0x26:"MCP23017",0x27:"MCP23017"};
async function i2c(){try{const b=await (await fetch("/i2c.json")).json();
let h=b.found.length?b.found.map(a=>kv("0x"+a.toString(16).toUpperCase().padStart(2,"0"),I2CNAME[a]||"unknown device")).join(""):kv("Bus","nothing found");
$("i2c").innerHTML=h;}catch(e){}}
poll();setInterval(poll,1000);i2c();setInterval(i2c,1000);imu();setInterval(imu,500);spd();setInterval(spd,1000);clk();setInterval(clk,1000);sh();setInterval(sh,1000);wf();setInterval(wf,3000);
</script></body></html>)HTML";

static const char SENSORS_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Sensors</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:640px;margin:auto;padding:12px}h2{color:#4dd0e1}a{color:#4dd0e1}
table{width:100%;border-collapse:collapse}td,th{padding:5px 8px;text-align:left;border-bottom:1px solid #222}th{color:#999;font-weight:normal}
.v{font-family:monospace;text-align:right}.tag{padding:1px 6px;border-radius:4px;font-size:12px}.hub{background:#1b5e20}.sim{background:#e65100}.can{background:#0d47a1}
.stale td{color:#666}.note{color:#999;font-size:13px})HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Sensors</h2><div class="note" id="sum"></div>
<table><thead><tr><th>Signal</th><th class="v">Value</th><th>Source</th><th class="v">Age</th></tr></thead><tbody id="t"></tbody></table>
<p class="note">Hub = a sensor wired to this board. ECU = decoded from a CAN frame. Grey = not heard for 2 s.</p>
<h3 style="color:#4dd0e1;margin:16px 0 4px">On the bus, not decoded</h3>
<div class="note" id="usum"></div>
<table><thead><tr><th>CAN ID</th><th>From</th><th>Bytes</th><th class="v">Age</th><th></th></tr></thead><tbody id="u"></tbody></table>
<p class="note">Frames heard on the CAN bus that no sensor reads yet. They show up here by themselves as soon as they appear. "Make a sensor" opens a custom slot for that frame.</p>
<p><a href="/">Back</a> | <a href="/can">Raw CAN</a></p>
<script>
const NAME={1:"RPM",2:"Coolant Temp",3:"MAP/Boost",4:"AFR",5:"Oil Pressure",6:"Oil Temp",7:"Fuel Pressure",8:"Ethanol",9:"Throttle Position",10:"Battery Voltage",
11:"Rev Limiter",12:"Main Relay",13:"Fuel Pump",14:"Check Engine",15:"O2 Heater",16:"Lambda Protection",17:"Fan 1",18:"Fan 2",19:"Brake Pedal",
20:"EGT",21:"Intake Air Temp",22:"Ambient Temp",23:"Humidity",24:"Fuel Level",25:"Speed",26:"GPS Speed",27:"GPS Satellites",45:"Lateral G",46:"Accel/Brake G",47:"Vertical G",48:"Yaw Rate"};
const UNIT={1:"rpm",2:"F",3:"psi",4:"",5:"psi",6:"F",7:"psi",8:"%",9:"%",10:"V",20:"F",21:"F",22:"F",23:"%",24:"%",25:"mph",26:"mph",27:"sats"};
const DEC={1:0,2:0,3:0,4:1,5:0,6:0,7:0,8:0,9:0,10:1,20:0,21:0,22:1,23:0,24:0,25:0,26:0,27:0};
const HUB={4294901761:"MAX6675 thermocouple (GPIO 18/19/23)",4294901762:"DHT22 (GPIO 4)",4294901763:"Fuel sender pot (GPIO 36)",4294901764:"Bench simulator (FAKE)",4294901766:"GT-U7 GPS (GPIO 16/17, R17/R16)"};
function ecu(id){return id>=0x200&&id<=0x20F?"rusEFI":id>=0x5F0&&id<=0x63F?"MegaSquirt/Link":"";}
async function u(){try{const l=(await (await fetch("/sensors.json")).json()).sort((a,b)=>(NAME[a.sig]||"").localeCompare(NAME[b.sig]||""));let h=0,c=0,e=new Set();
document.getElementById("t").innerHTML=l.map(s=>{const isHub=HUB[s.src]!==undefined;isHub?h++:c++;const k=ecu(s.src);if(!isHub&&k)e.add(k);
const flag=s.sig>=11&&s.sig<=19;const val=flag?(s.v>0.5?"ON":"off"):s.v.toFixed(DEC[s.sig]??1)+" "+(UNIT[s.sig]||"");
const src=s.src===4294901764?'<span class="tag sim">Sim</span> '+HUB[s.src]:isHub?'<span class="tag hub">Hub</span> '+HUB[s.src]:'<span class="tag can">ECU</span> CAN 0x'+s.src.toString(16).toUpperCase()+(k?" ("+k+")":"");
return '<tr class="'+(s.age>2000?'stale':'')+'"><td>'+(NAME[s.sig]||("Signal "+s.sig))+'</td><td class="v">'+val+'</td><td>'+src+'</td><td class="v">'+(s.age<1000?s.age+" ms":(s.age/1000).toFixed(0)+" s")+'</td></tr>';}).join("");
document.getElementById("sum").textContent=h+" from the hub, "+c+" from the ECU"+(e.size?" ("+[...e].join(", ")+")":"");}catch(x){}}
async function und(){try{const used=new Set((await (await fetch("/sensors.json")).json()).map(s=>s.src));
const j=await (await fetch("/can.json")).json();
const l=j.ids.filter(x=>!used.has(x.id)&&x.age<10000).sort((a,b)=>a.id-b.id);
document.getElementById("usum").textContent=l.length?l.length+" frame"+(l.length===1?"":"s")+" not read by any sensor":"Every frame on the bus is read by a sensor (or the bus is quiet).";
document.getElementById("u").innerHTML=l.map(x=>'<tr class="'+(x.age>2000?'stale':'')+'"><td class="v" style="text-align:left">0x'+x.id.toString(16).toUpperCase().padStart(3,"0")+'</td><td>'+(ecu(x.id)||"unknown")+'</td><td class="v" style="text-align:left">'+x.d+'</td><td class="v">'+(x.age<1000?x.age+" ms":(x.age/1000).toFixed(0)+" s")+'</td><td><a href="/custom?id=0x'+x.id.toString(16).toUpperCase()+'">Make a sensor</a></td></tr>').join("");}catch(x){}}
(async()=>{try{for(const c of await (await fetch("/custom.json")).json())if(c.name)NAME[28+c.i]=c.name;}catch(x){}u();})();setInterval(u,500);und();setInterval(und,1000);
</script></body></html>)HTML";

static const char SIM_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Simulator</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:480px;margin:auto;padding:12px}h2{color:#4dd0e1}a{color:#4dd0e1}
.warnbox{background:#2a1f00;border:1px solid #f5a623;border-radius:8px;padding:10px;font-size:.85em;color:#f5c26b}
.statebox{margin:12px 0;padding:10px;border-radius:8px;background:#111;text-align:center}.statebox.on{background:#f5a623;color:#000;font-weight:bold}
.btnrow{display:flex;gap:8px;justify-content:center;flex-wrap:wrap}button{padding:10px 14px;background:#4dd0e1;color:#000;border:0;border-radius:6px}
#msg{text-align:center;margin:10px 0;min-height:1.2em;color:#4dd0e1}#msg.bad{color:#ff6b6b}
.sl{margin:10px 0}.sl label{display:flex;justify-content:space-between;font-size:.85em;color:#4dd0e1}.sl input{width:100%})HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Bench Simulator</h2>
<div class="warnbox"><b>This feeds FAKE values</b> to every screen, so you can test with no ECU connected. It switches itself off after 10 minutes without a change, at every reboot of the hub, and the moment a real CAN frame arrives. The screens say SIMULATED and the main screen does not log while it runs.</div>
<div id="state" class="statebox">Loading...</div>
<div class="btnrow"><button onclick="cmd('start','demo')">Start demo</button><button onclick="cmd('start','manual')">Start manual</button><button onclick="cmd('stop')">Stop</button></div>
<div id="msg"></div><div id="sliders"></div>
<p><a href="/">Back</a> | <a href="/sensors">Sensors</a></p>
<script>
const NAME={1:"RPM",2:"Coolant Temp (F)",3:"Boost (psi)",4:"AFR",5:"Oil Pressure (psi)",6:"Oil Temp (F)",7:"Fuel Pressure (psi)",8:"Ethanol %",9:"Throttle %",10:"Battery (V)",
11:"Rev Limiter",12:"Main Relay",13:"Fuel Pump",14:"Check Engine",15:"O2 Heater",16:"Lambda Protection",17:"Fan 1",18:"Fan 2",19:"Brake Pedal",
20:"EGT (F)",21:"Intake Air Temp (F)",22:"Ambient Temp (F)",23:"Humidity %",24:"Fuel Level %",25:"Speed (mph)",26:"GPS Speed (mph)",27:"GPS Satellites",45:"Lateral G",46:"Accel/Brake G",47:"Vertical G",48:"Yaw Rate"};
let info=null,building="",sending=false,pending=null;
function setMsg(t,bad){const m=document.getElementById("msg");m.textContent=t;m.className=bad?"bad":"";}
async function post(p){const r=await fetch("/sim",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams(p).toString()});
const t=await r.text();if(!r.ok)throw new Error(t||("board returned "+r.status));return t;}
async function cmd(c,mode){try{await post(mode?{cmd:c,mode:mode}:{cmd:c});setMsg("");}catch(e){setMsg(e.message,true);}refresh();}
function fmtTime(s){return Math.floor(s/60)+":"+String(s%60).padStart(2,"0");}
function shown(s){return Number(s.value).toFixed(s.step<1?1:0);}
function render(){const st=document.getElementById("state");
if(info.active){st.className="statebox on";st.textContent="SIMULATING ("+(info.manual?"manual":"demo")+") - switches off in "+fmtTime(info.seconds_left);}
else{st.className="statebox";st.textContent="Off"+(info.stop_reason?" - last stopped: "+info.stop_reason:"");}
const box=document.getElementById("sliders"),key=info.active?(info.manual?"manual":"demo"):"off";
if(key!==building){building=key;if(key==="off"){box.innerHTML="";return;}
box.innerHTML=info.signals.map(s=>'<div class="sl"><label>'+(NAME[s.id]||("Signal "+s.id))+' <span id="v'+s.id+'"></span></label><input id="r'+s.id+'" type="range" min="'+s.min+'" max="'+s.max+'" step="'+s.step+'" '+(info.manual?"":"disabled")+' oninput="slide('+s.id+',this.value)"></div>').join("");}
for(const s of info.signals){const lab=document.getElementById("v"+s.id),rng=document.getElementById("r"+s.id);if(lab)lab.textContent=shown(s);if(rng&&document.activeElement!==rng)rng.value=s.value;}}
function slide(id,val){document.getElementById("v"+id).textContent=val;pending={id:id,val:val};pump();}
async function pump(){if(sending)return;sending=true;while(pending){const p=pending;pending=null;try{await post({cmd:"set",sig:p.id,val:p.val});}catch(e){setMsg(e.message,true);}}sending=false;}
async function refresh(){try{info=await (await fetch("/sim.json")).json();render();}catch(e){document.getElementById("state").textContent="Could not reach the hub - retrying...";}}
setInterval(refresh,1000);refresh();
</script></body></html>)HTML";

// /custom: the 16 custom sensor slots (custom_slots.h). Stage 1: each reads a field of a CAN frame.
static const char CUSTOM_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Custom Sensors</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:640px;margin:auto;padding:12px}h2,h3{color:#4dd0e1}a{color:#4dd0e1}
table{width:100%;border-collapse:collapse}td,th{padding:5px 8px;text-align:left;border-bottom:1px solid #222}th{color:#999;font-weight:normal}
.v{font-family:monospace;text-align:right}.note{color:#888;font-size:13px}.stale{color:#666}
.box{background:#111;border-radius:8px;padding:12px;margin:12px 0}label{display:block;margin:6px 0 2px;color:#4dd0e1;font-size:13px}
input,select{width:100%;box-sizing:border-box;padding:6px;background:#000;color:#ddd;border:1px solid #333;border-radius:4px}
.row{display:flex;gap:8px}.row>div{flex:1}button{padding:8px 14px;background:#4dd0e1;color:#000;border:0;border-radius:6px;margin:8px 8px 0 0}
button.del{background:#c62828;color:#fff}#preview{font-family:monospace;font-size:18px;color:#4dd0e1;margin-top:8px}#msg{min-height:1.2em;color:#4dd0e1}#msg.bad{color:#ff6b6b})HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Custom Sensors</h2>
<p class="note">Read any value from a CAN frame and send it to the screens. Each slot shows up in the screens' gauge, lamp and logging lists under its name within a few seconds, no reflash.</p>
<table><thead><tr><th>Slot</th><th>Name</th><th>From</th><th class="v">Value</th><th></th></tr></thead><tbody id="list"></tbody></table>
<div class="box" id="ed" style="display:none">
<h3 id="edtitle">Slot</h3>
<div class="row"><div><label>Name</label><input id="name" maxlength="15"></div><div><label>Unit</label><input id="unit" maxlength="7"></div></div>
<div class="row"><div><label>Decimals</label><select id="dec"><option>0</option><option>1</option><option>2</option><option>3</option></select></div>
<div><label>Gauge min</label><input id="min" type="number" step="any"></div><div><label>Gauge max</label><input id="max" type="number" step="any"></div></div>
<label>CAN ID (frames seen on the bus, or type one like 0x208)</label><input id="cid" list="ids" oninput="pv()"><datalist id="ids"></datalist>
<div class="row"><div><label>Start byte (0-7)</label><input id="start" type="number" min="0" max="7" oninput="pv()"></div>
<div><label>Size</label><select id="bits" onchange="shape();pv()"><option value="8">8 bit</option><option value="16">16 bit</option><option value="32">32 bit</option><option value="1">1 bit (on/off)</option></select></div></div>
<div class="row" id="multi"><div><label>Byte order</label><select id="be" onchange="pv()"><option value="0">Little endian (Intel, rusEFI)</option><option value="1">Big endian (Motorola, MegaSquirt/Link)</option></select></div>
<div><label>Signed</label><select id="sgn" onchange="pv()"><option value="0">No</option><option value="1">Yes</option></select></div></div>
<div class="row" id="onebit" style="display:none"><div><label>Bit (0-7)</label><input id="bit" type="number" min="0" max="7" oninput="pv()"></div></div>
<div class="row" id="lin"><div><label>Scale (value = raw x scale + offset)</label><input id="scale" type="number" step="any" oninput="pv()"></div><div><label>Offset</label><input id="offset" type="number" step="any" oninput="pv()"></div></div>
<div id="preview">-</div><div class="note" id="raw"></div>
<button onclick="save()">Save</button><button onclick="closeEd()">Cancel</button><button class="del" onclick="clr()">Clear slot</button>
<div id="msg"></div></div>
<p><a href="/">Back</a> | <a href="/sensors">Sensors</a></p>
<script>
let slots=[],can={ids:[]},cur=-1;
const $=id=>document.getElementById(id);
const esc=t=>String(t).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;");
const hex=n=>"0x"+Number(n).toString(16).toUpperCase().padStart(3,"0");
// Same maths as custom_can_decode() in custom_slots.h. null = the frame is too short for this field.
function decode(c,bytes){
  if(c.bits==1){if(c.start>=bytes.length)return null;return (bytes[c.start]>>c.bit)&1;}
  const n=c.bits/8;if(c.start+n>bytes.length)return null;
  let raw=0;for(let i=0;i<n;i++){const b=c.be?bytes[c.start+i]:bytes[c.start+n-1-i];raw=raw*256+b;}
  if(c.sgn){const top=Math.pow(2,c.bits);if(raw>=top/2)raw-=top;}
  return raw*c.scale+c.offset;}
function bytesOf(id){const f=can.ids.find(x=>x.id===id);return f?f.d.trim().split(/\s+/).filter(Boolean).map(h=>parseInt(h,16)):null;}
function form(){return{i:cur,name:$("name").value.trim(),unit:$("unit").value.trim(),dec:+$("dec").value,min:parseFloat($("min").value),max:parseFloat($("max").value),
  id:parseInt($("cid").value),start:parseInt($("start").value),bits:+$("bits").value,bit:parseInt($("bit").value)||0,be:+$("be").value,sgn:+$("sgn").value,
  scale:$("scale").value===""?1:parseFloat($("scale").value),offset:parseFloat($("offset").value)||0};}
function shape(){const one=$("bits").value==="1";$("onebit").style.display=one?"":"none";$("multi").style.display=one?"none":"";$("lin").style.display=one?"none":"";}
function pv(){const c=form();const b=bytesOf(c.id);
  if(isNaN(c.id)){$("preview").textContent="-";$("raw").textContent="Pick or type a CAN ID";return;}
  if(!b){$("preview").textContent="-";$("raw").textContent=hex(c.id)+" not heard on the bus right now";return;}
  $("raw").textContent=hex(c.id)+": "+b.map(x=>x.toString(16).toUpperCase().padStart(2,"0")).join(" ");
  const v=isNaN(c.start)?null:decode(c,b);
  $("preview").textContent=v===null?"does not fit in this frame":(c.bits==1?(v?"ON":"off"):v.toFixed(c.dec)+(c.unit?" "+c.unit:""));}
function srcText(s){return s.src==1?(hex(s.id)+" byte "+s.start+(s.bits==1?" bit "+s.bit:", "+s.bits+" bit")):"";}
function render(){$("list").innerHTML=slots.map(s=>'<tr class="'+(s.name&&!s.has?'stale':'')+'"><td>'+(s.i+1)+'</td><td>'+(s.name?esc(s.name):'<span class="note">empty</span>')+'</td><td class="note">'+srcText(s)+'</td><td class="v">'+
  (s.name&&s.has?(s.bits==1?(s.value?"ON":"off"):s.value.toFixed(s.dec)+(s.unit?" "+esc(s.unit):"")):"--")+'</td><td><button onclick="edit('+s.i+')">'+(s.name?"Edit":"Set up")+'</button></td></tr>').join("");
  $("ids").innerHTML=can.ids.map(x=>'<option value="'+hex(x.id)+'">').join("");}
function edit(i,id){cur=i;const s=slots[i];$("ed").style.display="";$("edtitle").textContent="Slot "+(i+1);$("msg").textContent="";
  $("name").value=s.name;$("unit").value=s.unit;$("dec").value=String(s.dec);$("min").value=s.name?s.min:0;$("max").value=s.name?s.max:100;
  $("cid").value=s.src==1?hex(s.id):(id||"");$("start").value=s.src==1?s.start:0;$("bits").value=String(s.src==1?s.bits:8);$("bit").value=s.bit;
  $("be").value=String(s.be);$("sgn").value=String(s.sgn);$("scale").value=s.src==1?s.scale:1;$("offset").value=s.src==1?s.offset:0;shape();pv();}
function closeEd(){cur=-1;$("ed").style.display="none";}
function setMsg(t,bad){$("msg").textContent=t;$("msg").className=bad?"bad":"";}
async function save(){const c=form();if(!c.name){setMsg("Give the sensor a name",true);return;}
  const q=new URLSearchParams({i:c.i,name:c.name,unit:c.unit,dec:c.dec,min:c.min,max:c.max,id:hex(c.id),start:c.start,bits:c.bits,bit:c.bit,be:c.be,sgn:c.sgn,scale:c.scale,offset:c.offset});
  const r=await fetch("/custom/set?"+q.toString());const t=await r.text();setMsg(t,!r.ok);if(r.ok)load();}
async function clr(){if(cur<0)return;const r=await fetch("/custom/clear?i="+cur);setMsg(await r.text(),!r.ok);if(r.ok){closeEd();load();}}
async function load(){try{slots=await (await fetch("/custom.json")).json();render();}catch(e){}}
async function tick(){try{can=await (await fetch("/can.json")).json();if(cur>=0)pv();}catch(e){}load();}
(async()=>{await load();await tick();const want=new URLSearchParams(location.search).get("id");
  if(want){const free=slots.findIndex(s=>!s.name);if(free>=0)edit(free,want);}})();
setInterval(tick,1000);
</script></body></html>)HTML";
// /rf: CC1101 433 MHz listen mode - every burst it hears, as pulses, bits and Manchester bytes.
static const char RF_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH 433 MHz</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:720px;margin:auto;padding:12px}h2{color:#4dd0e1}a{color:#4dd0e1}
.box{background:#111;border-radius:8px;padding:10px;margin:10px 0}.note{color:#888;font-size:13px}.bad{color:#ff6b6b}
.mono{font-family:monospace;font-size:12px;word-break:break-all}.b{border-top:1px solid #222;padding:8px 0}.hex{color:#4dd0e1;font-size:14px}
select,input{background:#000;color:#ddd;border:1px solid #333;border-radius:4px;padding:5px}button{padding:6px 12px;background:#4dd0e1;color:#000;border:0;border-radius:6px}
svg{width:100%;height:140px;background:#000;border:1px solid #222;border-radius:4px})HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>433 MHz listen</h2>
<div class="box" id="st">Loading...</div>
<div class="box"><svg id="ch" viewBox="0 0 240 100" preserveAspectRatio="none"></svg>
<div class="note">Last 2 minutes, one point per second: <span style="color:#4dd0e1">strongest signal</span>, <span style="color:#888">background</span>, <span style="color:#f5a623">threshold</span> (scale -110 to -30 dBm). A sensor sending shows as a spike.</div></div>
<div class="box">Mode <select id="mod"><option value="fsk">2-FSK (most tire sensors)</option><option value="ook">OOK / ASK (remotes, some sensors)</option></select>
<label><input type="checkbox" id="auto" onchange="shape()"> Automatic threshold</label>
<span id="mbox">margin <input id="margin" type="number" min="3" max="40" style="width:55px"> dB above the background</span>
<span id="tbox">threshold <input id="thr" type="number" min="-110" max="-30" style="width:70px"> dBm</span>
<button onclick="save()">Save</button> <span id="msg" class="note"></span>
<div><label><input type="checkbox" id="hide" checked onchange="draw()"> Hide noise (show only bursts with 24+ Manchester bits)</label></div>
<div class="note">Tire sensors send when the wheel turns, when the pressure changes, or every few minutes when parked. Watch for a spike when the kit's display changes.</div></div>
<h3 style="color:#4dd0e1">Strongest bursts</h3><div class="note">The 5 strongest since the last clear. Noise cannot push these out. <button onclick="clr()">Clear</button></div>
<div id="top"></div><pre id="full" class="mono box" style="display:none;white-space:pre-wrap"></pre>
<h3 style="color:#4dd0e1">Latest bursts</h3>
<div id="list"></div>

<script>
const $=id=>document.getElementById(id);let loaded=false,last=null;
function shape(){$("mbox").style.display=$("auto").checked?"":"none";$("tbox").style.display=$("auto").checked?"none":"";}
async function save(){const q=new URLSearchParams({mod:$("mod").value,auto:$("auto").checked?1:0,margin:$("margin").value,thr:$("thr").value});const r=await fetch("/rf/set?"+q);$("msg").textContent=await r.text();}
function age(ms){return ms<60000?(ms/1000).toFixed(0)+" s ago":(ms/60000).toFixed(1)+" min ago";}
function y(d){return Math.max(0,Math.min(100,(-30-d)*100/80));}
function chart(j){const h=j.hist,n=h.length;if(!n){$("ch").innerHTML="";return;}const x=i=>(240-(n-1-i)*2);
 const line=(k,c)=>'<polyline fill="none" stroke="'+c+'" stroke-width="1" points="'+h.map((p,i)=>x(i)+","+y(p[k]).toFixed(1)).join(" ")+'"/>';
 const thr=j.auto?h.map((p,i)=>x(i)+","+y(p[1]+j.margin).toFixed(1)).join(" "):(x(0)+","+y(j.thr)+" 240,"+y(j.thr));
 $("ch").innerHTML=line(1,"#888")+'<polyline fill="none" stroke="#f5a623" stroke-width="0.7" stroke-dasharray="2,2" points="'+thr+'"/>'+line(0,"#4dd0e1");}
function card(b){return '<div class="b"><b>'+age(b.age)+'</b> | '+b.rssi+' dBm | '+b.pulses+' pulses | half-bit '+b.unit+' us (about '+(b.unit?Math.round(500000/b.unit):0)+' bit/s) | '+b.nbits+' Manchester bits'+(b.inv?' (inverted)':'')+
 ' <button onclick="full('+b.ms+')">All pulses</button><div class="hex mono">'+(b.hex||'(no Manchester bytes)')+'</div><div class="mono note">bits: '+b.bits+'</div><div class="mono note">first pulses (us, first level '+b.first+'): '+b.w.join(" ")+'</div></div>';}
async function full(ms){const r=await fetch("/rf/burst?ms="+ms);$("full").style.display="";$("full").textContent=await r.text();}
async function clr(){await fetch("/rf/clear");$("full").style.display="none";}
function draw(){const j=last;if(!j)return;const hide=$("hide").checked;const l=j.bursts.filter(b=>!hide||b.nbits>=24);
 const t=(j.top||[]).slice().sort((a,b)=>b.rssi-a.rssi);$("top").innerHTML=t.length?t.map(card).join(""):'<p class="note">None yet.</p>';
 $("list").innerHTML=l.length?l.map(b=>'<div class="b"><b>'+age(b.age)+'</b> | '+b.rssi+' dBm | '+b.pulses+' pulses | half-bit '+b.unit+' us (about '+(b.unit?Math.round(500000/b.unit):0)+' bit/s) | '+b.nbits+' Manchester bits'+(b.inv?' (inverted)':'')+
 '<div class="hex mono">'+(b.hex||'(no Manchester bytes)')+'</div><div class="mono note">bits: '+b.bits+'</div><div class="mono note">pulses (us, first level '+b.first+'): '+b.w.join(" ")+'</div></div>').join("")
 :'<p class="note">'+(j.bursts.length&&hide?j.bursts.length+" bursts hidden as noise (untick Hide noise to see them).":"Nothing heard yet above the threshold.")+'</p>';}
async function poll(){try{const j=await (await fetch("/rf.json")).json();last=j;
if(!j.ok){$("st").innerHTML='<span class="bad">CC1101 not found</span> (read part 0x'+j.part.toString(16)+', version 0x'+j.version.toString(16)+'; expected 00 / 14). Check VCC on 3.3 V, GND, and SCK/MOSI/MISO/CSN.';return;}
$("st").innerHTML="CC1101 found (version 0x"+j.version.toString(16)+") | 433.92 MHz, "+(j.ook?"OOK":"2-FSK")+" | signal now "+j.rssi+" dBm | background "+j.floor+" dBm | threshold "+j.active+" dBm"+(j.auto?" (auto)":"")+" | bursts heard: "+j.total;
if(!loaded){$("mod").value=j.ook?"ook":"fsk";$("thr").value=j.thr;$("auto").checked=j.auto;$("margin").value=j.margin;shape();loaded=true;}
chart(j);draw();
}catch(e){$("st").textContent="Could not reach the hub - retrying...";}}
poll();setInterval(poll,1000);
</script></body></html>)HTML";
// /buttons: what each button does, for a press and for holds of 1, 2, 3, 5 and 10 s (button_actions.h).
static const char BUTTONS_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Buttons</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:980px;padding:12px}h2{color:#4dd0e1}h3{color:#4dd0e1;margin:0 0 8px;font-size:16px}
.card{background:#111;border-radius:8px;padding:12px;margin:0 0 12px}.note{color:#888;font-size:13px}table{width:100%;border-collapse:collapse}
td,th{padding:5px 6px;border-bottom:1px solid #222;text-align:left;vertical-align:middle;font-size:14px}th{color:#4dd0e1;font-weight:normal}
select,input{background:#000;color:#ddd;border:1px solid #333;border-radius:4px;padding:5px;max-width:100%}button{padding:6px 11px;background:#4dd0e1;color:#000;border:0;border-radius:6px}
button.sec{background:#1d2b2e;color:#4dd0e1}option:disabled{color:#666}.form{display:grid;grid-template-columns:repeat(auto-fill,minmax(200px,1fr));gap:8px 12px;margin:8px 0}
.form label{display:flex;flex-direction:column;gap:3px;font-size:13px;color:#aaa}#msg{color:#4dd0e1;min-height:1.3em}#msg.bad{color:#ff6b6b}.on{color:#7bd88f;font-weight:bold}
.live{font-family:monospace;font-size:18px;color:#4dd0e1}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Buttons</h2>
<div id="msg"></div>
<div class="card"><h3>Buttons</h3><div id="list"></div>
<p><button onclick="edit(-1,1)">Add a button</button> <button onclick="edit(-1,2)">Add a ladder button</button></p>
<div id="editor"></div>
<p class="note">Each button does one thing when pressed. Page actions (next, previous, go to page) go to the screen you pick; the others are for every screen.
Single button: a switch to ground on any free pin (active when LOW), or a 12 V signal through a divider (active when HIGH).
Ladder: several buttons on one analog pin, each through its own resistor (steering wheel buttons). Add one ladder button at a time: hold it, press Learn, save.</p>
<p class="note">Ladder wiring: 10k from the pin to 3.3 V, and each button from the pin to ground through its own resistor. With 0, 1k, 2.2k, 4.7k, 10k and 22k the pin reads about 0, 0.30, 0.60, 1.05, 1.65 and 2.27 V. Two buttons must be at least 0.12 V apart. GPIO 32-36, 39 or an ADS1115 channel (0 to 3.3 V only).</p></div>
<div class="card"><h3>ECU maps</h3><div id="ecu"></div>
<p class="note">For a button set to "ECU map (next)": each press moves to the next map, back to the first after the last. The hub sends the map number on this CAN id (byte 0 = map 1, 2, ...; byte 1 = how many maps) 4 times a second while it hears the ECU, and remembers the map after a restart. Every screen shows the map's name. A rusEFI Lua script applies it: docs/RUSEFI_LUA.md.</p></div>
<script>
const $=id=>document.getElementById(id);let D=null,ed=null,liveT=null;
const GOTO=4,COLOR=7,RACE=8,OUT=13,ECU=14,CYCLE=255,PAGE_ACTS=[1,2,4];
const esc=t=>String(t).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/"/g,"&quot;");
const opt=(v,t,sel,dis)=>'<option value="'+v+'"'+(sel?" selected":"")+(dis?" disabled":"")+'>'+t+'</option>';
const pinName=p=>p>=140?"ADS1115 #"+(Math.floor((p-140)/4)+1)+" A"+((p-140)%4):p>=100?"MCP23017 #"+(p>=116?2:1)+" "+((p-100)%16<8?"PA":"PB")+((p-100)%8):"GPIO "+p;
function msg(t,bad){$("msg").textContent=t;$("msg").className=bad?"bad":"";}
async function load(){try{D=await (await fetch("/buttons.json")).json();render();}catch(e){msg("Could not reach the hub",true);}}
function what(b){let t=D.actions[b.a]||"?";
 if(b.a===GOTO)t+=" "+b.p;if(b.a===RACE)t+=", page "+b.p;if(b.a===OUT)t+=" "+b.p;
 if(b.a===COLOR)t+=b.p===CYCLE?" (next each time)":": "+(D.colors[b.p]||"");
 if(PAGE_ACTS.includes(b.a))t+=" on "+(b.t?D.targets[b.t]:"all screens");
 return esc(t);}
function render(){
 const used=D.buttons.map((b,i)=>[b,i]).filter(a=>a[0].kind);
 $("list").innerHTML=used.length?'<table><thead><tr><th>#</th><th>Name</th><th>Pin</th><th>Now</th><th>On a press</th><th></th></tr></thead><tbody>'+used.map(([b,i])=>
  '<tr><td>'+(i+1)+'</td><td>'+esc(b.name)+'</td><td>'+pinName(b.pin)+(b.kind===2?' (ladder, '+(b.mv/1000).toFixed(2)+' V)':(b.high?' (active HIGH)':''))+'</td><td>'+
  (b.down?'<span class="on">PRESSED</span>':(b.kind===2?(b.now/1000).toFixed(2)+' V':'-'))+'</td><td>'+what(b)+'</td><td><button class="sec" onclick="edit('+i+')">Edit</button> <button class="sec" onclick="clearB('+i+')">Remove</button></td></tr>').join("")+
  '</tbody></table>':'<p class="note">No buttons set up yet.</p>';
 if(ed===null&&ecuDraft===null)renderEcu();}
// Pins for a kind: every pin that can do it; the ones in use are shown but cannot be picked (a ladder pin can take more ladder buttons).
function pinList(kind,i){const cur=D.buttons[i]||{};
 return D.pins.filter(([p,c])=>kind===1?((p>=100&&p<140)||(p<100&&(c&1))):(p>=140||(p<100&&(c&4)))).map(([p,c,u])=>{
  const mine=cur.kind&&cur.pin===p,share=kind===2&&/^Button ladder/.test(u),free=!u||mine||share;
  return {p:p,free:free,label:pinName(p)+(u&&!mine?(share?" - "+u:" - used by "+u):"")};});}
function edit(i,kind){if(i<0){i=D.buttons.findIndex(b=>!b.kind);if(i<0){msg("All 24 buttons are in use",true);return;}}
 const b=D.buttons[i];ed={i:i,kind:b.kind||kind||1,pin:b.pin,high:b.high,a:b.kind?b.a:1,p:b.p,t:b.t,mv:b.mv,name:b.name||"",now:null};
 if(!b.kind&&ed.kind===2){const lad=D.buttons.find(x=>x.kind===2);if(lad)ed.pin=lad.pin;}   // the next ladder button: on the same pin as the last
 draw();setTimeout(()=>{const e=$("editor");if(e&&e.scrollIntoView)e.scrollIntoView({behavior:"smooth"});},0);}
function val(id,d){const x=$(id);return x&&x.value!==undefined&&x.value!==""?x.value:d;}
function keep(){if(!ed)return;ed.name=val("b_name",ed.name);ed.pin=+val("b_pin",ed.pin);ed.high=+val("b_high",ed.high);ed.p=+val("b_p",ed.p);ed.t=+val("b_t",ed.t);ed.mv=+val("b_mv",ed.mv);}
function paramHtml(){const a=ed.a,p=ed.p;let o="";
 if(a===GOTO||a===RACE){for(let n=1;n<=8;n++)o+=opt(n,(a===RACE?"Race page ":"Page ")+n,n===p);return '<label>Which page<select id="b_p">'+o+'</select></label>';}
 if(a===COLOR){o=opt(CYCLE,"Next scheme each time",p===CYCLE);D.colors.forEach((n,k)=>o+=opt(k,(k+1)+". "+esc(n),k===p));return '<label>Color scheme<select id="b_p">'+o+'</select></label>';}
 if(a===OUT){for(let n=1;n<=16;n++)o+=opt(n,"Output "+n,n===p);return '<label>Which output<select id="b_p">'+o+'</select></label>';}
 return "";}
function draw(){if(liveT){clearInterval(liveT);liveT=null;}
 if(!ed){$("editor").innerHTML="";renderEcu();return;}
 const e=ed,pins=pinList(e.kind,e.i);if(!pins.some(x=>x.p===e.pin&&x.free)){const f=pins.find(x=>x.free);e.pin=f?f.p:-1;}
 let h='<div class="card" style="background:#0b1a1d"><h3>Button '+(e.i+1)+'</h3><div class="form">'+
  '<label>Name<input id="b_name" maxlength="15" value="'+esc(e.name)+'"></label>'+
  '<label>Kind<select onchange="keep();ed.kind=+this.value;draw()">'+opt(1,"Single button",e.kind===1)+opt(2,"On a resistor ladder",e.kind===2)+'</select></label>'+
  '<label>Pin<select id="b_pin" onchange="keep();ed.now=null;draw()">'+(pins.some(x=>x.free)?pins.map(x=>opt(x.p,esc(x.label),x.p===e.pin,!x.free)).join(""):'<option value="">(no free pin)</option>')+'</select></label>';
 if(e.kind===1)h+='<label>Active when<select id="b_high">'+opt(0,"LOW (switch to ground, optocoupler)",!e.high)+opt(1,"HIGH (12 V through a divider)",!!e.high)+'</select></label>';
 else h+='<label>Pin now<span class="live" id="b_now">'+(e.now===null?"...":(e.now/1000).toFixed(3)+" V")+'</span></label>'+
  '<label>Learned (mV)<input id="b_mv" type="number" min="1" max="3300" value="'+(e.mv||"")+'"></label>'+
  '<label>&nbsp;<button class="sec" onclick="learn()">Learn (hold the button)</button></label>';
 h+='<label>On a press<select onchange="keep();ed.a=+this.value;ed.p=(ed.a===GOTO||ed.a===RACE||ed.a===OUT)?1:(ed.a===COLOR?CYCLE:0);draw()">'+D.actions.map((n,k)=>opt(k,esc(n),k===e.a)).join("")+'</select></label>'+paramHtml();
 if(PAGE_ACTS.includes(e.a))h+='<label>On which screen<select id="b_t">'+D.targets.map((n,k)=>opt(k,esc(n),k===e.t)).join("")+'</select></label>';
 h+='</div>';
 if(e.a===ECU)h+='<p class="note">Set up the maps under ECU maps below.</p>';
 if(e.a===1&&e.t===4||e.a===3)h+='<p class="note">"Next gauge" picks a screen; buttons set to "Chosen screen" then control it (all screens until one is picked, back to all after a minute).</p>';
 h+='<button onclick="saveB()">Save button '+(e.i+1)+'</button> <button class="sec" onclick="ed=null;draw()">Cancel</button></div>';
 $("editor").innerHTML=h;
 if(e.kind===2&&e.pin>=0){readPin();liveT=setInterval(readPin,800);}}
async function readPin(){if(!ed||ed.kind!==2)return;try{const r=await fetch("/buttons/read?pin="+ed.pin);if(!r.ok)return;ed.now=+(await r.text());const x=$("b_now");if(x)x.textContent=(ed.now/1000).toFixed(3)+" V";}catch(e){}}
async function learn(){keep();await readPin();if(ed.now===null){msg("Could not read the pin",true);return;}
 const near=D.buttons.map((b,k)=>[b,k]).find(([b,k])=>k!==ed.i&&b.kind===2&&b.pin===ed.pin&&Math.abs(b.mv-ed.now)<120);
 ed.mv=ed.now;$("b_mv").value=ed.mv;
 msg(near?"Learned "+ed.mv+" mV, but that is button "+(near[1]+1)+" ("+near[0].name+"). Hold the new button and Learn again.":"Learned "+ed.mv+" mV. Save to keep it.",!!near);}
async function saveB(){keep();const e=ed;
 const q={i:e.i,kind:e.kind,pin:e.pin,high:e.kind===1?e.high:0,a:e.a,p:e.p,t:PAGE_ACTS.includes(e.a)?e.t:0,mv:e.kind===2?e.mv:0,name:e.name};
 const r=await fetch("/buttons/set?"+new URLSearchParams(q));msg(await r.text(),!r.ok);if(r.ok){ed=null;draw();load();}}
async function clearB(i){if(!confirm("Remove button "+(i+1)+"?"))return;const r=await fetch("/buttons/clear?i="+i);msg(await r.text(),!r.ok);load();}
// ---- ECU maps ----
let ecuDraft=null;
function renderEcu(){const c=ecuDraft||D.ecu;
 let h='<div class="form"><label>CAN id (hex)<input id="m_id" oninput="keepEcu()" value="0x'+c.id.toString(16).toUpperCase()+'"></label>'+
  '<label>Maps<select onchange="keepEcu();ecuDraft.count=+this.value;renderEcu()">'+[1,2,3,4,5,6,7,8].map(n=>opt(n,n,n===c.count)).join("")+'</select></label>';
 for(let m=0;m<c.count;m++)h+='<label>Map '+(m+1)+' name'+(m===D.ecu.cur?' (in use)':'')+'<input id="m_n'+m+'" maxlength="12" oninput="keepEcu()" value="'+esc(c.names[m]||"")+'"></label>';
 h+='</div><button onclick="saveEcu()">Save the maps</button>'+(D.ecu.used?'':' <span class="note">No button is set to "ECU map (next)" yet, so nothing is sent.</span>');
 $("ecu").innerHTML=h;}
function keepEcu(){const c=ecuDraft||JSON.parse(JSON.stringify(D.ecu));c.id=parseInt(val("m_id","0x"+c.id.toString(16)))||c.id;
 for(let m=0;m<c.count;m++)c.names[m]=val("m_n"+m,"");ecuDraft=c;}
async function saveEcu(){keepEcu();const c=ecuDraft,q={id:"0x"+c.id.toString(16),count:c.count};for(let m=0;m<8;m++)q["n"+m]=m<c.count?c.names[m]:"";
 const r=await fetch("/buttons/ecu?"+new URLSearchParams(q));msg(await r.text(),!r.ok);if(r.ok){ecuDraft=null;load();}}
load();setInterval(()=>{if(ed===null)load();},1500);
</script></body></html>)HTML";
// /oled: the warning-light panel on the 0.96" OLED - a live copy of the display and the 8 lamps.
static const char OLED_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH OLED</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:760px;margin:auto;padding:12px}h2{color:#4dd0e1}a{color:#4dd0e1}
.card{background:#111;border-radius:8px;padding:12px;margin:0 0 12px}.note{color:#888;font-size:13px}canvas{image-rendering:pixelated;width:100%;max-width:512px;border:1px solid #333;border-radius:4px;background:#000}
select,input{background:#000;color:#ddd;border:1px solid #333;border-radius:4px;padding:5px;max-width:100%}button{padding:7px 12px;background:#4dd0e1;color:#000;border:0;border-radius:6px}
.lamp{border-top:1px solid #222;padding:10px 0}.lamp:first-child{border-top:0}.l1,.l2{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin:4px 0}
.n{width:26px;color:#888}.st{font-size:12px;padding:2px 6px;border-radius:4px}.WARNING{background:#5a4300;color:#ffd54f}.DANGER{background:#5a0000;color:#ff6b6b}.off{color:#666}
#msg{color:#4dd0e1;min-height:1.2em}input[type=number]{width:80px}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>OLED warning panel</h2>
<div class="card"><canvas id="cv" width="128" height="64"></canvas><div class="note" id="st"></div></div>
<div class="card"><h3 style="color:#4dd0e1;margin:0 0 6px">Pages</h3><div id="pages"></div><div id="pmsg" style="color:#4dd0e1;min-height:1.2em"></div>
<p><button onclick="addPage()">Add a page</button> &nbsp; Change page every <input type="number" id="cyc" min="0" max="60" style="width:60px"> s (0 = only the button)
 &nbsp; <label><input type="checkbox" id="tko"> A lit lamp takes over the screen</label> <button onclick="pageOpts()">Save</button></p>
<p><button onclick="pagesDefaults()">Pages back to the defaults</button> <button onclick="peakReset()">Clear peak G</button></p>
<p class="note">Change pages with the "OLED next page" button action (Buttons page) or the timer. With the takeover on, any lit lamp switches to the lamps page until it clears.</p></div>
<h3 style="color:#4dd0e1">Warning lamps</h3>
<div class="card"><div id="rows"></div><div id="msg"></div>
<p>Layout <select id="layout" onchange="opts()"><option value="0">Each lamp in its own spot</option><option value="1">Lit lamps packed together</option></select>
 <button onclick="defaults()">Back to the defaults</button></p>
<p class="note">The OLED shows one colour, so the two levels look different instead: WARNING = the icon (it flashes if Blink is ticked); DANGER = a white box with the icon cut out, flashing. Spots go left to right, top row first.</p>
<p class="note">Input wires: any free hub GPIO or MCP23017 pin (pins in use are greyed out with what uses them). Inputs have pull-ups: wire a switch to ground and pick "lit when LOW" (GPIO 34-39 need an outside 10 k pull-up).</p></div>
<script>
)HTML" OLED_ICONS_JS R"HTML(
const NAME={1:"RPM",2:"Coolant Temp",3:"MAP/Boost",4:"AFR",5:"Oil Pressure",6:"Oil Temp",7:"Fuel Pressure",8:"Ethanol",9:"Throttle Position",10:"Battery Voltage",11:"Rev Limiter",12:"Main Relay",13:"Fuel Pump",14:"Check Engine",15:"O2 Heater",16:"Lambda Protection",17:"Fan 1",18:"Fan 2",19:"Brake Pedal",20:"EGT",21:"Intake Air Temp",22:"Ambient Temp",23:"Humidity",24:"Fuel Level",25:"Speed",26:"GPS Speed",27:"GPS Satellites",45:"Lateral G",46:"Accel/Brake G",47:"Vertical G",48:"Yaw Rate"};
const $=id=>document.getElementById(id);let cfg=null;
const OPS=["Is on","Above","Below","Input wire"],STATE=["off","WARNING","DANGER"];
const INPUTS=[0,2,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33,34,35,36,39].map(g=>[64+g,"GPIO "+g]);   // see the Inputs page for what is free
for(let e=0;e<2;e++)for(let b=0;b<16;b++)INPUTS.push([16+e*16+b,"MCP23017 #"+(e+1)+" "+(b<8?"PA":"PB")+(b%8)]);
const opt=(v,t,sel,dis)=>'<option value="'+v+'"'+(sel?" selected":"")+(dis?" disabled":"")+'>'+t+'</option>';
// What else uses an input wire's pin ("" = free, or this lamp's own): from the hub's shared pin list.
const inUser=(code,i)=>{const u=(cfg.inusers||{})[code]||"";return u==="OLED lamp "+(i+1)?"":u;};
function svg(i){const ic=ICONS[i];return ic&&ic[1]?'<svg viewBox="0 0 24 24" width="24" height="24" style="vertical-align:middle;fill:#4dd0e1"><path d="'+ic[1]+'"/></svg>':"";}
function rowHtml(l,i){
 const st=STATE[l.state]||"off",lvl=l.op===1||l.op===2;
 let h='<div class="lamp" id="lamp'+i+'"><div class="l1"><span class="n">'+(i+1)+'</span>'+svg(l.icon)+
  ' <select id="ic'+i+'">'+ICONS.map((c,k)=>opt(k,k?c[0]:"(unused)",k===l.icon)).join("")+'</select>'+
  ' <select id="op'+i+'" onchange="chOp('+i+',+this.value)">'+OPS.map((o,k)=>opt(k,o,k===l.op)).join("")+'</select>';
 if(l.op===3){h+=' <select id="in'+i+'">'+INPUTS.map(x=>{const u=inUser(x[0],i);return opt(x[0],x[1]+(x[0]>=16&&x[0]<48&&!(cfg.mcp||[])[x[0]>=32?1:0]?" (not found)":"")+(u?" - used by "+u:""),x[0]===l.input,!!u&&x[0]!==l.input);}).join("")+'</select>'+
   ' <select id="lo'+i+'">'+opt(1,"lit when LOW",l.input_low)+opt(0,"lit when HIGH",!l.input_low)+'</select>';}
 else h+=' <select id="sg'+i+'">'+Object.keys(NAME).map(k=>opt(k,NAME[k],+k===l.sig)).join("")+'</select>';
 h+=' <span class="st '+st+'">'+st+(l.op===3?" | input "+(l.in_low?"LOW":"HIGH"):"")+'</span></div><div class="l2"><span class="n"></span>';
 if(lvl){h+='Warning '+(l.op===1?"above":"below")+' <input id="lv'+i+'" type="number" step="any" value="'+l.level+'">'+
   ' <label><input type="checkbox" id="hd'+i+'"'+(l.has_danger?" checked":"")+'> Danger '+(l.op===1?"above":"below")+'</label> <input id="dg'+i+'" type="number" step="any" value="'+l.danger+'">';}
 else h+='<label><input type="checkbox" id="hd'+i+'"'+(l.has_danger?" checked":"")+'> Lit means DANGER (not just a warning)</label>';
 h+=' <label><input type="checkbox" id="bl'+i+'"'+(l.blink?" checked":"")+'> Blink the warning</label> <button onclick="save('+i+')">Save</button></div></div>';
 return h;}
function render(){$("rows").innerHTML=cfg.lamps.map(rowHtml).join("");$("layout").value=String(cfg.layout);}
function chOp(i,op){const l=cfg.lamps[i];l.op=op;if(op===3&&!l.input){l.input=66;l.input_low=1;}const d=document.createElement("div");$("lamp"+i).outerHTML=rowHtml(l,i);}
function val(id,def){const e=$(id);return e?e.value:def;}
async function save(i){const l=cfg.lamps[i],op=+$("op"+i).value;
 const q=new URLSearchParams({i:i,icon:$("ic"+i).value,op:op,sig:op===3?0:val("sg"+i,l.sig),input:op===3?val("in"+i,66):0,input_low:op===3?val("lo"+i,1):1,
  level:val("lv"+i,l.level)||0,danger:val("dg"+i,l.danger)||0,has_danger:$("hd"+i).checked?1:0,blink:$("bl"+i).checked?1:0});
 const r=await fetch("/oled/set?"+q);$("msg").textContent=r.ok?"Spot "+(i+1)+" saved":await r.text();if(r.ok)load();}
async function opts(){$("msg").textContent=await (await fetch("/oled/opts?layout="+$("layout").value)).text();}
async function defaults(){$("msg").textContent=await (await fetch("/oled/defaults")).text();load();}
async function load(){try{cfg=await (await fetch("/oled.json")).json();render();}catch(e){}}
async function live(){try{const b=new Uint8Array(await (await fetch("/oled.bin")).arrayBuffer());const cx=$("cv").getContext("2d"),img=cx.createImageData(128,64);
 for(let y=0;y<64;y++)for(let x=0;x<128;x++){const on=b[(y>>3)*128+x]>>(y&7)&1,p=(y*128+x)*4;img.data[p]=img.data[p+1]=img.data[p+2]=on?255:0;img.data[p+3]=255;}
 cx.putImageData(img,0,0);const j=await (await fetch("/oled.json")).json();
 $("st").textContent=(j.found?"Display found":"Display NOT found: check VCC 3.3 V, GND, SDA GPIO 5 (R15), SCL GPIO 15 (R21)")+" | expanders: "+((j.mcp||[]).filter(a=>a).map((a,e)=>"#"+(e+1)+" at 0x"+a.toString(16).toUpperCase()).join(", ")||"none")+
  " | lit: "+(j.lamps.map((l,i)=>l.state?(i+1)+(l.state===2?" DANGER":" warning"):0).filter(x=>x).join(", ")||"none");}catch(e){}}
const PTYPES=["Warning lamps","Big number","Values (1-4)","G-meter","Clock"];let pg=null;
function pageHtml(p,i){
 let h='<div class="lamp"><div class="l1"><span class="n">'+(i+1)+'</span><select id="pt'+i+'" onchange="pg.pages['+i+'].type=+this.value;renderPages()">'+PTYPES.map((t,k)=>opt(k,t,k===p.type)).join("")+'</select>';
 if(p.type===2)h+=' <select id="pc'+i+'" onchange="pg.pages['+i+'].count=+this.value;renderPages()">'+[1,2,3,4].map(k=>opt(k,k+" value"+(k>1?"s":""),k===p.count)).join("")+'</select>';
 h+=' <button onclick="showPage('+i+')">Show</button> <button onclick="movePage('+i+',-1)"'+(i?"":" disabled")+'>&uarr;</button> <button onclick="movePage('+i+',1)"'+(i<pg.count-1?"":" disabled")+'>&darr;</button>'+
  ' <button onclick="delPage('+i+')">Remove</button> <button onclick="savePage('+i+')">Save</button>'+(pg.shown===i?' <span class="st WARNING">ON SCREEN</span>':'')+'</div>';
 const n=p.type===1?1:p.type===2?p.count:0;
 for(let k=0;k<n;k++)h+='<div class="l2"><span class="n"></span><select id="ps'+i+'_'+k+'" onchange="autoLabel('+i+','+k+')">'+Object.keys(NAME).map(s=>opt(s,NAME[s],+s===p.sig[k])).join("")+'</select>'+
  ' label <input id="pl'+i+'_'+k+'" maxlength="9" style="width:100px" value="'+(p.label[k]||"").replace(/"/g,"")+'">'+
  ' <select id="pd'+i+'_'+k+'">'+[0,1,2].map(d=>opt(d,d+" decimals",d===p.dec[k])).join("")+'</select></div>';
 return h+'</div>';}
function renderPages(){$("pages").innerHTML=pg.pages.map(pageHtml).join("");$("cyc").value=pg.cycle;$("tko").checked=pg.takeover;}
function autoLabel(i,k){const s=+$("ps"+i+"_"+k).value;$("pl"+i+"_"+k).value=(NAME[s]||"").toUpperCase().slice(0,9);}
async function loadPages(){try{pg=await (await fetch("/oled/pages.json")).json();renderPages();}catch(e){}}
async function pcall(u){const r=await fetch(u);$("pmsg").textContent=await r.text();loadPages();return r.ok;}
function savePage(i){const p=pg.pages[i],q={i:i,type:$("pt"+i).value,count:$("pc"+i)?$("pc"+i).value:p.count||1};
 for(let k=0;k<4;k++){const s=$("ps"+i+"_"+k);q["s"+k]=s?s.value:p.sig[k];q["l"+k]=$("pl"+i+"_"+k)?$("pl"+i+"_"+k).value:p.label[k];q["d"+k]=$("pd"+i+"_"+k)?$("pd"+i+"_"+k).value:p.dec[k];}
 return pcall("/oled/page/set?"+new URLSearchParams(q));}
const addPage=()=>pcall("/oled/page/add"),delPage=i=>pcall("/oled/page/del?i="+i),movePage=(i,d)=>pcall("/oled/page/move?i="+i+"&d="+d),showPage=i=>pcall("/oled/page/show?i="+i);
const pageOpts=()=>pcall("/oled/pageopts?cycle="+$("cyc").value+"&takeover="+($("tko").checked?1:0)),pagesDefaults=()=>pcall("/oled/pages/defaults"),peakReset=()=>pcall("/oled/peak/reset");
load();loadPages();live();setInterval(live,700);
</script></body></html>)HTML";
// /screen: one screen's own pages (live view, gauge editor, logging, SD card, settings) inside the hub's site, with a
// Left / Center / Right picker, so no screen address has to be typed. ?s=<screen name>&p=<page>.
static const char SCREEN_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Screens</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;margin:0;padding:12px}h2{color:#4dd0e1;margin:4px 0 10px}
.bar{display:flex;flex-wrap:wrap;gap:6px;align-items:center;margin:0 0 8px}.bar button{background:#1d2b2e;color:#4dd0e1;border:1px solid #1d3b40;border-radius:6px;padding:7px 12px;font-size:14px}
.bar button.on{background:#4dd0e1;color:#000;font-weight:bold}.bar button:disabled{opacity:.4}.sep{width:12px}
iframe{width:100%;height:calc(100vh - 130px);border:1px solid #222;border-radius:8px;background:#000}.note{color:#888;font-size:13px}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body class="wide">)HTML" HUB_NAV R"HTML(
<h2>Screens</h2>
<div class="bar" id="who"></div>
<div class="bar" id="tabs"></div>
<div class="note" id="where"></div>
<iframe id="fr" src="about:blank"></iframe>
<script>
const TABS=[["","Live"],["gauges","Gauges"],["log-channels","Logging"],["sd","SD card"],["settings","Settings"]];
const q=new URLSearchParams(location.search);let cur=q.get("s")||"",page=q.get("p")||"gauges",list=[];
const $=id=>document.getElementById(id),order=n=>({left:0,center:1,centre:1,middle:1,right:2})[(n||"").toLowerCase()]??3;
function show(){const s=list.find(x=>(x.name||x.ip)===cur)||list[0];if(!s){$("where").textContent="No screens heard yet.";return;}
 cur=s.name||s.ip;history.replaceState(null,"","/screen?s="+encodeURIComponent(cur)+"&p="+page);
 $("who").innerHTML=list.map(x=>'<button class="'+((x.name||x.ip)===cur?"on":"")+'" onclick="pick(\''+(x.name||x.ip).replace(/'/g,"")+'\')">'+(x.name||x.ip)+(x.online?"":" (offline)")+'</button>').join("");
 $("tabs").innerHTML=TABS.map(t=>'<button class="'+(t[0]===page?"on":"")+'" onclick="tab(\''+t[0]+'\')">'+t[1]+'</button>').join("");
 const u="http://"+s.ip+"/"+page;$("where").innerHTML=(s.online?"":"<b style=color:#ff6b6b>Offline</b> - ")+s.ip+" | build "+s.build+' | <a style="color:#4dd0e1" href="'+u+'" target="_blank">open on its own</a>';
 if($("fr").src!==u)$("fr").src=u;}
function pick(n){cur=n;show();}function tab(p){page=p;show();}
async function load(){try{list=await (await fetch("/screens.json")).json();list.sort((a,b)=>order(a.name)-order(b.name)||(a.ip<b.ip?-1:1));show();}catch(e){$("where").textContent="Could not reach the hub";}}
load();
</script></body></html>)HTML";

// /can: every CAN frame the hub hears, live.
static const char CAN_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Raw CAN</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:900px;padding:12px}h2{color:#4dd0e1}td,th{padding:3px 10px;font-family:monospace}th{text-align:left;color:#4dd0e1}.old{color:#666}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Raw CAN</h2><div id="s"></div><table><thead><tr><th>ID</th><th>Bytes</th><th>Count</th><th>Age ms</th></tr></thead><tbody id="t"></tbody></table>
<script>
const ST=['stopped','running','bus-off','recovering'];
async function u(){try{const r=await fetch('/can.json');const j=await r.json();
document.getElementById('s').textContent='Frames: '+j.total+' | bus: '+(ST[j.state]||'?')+' | rx errors: '+j.rxerr+' | missed: '+j.missed;
j.ids.sort((a,b)=>a.id-b.id);document.getElementById('t').innerHTML=j.ids.map(e=>'<tr class='+(e.age>2000?'old':'')+'><td>0x'+e.id.toString(16).toUpperCase().padStart(3,'0')+'</td><td>'+e.d+'</td><td>'+e.n+'</td><td>'+e.age+'</td></tr>').join('');}catch(x){}}
setInterval(u,500);u();
</script></body></html>)HTML";

// /firmware: every board's build in one table; the hub's own update form; each screen's update page.
static const char FIRMWARE_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Firmware</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:760px;padding:12px}h2{color:#4dd0e1}h3{color:#4dd0e1;margin:0 0 8px;font-size:16px}.card{background:#111;border-radius:8px;padding:12px;margin:0 0 12px}
table{width:100%;border-collapse:collapse}td,th{padding:6px;border-bottom:1px solid #222;text-align:left}th{color:#4dd0e1;font-weight:normal}a{color:#4dd0e1}.note{color:#888;font-size:13px}
input,button{background:#000;color:#ddd;border:1px solid #333;border-radius:4px;padding:6px}button{background:#4dd0e1;color:#000;border:0}button:disabled{opacity:.4}
progress{width:100%;height:14px}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Firmware</h2>
<div class="card"><table><thead><tr><th></th><th>Board</th><th>Address</th><th>Build</th><th>Status</th><th>Update</th></tr></thead><tbody id="b"></tbody></table></div>
<div class="card"><h3>Update the screens</h3>
<p class="note">Pick the screens' .bin once (C:\esp32build\esp32s3-amoled-bsp-base.ino.bin); it goes to every ticked screen, one after another. Each restarts and comes back with its new build.</p>
<input type="file" id="sbin" accept=".bin"> <button id="sgo" onclick="updScreens()">Update ticked screens</button></div>
<div class="card"><h3>Update the hub</h3>
<p class="note">C:\esp32build-input\esp32-input.ino.bin. The hub restarts at the end; this page comes back by itself.</p>
<input type="file" id="hbin" accept=".bin"> <button id="hgo" onclick="updHub()">Upload</button><progress id="hp" max="100" value="0" style="display:none"></progress></div>
<p class="note">The PowerShell flash commands still work and also build the firmware. Keep the boards powered during an update.</p>
<script>
const $=id=>document.getElementById(id),order=n=>({left:0,center:1,centre:1,middle:1,right:2})[(n||"").toLowerCase()]??3;
let st={},scr=[],stat={},busy=false;
function render(){
 let h='<tr><td></td><td><b>Hub</b></td><td>'+(st.ip||"")+'</td><td>'+(st.build||"")+'</td><td><span class="chip ok">online</span></td><td>'+(stat.hub||"")+'</td></tr>';
 h+=scr.map(s=>'<tr><td><input type="checkbox" id="t_'+s.ip+'"'+(s.online?" checked":" disabled")+'></td><td><b>'+(s.name||"(no name)")+'</b></td><td>'+s.ip+'</td><td>'+s.build+'</td><td><span class="chip '+(s.online?"ok":"bad")+'">'+(s.online?"online":"offline")+'</span></td><td>'+(stat[s.ip]||'<a href="http://'+s.ip+'/update" target="_blank">update page</a>')+'</td></tr>').join("");
 const keep={};scr.forEach(s=>{const e=$("t_"+s.ip);if(e)keep[s.ip]=e.checked;});
 $("b").innerHTML=h;scr.forEach(s=>{const e=$("t_"+s.ip);if(e&&s.ip in keep&&s.online)e.checked=keep[s.ip];});}
async function load(){try{st=await (await fetch("/state")).json();scr=await (await fetch("/screens.json")).json();scr.sort((a,b)=>order(a.name)-order(b.name));render();}catch(e){}}
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
async function updScreens(){
 const f=$("sbin").files[0];if(!f){alert("Pick the screens' .bin first");return;}
 const targets=scr.filter(s=>s.online&&$("t_"+s.ip)&&$("t_"+s.ip).checked);if(!targets.length){alert("Tick at least one online screen");return;}
 if(!confirm("Update "+targets.map(s=>s.name||s.ip).join(", ")+" with "+f.name+"?"))return;
 busy=true;$("sgo").disabled=true;
 for(const s of targets){
  stat[s.ip]="uploading...";render();
  const before=s.build,upBefore=s.up||0,t0=Date.now();const fd=new FormData();fd.append("firmware",f,f.name);
  // The screen restarts as soon as it has the file, often before it answers, so a dropped connection is normal here.
  // Success is seen afterwards: its uptime starts again from 0 (screens from build 195 send it), or for older screens
  // it goes quiet and comes back.
  try{await fetch("http://"+s.ip+"/update",{method:"POST",mode:"no-cors",body:fd});}catch(e){}
  stat[s.ip]="sent, waiting for the restart...";render();
  let back=false,wentQuiet=false;
  for(let k=0;k<45&&!back;k++){await sleep(2000);await load();const n=scr.find(x=>x.ip===s.ip);if(!n)continue;
   if(!n.online)wentQuiet=true;
   const restarted=(upBefore&&n.up&&n.up<upBefore+(Date.now()-t0)/1000-5)||(wentQuiet&&n.online)||n.build!==before;
   if(n.online&&restarted){back=true;stat[s.ip]='<span class="chip ok">done, build '+n.build+(n.build===before?' (same build)':'')+'</span>';}}
  if(!back)stat[s.ip]='<span class="chip warn">no restart seen: try the curl command</span>';render();
 }
 busy=false;$("sgo").disabled=false;}
function updHub(){
 const f=$("hbin").files[0];if(!f){alert("Pick the hub's .bin first");return;}
 if(!confirm("Update the hub with "+f.name+"?"))return;
 const fd=new FormData();fd.append("firmware",f,f.name);const x=new XMLHttpRequest();$("hp").style.display="block";$("hgo").disabled=true;
 x.upload.onprogress=e=>{if(e.lengthComputable){$("hp").value=Math.round(e.loaded*100/e.total);stat.hub="uploading "+$("hp").value+" %";render();}};
 x.onload=async()=>{stat.hub=x.responseText==="OK"?"restarting...":'<span class="chip bad">'+x.responseText+'</span>';render();
  if(x.responseText!=="OK"){$("hgo").disabled=false;return;}
  const before=st.build;for(let k=0;k<40;k++){await sleep(2000);try{const n=await (await fetch("/state")).json();st=n;stat.hub='<span class="chip ok">done, build '+n.build+'</span>';break;}catch(e){}}
  $("hgo").disabled=false;$("hp").style.display="none";load();};
 x.onerror=()=>{stat.hub='<span class="chip bad">upload failed</span>';render();$("hgo").disabled=false;};
 x.open("POST","/update");x.send(fd);}
load();setInterval(()=>{if(!busy)load();},3000);
</script></body></html>)HTML";
// /inputs: every GPIO (and MCP23017 pin) with what uses it; move a fixed function; set up configurable inputs
// (buttons, switches, analog sensors) on any free pin.
static const char INPUTS_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Inputs</title>
<style>body{background:#000;color:#ddd;font-family:sans-serif;max-width:980px;padding:12px}h2{color:#4dd0e1}h3{color:#4dd0e1;margin:0 0 8px;font-size:16px}
.card{background:#111;border-radius:8px;padding:12px;margin:0 0 12px}.note{color:#888;font-size:13px}table{width:100%;border-collapse:collapse}
td,th{padding:5px 6px;border-bottom:1px solid #222;text-align:left;vertical-align:middle;font-size:14px}th{color:#4dd0e1;font-weight:normal}
select,input{background:#000;color:#ddd;border:1px solid #333;border-radius:4px;padding:5px;max-width:100%}button{padding:6px 11px;background:#4dd0e1;color:#000;border:0;border-radius:6px}
button.sec{background:#1d2b2e;color:#4dd0e1}.free{color:#666}.cap{font-size:11px;background:#1d2b2e;color:#9adbe6;border-radius:4px;padding:1px 5px;margin-right:3px}
.form{display:grid;grid-template-columns:repeat(auto-fill,minmax(200px,1fr));gap:8px 12px;margin:8px 0}.form label{display:flex;flex-direction:column;gap:3px;font-size:13px;color:#aaa}
#msg{color:#4dd0e1;min-height:1.3em}.on{color:#7bd88f;font-weight:bold}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Inputs and pins</h2>
<div id="msg"></div>
<div class="card"><h3>Inputs</h3><div id="inputs"></div><p><button onclick="edit(-1)">Add an input</button></p>
<div id="editor"></div>
<p class="note">Buttons are set up on the <a href="/buttons" style="color:#4dd0e1">Buttons</a> page. Switch: an on/off sensor (1 / 0) for gauges, OLED lamps and logging. Analog sensor: a reading in a custom sensor slot. Active when LOW: a switch to ground (closed = active), or a 12 V signal through a PC817 optocoupler board. Active when HIGH: a switch that opens when active, or a 12 V signal through a 47k / 10k divider with a clamp. Never 12 V straight to a pin (3.3 V at most). Hub pins and MCP23017 pins have pull-ups (GPIO 34-39 do not: add 10 k to 3.3 V).</p></div>
<div class="card"><h3>Outputs</h3><div id="outputs"></div><p><button onclick="editOut(-1)">Add an output</button></p>
<div id="oeditor"></div>
<p class="note">An output switches on when a signal crosses a level (with hysteresis so it does not chatter), when an input is active, or when you toggle it (here, or with the "Toggle output" button action and its number). A pin gives 3.3 V and a few mA: drive a relay module, a transistor or an LED, never a motor or a lamp directly. Most relay modules switch on with LOW.</p></div>
<div class="card"><h3>Pins</h3><table><thead><tr><th>Pin</th><th>Can do</th><th>Used by</th><th></th></tr></thead><tbody id="pins"></tbody></table>
<p><button class="sec" onclick="pinsDefaults()">Every function back on its default GPIO</button></p>
<p class="note">ADS1115 channels (if fitted, ADDR to GND = 0x48) are analog pins: sensors and the analog functions can use them; 0 to 3.3 V only. Free pin: "Set up an input" makes it a switch or sensor, "Set up a button" opens the Buttons page. Moving a function: pick the new GPIO, move the wire, the hub restarts with it. Only GPIOs that can do the job are offered. Analog functions need GPIO 32-39 (ADC1).</p></div>
<script>
const NAME={1:"RPM",2:"Coolant Temp",3:"MAP/Boost",4:"AFR",5:"Oil Pressure",6:"Oil Temp",7:"Fuel Pressure",8:"Ethanol",9:"Throttle Position",10:"Battery Voltage",
11:"Rev Limiter",12:"Main Relay",13:"Fuel Pump",14:"Check Engine",15:"O2 Heater",16:"Lambda Protection",17:"Fan 1",18:"Fan 2",19:"Brake Pedal",
20:"EGT",21:"Intake Air Temp",22:"Ambient Temp",23:"Humidity",24:"Fuel Level",25:"Speed",26:"GPS Speed",27:"GPS Satellites",45:"Lateral G",46:"Accel/Brake G",47:"Vertical G",48:"Yaw Rate"};
const $=id=>document.getElementById(id);let D=null,editing=null,oedit=null;
const TYPES=["","Button","Switch","Analog sensor"],CAPS={1:"in",2:"out",4:"analog",8:"pull-up"};
const PARAM_ACTIONS={4:"page",7:"scheme",8:"race page"};
const esc=t=>String(t).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/"/g,"&quot;");
const opt=(v,t,sel,dis)=>'<option value="'+v+'"'+(sel?" selected":"")+(dis?" disabled":"")+'>'+t+'</option>';
// A pin list for a picker: every pin that fits, the ones in use shown but not pickable ([pin, user] -> options).
const pinOpts=(list,cur)=>list.map(([p,u])=>opt(p,pinName(p)+(u?" - used by "+esc(u):""),p===cur,!!u)).join("");
const pinName=p=>p>=140?"ADS1115 #"+(Math.floor((p-140)/4)+1)+" A"+((p-140)%4):p>=100?"MCP23017 #"+(p>=116?2:1)+" "+((p-100)%16<8?"PA":"PB")+((p-100)%8):"GPIO "+p;
// ADS1115 channels found, free or not: [pin, user]
function adsPins(){const out=[];(D.ads||[]).forEach((a,b)=>{if(a)for(let c=0;c<4;c++)out.push([140+b*4+c,(D.adsusers||[])[b*4+c]||""]);});return out;}
function caps(c){return Object.keys(CAPS).filter(k=>c&k).map(k=>'<span class="cap">'+CAPS[k]+'</span>').join("");}
function msg(t){$("msg").textContent=t;}
async function load(){try{D=await (await fetch("/inputs.json")).json();render();}catch(e){msg("Could not reach the hub");}}
function render(){renderOutputs();
 const rows=D.gpios.map(g=>{const role=D.roles.find(r=>r.gpio===g.g);
  let mv="";if(role){const fits=D.gpios.filter(x=>(x.caps&role.need)===role.need&&(!x.user||x.g===g.g));
   const mcpFree=[];if(role.mcp)for(let e=0;e<2;e++)if(D.mcp[e])for(let n=0;n<16;n++)if(!D.mcpusers[e*16+n])mcpFree.push(100+e*16+n);
   if(role.need===4)adsPins().forEach(([p,u])=>{if(!u)mcpFree.push(p);});   // analog-only roles: an ADS1115 channel
   const usedG=D.gpios.filter(x=>(x.caps&role.need)===role.need&&x.user&&x.g!==g.g).map(x=>[x.g,x.user]);
   mv='<select onchange="if(this.value!==\'\')movePin(\''+role.key+'\',+this.value)"><option value="">Move to...</option>'+fits.filter(x=>x.g!==g.g).map(x=>opt(x.g,"GPIO "+x.g)).join("")+mcpFree.map(p=>opt(p,pinName(p))).join("")+pinOpts(usedG,-1)+'</select>'+(role.gpio!==role.def?' <span class="note">(default GPIO '+role.def+')</span>':'');}
  if(!role)mv=pinAction(g.g,g.user,g.caps);
  return '<tr><td>GPIO '+g.g+'</td><td>'+caps(g.caps)+(g.note?'<div class="note">'+esc(g.note)+'</div>':'')+'</td><td'+(g.user?'':' class="free"')+'>'+(g.user?esc(g.user):"free")+'</td><td>'+mv+'</td></tr>';});
 for(let e=0;e<2;e++)if(D.mcp[e])for(let n=0;n<16;n++){const u=D.mcpusers[e*16+n];const rl=D.roles.find(r=>r.gpio===100+e*16+n);rows.push('<tr><td>'+pinName(100+e*16+n)+'</td><td><span class="cap">in</span><span class="cap">out</span><span class="cap">pull-up</span><div class="note">expander at 0x'+D.mcp[e].toString(16).toUpperCase()+'</div></td><td'+(u?'':' class="free"')+'>'+(u?esc(u):"free")+'</td><td>'+(rl?moveList(rl,100+e*16+n):pinAction(100+e*16+n,u,11))+'</td></tr>');}
 adsPins().forEach(([p,u])=>{const rl=D.roles.find(r=>r.gpio===p),b=Math.floor((p-140)/4);
  rows.push('<tr><td>'+pinName(p)+'</td><td><span class="cap">analog</span><div class="note">0x'+D.ads[b].toString(16).toUpperCase()+', now '+((D.adsv||[])[p-140]||0).toFixed(3)+' V (0-3.3 V)</div></td><td'+(u?'':' class="free"')+'>'+(u?esc(u):"free")+'</td><td>'+
   (rl?moveList(rl,p):pinAction(p,u,4))+'</td></tr>');});
 $("pins").innerHTML=rows.join("");
 const used=D.inputs.map((x,i)=>[x,i]).filter(a=>a[0].type);
 $("inputs").innerHTML=used.length?'<table><thead><tr><th>#</th><th>Name</th><th>Type</th><th>Pin</th><th>Now</th><th></th></tr></thead><tbody>'+used.map(([x,i])=>{
  const now=x.type===3?(x.active?x.value.toFixed(2)+" "+esc((D.presets[x.preset]||{}).unit||""):"--"):(x.active?'<span class="on">ACTIVE</span>':"off");
  const lvl=x.contact?"active HIGH":"active LOW";const what=x.type===2?"Switch ("+lvl+") to sensor "+(x.slot+1):esc(D.presets[x.preset].name)+", sensor "+(x.slot+1);
  return '<tr><td>'+(i+1)+'</td><td>'+esc(x.name)+'</td><td>'+what+'</td><td>'+pinName(x.pin)+'</td><td>'+now+'</td><td><button class="sec" onclick="edit('+i+')">Edit</button> <button class="sec" onclick="clearIn('+i+')">Remove</button></td></tr>';}).join("")+'</tbody></table>':'<p class="note">No inputs set up yet.</p>';
 if(editing!==null)drawEditor();}
// The button beside a pin: set up an input on a free pin, edit the input already on it, or open the OLED lamps.
function moveList(role,at){   // a fixed function on an expander or ADS pin: move it to a GPIO or another such pin
 const g=D.gpios.filter(x=>(x.caps&role.need)===role.need&&!x.user).map(x=>x.g),m=[];
 if(role.mcp)for(let e=0;e<2;e++)if(D.mcp[e])for(let n=0;n<16;n++)if(!D.mcpusers[e*16+n])m.push(100+e*16+n);
 if(role.need===4)adsPins().forEach(([p,u])=>{if(!u)m.push(p);});
 return '<select onchange="if(this.value!==\'\')movePin(\''+role.key+'\',+this.value)"><option value="">Move to...</option>'+g.concat(m).filter(p=>p!==at).map(p=>opt(p,pinName(p))).join("")+'</select>';}
function pinAction(pin,user,c){
 if(!user)return '<button class="sec" onclick="assignPin('+pin+','+c+')">Set up an input</button>'+((c&2)?' <button class="sec" onclick="assignOut('+pin+')">Set up an output</button>':'')+' <a href="/buttons" style="color:#4dd0e1">Set up a button</a>';
 if(/^Button/.test(user))return '<a href="/buttons" style="color:#4dd0e1">Buttons</a>';
 const m=/^Input (\d+)/.exec(user);if(m)return '<button class="sec" onclick="edit('+(m[1]-1)+')">Edit input</button>';
 const o=/^Output (\d+)/.exec(user);if(o)return '<button class="sec" onclick="editOut('+(o[1]-1)+')">Edit output</button>';
 if(/^OLED lamp/.test(user))return '<a href="/oled" style="color:#4dd0e1">OLED lamps</a>';
 return "";}
function assignPin(pin,c){const i=D.inputs.findIndex(x=>!x.type);if(i<0){msg("All 16 inputs are in use");return;}
 edit(i);editing.pin=pin;editing.type=(c&4)&&!(c&8)?3:2;drawEditor();}   // analog-only pins start as a sensor, others as a switch
// Every pin that fits the type: [pin, user] ("" = free, or this input's own pin).
function freePins(type,i){const cur=D.inputs[i]||{};const out=[],mine=p=>cur.type&&cur.pin===p;
 D.gpios.forEach(g=>{const ok=type===3?(g.caps&4):(g.caps&1);if(ok)out.push([g.g,mine(g.g)?"":g.user]);});
 if(type!==3)for(let e=0;e<2;e++)if(D.mcp[e])for(let n=0;n<16;n++){const p=100+e*16+n;out.push([p,mine(p)?"":D.mcpusers[e*16+n]]);}
 if(type===3)adsPins().forEach(([p,u])=>out.push([p,mine(p)?"":u]));
 return out;}
function edit(i){if(i<0){i=D.inputs.findIndex(x=>!x.type);if(i<0){msg("All 16 inputs are in use");return;}}
 setTimeout(()=>{const ed=$("editor");if(ed&&ed.scrollIntoView)ed.scrollIntoView({behavior:"smooth"});},0);
 const x=D.inputs[i];editing={i:i,type:x.type>1?x.type:2,pin:x.pin,contact:x.contact,slot:x.slot,ap:x.ap,pp:x.pp,ah:x.ah,ph:x.ph,preset:x.preset,mult:x.mult||1,v1:x.v1,val1:x.val1,v2:x.v2||3.3,val2:x.val2||100,pullup:x.pullup||2490,name:x.name||"",unit:""};drawEditor();}
function freeSlots(e){return D.slots.map((s,k)=>[s,k]).filter(([s,k])=>!s.name||(D.inputs[e.i].type>1&&D.inputs[e.i].slot===k)).map(a=>a[1]);}
function drawEditor(){if(!editing){$("editor").innerHTML="";return;}   // Cancel
 const e=editing,all=freePins(e.type,e.i),pins=all.filter(x=>!x[1]).map(x=>x[0]);if(!pins.includes(e.pin))e.pin=pins[0];
 let h='<div class="card" style="background:#0b1a1d"><h3>Input '+(e.i+1)+'</h3><div class="form">'+
  '<label>Name<input id="e_name" maxlength="15" value="'+esc(e.name)+'"></label>'+
  '<label>Type<select onchange="editing.type=+this.value;keep();drawEditor()">'+[2,3].map(t=>opt(t,TYPES[t],t===e.type)).join("")+'</select></label>'+
  '<label>Pin<select id="e_pin">'+pinOpts(all,e.pin)+'</select></label>';
 if(e.type!==3)h+='<label>Active when<select id="e_contact">'+opt(0,"Active when LOW (switch to ground, optocoupler)",!e.contact)+opt(1,"Active when HIGH (12 V through a divider)",!!e.contact)+'</select></label>';
 {const sl=freeSlots(e);if(!sl.includes(e.slot))e.slot=sl[0];
  h+='<label>Custom sensor slot<select id="e_slot">'+sl.map(k=>opt(k,"Sensor "+(k+1)+(D.slots[k].name?" ("+esc(D.slots[k].name)+")":" (free)"),k===e.slot)).join("")+'</select></label>';}
 if(e.type===3){h+='<label>Sensor<select onchange="keep();editing.preset=+this.value;drawEditor()">'+D.presets.map((p,k)=>opt(k,esc(p.name),k===e.preset)).join("")+'</select></label>';
  if(e.preset===0)h+='<label>Divider multiplier<input id="e_mult" type="number" step="any" value="'+e.mult+'"></label><label>Unit<input id="e_unit" maxlength="7" value="'+esc(e.unit)+'"></label>'+
   '<label>Sensor volts, point 1<input id="e_v1" type="number" step="any" value="'+e.v1+'"></label><label>Reads<input id="e_val1" type="number" step="any" value="'+e.val1+'"></label>'+
   '<label>Sensor volts, point 2<input id="e_v2" type="number" step="any" value="'+e.v2+'"></label><label>Reads<input id="e_val2" type="number" step="any" value="'+e.val2+'"></label>';
  if(e.preset===6||e.preset===7)h+='<label>Pull-up resistor (ohms, to 3.3 V)<input id="e_pullup" type="number" step="1" value="'+e.pullup+'"></label>';}
 h+='</div><button onclick="saveIn()">Save input '+(e.i+1)+'</button> <button class="sec" onclick="editing=null;drawEditor()">Cancel</button>';
 if(e.type===3)h+='<p class="note">Presets assume the divider in their name (docs/TODO.md, Voltage dividers). Temperature presets: the sensor from the pin to ground, a pull-up resistor from the pin to 3.3 V.</p>';
 $("editor").innerHTML=editing?h+'</div>':"";}
function val(id,d){const x=$(id);return x&&x.value!==undefined&&x.value!==""?x.value:d;}
function keep(){const e=editing;if(!e)return;e.name=val("e_name",e.name);e.pin=+val("e_pin",e.pin);e.contact=+val("e_contact",e.contact);}
async function saveIn(){const e=editing;keep();
 const q={i:e.i,type:e.type,pin:e.pin,contact:e.contact,name:e.name,slot:val("e_slot",e.slot),preset:e.preset};
 if(e.type===3&&e.preset===0){Object.assign(q,{mult:val("e_mult",1),v1:val("e_v1",0),val1:val("e_val1",0),v2:val("e_v2",3.3),val2:val("e_val2",100),unit:val("e_unit","")});}
 if(e.type===3&&(e.preset===6||e.preset===7))q.pullup=val("e_pullup",2490);
 const r=await fetch("/inputs/set?"+new URLSearchParams(q));msg(await r.text());if(r.ok){editing=null;$("editor").innerHTML="";load();}}
async function clearIn(i){if(!confirm("Remove input "+(i+1)+"?"))return;const r=await fetch("/inputs/clear?i="+i);msg(await r.text());load();}
async function movePin(key,g){const role=D.roles.find(r=>r.key===key);if(!confirm("Move "+role.name+" from GPIO "+role.gpio+" to GPIO "+g+"? The hub restarts."))return;
 const r=await fetch("/pins/set?role="+key+"&gpio="+g);msg(await r.text());if(r.ok)setTimeout(load,6000);}
async function pinsDefaults(){if(!confirm("Put every function back on its default GPIO? The hub restarts."))return;msg(await (await fetch("/pins/defaults")).text());setTimeout(load,6000);}
// ---- outputs ----
const OTYPES=["","A signal","An input is active","Toggled (button action or this page)"],OCOND=["is on","above","below"];
function outPins(i){const cur=D.outputs[i]||{},out=[],mine=p=>cur.type&&cur.pin===p;   // [pin, user]
 D.gpios.forEach(g=>{if(g.caps&2)out.push([g.g,mine(g.g)?"":g.user]);});
 for(let e=0;e<2;e++)if(D.mcp[e])for(let n=0;n<16;n++){const p=100+e*16+n;out.push([p,mine(p)?"":D.mcpusers[e*16+n]]);}
 return out;}
function renderOutputs(){const used=D.outputs.map((x,i)=>[x,i]).filter(a=>a[0].type);
 $("outputs").innerHTML=used.length?'<table><thead><tr><th>#</th><th>Name</th><th>On when</th><th>Pin</th><th>Now</th><th></th></tr></thead><tbody>'+used.map(([x,i])=>{
  const when=x.type===1?(NAME[x.sig]||"signal "+x.sig)+" "+OCOND[x.cond]+(x.cond?" "+x.level:""):x.type===2?"input "+(x.input+1)+" ("+esc((D.inputs[x.input]||{}).name||"")+") active":"toggled";
  return '<tr><td>'+(i+1)+'</td><td>'+esc(x.name)+'</td><td>'+esc(when)+(x.blink?", blinking":"")+'</td><td>'+pinName(x.pin)+(x.low?" (LOW = on)":"")+'</td><td>'+(x.on?'<span class="on">ON</span>':"off")+'</td><td>'+
   (x.type===3?'<button onclick="toggleOut('+i+')">'+(x.on?"Turn off":"Turn on")+'</button> ':'')+'<button class="sec" onclick="editOut('+i+')">Edit</button> <button class="sec" onclick="clearOut('+i+')">Remove</button></td></tr>';}).join("")+'</tbody></table>':'<p class="note">No outputs set up yet.</p>';}
function editOut(i){if(i<0){i=D.outputs.findIndex(x=>!x.type);if(i<0){msg("All 16 outputs are in use");return;}}
 const x=D.outputs[i];oedit={i:i,type:x.type||1,pin:x.pin,low:x.low,blink:x.blink,sig:x.sig||2,cond:x.type?x.cond:1,input:x.input,level:x.level,hyst:x.hyst,name:x.name||""};drawOut();
 setTimeout(()=>{const ed=$("oeditor");if(ed&&ed.scrollIntoView)ed.scrollIntoView({behavior:"smooth"});},0);}
function assignOut(pin){editOut(-1);if(!oedit)return;oedit.pin=pin;drawOut();}
function okeep(){const o=oedit;if(!o)return;o.name=val("o_name",o.name);o.pin=+val("o_pin",o.pin);o.low=+val("o_low",o.low);o.sig=+val("o_sig",o.sig);o.cond=+val("o_cond",o.cond);
 o.level=val("o_level",o.level);o.hyst=val("o_hyst",o.hyst);o.input=+val("o_input",o.input);const b=$("o_blink");if(b&&b.checked!==undefined)o.blink=b.checked?1:0;}
function drawOut(){if(!oedit){$("oeditor").innerHTML="";return;}
 const o=oedit,all=outPins(o.i),pins=all.filter(x=>!x[1]).map(x=>x[0]);if(!pins.includes(o.pin))o.pin=pins[0];
 let h='<div class="card" style="background:#0b1a1d"><h3>Output '+(o.i+1)+'</h3><div class="form">'+
  '<label>Name<input id="o_name" maxlength="15" value="'+esc(o.name)+'"></label>'+
  '<label>Pin<select id="o_pin">'+pinOpts(all,o.pin)+'</select></label>'+
  '<label>On means<select id="o_low">'+opt(0,"HIGH (LED, transistor)",!o.low)+opt(1,"LOW (most relay modules)",!!o.low)+'</select></label>'+
  '<label>Switch on when<select onchange="okeep();oedit.type=+this.value;drawOut()">'+[1,2,3].map(t=>opt(t,OTYPES[t],t===o.type)).join("")+'</select></label>';
 if(o.type===1)h+='<label>Signal<select id="o_sig">'+Object.keys(NAME).map(k=>opt(k,esc(NAME[k]),+k===o.sig)).join("")+'</select></label>'+
  '<label>Condition<select id="o_cond" onchange="okeep();drawOut()">'+OCOND.map((c,k)=>opt(k,c,k===o.cond)).join("")+'</select></label>'+
  (o.cond?'<label>Level<input id="o_level" type="number" step="any" value="'+o.level+'"></label><label>Hysteresis (switches off this far back)<input id="o_hyst" type="number" step="any" value="'+o.hyst+'"></label>':'');
 if(o.type===2){const ins=D.inputs.map((x,k)=>[x,k]).filter(a=>a[0].type);h+='<label>Input<select id="o_input">'+(ins.length?ins.map(([x,k])=>opt(k,(k+1)+". "+esc(x.name),k===o.input)).join(""):'<option value="">(set up an input first)</option>')+'</select></label>';}
 if(o.type===3)h+='<p class="note">Turn it on and off from the outputs list, or with a button set to "Toggle output" and number '+(o.i+1)+'.</p>';
 h+='<label style="flex-direction:row;align-items:center;gap:6px"><input type="checkbox" id="o_blink"'+(o.blink?" checked":"")+'> Blink while on</label></div>'+
  '<button onclick="saveOut()">Save output '+(o.i+1)+'</button> <button class="sec" onclick="oedit=null;drawOut()">Cancel</button></div>';
 $("oeditor").innerHTML=h;}
async function saveOut(){okeep();const o=oedit;
 const q={i:o.i,type:o.type,pin:o.pin,low:o.low,blink:o.blink,name:o.name,sig:o.sig,cond:o.cond,level:o.level,hyst:o.hyst,input:o.input};
 const r=await fetch("/outputs/set?"+new URLSearchParams(q));msg(await r.text());if(r.ok){oedit=null;drawOut();load();}}
async function clearOut(i){if(!confirm("Remove output "+(i+1)+"?"))return;msg(await (await fetch("/outputs/clear?i="+i)).text());load();}
async function toggleOut(i){msg("Output "+(i+1)+": "+(await (await fetch("/outputs/toggle?i="+i)).text()));load();}
load();setInterval(()=>{if(editing===null&&oedit===null)load();},2000);
</script></body></html>)HTML";

static const char BOARDS_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Board status</title>
<style>h2{color:#4dd0e1;margin:6px 0 10px}h3{color:#4dd0e1;margin:0;font-size:16px}.card{background:#111;border-radius:8px;padding:12px;margin:0 0 12px}
.head{display:flex;flex-wrap:wrap;align-items:baseline;gap:4px 12px;margin:0 0 8px}.note{color:#888;font-size:13px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:8px}.kv{background:#000;border-radius:6px;padding:8px;border-left:3px solid #2e7d32}
.kv b{display:block;color:#888;font-weight:normal;font-size:12px;margin-bottom:2px}.kv span{font-size:18px}.kv.warn{border-left-color:#ffb300}.kv.bad{border-left-color:#e53935}.kv.na{border-left-color:#333}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%}.dot.on{background:#2e7d32}.dot.off{background:#e53935}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Board status</h2>
<div id="boards"><p class="note">Loading...</p></div>
<p class="note">Refreshed every 2 s. Screens report over ESP-NOW every 2 s, so this works in the car too. Green is fine, amber is worth a look, red needs attention.
CPU busy is an estimate per core. Free RAM is the ESP32's internal memory; "lowest" is the least it has had since it started, and "largest block" shows fragmentation.</p>
<script>
const $=id=>document.getElementById(id);
const esc=t=>String(t).replace(/&/g,"&amp;").replace(/</g,"&lt;");
// value, label, level: 0 fine, 1 look, 2 attention, -1 no data
const kv=(k,v,l)=>'<div class="kv'+(l===1?" warn":l===2?" bad":l===-1?" na":"")+'"><b>'+k+'</b><span>'+v+'</span></div>';
const lvl=(v,warn,bad,low)=>low?(v<=bad?2:v<=warn?1:0):(v>=bad?2:v>=warn?1:0);
function up(s){if(!s)return"--";const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return d?d+"d "+h+"h":h?h+"h "+m+"m":m+"m "+(s%60)+"s";}
function common(b){return kv("CPU busy, core 0",b.cpu0+" %",lvl(b.cpu0,80,95))+kv("CPU busy, core 1",b.cpu1+" %",lvl(b.cpu1,80,95))+
 kv("Free RAM",b.heap+" KB",lvl(b.heap,30,15,1))+kv("Lowest free RAM",b.heapMin+" KB",lvl(b.heapMin,20,10,1))+kv("Largest block",b.heapBig+" KB",lvl(b.heapBig,16,8,1))+
 kv("Chip temperature",b.temp+" &deg;C",lvl(b.temp,70,80))+kv("Wi-Fi signal",b.rssi?b.rssi+" dBm":"not on a network",b.rssi?lvl(b.rssi,-75,-85,1):-1)+
 kv("CPU clock",b.mhz+" MHz",0)+kv("Up for",up(b.uptime),0);}
function hub(h){return '<div class="card"><div class="head"><span class="dot on"></span><h3>Hub</h3><span class="note">build '+h.build+' | '+esc(h.ip)+'</span></div><div class="grid">'+
 kv("Main loop",h.loops+" /s",lvl(h.loops,200,50,1))+kv("CAN frames in",h.canFps+" /s",h.canFps?0:1)+kv("CAN bus",h.canState,h.canState==="running"?0:2)+
 kv("CAN missed / errors",h.canRxMissed+" / "+h.canBusErr,h.canRxMissed||h.canBusErr?1:0)+kv("CAN send failures",h.canTxFail,h.canTxFail?1:0)+
 kv("Sent to screens",h.sentPs+" /s",h.sentPs?0:1)+kv("Joined its Wi-Fi",h.apClients,0)+common(h)+'</div></div>';}
function scr(c){const head='<div class="head"><span class="dot '+(c.online?"on":"off")+'"></span><h3>'+esc(c.name||"Screen")+'</h3><span class="note">build '+c.build+' | '+esc(c.ip)+
 (c.online?' | heard '+(c.age/1000).toFixed(1)+' s ago':' | offline, last heard '+Math.round(c.age/1000)+' s ago')+'</span></div>';
 if(!c.stats)return '<div class="card">'+head+'<p class="note">This screen runs an older build that does not send its numbers. Update it to see them.</p></div>';
 const age=c.rxAge<0?"never":(c.rxAge/10).toFixed(1)+" s ago";
 return '<div class="card'+(c.online?'':'" style="opacity:.6')+'">'+head+'<div class="grid">'+
  kv("Frames per second",c.fps,lvl(c.fps,20,8,1))+kv("Time spent drawing",c.draw+" %",lvl(c.draw,85,95))+kv("Data from the hub",c.rxPs+" /s",c.rxPs?0:2)+
  kv("Last data",age,c.rxAge<0?2:lvl(c.rxAge,10,30))+kv("PSRAM free",(c.psram/1024).toFixed(1)+" MB",lvl(c.psram,1024,256,1))+
  kv("SD logging",c.logging?"running":"not running",c.logging?0:-1)+common(c)+'</div></div>';}
async function load(){try{const d=await (await fetch("/boards.json")).json();
 const order=n=>{n=(n||"").toLowerCase();return n==="left"?0:/cent|middle/.test(n)?1:n==="right"?2:3;};
 d.screens.sort((a,b)=>order(a.name)-order(b.name));
 $("boards").innerHTML=hub(d.hub)+(d.screens.length?d.screens.map(scr).join(""):'<div class="card"><p class="note">No screens heard yet.</p></div>');}
 catch(e){$("boards").innerHTML='<p class="note">Could not reach the hub</p>';}}
load();setInterval(load,2000);
</script></body></html>)HTML";

static const char BACKUP_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Backup</title>
<style>h2{color:#4dd0e1;margin:6px 0 10px}h3{color:#4dd0e1;margin:0 0 8px;font-size:16px}.card{background:#111;border-radius:8px;padding:12px;margin:0 0 12px}.note{color:#888;font-size:13px}
button{padding:8px 14px;background:#4dd0e1;color:#000;border:0;border-radius:6px}select,input{background:#000;color:#ddd;border:1px solid #333;border-radius:4px;padding:5px}
table{width:100%;border-collapse:collapse}td,th{padding:6px;border-bottom:1px solid #222;text-align:left;font-size:14px}th{color:#4dd0e1;font-weight:normal}#msg{min-height:1.3em;color:#4dd0e1}#msg.bad{color:#ff6b6b}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Backup</h2>
<div id="msg"></div>
<div class="card"><h3>Export</h3>
<p class="note">One file with every board's settings: the hub and each screen that is online. Keep it somewhere safe; it restores a board after a factory reset, or copies one screen's layout to another.</p>
<label class="note"><input type="checkbox" id="pw" style="width:auto"> Include Wi-Fi passwords</label><br><br>
<button onclick="exportAll()">Export everything</button> <span class="note" id="exp"></span></div>
<div class="card"><h3>Restore</h3>
<p class="note">Pick a backup file, tick what to restore, and pick where each screen's part goes. Each board checks its part first and changes nothing if it is not right, then restarts.
A screen's part put onto a different screen brings only its layout (pages, gauges, colours, logging channels), not its name or network.</p>
<input type="file" id="file" accept=".json" onchange="readFile()"><div id="parts"></div></div>
<script>
const $=id=>document.getElementById(id);let B=null,SCR=[];
const esc=t=>String(t).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/"/g,"&quot;");
function msg(t,bad){$("msg").textContent=t;$("msg").className=bad?"bad":"";}
async function screens(){try{const d=await (await fetch("/boards.json")).json();SCR=d.screens.filter(s=>s.online&&s.ip&&s.ip!=="0.0.0.0");}catch(e){SCR=[];}return SCR;}
async function exportAll(){msg("Collecting...");const pw=$("pw").checked?1:0,out={msp_dash_backup:1,date:new Date().toISOString(),parts:[]};
 try{out.parts.push({board:"Hub",kind:"hub",text:await (await fetch("/backup.txt?pw="+pw)).text()});}catch(e){msg("Could not read the hub",true);return;}
 const miss=[];for(const s of await screens()){try{const r=await fetch("http://"+s.ip+"/backup.txt?pw="+pw);if(!r.ok)throw 0;out.parts.push({board:s.name,kind:"screen",ip:s.ip,build:s.build,text:await r.text()});}catch(e){miss.push(s.name);}}
 const a=document.createElement("a");a.href=URL.createObjectURL(new Blob([JSON.stringify(out,null,1)],{type:"application/json"}));
 a.download="msp-dash-backup-"+out.date.slice(0,10)+".json";document.body.appendChild(a);a.click();a.remove();
 $("exp").textContent=out.parts.map(p=>p.board).join(", ")+" saved"+(miss.length?"; not reached: "+miss.join(", "):"");msg(miss.length?"Saved, but some screens did not answer":"Saved",!!miss.length);}
async function readFile(){const f=$("file").files[0];if(!f)return;try{B=JSON.parse(await f.text());if(!B.msp_dash_backup||!Array.isArray(B.parts))throw 0;}catch(e){B=null;msg("Not an MSP-DASH backup file",true);$("parts").innerHTML="";return;}
 await screens();const opts=n=>SCR.map(s=>'<option value="'+esc(s.ip)+'"'+(s.name===n?" selected":"")+'>'+esc(s.name)+' ('+esc(s.ip)+')</option>').join("");
 $("parts").innerHTML='<p class="note">From '+esc(B.date||"?")+'</p><table><tr><th></th><th>Part</th><th>Goes to</th></tr>'+B.parts.map((p,i)=>'<tr><td><input type="checkbox" id="c'+i+'" checked style="width:auto"></td><td>'+esc(p.board)+
  (p.build?' <span class="note">build '+p.build+'</span>':'')+'</td><td>'+(p.kind==="hub"?"this hub":SCR.length?'<select id="t'+i+'">'+opts(p.board)+'</select>':'<span class="note">no screen online</span>')+'</td></tr>').join("")+'</table><br><button onclick="restore()">Restore the ticked parts</button>';}
// A screen part onto another screen: its layout only (not its name, role, network or the hub's Wi-Fi details).
const IDENT=/^n (link|net|wifi|hubnet) /;
async function restore(){if(!B)return;if(!confirm("Restore the ticked parts? Those boards' settings are replaced and they restart."))return;
 const res=[],order=B.parts.map((p,i)=>i).sort((a,b)=>(B.parts[a].kind==="hub")-(B.parts[b].kind==="hub"));   // screens first, the hub last
 for(const i of order){const p=B.parts[i];if(!$("c"+i)||!$("c"+i).checked)continue;
  try{let r;if(p.kind==="hub")r=await fetch("/restore",{method:"POST",body:p.text});
   else{const t=$("t"+i);if(!t){res.push(p.board+": no screen to send it to");continue;}
    const same=SCR.find(s=>s.ip===t.value&&s.name===p.board),body=same?p.text:p.text.split("\n").filter(l=>!IDENT.test(l)).join("\n");
    r=await fetch("http://"+t.value+"/restore",{method:"POST",headers:{"X-MSP":"1"},body:body});}
   res.push(p.board+": "+await r.text());}catch(e){res.push(p.board+": no answer");}}
 msg(res.join(" | ")||"Nothing ticked",res.some(x=>/Not restored|no answer|errors/.test(x)));}
</script></body></html>)HTML";

// Stock gauges (aircore board, hub 64): modes, PWM, calibration, over ESP-NOW.
static const char AIRCORE_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"><title>MSP-DASH Stock gauges</title>
<style>h2{color:#4dd0e1;margin:6px 0 10px}h3{color:#4dd0e1;margin:0;font-size:17px}.card{background:#111;border-radius:8px;padding:12px;margin:0 0 12px}.note{color:#888;font-size:13px}
button{padding:7px 12px;background:#4dd0e1;color:#000;border:0;border-radius:6px;margin:2px}button.off{background:#26363a;color:#cfe8ec}button.danger{background:#e53935;color:#fff}
input,select{background:#000;color:#ddd;border:1px solid #333;border-radius:4px;padding:5px;width:90px}input[type=range]{width:100%;padding:0}
.row{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin:6px 0}.big{font-size:22px;color:#fff}.lab{color:#888;font-size:12px;display:block}
table{border-collapse:collapse}td,th{padding:4px 10px;border-bottom:1px solid #222;text-align:left;font-size:14px}th{color:#4dd0e1;font-weight:normal}#msg{min-height:1.2em;color:#4dd0e1}
)HTML" HUB_NAV_STYLE R"HTML(</style></head><body>)HTML" HUB_NAV R"HTML(
<h2>Stock gauges</h2>
<p class="note">The original tach and speedo, driven by the aircore board (build 7 or later). Modes are not saved: a restart goes back to live data.</p>
<div id="msg"></div><div id="gauges"><p class="note">Looking for the gauge board...</p></div>
<script>
const $=id=>document.getElementById(id);
const OP={live:1,sweep:2,target:3,angle:4,pwm:5,decay:6,strength:7,slew:8,range:9,mark:10,markHere:11,unmark:12,clear:13,defaults:14};
const MODES=["Live","Sweep","Target","Angle"];
const PWMS=[100,200,250,500,1000,2000,2500,4000,5000,8000,10000,12500,15000,16000,20000,25000,30000,32000,40000];
let G=[],built="";
const key=g=>g.mac+"/"+g.g;
let pend={};   // board MAC -> the command number it should apply next (shown until its status says it did)
async function cmd(g,op,a,b){try{const q=new URLSearchParams({mac:g.mac,g:g.g,op:op,a:a===undefined?0:a,b:b===undefined?0:b});
 const r=await fetch("/aircore/cmd?"+q),t=await r.text();
 if(!r.ok){$("msg").textContent=t;return;}$("msg").textContent="";const n=parseInt(t.replace("Sent ",""));if(n)pend[g.mac]={seq:n,t:Date.now()};}catch(e){$("msg").textContent="No answer from the hub";}}
function byKey(k){return G.find(g=>key(g)===k);}
function C(k,op,a,b){const g=byKey(k);if(g)cmd(g,op,a,b);}
let tmr={};function soon(k,fn){clearTimeout(tmr[k]);tmr[k]=setTimeout(fn,150);}
function card(g){const k=key(g),u=g.name==="tach"?"RPM":"mph",id=k.replace(/[^A-Za-z0-9]/g,"");
 return '<div class="card" data-k="'+k+'"><div class="row"><h3>'+(g.name==="tach"?"Tach":"Speedo")+'</h3><span class="note">'+g.mac+' | build '+g.build+' | channel '+g.ch+'</span></div>'+
 '<div class="row"><span><span class="lab">Showing</span><span class="big" id="v'+id+'"></span></span><span><span class="lab">Needle</span><span class="big" id="n'+id+'"></span></span><span><span class="lab">Mode</span><span class="big" id="m'+id+'"></span></span></div>'+
 '<div class="row"><button onclick="C(\''+k+'\','+OP.live+')">Live</button>'+
 '<button onclick="C(\''+k+'\','+OP.sweep+',$(\'sw'+id+'\').value)">Sweep</button><input id="sw'+id+'" type="number" min="1" max="60" step="0.5" value="'+g.sweepS+'"> s end to end and back</div>'+
 '<div class="row"><span class="lab">Target ('+u+'): the needle follows the slider</span><input type="range" id="tr'+id+'" min="'+g.min+'" max="'+g.max+'" step="'+(g.name==="tach"?50:1)+'" value="'+g.min+'" oninput="$(\'tv'+id+'\').value=this.value;soon(\'t'+id+'\',()=>C(\''+k+'\','+OP.target+',this.value))">'+
 '<input id="tv'+id+'" type="number" value="'+g.min+'"><button onclick="$(\'tr'+id+'\').value=$(\'tv'+id+'\').value;C(\''+k+'\','+OP.target+',$(\'tv'+id+'\').value)">Go</button></div>'+
 '<h3 style="margin-top:12px">Calibration</h3><p class="note">Angle mode holds the needle at a coil angle: move it onto a dial number and save that number as a mark. With two or more marks the needle goes from mark to mark.</p>'+
 '<div class="row"><span class="lab">Coil angle</span><input type="range" id="ar'+id+'" min="-360" max="360" step="0.5" value="'+g.needle+'" oninput="$(\'av'+id+'\').value=this.value;soon(\'a'+id+'\',()=>C(\''+k+'\','+OP.angle+',this.value))">'+
 '<input id="av'+id+'" type="number" step="0.5" value="'+g.needle+'"><button onclick="$(\'ar'+id+'\').value=$(\'av'+id+'\').value;C(\''+k+'\','+OP.angle+',$(\'av'+id+'\').value)">Hold</button>'+
 '<button class="off" onclick="var x=$(\'av'+id+'\');x.value=(+x.value-0.5);$(\'ar'+id+'\').value=x.value;C(\''+k+'\','+OP.angle+',x.value)">-0.5</button><button class="off" onclick="var x=$(\'av'+id+'\');x.value=(+x.value+0.5);$(\'ar'+id+'\').value=x.value;C(\''+k+'\','+OP.angle+',x.value)">+0.5</button></div>'+
 '<div class="row">Dial number <input id="mk'+id+'" type="number"> <button onclick="C(\''+k+'\','+OP.markHere+',$(\'mk'+id+'\').value)">Save as mark</button><span class="note">(the needle\'s angle now)</span></div>'+
 '<div id="pt'+id+'"></div>'+
 '<div class="row"><span class="lab">Strength</span><input id="st'+id+'" type="number" min="0.1" max="1" step="0.05" value="'+g.strength+'"><button class="off" onclick="C(\''+k+'\','+OP.strength+',$(\'st'+id+'\').value)">Set</button>'+
 '<span class="lab">Needle speed, deg/s</span><input id="sl'+id+'" type="number" min="10" max="5000" value="'+g.slew+'"><button class="off" onclick="C(\''+k+'\','+OP.slew+',$(\'sl'+id+'\').value)">Set</button></div>'+
 '<div class="row"><span class="lab">Dial range ('+u+')</span><input id="mn'+id+'" type="number" value="'+g.min+'"> to <input id="mx'+id+'" type="number" value="'+g.max+'"><button class="off" onclick="C(\''+k+'\','+OP.range+',$(\'mn'+id+'\').value,$(\'mx'+id+'\').value)">Set</button></div>'+
 '<div class="row"><button class="off" onclick="if(confirm(\'Remove every mark?\'))C(\''+k+'\','+OP.clear+')">Clear marks</button><button class="danger" onclick="if(confirm(\'Factory settings for this gauge (marks too)?\'))C(\''+k+'\','+OP.defaults+')">Factory settings</button></div></div>';}
function boardCard(g){const m=g.mac,id=m.replace(/:/g,"");
 return '<div class="card"><h3>Board '+m+'</h3><p class="note" id="bs'+id+'"></p><p class="note">Shared by its gauges. Lower PWM can be steadier, higher is silent.</p><div class="row"><span class="lab">PWM</span><select id="pw'+id+'">'+
 PWMS.map(v=>'<option value="'+v+'"'+(v===g.pwm?" selected":"")+'>'+(v>=1000?v/1000+" kHz":v+" Hz")+'</option>').join("")+(PWMS.includes(g.pwm)?"":'<option value="'+g.pwm+'" selected>'+g.pwm+' Hz</option>')+'</select>'+
 '<button class="off" onclick="cmd({mac:\''+m+'\',g:0},'+OP.pwm+',$(\'pw'+id+'\').value)">Set</button><input id="pc'+id+'" type="number" min="100" max="40000" placeholder="Hz"><button class="off" onclick="cmd({mac:\''+m+'\',g:0},'+OP.pwm+',$(\'pc'+id+'\').value)">Set any</button>'+
 '<span class="lab">Coil drive</span><select id="dc'+id+'"><option value="1"'+(g.slow?" selected":"")+'>Slow decay (stronger)</option><option value="0"'+(g.slow?"":" selected")+'>Fast decay</option></select><button class="off" onclick="cmd({mac:\''+m+'\',g:0},'+OP.decay+',$(\'dc'+id+'\').value)">Set</button></div></div>';}
function live(g){const id=key(g).replace(/[^A-Za-z0-9]/g,""),u=g.name==="tach"?" RPM":" mph";
 $("v"+id).textContent=Math.round(g.shown)+u+(g.mode===0&&!g.has?" (no data)":"");$("n"+id).textContent=g.needle.toFixed(1)+"\u00b0";
 $("m"+id).textContent=MODES[g.mode]||"?";
 const pts=g.pts.length?'<table><tr><th>Mark</th><th>Coil angle</th><th></th></tr>'+g.pts.map(p=>'<tr><td>'+p[0]+'</td><td>'+p[1]+'\u00b0</td><td><button class="off" onclick="C(\''+key(g)+'\','+OP.unmark+','+p[0]+')">Remove</button></td></tr>').join("")+'</table>'
  +(g.pts.length<2?'<p class="note">One more mark and they replace zero / span.</p>':''):'<p class="note">No marks yet: zero '+g.zero+'\u00b0, span '+g.span+'\u00b0 in use.</p>';
 const t=$("pt"+id);if(t.innerHTML!==pts)t.innerHTML=pts;}
let ups={};
function boardLive(g){const id=g.mac.replace(/:/g,""),e=$("bs"+id);if(!e)return;
 if(pend[g.mac]&&Date.now()-pend[g.mac].t>8000)delete pend[g.mac];   // an old warning (say, from before the board restarted) goes away
 const p=pend[g.mac],late=p&&g.seq!==p.seq&&Date.now()-p.t>1500;
 if(p&&g.seq===p.seq){delete pend[g.mac];$("msg").style.color="#7fdf7f";$("msg").textContent="Board "+g.mac+" applied command "+g.seq;clearTimeout(boardLive.t);boardLive.t=setTimeout(()=>{$("msg").textContent="";$("msg").style.color="";},3000);}
 // both gauges report the board's uptime, a second apart at most: a restart is a drop of more than 5 s
 const restarted=ups[g.mac]!==undefined&&g.up+5<ups[g.mac];ups[g.mac]=Math.max(g.up,restarted?0:(ups[g.mac]||0));if(restarted){ups[g.mac]=g.up;ups["r"+g.mac]=Date.now();}
 const age=g.age>4000?" | not heard for "+(g.age/1000).toFixed(0)+" s":"";
 e.textContent="Up "+Math.floor(g.up/60)+" min "+(g.up%60)+" s"+age+(late?" | last command not confirmed: press again":"")+(Date.now()-(ups["r"+g.mac]||0)<20000?" | it RESTARTED":"");
 e.style.color=(age||late||Date.now()-(ups["r"+g.mac]||0)<20000)?"#ff8080":"#888";}
async function poll(){try{const d=await (await fetch("/aircore.json")).json();G=d.gauges;
 const sig=G.map(g=>key(g)+":"+g.min+":"+g.max).join("|");
 if(!G.length){$("gauges").innerHTML='<p class="note">No gauge board heard. It needs aircore build 7 or later and the hub\'s data (same channel). Status packets heard since the hub started: '+(d.rx||0)+(d.short?', '+d.short+' too short (update the gauge board)':'')+'. On the gauge board, <b>show</b> says how many it sent and how many failed.</p>';built="";return;}
 if(sig!==built){const macs=[...new Set(G.map(g=>g.mac))];$("gauges").innerHTML=macs.map(m=>boardCard(G.find(g=>g.mac===m))+G.filter(g=>g.mac===m).map(card).join("")).join("");built=sig;}
 G.forEach(live);G.filter((g,i)=>G.findIndex(x=>x.mac===g.mac)===i).forEach(boardLive);}catch(e){}}   // one board line per board
poll();setInterval(poll,500);
</script></body></html>)HTML";
