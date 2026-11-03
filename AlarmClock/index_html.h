#pragma once
#include <pgmspace.h>

// Web UI served by the ESP32. Talks to the firmware via /api/status and /api/*.
const char INDEX_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Alarm Clock</title>
<style>
  :root { --bg:#0f1220; --card:#1a1f36; --fg:#eef0fa; --muted:#8b93b8; --accent:#5b8cff; --danger:#ff5b6e; --ok:#3ecf8e; }
  * { box-sizing:border-box; }
  body { margin:0; font-family:system-ui,sans-serif; background:var(--bg); color:var(--fg); display:flex; justify-content:center; padding:16px; }
  main { width:100%; max-width:420px; display:grid; gap:16px; }
  .card { background:var(--card); border-radius:16px; padding:20px; }
  h1 { font-size:1rem; margin:0 0 4px; color:var(--muted); font-weight:500; }
  #now { font-size:3rem; font-variant-numeric:tabular-nums; font-weight:600; }
  #state { margin-top:6px; color:var(--muted); }
  #state.ringing { color:var(--danger); font-weight:600; }
  label { display:block; color:var(--muted); font-size:.85rem; margin:14px 0 6px; }
  input[type=time], input[type=number] { width:100%; font-size:1.6rem; padding:10px; border-radius:10px; border:1px solid #2c3354; background:#11152a; color:var(--fg); }
  .row { display:flex; align-items:center; justify-content:space-between; }
  .switch { width:52px; height:30px; appearance:none; background:#2c3354; border-radius:15px; position:relative; cursor:pointer; transition:.2s; }
  .switch::after { content:""; position:absolute; top:3px; left:3px; width:24px; height:24px; border-radius:50%; background:#fff; transition:.2s; }
  .switch:checked { background:var(--ok); }
  .switch:checked::after { left:25px; }
  button { font-size:1rem; padding:12px; border:0; border-radius:10px; color:#fff; background:var(--accent); cursor:pointer; width:100%; }
  button.danger { background:var(--danger); }
  button.ghost { background:#2c3354; }
  .btns { display:grid; grid-template-columns:1fr 1fr; gap:10px; margin-top:16px; }
  #msg { text-align:center; color:var(--muted); font-size:.85rem; min-height:1.2em; }
</style>
</head>
<body>
<main>
  <section class="card">
    <h1>Current time</h1>
    <div id="now">--:--:--</div>
    <div id="state">Connecting...</div>
  </section>

  <section class="card">
    <div class="row"><h1 style="margin:0">Alarm</h1><input id="enabled" class="switch" type="checkbox"></div>
    <label for="time">Wake-up time</label>
    <input id="time" type="time" value="07:00">
    <label for="snooze">Snooze length (minutes)</label>
    <input id="snooze" type="number" min="1" max="60" value="9">
    <div class="btns">
      <button id="save">Save</button>
      <button id="test" class="ghost">Test buzzer</button>
    </div>
  </section>

  <section class="card" id="ringCard" hidden>
    <div class="btns" style="margin-top:0">
      <button id="snoozeBtn" class="ghost">Snooze</button>
      <button id="stopBtn" class="danger">Stop</button>
    </div>
  </section>

  <div id="msg"></div>
</main>

<script>
const $ = id => document.getElementById(id);
let editing = false;

async function post(path, params) {
  const r = await fetch(path, { method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:new URLSearchParams(params||{}) });
  if (!r.ok) throw new Error(await r.text());
}
function say(t) { $('msg').textContent = t; setTimeout(() => { if ($('msg').textContent === t) $('msg').textContent = ''; }, 3000); }

async function refresh() {
  try {
    const s = await (await fetch('/api/status')).json();
    $('now').textContent = s.now;
    const st = $('state');
    st.className = s.ringing ? 'ringing' : '';
    st.textContent = s.ringing ? 'RINGING' : s.snoozed ? 'Snoozed until ' + s.snoozeUntil
                   : s.enabled ? 'Next alarm at ' + s.alarm : 'Alarm off';
    $('ringCard').hidden = !(s.ringing || s.snoozed);
    if (!editing) { $('time').value = s.alarm; $('enabled').checked = s.enabled; $('snooze').value = s.snoozeMin; }
  } catch (e) { $('state').textContent = 'Lost connection to alarm clock'; }
}

['time','snooze','enabled'].forEach(id => $(id).addEventListener('input', () => editing = true));
$('save').onclick = async () => {
  try { await post('/api/alarm', { time:$('time').value, enabled:$('enabled').checked?1:0, snooze:$('snooze').value }); editing = false; say('Saved'); refresh(); }
  catch (e) { say('Error: ' + e.message); }
};
$('test').onclick = () => post('/api/test').then(() => say('Beeped')).catch(e => say(e.message));
$('snoozeBtn').onclick = () => post('/api/snooze').then(refresh);
$('stopBtn').onclick = () => post('/api/stop').then(refresh);

refresh(); setInterval(refresh, 1000);
</script>
</body>
</html>
)rawliteral";
