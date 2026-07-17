#pragma once

#include <pgmspace.h>

// Dashboard HTML served from flash (PROGMEM) — no SRAM cost
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>WHEEL-E</title>
<style>
:root {
  --a: #e07a5f;
  --a-glow: rgba(224,122,95,.5);
  --a-soft: rgba(224,122,95,.08);
  --m: #ffb703;
  --m-glow: rgba(255,183,3,.5);
  --m-soft: rgba(255,183,3,.07);
}

*,*::before,*::after {
  box-sizing:border-box; margin:0; padding:0;
  user-select:none; -webkit-tap-highlight-color:transparent;
}
html,body { height:100%; overflow:hidden; touch-action:none; }

body {
  font-family:system-ui,-apple-system,sans-serif;
  background:#060302;
  display:flex; align-items:center; justify-content:center;
  touch-action:none;
  transition:background .4s ease;
}

/* ── Animated background grid ─────────────────────────────────────────── */
.bg {
  position:fixed; inset:0; pointer-events:none;
  background-image:
    linear-gradient(rgba(224,122,95,.022) 1px, transparent 1px),
    linear-gradient(90deg, rgba(224,122,95,.022) 1px, transparent 1px);
  background-size:46px 46px;
  animation:bgMove 28s linear infinite;
  transition:opacity .4s ease;
}
@keyframes bgMove { to { background-position:46px 46px; } }

.vignette {
  position:fixed; inset:0; pointer-events:none;
  background:radial-gradient(ellipse at center, transparent 15%, #060302 80%);
  transition:background .4s ease;
}

/* ── Light mode ───────────────────────────────────────────────────────── */
.light body { background:#f5ede8; }
.light .bg { opacity:.35; filter:hue-rotate(10deg); }
.light .vignette { background:radial-gradient(ellipse at center, transparent 15%, #f5ede8 80%); }

.light .panel {
  background:rgba(255,252,250,.96);
  border-color:rgba(224,122,95,.2);
  box-shadow:
    0 0 0 1px rgba(255,255,255,.8),
    0 18px 55px rgba(160,80,40,.12),
    inset 0 1px 0 rgba(255,255,255,.95);
}
.light .panel.music {
  border-color:rgba(255,183,3,.25);
  box-shadow:
    0 0 0 1px rgba(255,255,255,.8),
    0 18px 55px rgba(160,80,40,.12),
    0 0 50px rgba(255,183,3,.06),
    inset 0 1px 0 rgba(255,255,255,.95);
}
.light .c { opacity:.45; }

.light .btn {
  background:linear-gradient(155deg, #ffffff, rgba(238,225,215,.6));
  border-color:rgba(0,0,0,.09);
  box-shadow:0 2px 8px rgba(0,0,0,.1), inset 0 1px 0 rgba(255,255,255,.95);
}
.light .ico  { color:rgba(90,45,20,.45); }
.light .note { color:rgba(90,45,20,.5); }

.light .panel.music .btn {
  background:linear-gradient(155deg, #ffffff, rgba(238,225,215,.6));
  border-color:rgba(255,183,3,.15);
}
.light .panel.music .btn .note { color:rgba(160,110,0,.6); }

.light .btn.on {
  box-shadow:
    0 0 26px rgba(224,122,95,.5),
    0 3px 10px rgba(224,122,95,.35),
    inset 0 1px 0 rgba(255,255,255,.2);
}
.light .panel.music .btn.on {
  box-shadow:
    0 0 26px rgba(255,183,3,.55),
    0 3px 10px rgba(255,183,3,.35),
    inset 0 1px 0 rgba(255,255,255,.22);
}

.light .ring { border-color:rgba(224,122,95,.2); }
.light .ring.on { border-color:rgba(224,122,95,.45); }
.light .dot { background:#e8d0c0; }
.light .conn-lbl { color:rgba(160,100,60,.65); }
.light .ring.on ~ .conn-lbl { color:var(--a); }

.light .strip-line {
  background:linear-gradient(90deg, transparent, rgba(224,122,95,.18), transparent);
}
.light .panel.music .strip-line {
  background:linear-gradient(90deg, transparent, rgba(255,183,3,.2), transparent);
}

.light .theme-btn { border-color:rgba(224,122,95,.25); color:var(--a); }
.light .theme-btn:hover { background:rgba(224,122,95,.1); }

/* ── Panel ────────────────────────────────────────────────────────────── */
.panel {
  position:relative; z-index:1;
  width:min(90vw, 345px);
  padding:22px 18px 24px;
  background:rgba(9,4,2,.9);
  border:1px solid rgba(224,122,95,.13);
  border-radius:28px;
  box-shadow:
    0 0 0 1px rgba(255,255,255,.028),
    0 28px 90px rgba(0,0,0,.88),
    inset 0 1px 0 rgba(255,255,255,.05);
  backdrop-filter:blur(52px) saturate(1.7);
  -webkit-backdrop-filter:blur(52px) saturate(1.7);
  transition:border-color .6s ease, box-shadow .6s ease, background .4s ease;
  overflow:hidden;
}
.panel.music {
  border-color:rgba(255,183,3,.18);
  box-shadow:
    0 0 0 1px rgba(255,255,255,.03),
    0 28px 90px rgba(0,0,0,.88),
    0 0 80px rgba(255,183,3,.05),
    inset 0 1px 0 rgba(255,255,255,.05);
}

/* Corner brackets */
.c { position:absolute; width:16px; height:16px; opacity:.38; transition:border-color .6s, opacity .6s; }
.c-tl { top:11px; left:11px;  border-top:1.5px solid var(--a); border-left:1.5px solid var(--a); }
.c-tr { top:11px; right:11px; border-top:1.5px solid var(--a); border-right:1.5px solid var(--a); }
.c-bl { bottom:11px; left:11px;  border-bottom:1.5px solid var(--a); border-left:1.5px solid var(--a); }
.c-br { bottom:11px; right:11px; border-bottom:1.5px solid var(--a); border-right:1.5px solid var(--a); }
.panel.music .c { border-color:var(--m) !important; opacity:.52; }

/* Direction ambient glow */
.glow {
  position:absolute; inset:0; border-radius:inherit; pointer-events:none;
  opacity:0;
  background:radial-gradient(circle at var(--gx,50%) var(--gy,50%), rgba(224,122,95,.12) 0%, transparent 62%);
  transition:opacity .3s ease;
}
.panel.music .glow {
  background:radial-gradient(circle at var(--gx,50%) var(--gy,50%), rgba(255,183,3,.12) 0%, transparent 62%);
}

/* ── Header ───────────────────────────────────────────────────────────── */
.header {
  display:flex; align-items:center; justify-content:space-between; margin-bottom:16px;
}

.logo {
  font-size:.78rem; font-weight:800; letter-spacing:5px; text-transform:uppercase;
  color:var(--a); text-shadow:0 0 18px rgba(224,122,95,0);
  transition:color .6s, text-shadow .6s;
}
.panel.music .logo { color:var(--m); text-shadow:0 0 18px var(--m-glow); }

.header-right { display:flex; align-items:center; gap:8px; }

/* Theme toggle button */
.theme-btn {
  width:26px; height:26px; border-radius:50%; border:1px solid rgba(224,122,95,.2);
  background:transparent; cursor:pointer; font-size:.8rem; line-height:1;
  display:flex; align-items:center; justify-content:center;
  color:var(--a); opacity:.7; transition:opacity .3s, background .3s, border-color .3s;
  padding:0;
}
.theme-btn:hover { opacity:1; background:rgba(224,122,95,.08); }

/* Connection indicator */
.conn { display:flex; align-items:center; gap:6px; }
.ring {
  width:22px; height:22px; border-radius:50%;
  border:1.5px solid #22100a;
  display:flex; align-items:center; justify-content:center;
  position:relative; transition:border-color .4s;
}
.ring.on { border-color:rgba(224,122,95,.45); }
.ring.on::after {
  content:''; position:absolute; inset:-5px; border-radius:50%;
  border:1px solid var(--a); opacity:0;
  animation:pulse 2.4s ease-in-out infinite;
}
@keyframes pulse {
  0%,100% { opacity:0; transform:scale(1); }
  50%      { opacity:.3; transform:scale(1.22); }
}
.dot {
  width:8px; height:8px; border-radius:50%; background:#1e0c06;
  transition:background .4s, box-shadow .4s;
}
.ring.on .dot { background:var(--a); box-shadow:0 0 10px var(--a-glow); }
.conn-lbl {
  font-size:.52rem; font-weight:700; letter-spacing:2.5px; text-transform:uppercase;
  color:#2e1509; transition:color .4s;
}
.ring.on ~ .conn-lbl { color:var(--a); }

/* ── Mode strip ───────────────────────────────────────────────────────── */
.strip { display:flex; align-items:center; gap:9px; margin-bottom:18px; }
.strip-line {
  flex:1; height:1px;
  background:linear-gradient(90deg, transparent, rgba(224,122,95,.1), transparent);
  transition:background .6s;
}
.panel.music .strip-line {
  background:linear-gradient(90deg, transparent, rgba(255,183,3,.12), transparent);
}
.badge {
  font-size:.52rem; font-weight:800; letter-spacing:3px; text-transform:uppercase;
  padding:3px 11px; border-radius:100px;
  background:var(--a-soft); border:1px solid rgba(224,122,95,.2);
  color:rgba(224,122,95,.75); transition:all .6s ease;
}
.panel.music .badge {
  background:var(--m-soft); border-color:rgba(255,183,3,.25); color:rgba(255,183,3,.85);
}

/* ── Control grid ─────────────────────────────────────────────────────── */
.grid { display:grid; grid-template-columns:repeat(3,1fr); gap:9px; touch-action:none; }

/* ── Button ───────────────────────────────────────────────────────────── */
.btn {
  position:relative; aspect-ratio:1; border-radius:15px;
  display:flex; flex-direction:column; align-items:center; justify-content:center; gap:2px;
  cursor:pointer; overflow:hidden;
  background:linear-gradient(155deg, rgba(255,255,255,.032), rgba(0,0,0,.28));
  border:1px solid rgba(255,255,255,.06);
  box-shadow:0 3px 10px rgba(0,0,0,.5), inset 0 1px 0 rgba(255,255,255,.04);
  /* Release: smooth fade back to idle */
  transition:background .12s, border-color .12s, box-shadow .14s, transform .12s, opacity .35s;
  will-change:transform, background-color;
}

.ico  { font-size:1.3rem; line-height:1; z-index:1; color:rgba(245,234,228,.28); transition:color .1s; }
.note { font-size:.64rem; font-weight:900; letter-spacing:.8px; z-index:1; color:rgba(245,234,228,.32); transition:color .1s, font-size .2s; }

@media (hover:hover) {
  .btn:not(.ghost):not(.on):hover {
    border-color:rgba(224,122,95,.18);
    background:linear-gradient(155deg, rgba(224,122,95,.055), rgba(0,0,0,.22));
  }
}

/* Press — 0s transition so press is instant; release animates via base .btn rule */
.btn.on {
  background:linear-gradient(145deg, #e07a5f, #bf5d3c);
  border-color:#d0714f;
  box-shadow:
    0 0 28px rgba(224,122,95,.58),
    0 4px 12px rgba(224,122,95,.38),
    inset 0 1px 0 rgba(255,255,255,.2);
  transform:scale(.94) translateY(1px);
  transition:background 0s, border-color 0s, box-shadow 0s, transform 0s;
}
.btn.on .ico, .btn.on .note { color:rgba(18,6,2,.9); }

/* Ghost (invisible in drive mode) */
.btn.ghost { opacity:0; pointer-events:none; }

/* ── Music mode overrides ─────────────────────────────────────────────── */
.panel.music .btn {
  opacity:1; pointer-events:auto;
  border-color:rgba(255,183,3,.1);
  background:linear-gradient(155deg, rgba(255,183,3,.025), rgba(0,0,0,.26));
}
.panel.music .btn .ico { display:none; }
.panel.music .btn .note { font-size:.72rem; letter-spacing:.5px; color:rgba(255,183,3,.5); }

@media (hover:hover) {
  .panel.music .btn:not(.on):hover {
    border-color:rgba(255,183,3,.22);
    background:linear-gradient(155deg, rgba(255,183,3,.055), rgba(0,0,0,.22));
  }
}
.panel.music .btn.on {
  background:linear-gradient(145deg, #ffb703, #d99500);
  border-color:#f0ac00;
  box-shadow:
    0 0 28px rgba(255,183,3,.62),
    0 4px 12px rgba(255,183,3,.38),
    inset 0 1px 0 rgba(255,255,255,.22);
  transition:background 0s, border-color 0s, box-shadow 0s, transform 0s;
}
.panel.music .btn.on .note { color:rgba(20,12,0,.92); }

/* Ripple */
.rip {
  position:absolute; border-radius:50%;
  width:36px; height:36px; margin:-18px;
  background:rgba(255,255,255,.22);
  transform:scale(0); pointer-events:none;
  animation:ripout .55s ease-out forwards;
}
@keyframes ripout { to { transform:scale(5.5); opacity:0; } }
</style>
</head>
<body>

<div class="bg"></div>
<div class="vignette"></div>

<div class="panel" id="panel">
  <div class="c c-tl"></div><div class="c c-tr"></div>
  <div class="c c-bl"></div><div class="c c-br"></div>
  <div class="glow" id="glow"></div>

  <!-- Header -->
  <div class="header">
    <span class="logo" id="logo">WHEEL-E</span>
    <div class="header-right">
      <button class="theme-btn" id="themeBtn" title="Toggle theme">&#9788;</button>
      <div class="conn">
        <div class="ring" id="ring"><div class="dot"></div></div>
        <span class="conn-lbl" id="connLbl">OFFLINE</span>
      </div>
    </div>
  </div>

  <!-- Mode strip -->
  <div class="strip">
    <div class="strip-line"></div>
    <span class="badge" id="badge">DRIVE</span>
    <div class="strip-line"></div>
  </div>

  <!-- Control grid — 3x3, ghost buttons invisible in drive mode -->
  <div class="grid" id="grid">
    <div class="btn ghost" id="nw" data-cmd="NW">     <span class="ico"></span><span class="note">C5</span></div>
    <div class="btn"       id="up" data-cmd="FORWARD"> <span class="ico">&#9650;</span><span class="note">D5</span></div>
    <div class="btn ghost" id="ne" data-cmd="NE">     <span class="ico"></span><span class="note">E5</span></div>

    <div class="btn"       id="left"   data-cmd="LEFT">  <span class="ico">&#9664;</span><span class="note">F5</span></div>
    <div class="btn ghost" id="center" data-cmd="CENTER"><span class="ico">&#9632;</span><span class="note">G5</span></div>
    <div class="btn"       id="right"  data-cmd="RIGHT"> <span class="ico">&#9654;</span><span class="note">A5</span></div>

    <div class="btn ghost" id="sw"   data-cmd="SW">      <span class="ico"></span><span class="note">B5</span></div>
    <div class="btn"       id="down" data-cmd="BACKWARD"><span class="ico">&#9660;</span><span class="note">C6</span></div>
    <div class="btn ghost" id="se"   data-cmd="SE">      <span class="ico"></span><span class="note">D6</span></div>
  </div>
</div>

<script>
const panel    = document.getElementById('panel');
const logo     = document.getElementById('logo');
const ring     = document.getElementById('ring');
const connLbl  = document.getElementById('connLbl');
const badge    = document.getElementById('badge');
const glowEl   = document.getElementById('glow');
const grid     = document.getElementById('grid');
const themeBtn = document.getElementById('themeBtn');

let ws, isMouseDown = false;
let lastSent = 'STOP', lastValid = 'STOP', lastCmdTime = 0;
const DEBOUNCE = 50;
let isMusic = false;

// ── Theme ─────────────────────────────────────────────────────────────────────
function applyTheme(light) {
  document.documentElement.classList.toggle('light', light);
  themeBtn.innerHTML = light ? '&#9790;' : '&#9788;'; // moon : sun
  try { localStorage.setItem('we-theme', light ? '1' : '0'); } catch(e) {}
}
// Restore saved theme on load
try {
  applyTheme(localStorage.getItem('we-theme') === '1');
} catch(e) {}
themeBtn.addEventListener('click', () =>
  applyTheme(!document.documentElement.classList.contains('light'))
);

// ── Ambient glow ──────────────────────────────────────────────────────────────
const GLOW_POS = {
  FORWARD: ['50%','6%'], BACKWARD: ['50%','94%'],
  LEFT: ['6%','50%'],    RIGHT: ['94%','50%'],
};
function setGlow(cmd) {
  const p = GLOW_POS[cmd];
  if (p) { glowEl.style.setProperty('--gx',p[0]); glowEl.style.setProperty('--gy',p[1]); glowEl.style.opacity='1'; }
  else    { glowEl.style.opacity='0'; }
}

// ── WebSocket ─────────────────────────────────────────────────────────────────
function connect() {
  ws = new WebSocket('ws://' + location.hostname + ':81/');
  ws.onopen  = () => { ring.classList.add('on'); connLbl.textContent='ONLINE'; };
  ws.onmessage = e => {
    if (e.data === 'UI_MUSIC') setMode(true);
    else if (e.data === 'UI_DRIVE') setMode(false);
  };
  ws.onclose = () => {
    ring.classList.remove('on'); connLbl.textContent='OFFLINE';
    setMode(false); setTimeout(connect, 2000);
  };
}

function setMode(music) {
  isMusic = music;
  panel.classList.toggle('music', music);
  logo.textContent  = music ? 'SYNTH' : 'WHEEL-E';
  badge.textContent = music ? 'SYNTH' : 'DRIVE';
  if (!music) setGlow(null);
}
function send(cmd) { if (ws && ws.readyState === WebSocket.OPEN) ws.send(cmd); }

// ── Ripple ────────────────────────────────────────────────────────────────────
function ripple(el, x, y) {
  const r = document.createElement('div');
  r.className = 'rip';
  const rc = el.getBoundingClientRect();
  r.style.left = (x - rc.left) + 'px';
  r.style.top  = (y - rc.top)  + 'px';
  el.appendChild(r);
  r.addEventListener('animationend', () => r.remove(), { once:true });
}

// ── Input evaluation ──────────────────────────────────────────────────────────
// IMPORTANT: always use .closest('.btn') after elementFromPoint — the raw result
// may be a child element (.ico or .note span) which lacks the 'btn' class.
function evalPads(pts, isEnd) {
  const now = Date.now();
  let cmds = [], els = [];

  if (!isMusic) {
    if (pts.length > 1) {
      if (isEnd) { lastValid = 'STOP'; } else { cmds = [lastValid]; }
    } else if (pts.length === 1) {
      const raw = document.elementFromPoint(pts[0].x, pts[0].y);
      const el  = raw ? raw.closest('.btn') : null;  // bubble up from child spans
      if (el && !el.classList.contains('ghost')) {
        cmds.push(el.dataset.cmd); els.push(el); lastValid = el.dataset.cmd;
      } else if (!isEnd) {
        cmds = [lastValid];   // finger slid off grid — hold last command
      }
    } else if (isEnd) {
      lastValid = 'STOP';
    } else {
      cmds = [lastValid];
    }
  } else {
    // Music mode: all 9 pads active. Use .closest() so taps on .note text
    // still resolve to the parent .btn (this was the core synth miss bug).
    pts.forEach(pt => {
      const raw = document.elementFromPoint(pt.x, pt.y);
      const el  = raw ? raw.closest('.btn') : null;
      if (el && !cmds.includes(el.dataset.cmd)) {
        cmds.push(el.dataset.cmd); els.push(el);
      }
    });
  }

  document.querySelectorAll('.btn').forEach(b =>
    b.classList.toggle('on', els.includes(b))
  );

  if (!isMusic) setGlow(cmds[0] || null);

  const str = cmds.length ? cmds.sort().join(',') : 'STOP';

  // Debounce rules:
  //  • STOP       → always instant (safety release)
  //  • lastSent=STOP → first command after release is always immediate (enables fast taps & combos)
  //  • isMusic    → no debounce; real-time chord/arpeggio response
  //  • otherwise  → 50 ms debounce to prevent slide-flooding in drive mode
  if (str !== lastSent &&
      (str === 'STOP' || lastSent === 'STOP' || isMusic || now - lastCmdTime >= DEBOUNCE)) {
    lastSent = str; lastCmdTime = now; send(str);
  }
}

function release() {
  isMouseDown = false; lastValid = 'STOP';
  evalPads([], true); setGlow(null);
  document.querySelectorAll('.btn').forEach(b => b.classList.remove('on'));
}

// ── Mouse ─────────────────────────────────────────────────────────────────────
window.addEventListener('mousedown', e => {
  isMouseDown = true;
  const el = e.target.closest('.btn');
  if (el && (isMusic || !el.classList.contains('ghost'))) ripple(el, e.clientX, e.clientY);
  evalPads([{ x:e.clientX, y:e.clientY }]);
});
window.addEventListener('mousemove', e => { if (isMouseDown) evalPads([{ x:e.clientX, y:e.clientY }]); });
window.addEventListener('mouseup',    release);
window.addEventListener('mouseleave', release);
window.addEventListener('blur',       release);

// ── Touch ─────────────────────────────────────────────────────────────────────
function onTouch(e) {
  e.preventDefault();
  const isEnd = e.type === 'touchend' || e.type === 'touchcancel';

  // touchstart: use e.target directly — most accurate hit-test for the initial tap.
  // Also seeds lastValid immediately so the fallback path in evalPads is correct.
  if (e.type === 'touchstart' && e.changedTouches.length > 0) {
    const ct  = e.changedTouches[0];
    const btn = ct.target ? ct.target.closest('.btn') : null;
    if (btn) {
      const active = isMusic || !btn.classList.contains('ghost');
      if (active) {
        ripple(btn, ct.clientX, ct.clientY);
        if (!isMusic) lastValid = btn.dataset.cmd;  // immediate seed for drive mode
      }
    }
  }

  const pts = Array.from(e.touches).map(t => ({ x:t.clientX, y:t.clientY }));
  evalPads(pts, isEnd);
}

grid.addEventListener('touchstart',  onTouch, { passive:false });
grid.addEventListener('touchmove',   onTouch, { passive:false });
grid.addEventListener('touchend',    onTouch, { passive:false });
grid.addEventListener('touchcancel', onTouch, { passive:false });

connect();
</script>
</body>
</html>
)rawliteral";
