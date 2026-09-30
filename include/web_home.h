#pragma once
#include <Arduino.h>
// Everyday page served at "/". Self-contained: no CDN, no filesystem.
const char HOME_UI[] PROGMEM = R"HTML(<!doctype html>
<html lang="it"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#f4efe8" media="(prefers-color-scheme: light)">
<meta name="theme-color" content="#151210" media="(prefers-color-scheme: dark)">
<title>Tonepie</title>
<style>
:root{--bg:#f4efe8;--bg2:#fbe9dc;--card:#fffdfa;--ink:#2a2420;--muted:#8c8178;--line:#ece4da;--soft:#f3ece3;
--accent:#d9694a;--ok:#4f9a76;--warn:#dd9a2f;--bad:#cf4f3c;--shadow:0 1px 2px rgba(70,45,25,.05),0 12px 32px -12px rgba(70,45,25,.18)}
@media (prefers-color-scheme:dark){:root{--bg:#151210;--bg2:#2a1a12;--card:#211c19;--ink:#f4ede6;--muted:#a3978c;--line:#342d28;--soft:#2b2521;
--accent:#ec8465;--ok:#6fbf97;--warn:#e9b455;--bad:#ea6d5a;--shadow:0 1px 2px rgba(0,0,0,.3),0 14px 34px -14px rgba(0,0,0,.6)}}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}[hidden]{display:none!important}
html{background:var(--bg)}
body{margin:0;color:var(--ink);font:16px/1.45 ui-rounded,"SF Pro Rounded","Segoe UI Variable Display","Segoe UI",system-ui,sans-serif;
background:radial-gradient(900px 420px at 85% -120px,var(--bg2),transparent 70%),var(--bg);min-height:100vh;-webkit-font-smoothing:antialiased}
main{max-width:600px;margin:auto;padding:max(22px,env(safe-area-inset-top)) 16px 40px}
h1,h2,h3,p{margin:0}button,input{font:inherit;color:inherit}
header{display:flex;align-items:flex-end;justify-content:space-between;gap:12px;margin:6px 4px 20px}
.eyebrow{font-size:12px;letter-spacing:.14em;text-transform:uppercase;color:var(--muted);font-weight:600}
h1{font-size:34px;line-height:1.1;letter-spacing:-.02em;font-weight:800}
.head-right{display:flex;align-items:center;gap:8px}
.pill{display:inline-flex;align-items:center;gap:8px;padding:8px 13px;border-radius:99px;background:var(--card);box-shadow:var(--shadow);font-size:14px;font-weight:600;white-space:nowrap}
.pill i{width:9px;height:9px;border-radius:50%;background:var(--muted)}
.pill.ok i{background:var(--ok)}.pill.bad i{background:var(--bad)}.pill.cat i{background:var(--accent);animation:pulse 1.4s infinite}
@keyframes pulse{50%{transform:scale(1.7);opacity:.35}}
.icon-btn{width:40px;height:40px;border:0;border-radius:50%;background:var(--card);box-shadow:var(--shadow);display:grid;place-items:center;cursor:pointer;color:var(--muted)}
.icon-btn svg{width:20px;height:20px}
.card{background:var(--card);border-radius:26px;box-shadow:var(--shadow);padding:20px;margin-top:14px}
.muted{color:var(--muted)}.small{font-size:13px}
.alert{display:flex;gap:12px;align-items:center;padding:14px 18px;border-radius:20px;margin-top:14px;font-weight:600;color:#fff;background:var(--bad)}
.alert.warn{background:var(--warn);color:#3a2605}
.alert svg{width:22px;height:22px;flex:none}
/* cats */
#cats{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}
#cats .card{margin:0;padding:18px;position:relative;overflow:hidden}
#cats .cat{display:flex;flex-direction:column}.cat .bars{margin-top:auto;padding-top:14px;height:66px}
#cats .card:only-child,#cats .wide{grid-column:1/-1}
.cat::before{content:"";position:absolute;inset:-40px -40px auto auto;width:130px;height:130px;border-radius:50%;background:var(--c);opacity:.12}
.cat-top{display:flex;align-items:center;gap:11px;position:relative}
.avatar{--s:46px;width:var(--s);height:var(--s);flex:none;border-radius:50%;display:grid;place-items:center;color:var(--c);background:color-mix(in srgb,var(--c) 20%,var(--card))}
.avatar svg{width:70%;height:70%}.avatar.sm{--s:36px}.avatar.q{--c:var(--muted);font-weight:800}
.cat h2{font-size:18px;font-weight:750;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.cat-top>div:last-child{min-width:0}
.big{display:flex;align-items:baseline;gap:7px;margin-top:14px}
.big strong{font-size:44px;line-height:1;font-weight:800;letter-spacing:-.03em}
.big span{color:var(--muted);font-size:14px;line-height:1.15}
.last{margin-top:6px;font-size:13.5px;color:var(--muted)}
.bars{display:flex;gap:5px;align-items:flex-end;height:52px;margin-top:14px}
.bar{flex:1;display:flex;flex-direction:column;align-items:center;gap:4px;height:100%;justify-content:flex-end;font-size:10px;color:var(--muted);font-weight:600}
.bar i{display:block;width:100%;min-height:4px;border-radius:5px;background:color-mix(in srgb,var(--c) 32%,var(--card));transition:height .5s}
.bar.today i{background:var(--c)}.bar.today{color:var(--ink)}
.wide{display:flex;align-items:center;gap:12px}
/* bin */
.bin{display:flex;gap:18px;align-items:center}
.ring{position:relative;width:118px;height:118px;flex:none}
.ring svg{width:100%;height:100%;transform:rotate(-90deg)}
.ring circle{fill:none;stroke-width:11;stroke:var(--soft)}
.ring .fill{stroke:var(--lvl);stroke-linecap:round;transition:stroke-dashoffset .8s cubic-bezier(.2,.8,.2,1),stroke .3s}
.ring b{position:absolute;inset:0;display:grid;place-items:center;font-size:26px;font-weight:800;letter-spacing:-.02em}
.bin h2{font-size:14px;color:var(--muted);font-weight:650;letter-spacing:.02em}
.bin-main{font-size:28px;font-weight:800;letter-spacing:-.02em;line-height:1.15;margin-top:2px}
.bin-main span{font-size:15px;font-weight:600;color:var(--muted);letter-spacing:0}
.bin .btn{margin-top:12px}
.btn{border:0;border-radius:99px;padding:11px 18px;font-weight:700;font-size:15px;cursor:pointer;background:var(--soft);color:var(--ink);transition:transform .12s,opacity .2s}
.btn:active{transform:scale(.97)}.btn:disabled{opacity:.45;cursor:not-allowed}
.btn.primary{background:var(--accent);color:#fff}.btn.ghost{background:none;color:var(--muted)}.btn.danger{background:none;color:var(--bad)}
.btn.block{display:flex;width:100%;justify-content:center;align-items:center;gap:9px;padding:16px;font-size:16px}
.btn svg{width:20px;height:20px}
/* visits */
.section-title{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:6px}
.section-title h2{font-size:18px;font-weight:750}
.day{font-size:12px;letter-spacing:.1em;text-transform:uppercase;color:var(--muted);font-weight:700;margin:16px 0 4px}
.visit{display:flex;align-items:center;gap:12px;width:100%;padding:10px 6px;border:0;border-radius:16px;background:none;text-align:left;cursor:pointer}
.visit:hover{background:var(--soft)}
.visit .who{flex:1;min-width:0}.visit .who b{display:block;font-weight:700;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.visit .who span{font-size:13px;color:var(--muted)}
.visit time{font-weight:700;font-variant-numeric:tabular-nums}
.empty{text-align:center;padding:22px 10px;color:var(--muted)}
.welcome{text-align:center;padding:30px 22px}.welcome .avatar{--s:72px;--c:var(--accent);margin:0 auto 14px}
.welcome h2{font-size:22px;font-weight:800}.welcome p{color:var(--muted);margin:6px 0 18px}
footer{text-align:center;color:var(--muted);font-size:12px;margin-top:26px}
footer a{color:inherit;text-underline-offset:3px}
/* weight trend */
.seg{display:inline-flex;background:var(--soft);border-radius:99px;padding:3px}
.seg button{white-space:nowrap;border:0;background:none;border-radius:99px;padding:5px 12px;font-size:13px;font-weight:700;color:var(--muted);cursor:pointer}
.seg button[aria-pressed=true]{background:var(--card);color:var(--ink);box-shadow:0 1px 3px rgba(0,0,0,.12)}
.legend{display:flex;flex-wrap:wrap;gap:6px 18px;margin:10px 0 6px}
.legend div{display:flex;align-items:center;gap:7px;font-size:14px;font-weight:700;min-width:0}
.legend i{width:10px;height:10px;border-radius:50%;background:var(--c);flex:none}
.legend span{font-weight:500;color:var(--muted)}
.plot{position:relative;touch-action:pan-y}
.plot svg{display:block;width:100%;overflow:visible}
.plot text{font-size:11px;fill:var(--muted);font-weight:600}.plot text.end{fill:var(--ink);font-size:12px;font-weight:750}
.plot .grid{stroke:var(--line);stroke-width:1}.plot .cross{stroke:var(--muted);stroke-width:1;stroke-dasharray:3 3}
.plot path{fill:none;stroke-width:2;stroke-linejoin:round;stroke-linecap:round}
.plot circle{stroke:var(--card);stroke-width:2}
.tip{position:absolute;top:0;pointer-events:none;background:var(--ink);color:var(--bg);border-radius:12px;padding:8px 11px;font-size:12.5px;white-space:nowrap;z-index:2;box-shadow:var(--shadow)}
.tip b{display:block;margin-bottom:2px}.tip div{display:flex;align-items:center;gap:6px}.tip i{width:8px;height:8px;border-radius:50%;background:var(--c)}
.trend table{width:100%;border-collapse:collapse;font-size:13px;margin-top:6px}
.trend td,.trend th{text-align:right;padding:5px 4px;border-bottom:1px solid var(--line);font-variant-numeric:tabular-nums}
.trend td:first-child,.trend th:first-child{text-align:left}.trend th{color:var(--muted);font-weight:650}
.trend details{margin-top:8px}.trend summary{font-size:13px}
/* dialogs */
dialog{border:0;padding:0;background:var(--card);color:var(--ink);width:100%;max-width:600px;margin:auto auto 0;border-radius:28px 28px 0 0;max-height:92vh;box-shadow:0 -20px 60px rgba(0,0,0,.25)}
dialog[open]{animation:up .28s cubic-bezier(.2,.8,.2,1)}
@keyframes up{from{transform:translateY(40px);opacity:0}}
dialog::backdrop{background:rgba(24,16,10,.5);backdrop-filter:blur(3px)}
@media (min-width:640px){dialog{margin:auto;border-radius:28px;max-width:520px}}
.sheet{padding:22px 20px max(22px,env(safe-area-inset-bottom));overflow:auto;max-height:92vh}
.sheet h2{font-size:22px;font-weight:800}.sheet>p{color:var(--muted);margin:4px 0 16px}
.choices{display:grid;gap:8px}
.choice{display:flex;align-items:center;gap:12px;padding:12px 14px;border:0;border-radius:18px;background:var(--soft);font-weight:700;font-size:16px;cursor:pointer;text-align:left}
.actions{display:flex;gap:10px;justify-content:flex-end;margin-top:20px}
.cat-edit{background:var(--soft);border-radius:20px;padding:14px;margin-bottom:10px}
.row{display:flex;gap:10px;align-items:center}
.field{display:flex;align-items:center;gap:6px;background:var(--card);border-radius:13px;padding:0 12px;border:1.5px solid transparent;min-width:0}
.field:focus-within{border-color:var(--accent)}
.field input{border:0;background:none;outline:0;padding:11px 0;width:100%;min-width:0;font-weight:650}
.field span{color:var(--muted);font-size:14px;white-space:nowrap}
.field.grow{flex:1}.field.kg{width:104px;flex:none}
.swatches{display:flex;gap:9px;margin-top:12px;align-items:center}
.swatch{width:28px;height:28px;border-radius:50%;border:0;background:var(--c);cursor:pointer;outline:3px solid transparent;outline-offset:2px}
.swatch[aria-pressed=true]{outline-color:var(--c)}
.swatches .btn{margin-left:auto;padding:6px 10px;font-size:13px}
details{margin-top:14px}summary{cursor:pointer;font-weight:700;color:var(--muted);padding:6px 0}
.opt{display:flex;justify-content:space-between;align-items:center;gap:12px;margin-top:10px}
.opt label{font-size:14.5px}.opt small{display:block;color:var(--muted);font-size:12.5px}
.opt .field{width:122px;flex:none;background:var(--soft)}
.hint{color:var(--warn);font-size:13px;font-weight:600;margin:4px 2px 0}
.sub{font-size:16px;font-weight:800;margin:22px 0 2px}
.tiles{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px;margin-top:14px}
.tile{margin:0;border:0;padding:16px 6px 14px;display:flex;flex-direction:column;align-items:center;gap:7px;text-align:center;cursor:pointer;font-size:13.5px;color:var(--ink);transition:transform .12s,opacity .2s}
.tile:active{transform:scale(.97)}.tile:disabled{opacity:.45;cursor:not-allowed}
.tile b{font-weight:750;line-height:1.2}.tile small{color:var(--muted);font-size:11.5px;line-height:1.2}
.tile .ico{width:42px;height:42px;border-radius:50%;display:grid;place-items:center;color:var(--accent);background:color-mix(in srgb,var(--accent) 16%,var(--card))}.tile svg{width:21px;height:21px}
.switch{appearance:none;-webkit-appearance:none;width:50px;height:30px;border-radius:99px;background:var(--line);position:relative;cursor:pointer;flex:none;margin:0;transition:background .2s}
.switch::after{content:"";position:absolute;top:3px;left:3px;width:24px;height:24px;border-radius:50%;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3);transition:transform .2s}
.switch:checked{background:var(--ok)}.switch:checked::after{transform:translateX(20px)}.switch:disabled,.field:has(input:disabled){opacity:.45;cursor:not-allowed}
#toast{position:fixed;left:50%;bottom:max(22px,env(safe-area-inset-bottom));transform:translate(-50%,90px);background:var(--ink);color:var(--bg);padding:12px 20px;border-radius:99px;font-weight:650;font-size:14.5px;transition:transform .3s cubic-bezier(.2,.8,.2,1);max-width:calc(100vw - 32px);text-align:center;z-index:9;pointer-events:none}
#toast.on{transform:translate(-50%,0)}
@media (max-width:420px){h1{font-size:28px}}
@media (max-width:380px){.big strong{font-size:38px}.ring{width:100px;height:100px}.bin-main{font-size:24px}}
@media (prefers-reduced-motion:reduce){*{animation:none!important;transition:none!important}}
/* language, cat popup */
.lang{height:40px;min-width:40px;padding:0 11px;border:0;border-radius:99px;background:var(--card);box-shadow:var(--shadow);font-weight:800;font-size:13px;letter-spacing:.06em;color:var(--muted);cursor:pointer}
#cats .cat{cursor:pointer;transition:transform .12s}#cats .cat:active{transform:scale(.985)}
.cat h2::after{content:" \203A";color:var(--muted);font-weight:600}
.cat-head{display:flex;align-items:center;gap:12px}.cat-head .avatar{--s:52px}
.stats{display:flex;gap:10px;margin:16px 0 8px}.stats div{flex:1;background:var(--soft);border-radius:16px;padding:10px 12px}
.stats b{display:block;font-size:22px;font-weight:800;letter-spacing:-.02em}.stats span{font-size:12.5px;color:var(--muted)}
.drow{display:grid;grid-template-columns:minmax(0,9.5em) 1fr 2.2em;align-items:center;gap:12px;padding:7px 2px;border-bottom:1px solid var(--line);font-size:14.5px}
.drow:last-child{border:0}.drow span{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.drow.zero{color:var(--muted)}
.drow i{display:block;height:10px;border-radius:5px;background:var(--c);min-width:3px}.drow.zero i{background:var(--line)}
.drow b{text-align:right;font-variant-numeric:tabular-nums}
/* history from the server */
.chips{display:flex;flex-wrap:wrap;gap:8px;margin:12px 0 4px}
.chip{display:flex;align-items:center;gap:8px;background:var(--soft);border-radius:14px;padding:8px 12px;font-size:13px;min-width:0}
.chip i{width:10px;height:10px;border-radius:50%;background:var(--c);flex:none}.chip b{font-size:15px}.chip span{color:var(--muted)}
.hsel{min-height:20px;font-size:13px;color:var(--muted);margin-top:4px;display:flex;flex-wrap:wrap;gap:4px 12px}.hsel b{color:var(--ink)}
.hsel i{display:inline-block;width:8px;height:8px;border-radius:50%;background:var(--c);margin-right:5px}
.plot rect.sel{opacity:.55}
.wide-field{margin-top:10px;background:var(--soft)}
</style>
<link rel="manifest" href="/manifest.webmanifest"><link rel="apple-touch-icon" href="/apple-touch-icon.png"><link rel="icon" type="image/png" href="/icon-192.png">
<meta name="apple-mobile-web-app-capable" content="yes"><meta name="mobile-web-app-capable" content="yes"><meta name="apple-mobile-web-app-title" content="Tonepie">
</head><body><main>
<header><div><p class="eyebrow">Tonepie Ti Pro</p><h1 data-t="title"></h1></div>
<div class="head-right"><span id="status" class="pill"><i></i><span></span></span>
<button class="lang" id="lang"></button>
<button class="icon-btn" id="gear" data-t-aria="settings"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.7 1.7 0 0 0 .3 1.8l.1.1a2 2 0 1 1-2.8 2.8l-.1-.1a1.7 1.7 0 0 0-2.8 1.2V21a2 2 0 1 1-4 0v-.1a1.7 1.7 0 0 0-2.9-1.2l-.1.1a2 2 0 1 1-2.8-2.8l.1-.1A1.7 1.7 0 0 0 3.100 14H3a2 2 0 1 1 0-4h.1a1.7 1.7 0 0 0 1.200-2.900l-.1-.1a2 2 0 1 1 2.800-2.800l.1.1a1.7 1.7 0 0 0 2.900-1.200V3a2 2 0 1 1 4 0v.1a1.7 1.7 0 0 0 2.900 1.200l.1-.1a2 2 0 1 1 2.800 2.800l-.1.1a1.7 1.7 0 0 0 1.200 2.900H21a2 2 0 1 1 0 4h-.1a1.7 1.7 0 0 0-1.500 1z"/></svg></button></div></header>
<div id="alerts"></div>
<section id="cats"></section>
<section class="card bin" id="bin" hidden>
  <div class="ring"><svg viewBox="0 0 120 120"><circle cx="60" cy="60" r="50"/><circle class="fill" id="ringFill" cx="60" cy="60" r="50" stroke-dasharray="314.16" stroke-dashoffset="314.16"/></svg><b id="ringPct">0%</b></div>
  <div><h2 data-t="binTitle"></h2><p class="bin-main"><b id="binN">0</b> <span id="binOf"></span></p><p class="muted small" id="binSub"></p></div>
</section>
<section class="card trend" id="trend" hidden><div class="section-title"><h2 data-t="trend"></h2>
  <div class="seg" role="group" data-t-aria="period"><button data-range="30" aria-pressed="true" data-t="d30"></button><button data-range="90" aria-pressed="false" data-t="d90"></button></div></div>
  <div class="legend" id="legend"></div><div class="plot" id="plot"></div>
  <details id="trendTable"><summary data-t="showValues"></summary><div id="trendRows"></div></details></section>
<section class="card trend" id="hist" hidden><div class="section-title"><h2 data-t="histTitle"></h2>
  <div class="seg" role="group" data-t-aria="period"><button data-hist="90" aria-pressed="true" data-t="m3"></button><button data-hist="365" aria-pressed="false" data-t="y1"></button><button data-hist="0" aria-pressed="false" data-t="all"></button></div></div>
  <div id="histBody"></div></section>
<section class="card" id="visitsCard" hidden><div class="section-title"><h2 data-t="recent"></h2><span class="muted small" id="visitsHint"></span></div><div id="visits"></div></section>
<section class="tiles" id="cleanWrap" hidden>
  <button class="tile card" id="clean"><span class="ico"><svg viewBox="0 0 24 24" fill="currentColor"><path d="M12 2l1.800 5.200L19 9l-5.200 1.800L12 16l-1.800-5.200L5 9l5.200-1.800zM19 15l.9 2.600 2.600.9-2.600.9L19 22l-.9-2.600-2.600-.9 2.600-.9zM5 15l.7 2 2 .7-2 .7L5 20.500l-.7-2.100-2-.7 2-.7z"/></svg></span><b data-t="cleanNow"></b><small data-t="cleanSub"></small></button>
  <button class="tile card" id="bag"><span class="ico"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9 7c-1-1.500-1-3 0-4h6c1 1 1 2.500 0 4M7 7h10l2 11a2.500 2.500 0 0 1-2.500 3h-9A2.500 2.500 0 0 1 5 18z"/></svg></span><b data-t="bag"></b><small id="bagSub"></small></button>
  <button class="tile card" id="litter"><span class="ico"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3v8M8 7h8M3 17c2-2 4-2 6 0s4 2 6 0 4-2 6 0M3 21h18"/></svg></span><b data-t="litter"></b><small id="litterSub"></small></button>
</section>
<footer id="foot"></footer>
</main>
<dialog id="dlgSettings"><form class="sheet" method="dialog" id="formSettings">
  <h2 data-t="yourCats"></h2><p data-t="yourCatsText"></p>
  <div id="catEditors"></div><p class="hint" id="closeHint" data-t="closeHint" hidden></p>
  <button type="button" class="btn" id="addCat" data-t="addCat"></button>
  <h3 class="sub" data-t="boxTitle"></h3><p class="hint" id="mcuNote" data-t="mcuNote" hidden></p>
    <div class="opt"><label for="mAuto"><span data-t="auto"></span><small data-t="autoSub"></small></label><input class="switch" type="checkbox" id="mAuto"></div>
    <div class="opt"><label for="mWait"><span data-t="wait"></span><small data-t="waitSub"></small></label><div class="field"><input id="mWait" type="number" min="0" max="60" step="1" inputmode="numeric"><span>min</span></div></div>
    <div class="opt"><label for="mOdor"><span data-t="odor"></span><small data-t="odorSub"></small></label><input class="switch" type="checkbox" id="mOdor"></div>
  <h3 class="sub" data-t="serverTitle"></h3><p class="muted small" data-t="serverText"></p>
    <div class="opt"><label for="sOn"><span data-t="serverOn"></span><small id="sStatus"></small></label><input class="switch" type="checkbox" id="sOn"></div>
    <div class="field wide-field"><input id="sUrl" type="url" placeholder="http://192.168.178.10:8090" autocomplete="off" spellcheck="false" maxlength="120"></div>
    <div class="field wide-field"><input id="sKey" type="password" autocomplete="new-password" maxlength="64"></div>
  <details><summary data-t="binDetails"></summary>
    <div class="opt"><label for="optLimit"><span data-t="limit"></span><small data-t="limitSub"></small></label><div class="field"><input id="optLimit" type="number" min="5" max="500" step="1" inputmode="numeric" required><span data-t="visitsUnit"></span></div></div>
    <div class="opt"><label for="optTol"><span data-t="tol"></span><small data-t="tolSub"></small></label><div class="field"><input id="optTol" type="number" min="0.1" max="3" step="0.1" required><span>kg</span></div></div>
  </details>
  <div class="actions"><button type="button" class="btn ghost" data-close data-t="cancel"></button><button class="btn primary" value="save" data-t="save"></button></div>
</form></dialog>
<dialog id="dlgVisit"><div class="sheet"><h2 data-t="whoWas"></h2><p id="visitInfo"></p><div class="choices" id="visitChoices"></div>
  <div class="actions"><button class="btn ghost" data-close data-t="close"></button></div></div></dialog>
<dialog id="dlgCat"><div class="sheet"><div class="cat-head" id="catHead"></div><div class="stats" id="catStats"></div><div id="catDays"></div>
  <p class="muted small" id="catNote" style="margin-top:10px"></p>
  <div class="actions"><button class="btn ghost" data-close data-t="close"></button></div></div></dialog>
<dialog id="dlgConfirm"><div class="sheet"><h2 id="cfTitle"></h2><p id="cfText"></p>
  <div class="actions"><button class="btn ghost" data-close data-t="cancel"></button><button class="btn primary" id="cfOk"></button></div></div></dialog>
<div id="toast" role="status" aria-live="polite"></div>
<script>
const $=id=>document.getElementById(id);
const plural=(n,one,many)=>n+' '+(n===1?one:many);
const TEXT={
it:{title:'Lettiera',settings:'Impostazioni',binTitle:'CASSETTO',trend:'Andamento peso',period:'Periodo',d30:'30 gg',d90:'90 gg',showValues:'Vedi i valori',recent:'Ultime visite',
  cleanNow:'Pulisci ora',cleanSub:'avvia un ciclo',bag:'Cambio sacchetto',litter:'Aggiunta lettiera',
  yourCats:'I tuoi gatti',yourCatsText:'Li riconosco dal peso a ogni visita. Il peso di riferimento si aggiorna da solo, così continuo a riconoscerli anche se crescono o dimagriscono.',
  closeHint:'Pesi molto vicini: il riconoscimento potrebbe confonderli.',addCat:'+ Aggiungi gatto',boxTitle:'Lettiera',mcuNote:'Lettiera non collegata: queste tre impostazioni non si possono cambiare ora.',
  auto:'Pulizia automatica',autoSub:'Dopo ogni visita svuota da sola nel cassetto',wait:'Pausa prima della pulizia',waitSub:'Minuti di attesa dopo l’uscita del gatto',
  odor:'Deodorante automatico',odorSub:'Si attiva da solo dopo ogni pulizia',binDetails:'Cassetto e riconoscimento',limit:'Svuota il cassetto dopo',limitSub:'Visite dall’ultimo cambio sacchetto',visitsUnit:'visite',
  tol:'Tolleranza sul peso',tolSub:'Scarto massimo dal peso del gatto',cancel:'Annulla',save:'Salva',whoWas:'Chi era?',close:'Chiudi',
  connecting:'Collegamento…',offline:'Lettiera non collegata',fault:'Anomalia',catInside:'Gatto dentro',ready:'Pronta',booting:'Avvio…',unreachable:'Non raggiungibile',
  faultAlert:c=>'La lettiera segnala un’anomalia (codice '+c+'). Controllala.',fullAlert:'È ora di svuotare il cassetto.',almostAlert:'Il cassetto è quasi pieno.',
  welcome:'Chi usa la lettiera?',welcomeText:'Dimmi nome e peso dei tuoi gatti: li riconoscerò a ogni visita.',welcomeBtn:'Aggiungi i gatti',
  visitsToday:n=>n===1?'visita oggi':'visite oggi',last:a=>'Ultima '+a,at:c=>' alle '+c,lastNoTime:'Ultima: orario non disponibile',noVisits:'Nessuna visita ancora',
  nVisits:n=>plural(n,'visita','visite'),unknown:n=>n===1?'1 visita non riconosciuta':n+' visite non riconosciute',unknownSub:'Toccala nell’elenco per dirmi chi era.',
  of:n=>'su '+n,binCount:n=>n===1?'visita':'visite',bagChanged:a=>'sacchetto cambiato '+a,left:n=>'ancora '+plural(n,'visita','visite'),
  tapFix:'tocca per correggere',visitsEmpty:'Qui compariranno le visite dei tuoi gatti.',notRecognised:'Non riconosciuto',noData:'nessun dato',showLess:'Mostra meno',showAll:n=>'Mostra tutte ('+n+')',
  justNow:'proprio adesso',bagWhen:a=>'ultimo '+a,litterWhen:a=>'ultima '+a,bagNever:'mai registrato',litterNever:'mai registrata',devPage:'Pagina sviluppatore',
  now:'adesso',minAgo:n=>n+' min fa',hAgo:n=>n+' h fa',yesterday:'ieri',daysAgo:n=>n+' giorni fa',Today:'Oggi',Yesterday:'Ieri',noTime:'Orario non disponibile',
  inDays:(d,n)=>' · '+d+' in '+n+' giorni',noWeight:'nessun dato',trendEmpty:'Il grafico comparirà dopo le prime visite riconosciute.',today:'oggi',chart:'Andamento del peso dei gatti in chilogrammi',day:'Giorno',
  visitNoData:'Visita senza dati',dontKnow:'Non lo so',deleteVisit:'Non era una visita: elimina',name:'Nome',colour:n=>'Colore '+n,remove:'Rimuovi',
  sending:'Invio le impostazioni alla lettiera…',confirmed:'La lettiera ha confermato le nuove impostazioni',notConfirmed:'La lettiera non ha confermato: riapri le impostazioni per controllare',
  drum:' Il tamburo potrebbe muoversi: controlla che nessun gatto sia dentro.',notReady:' La lettiera non è pronta, quindi per ora lo registro soltanto.',refused:m=>'Registrato, ma la lettiera ha rifiutato: '+m,
  bagDone:'Conteggio azzerato e lettiera avvisata',bagText:'Azzero il conteggio del cassetto e avviso la lettiera che il sacchetto è nuovo.',bagOk:'Sacchetto cambiato',
  litterDone:'Registrato: la lettiera livella la sabbia',litterText:'Registro la data e chiedo alla lettiera di livellare la sabbia nuova.',litterOk:'Lettiera aggiunta',
  cleanAsk:'Avvio la pulizia?',cleanText:'Il tamburo ruoterà. Controlla che nessun gatto sia dentro o stia entrando.',cleanSent:'Pulizia richiesta alla lettiera',unreachableApi:'Lettiera non raggiungibile',
  dayByDay:'Visite giorno per giorno',total:'visite',perDay:'al giorno',lastDays:n=>'negli ultimi '+n+' giorni',untimed:n=>plural(n,'visita','visite')+' senza orario non incluse.',
  keeps:'Lo storico conserva le ultime 64 visite di tutti i gatti.',tapCat:'Tocca un gatto per le visite giorno per giorno',
  histTitle:'Storico',m3:'3 mesi',y1:'1 anno',all:'Tutto',serverTitle:'Storico sul server',
  serverText:'Invia ogni visita a un tuo server (cartella server/ del progetto): lo storico non ha più il limite delle 64 visite.',
  serverOn:'Invia i dati al server',keyKeep:'Chiave API (vuoto = invariata)',keyNew:'Chiave API del server',
  syncOk:a=>'Ultimo invio '+a,syncPending:'Invio in corso…',syncErr:e=>'Errore: '+e,syncOff:'Disattivato',syncNever:'Nessun invio ancora',
  histLoading:'Carico lo storico…',histError:u=>'Server storico non raggiungibile ('+u+').',histEmpty:'Sul server non ci sono ancora visite.',
  perDay2:'al giorno',weekOf:d=>'Settimana del '+d,bagsLine:(n,d,v)=>plural(n,'cambio sacchetto','cambi sacchetto')+(d?' · in media ogni '+d+' giorni':'')+(v?' · circa '+plural(v,'visita','visite')+' per sacchetto':''),
  months:'Per mese',month:'Mese',fromServer:'Include i dati del server storico.',unknownCats:'Non riconosciuti',
  letters:['D','L','M','M','G','V','S'],locale:'it-IT',other:'EN',otherName:'Switch to English'},
en:{title:'Litter box',settings:'Settings',binTitle:'BIN',trend:'Weight trend',period:'Period',d30:'30 d',d90:'90 d',showValues:'Show values',recent:'Recent visits',
  cleanNow:'Clean now',cleanSub:'start a cycle',bag:'Bag change',litter:'Litter added',
  yourCats:'Your cats',yourCatsText:'I recognise them by weight at every visit. The reference weight updates by itself, so I keep recognising them as they gain or lose weight.',
  closeHint:'Very close weights: recognition may mix them up.',addCat:'+ Add cat',boxTitle:'Litter box',mcuNote:'Litter box offline: these three settings cannot be changed now.',
  auto:'Automatic cleaning',autoSub:'Empties into the bin after every visit',wait:'Pause before cleaning',waitSub:'Minutes to wait after the cat leaves',
  odor:'Automatic deodoriser',odorSub:'Runs by itself after every cleaning',binDetails:'Bin and recognition',limit:'Empty the bin after',limitSub:'Visits since the last bag change',visitsUnit:'visits',
  tol:'Weight tolerance',tolSub:'Largest difference from the cat’s weight',cancel:'Cancel',save:'Save',whoWas:'Who was it?',close:'Close',
  connecting:'Connecting…',offline:'Litter box offline',fault:'Fault',catInside:'Cat inside',ready:'Ready',booting:'Starting…',unreachable:'Unreachable',
  faultAlert:c=>'The litter box reports a fault (code '+c+'). Please check it.',fullAlert:'Time to empty the bin.',almostAlert:'The bin is almost full.',
  welcome:'Who uses the litter box?',welcomeText:'Tell me your cats’ names and weights: I will recognise them at every visit.',welcomeBtn:'Add your cats',
  visitsToday:n=>n===1?'visit today':'visits today',last:a=>'Last '+a,at:c=>' at '+c,lastNoTime:'Last: time not available',noVisits:'No visits yet',
  nVisits:n=>plural(n,'visit','visits'),unknown:n=>n===1?'1 visit not recognised':n+' visits not recognised',unknownSub:'Tap it in the list to tell me who it was.',
  of:n=>'of '+n,binCount:n=>n===1?'visit':'visits',bagChanged:a=>'bag changed '+a,left:n=>plural(n,'visit','visits')+' to go',
  tapFix:'tap to correct',visitsEmpty:'Your cats’ visits will show up here.',notRecognised:'Not recognised',noData:'no data',showLess:'Show less',showAll:n=>'Show all ('+n+')',
  justNow:'just now',bagWhen:a=>'last '+a,litterWhen:a=>'last '+a,bagNever:'never recorded',litterNever:'never recorded',devPage:'Developer page',
  now:'just now',minAgo:n=>n+' min ago',hAgo:n=>n+' h ago',yesterday:'yesterday',daysAgo:n=>n+' days ago',Today:'Today',Yesterday:'Yesterday',noTime:'Time not available',
  inDays:(d,n)=>' · '+d+' in '+n+' days',noWeight:'no data',trendEmpty:'The chart will appear after the first recognised visits.',today:'today',chart:'Cats’ weight trend in kilograms',day:'Day',
  visitNoData:'Visit without data',dontKnow:'I don’t know',deleteVisit:'Not a visit: delete',name:'Name',colour:n=>'Colour '+n,remove:'Remove',
  sending:'Sending the settings to the litter box…',confirmed:'The litter box confirmed the new settings',notConfirmed:'The litter box did not confirm: reopen the settings to check',
  drum:' The drum may move: make sure no cat is inside.',notReady:' The litter box is not ready, so for now I only record it.',refused:m=>'Recorded, but the litter box refused: '+m,
  bagDone:'Count reset and litter box notified',bagText:'I reset the bin count and tell the litter box the bag is new.',bagOk:'Bag changed',
  litterDone:'Recorded: the litter box levels the new litter',litterText:'I record the date and ask the litter box to level the new litter.',litterOk:'Litter added',
  cleanAsk:'Start cleaning?',cleanText:'The drum will rotate. Make sure no cat is inside or about to enter.',cleanSent:'Cleaning requested',unreachableApi:'Litter box unreachable',
  dayByDay:'Visits day by day',total:'visits',perDay:'per day',lastDays:n=>'in the last '+n+' days',untimed:n=>plural(n,'visit','visits')+' without a time not included.',
  keeps:'The history keeps the last 64 visits of all cats.',tapCat:'Tap a cat for its visits day by day',
  histTitle:'History',m3:'3 months',y1:'1 year',all:'All',serverTitle:'History server',
  serverText:'Sends every visit to your own server (server/ folder of the project): history is no longer limited to 64 visits.',
  serverOn:'Send data to the server',keyKeep:'API key (empty = unchanged)',keyNew:'Server API key',
  syncOk:a=>'Last sent '+a,syncPending:'Sending…',syncErr:e=>'Error: '+e,syncOff:'Off',syncNever:'Nothing sent yet',
  histLoading:'Loading history…',histError:u=>'History server unreachable ('+u+').',histEmpty:'No visits on the server yet.',
  perDay2:'per day',weekOf:d=>'Week of '+d,bagsLine:(n,d,v)=>plural(n,'bag change','bag changes')+(d?' · every '+d+' days on average':'')+(v?' · about '+plural(v,'visit','visits')+' per bag':''),
  months:'By month',month:'Month',fromServer:'Includes data from the history server.',unknownCats:'Not recognised',
  letters:['S','M','T','W','T','F','S'],locale:'en-GB',other:'IT',otherName:'Passa all’italiano'}};
let LANG=(()=>{try{const l=localStorage.getItem('lang');if(TEXT[l])return l}catch(e){}return /^it\b/i.test(navigator.language||'')?'it':'en'})();
function t(k,...a){const v=TEXT[LANG][k];return typeof v==='function'?v(...a):v}
function applyLang(){
  document.documentElement.lang=LANG;document.title=t('title');
  document.querySelectorAll('[data-t]').forEach(e=>e.textContent=t(e.dataset.t));
  document.querySelectorAll('[data-t-aria]').forEach(e=>e.setAttribute('aria-label',t(e.dataset.tAria)));
  $('lang').textContent=t('other');$('lang').title=$('lang').ariaLabel=t('otherName');
}
$('lang').onclick=()=>{LANG=LANG==='it'?'en':'it';try{localStorage.setItem('lang',LANG)}catch(e){}applyLang();if(S)render();else setStatus('',t('connecting'))};
const darkQuery=matchMedia('(prefers-color-scheme: dark)');
const PALETTE={light:['#ef8a4a','#8a97ad','#4a4653','#d8ae78','#a56b46','#79a98c'],dark:['#f2955b','#9aa7bd','#a39cb0','#dcb887','#c48a65','#88ba9c']};
let COLORS=PALETTE[darkQuery.matches?'dark':'light'];
darkQuery.addEventListener('change',e=>{COLORS=PALETTE[e.matches?'dark':'light'];if(S)render()});
const CAT='<svg viewBox="0 0 48 48" fill="currentColor"><path d="M7 8c0-1.300 1.500-2 2.500-1.200L18 13.500c1.900-.6 3.900-.9 6-.9s4.100.3 6 .9l8.500-6.700C39.500 6 41 6.700 41 8v17c0 9.400-7.600 16-17 16S7 34.400 7 25z"/><circle cx="17.500" cy="25" r="2.400" fill="var(--card)"/><circle cx="30.500" cy="25" r="2.400" fill="var(--card)"/><path d="M21.600 30.500h4.800L24 33.200z" fill="var(--card)"/></svg>';
let S=null,token='',lastKey='',draft=null,busy=false,showAll=false,range=30;
function h(tag,attrs,...kids){const e=document.createElement(tag);for(const[k,v]of Object.entries(attrs||{})){if(v==null||v===false)continue;if(k==='class')e.className=v;else if(k==='html')e.innerHTML=v;else if(k.startsWith('on'))e[k]=v;else if(k==='style')e.style.cssText=v;else e.setAttribute(k,v)}for(const k of kids.flat())if(k!=null)e.append(k);return e}
const avatar=(color,cls)=>color==null?h('div',{class:'avatar q '+(cls||'')},'?'):h('div',{class:'avatar '+(cls||''),style:'--c:'+COLORS[color%6],html:CAT});
const num=(v,d)=>v.toLocaleString(t('locale'),{minimumFractionDigits:d,maximumFractionDigits:d});
const kg=g=>num(g/1000,1)+' kg';
const two=n=>String(n).padStart(2,'0');
const clock=s=>{const d=new Date(s*1000);return two(d.getHours())+':'+two(d.getMinutes())};
const dayStart=(off=0)=>{const d=new Date();d.setHours(0,0,0,0);d.setDate(d.getDate()-off);return d.getTime()/1000};
// Whole local days between today and the visit (0 = today), correct across DST changes.
const daysBack=s=>{const d=new Date(s*1000);d.setHours(0,0,0,0);return Math.round((dayStart()*1000-d)/864e5)};
function ago(s){const x=Date.now()/1000-s;if(x<90)return t('now');if(x<3600)return t('minAgo',Math.round(x/60));if(s>=dayStart())return t('hAgo',Math.round(x/3600));if(s>=dayStart(1))return t('yesterday');return t('daysAgo',daysBack(s))}
function dayLabel(s){if(!s)return t('noTime');if(s>=dayStart())return t('Today');if(s>=dayStart(1))return t('Yesterday');return new Date(s*1000).toLocaleDateString(t('locale'),{weekday:'long',day:'numeric',month:'long'})}
const dur=s=>s<60?s+' s':Math.floor(s/60)+' min'+(s%60?' '+s%60+' s':'');
function toast(m){if(!m)return;const e=$('toast');e.textContent=m;e.classList.add('on');clearTimeout(toast.t);toast.t=setTimeout(()=>e.classList.remove('on'),3200)}
async function api(path,data,json){const o={method:'POST',headers:{'X-Tonepie-Token':token,'X-Tonepie-Lang':LANG}};
  if(json){o.headers['Content-Type']='application/json';o.body=JSON.stringify(data)}else o.body=new URLSearchParams(data||{});
  try{const r=await fetch(path,o);const j=await r.json().catch(()=>({}));return{ok:r.ok,message:j.message||''}}catch(e){return{ok:false,message:t('unreachableApi')}}}
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
// The firmware accepts writes only on fresh data and inside an enabled session, one at a time.
async function mcuWrites(list){
  await api('/api/query');await sleep(1200);
  let r=await api('/api/arm',{enabled:1});if(!r.ok)return r;
  for(const[i,w]of list.entries()){if(i)await sleep(2600);r=await api(w.path,w.data);
    if(!r.ok){await sleep(3200);r=await api(w.path,w.data)} // previous write still waiting for its report
    if(!r.ok)break}
  await api('/api/arm',{enabled:0});return r;
}
function confirmBox(title,text,ok){return new Promise(res=>{$('cfTitle').textContent=title;$('cfText').textContent=text;$('cfOk').textContent=ok;const d=$('dlgConfirm');$('cfOk').onclick=()=>{res(true);d.close()};d.onclose=()=>res(false);d.showModal()})}
document.querySelectorAll('dialog').forEach(d=>{d.addEventListener('click',e=>{if(e.target===d||e.target.closest('[data-close]'))d.close()})});

function setStatus(cls,text){const p=$('status');p.className='pill '+cls;p.lastElementChild.textContent=text}
function render(){
  const m=S.mcu,cats=S.config.cats,vis=S.visits,limit=S.config.bin_limit_visits;
  if(!m.online)setStatus('',t('offline'));else if(m.fault)setStatus('bad',t('fault'));else if(m.presence)setStatus('cat',t('catInside'));else if(m.ready)setStatus('ok',t('ready'));else setStatus('',t('booting'));
  const pct=S.bin.visits/limit,al=$('alerts');al.replaceChildren();
  const warn='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.200" stroke-linecap="round" stroke-linejoin="round"><path d="M12 9v4m0 4h.01M10.300 3.900 2.500 17.500A2 2 0 0 0 4.200 20.500h15.600a2 2 0 0 0 1.700-3L13.700 3.900a2 2 0 0 0-3.400 0z"/></svg>';
  if(m.online&&m.fault)al.append(h('div',{class:'alert'},h('span',{html:warn,style:'display:flex'}),t('faultAlert',m.fault)));
  if(pct>=1)al.append(h('div',{class:'alert'},h('span',{html:warn,style:'display:flex'}),t('fullAlert')));
  else if(pct>=.8)al.append(h('div',{class:'alert warn'},h('span',{html:warn,style:'display:flex'}),t('almostAlert')));
  // cats
  const box=$('cats');box.replaceChildren();
  if(!cats.length){box.append(h('div',{class:'card welcome'},h('div',{class:'avatar',html:CAT}),h('h2',{},t('welcome')),h('p',{},t('welcomeText')),h('button',{class:'btn primary',onclick:openSettings},t('welcomeBtn'))))}
  const t0=dayStart();let maxDay=1;const days=cats.map((c,i)=>{const a=Array(7).fill(0);for(const v of vis)if(v.cat===i&&v.t){const k=daysBack(v.t);if(k>=0&&k<7)a[6-k]++}maxDay=Math.max(maxDay,...a);return a});
  const letters=t('letters'),wd=new Date().getDay();
  cats.forEach((c,i)=>{const mine=vis.filter(v=>v.cat===i),last=mine[0],lw=mine.find(v=>v.g);
    box.append(h('article',{class:'card cat',style:'--c:'+COLORS[c.color%6],role:'button',tabindex:0,title:t('tapCat'),onclick:()=>openCat(i),onkeydown:e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();openCat(i)}}},
      h('div',{class:'cat-top'},avatar(c.color),h('div',{},h('h2',{},c.name),h('p',{class:'muted small'},lw?kg(lw.g):kg(c.weight_g)))),
      h('div',{class:'big'},h('strong',{},String(days[i][6])),h('span',{},t('visitsToday',days[i][6]))),
      h('p',{class:'last'},last?(last.t?t('last',ago(last.t))+(last.t<t0&&last.t>=dayStart(1)?t('at',clock(last.t)):''):t('lastNoTime')):t('noVisits')),
      h('div',{class:'bars'},days[i].map((n,k)=>h('div',{class:'bar'+(k===6?' today':''),title:t('nVisits',n)},h('i',{style:'height:'+Math.round(n/maxDay*100)+'%'}),letters[(wd+1+k)%7])))))});
  const unk=vis.filter(v=>v.cat<0&&(!v.t||v.t>=t0)).length;
  if(cats.length&&unk)box.append(h('div',{class:'card wide'},avatar(null,'sm'),h('div',{},h('b',{},t('unknown',unk)),h('p',{class:'muted small'},t('unknownSub')))));
  // bin: visits since the last bag change
  $('bin').hidden=false;const lvl=pct>=1?'var(--bad)':pct>=.8?'var(--warn)':'var(--ok)';
  $('ringFill').style.stroke=lvl;$('ringFill').style.strokeDashoffset=314.16*(1-Math.min(pct,1));
  $('ringPct').textContent=Math.round(pct*100)+'%';$('binN').textContent=S.bin.visits+' '+t('binCount',S.bin.visits);$('binOf').textContent=t('of',limit);
  $('binSub').textContent=[S.bin.since?t('bagChanged',ago(S.bin.since)):null,pct<1?t('left',limit-S.bin.visits):null].filter(Boolean).join(' · ');
  // visits
  $('visitsCard').hidden=false;const list=$('visits');list.replaceChildren();$('visitsHint').textContent=vis.length?t('tapFix'):'';
  if(!vis.length)list.append(h('p',{class:'empty'},t('visitsEmpty')));
  let day='';for(const v of vis.slice(0,showAll?64:10)){const d=dayLabel(v.t);if(d!==day){day=d;list.append(h('p',{class:'day'},d))}
    const c=v.cat>=0?cats[v.cat]:null;
    list.append(h('button',{class:'visit',onclick:()=>openVisit(v)},avatar(c?c.color:null,'sm'),
      h('div',{class:'who'},h('b',{},c?c.name:t('notRecognised')),h('span',{},[v.g?kg(v.g):null,v.s?dur(v.s):null].filter(Boolean).join(' · ')||t('noData'))),
      v.t?h('time',{},clock(v.t)):null))}
  if(vis.length>10)list.append(h('button',{class:'btn ghost block',style:'padding:10px',onclick:()=>{showAll=!showAll;render()}},showAll?t('showLess'):t('showAll',vis.length)));
  $('cleanWrap').hidden=false;$('bag').disabled=$('litter').disabled=busy;
  const when=(s,key,never)=>!s?t(never):ago(s)===t('now')?t('justNow'):t(key,ago(s));
  $('bagSub').textContent=when(S.bin.since,'bagWhen','bagNever');$('litterSub').textContent=when(S.litter_at,'litterWhen','litterNever');
  $('clean').disabled=busy||!m.ready||!!m.presence||!!m.fault||!!m.lock||m.pending;
  $('foot').replaceChildren('Firmware '+S.firmware+' · ',h('a',{href:'/dev'},t('devPage')));drawTrend();drawHist();
  if($('dlgCat').open)openCat(openCat.i);
}
// Visits of one cat per local day, from today back to the oldest visit kept (at least a week).
function openCat(i){
  const c=S.config.cats[i];if(!c){$('dlgCat').close();return}openCat.i=i;
  const mine=S.visits.filter(v=>v.cat===i),timed=mine.filter(v=>v.t),oldest=S.visits.reduce((a,v)=>v.t&&v.t<a?v.t:a,Date.now()/1000);
  const srv=syncOn()?histCache[90]?.h:null,srvDays={};
  if(srv)for(const d of srv.days)srvDays[d.d]=d.v[c.name]||0;
  const span=Math.min(60,Math.max(7,daysBack(oldest)+1,srv&&srv.firstVisit?daysBack(srv.firstVisit)+1:0)),counts=Array(span).fill(0);
  for(const v of timed){const k=daysBack(v.t);if(k>=0&&k<span)counts[k]++}
  // Both sources hold only part of the truth (server: not yet sent; ESP: last 64 visits): take the larger.
  if(srv)for(let k=0;k<span;k++)counts[k]=Math.max(counts[k],srvDays[dateKey(dayStart(k))]||0);
  const total=counts.reduce((a,b)=>a+b,0),most=Math.max(1,...counts);
  $('dlgCat').style.setProperty('--c',COLORS[c.color%6]);
  $('catHead').replaceChildren(avatar(c.color),h('div',{},h('h2',{},c.name),h('p',{class:'muted'},t('dayByDay'))));
  $('catStats').replaceChildren(h('div',{},h('b',{},String(total)),h('span',{},t('total')+' '+t('lastDays',span))),
    h('div',{},h('b',{},num(total/span,1)),h('span',{},t('perDay'))));
  $('catDays').replaceChildren(...counts.map((n,k)=>{const s=dayStart(k);
    const label=k===0?t('Today'):k===1?t('Yesterday'):new Date(s*1000).toLocaleDateString(t('locale'),{weekday:'short',day:'numeric',month:'short'});
    return h('div',{class:'drow'+(n?'':' zero')},h('span',{},label),h('div',{},h('i',{style:'width:'+(n/most*100)+'%'})),h('b',{},String(n)))}));
  const untimed=mine.length-timed.length;$('catNote').textContent=(untimed?t('untimed',untimed)+' ':'')+(srv?t('fromServer'):t('keeps'));
  if(!$('dlgCat').open)$('dlgCat').showModal();
  if(syncOn()&&!srv)histFetch(90).then(()=>{if($('dlgCat').open&&openCat.i===i)openCat(i)}).catch(()=>{});
}
// ----- history kept by the optional server (the browser asks it directly; the ESP only sends data)
const syncOn=()=>!!(S&&S.sync&&S.sync.enabled&&S.sync.url);
const dateKey=s=>{const d=new Date(s*1000);return d.getFullYear()+'-'+two(d.getMonth()+1)+'-'+two(d.getDate())};
const parseDay=k=>{const[y,m,d]=k.split('-').map(Number);return new Date(y,m-1,d)};
let histRange=90;const histCache={};
async function histFetch(days){
  const sy=S.sync,c=histCache[days];if(c&&c.ok===sy.ok_at&&c.url===sy.url&&Date.now()-c.at<300000)return c.h;
  const r=await fetch(sy.url+'/api/history?device='+encodeURIComponent(sy.device)+'&days='+days,{cache:'no-store'});if(!r.ok)throw 0;
  const h=await r.json();histCache[days]={h,at:Date.now(),ok:sy.ok_at,url:sy.url};return h;
}
function syncText(){const y=S.sync;return !y.enabled?t('syncOff'):y.error?t('syncErr',y.error):y.pending?t('syncPending'):y.ok_at?t('syncOk',ago(y.ok_at)):t('syncNever')}
async function drawHist(){
  const card=$('hist'),body=$('histBody');card.hidden=!syncOn();if(card.hidden)return;
  const days=histRange;if(!histCache[days])body.replaceChildren(h('p',{class:'empty'},t('histLoading')));
  let H;try{H=await histFetch(days)}catch(e){body.replaceChildren(h('p',{class:'empty'},t('histError',S.sync.url)),h('p',{class:'muted small'},syncText()));return}
  if(days!==histRange)return;
  // Series: current cats first (their colours), then cats removed since, then unrecognised visits.
  const cats=S.config.cats,names=cats.map(c=>c.name);
  for(const d of H.days)for(const n of Object.keys(d.v))if(n&&!names.includes(n))names.push(n);
  // Removed cats take palette colours no current cat uses; unrecognised visits are grey.
  const free=COLORS.filter((x,k)=>!cats.some(c=>c.color%6===k));let nf=0;
  const series=names.map(n=>{const c=cats.find(x=>x.name===n);return{n,label:n,color:c?COLORS[c.color%6]:free[nf++%Math.max(1,free.length)]||'var(--muted)'}});
  if(H.days.some(d=>d.v['']))series.push({n:'',label:t('unknownCats'),color:'var(--muted)'});
  if(!H.days.length){body.replaceChildren(h('p',{class:'empty'},t('histEmpty')),h('p',{class:'muted small'},syncText()));return}
  const today=parseDay(dateKey(Date.now()/1000)),first=days?new Date(today.getFullYear(),today.getMonth(),today.getDate()-days+1):parseDay(H.days[0].d);
  const span=Math.max(1,Math.round((today-first)/864e5)+1),monthly=span>200;
  // Buckets: weeks starting on Monday, or months over long spans.
  const bucketStart=d=>monthly?new Date(d.getFullYear(),d.getMonth(),1):new Date(d.getFullYear(),d.getMonth(),d.getDate()-(d.getDay()+6)%7);
  const buckets=[];for(let d=bucketStart(first);d<=today;d=monthly?new Date(d.getFullYear(),d.getMonth()+1,1):new Date(d.getFullYear(),d.getMonth(),d.getDate()+7))buckets.push({start:d,v:{}});
  const find=d=>{const s=bucketStart(d).getTime();return buckets.find(b=>b.start.getTime()===s)};
  const totals={};for(const d of H.days){const b=find(parseDay(d.d));for(const[n,c]of Object.entries(d.v)){totals[n]=(totals[n]||0)+c;if(b)b.v[n]=(b.v[n]||0)+c}}
  const chips=h('div',{class:'chips'},series.map(s=>h('div',{class:'chip',style:'--c:'+s.color},h('i'),h('div',{},h('b',{},String(totals[s.n]||0)),' ',s.label,h('br'),h('span',{},num((totals[s.n]||0)/span,1)+' '+t('perDay2'))))));
  // stacked bars
  const plot=h('div',{class:'plot'}),sel=h('div',{class:'hsel'});
  body.replaceChildren(chips,plot,sel);
  const W=plot.clientWidth||320,Hh=150,L=26,R=6,T=8,B=20,n=buckets.length,bw=(W-L-R)/n;
  const max=Math.max(1,...buckets.map(b=>series.reduce((a,s)=>a+(b.v[s.n]||0),0)));
  const stepY=max<=10?2:max<=25?5:max<=60?10:max<=150?25:50,top=Math.ceil(max/stepY)*stepY,y=v=>T+(1-v/top)*(Hh-T-B);
  let g='';for(let v=0;v<=top;v+=stepY)g+=`<line class="grid" x1="${L}" x2="${W-R}" y1="${y(v).toFixed(1)}" y2="${y(v).toFixed(1)}"/><text x="${L-5}" y="${(y(v)+4).toFixed(1)}" text-anchor="end">${v}</text>`;
  const fmt=d=>d.toLocaleDateString(t('locale'),monthly?{month:'short',year:'2-digit'}:{day:'numeric',month:'short'});
  buckets.forEach((b,i)=>{let acc=0;const x=L+i*bw+Math.min(1.5,bw*.15),w=Math.max(1,bw-Math.min(3,bw*.3));
    for(const s of series){const c=b.v[s.n]||0;if(!c)continue;g+=`<rect data-i="${i}" x="${x.toFixed(1)}" y="${y(acc+c).toFixed(1)}" width="${w.toFixed(1)}" height="${(y(acc)-y(acc+c)).toFixed(1)}" rx="${Math.min(3,w/3).toFixed(1)}" fill="${s.color}"/>`;acc+=c}});
  const lab=[0,Math.floor(n/2),n-1].filter((v,i,a)=>a.indexOf(v)===i);
  for(const i of lab)g+=`<text x="${(L+(i+.5)*bw).toFixed(1)}" y="${Hh-5}" text-anchor="${i===0?'start':i===n-1?'end':'middle'}">${fmt(buckets[i].start)}</text>`;
  plot.innerHTML=`<svg width="${W}" height="${Hh}" viewBox="0 0 ${W} ${Hh}" role="img" aria-label="${t('histTitle')}">${g}</svg>`;
  const show=i=>{const b=buckets[i];plot.querySelectorAll('rect').forEach(r=>r.classList.toggle('sel',+r.dataset.i!==i));
    sel.replaceChildren(h('b',{},monthly?b.start.toLocaleDateString(t('locale'),{month:'long',year:'numeric'}):t('weekOf',fmt(b.start))),
      ...series.filter(s=>b.v[s.n]).map(s=>h('span',{},h('i',{style:'--c:'+s.color}),s.label+' '+b.v[s.n])))};
  plot.onpointermove=plot.onpointerdown=e=>{const i=Math.floor((e.clientX-plot.getBoundingClientRect().left-L)/bw);if(i>=0&&i<n)show(i)};
  plot.onpointerleave=()=>{plot.querySelectorAll('rect').forEach(r=>r.classList.remove('sel'));sel.replaceChildren()};
  // bag changes: interval and visits per bag
  const bags=H.bags.filter(b=>b>=first.getTime()/1000);
  if(bags.length){const gaps=bags.slice(1).map((b,i)=>(b-bags[i])/86400),per=bags.slice(1).map((b,i)=>H.days.filter(d=>{const s=parseDay(d.d).getTime()/1000;return s>=bags[i]-86399&&s<b-86399}).reduce((a,d)=>a+Object.values(d.v).reduce((x,y)=>x+y,0),0));
    body.append(h('p',{class:'muted small',style:'margin-top:8px'},t('bagsLine',bags.length,gaps.length?Math.round(gaps.reduce((a,b)=>a+b,0)/gaps.length):0,per.length?Math.round(per.reduce((a,b)=>a+b,0)/per.length):0)))}
  // per-month table: visits and average weight per cat
  const months={};for(const d of H.days){const m=d.d.slice(0,7),row=months[m]||(months[m]={v:{},w:{},wn:{}});
    for(const[k,c]of Object.entries(d.v))row.v[k]=(row.v[k]||0)+c;for(const[k,g]of Object.entries(d.w)){const c=d.v[k]||1;row.w[k]=(row.w[k]||0)+g*c;row.wn[k]=(row.wn[k]||0)+c}}
  const named=series.filter(s=>s.n);
  body.append(h('details',{},h('summary',{},t('months')),h('table',{},h('tr',{},h('th',{},t('month')),named.map(s=>h('th',{},s.label))),
    Object.keys(months).sort().reverse().map(m=>h('tr',{},h('td',{},parseDay(m+'-01').toLocaleDateString(t('locale'),{month:'short',year:'numeric'})),
      named.map(s=>{const r=months[m],c=r.v[s.n]||0;return h('td',{},c?c+(r.wn[s.n]?' · '+num(r.w[s.n]/r.wn[s.n]/1000,1)+' kg':''):'—')}))))),
    h('p',{class:'muted small',style:'margin-top:8px'},syncText()));
}
document.querySelectorAll('[data-hist]').forEach(b=>b.onclick=()=>{histRange=+b.dataset.hist;document.querySelectorAll('[data-hist]').forEach(o=>o.setAttribute('aria-pressed',String(o===b)));drawHist()});
const dayText=d=>new Date(d*864e5).toLocaleDateString(t('locale'),{day:'numeric',month:'short'});
const signed=v=>(v>0?'+':v<0?'−':'')+num(Math.abs(v),1)+' kg';
function drawTrend(){
  const cats=S.config.cats,card=$('trend');card.hidden=!cats.length;if(!cats.length)return;
  const today=Math.floor(Date.now()/864e5),from=today-range+1;
  const series=cats.map(c=>({name:c.name,color:COLORS[c.color%6],pts:(c.days||[]).map((d,k)=>[d,c.grams[k]/1000]).filter(p=>p[0]>=from&&p[0]<=today)}));
  $('legend').replaceChildren(...series.map(s=>{const a=s.pts[0],b=s.pts[s.pts.length-1];
    return h('div',{style:'--c:'+s.color},h('i'),s.name,h('span',{},b?num(b[1],1)+' kg'+(s.pts.length>1?t('inDays',signed(b[1]-a[1]),b[0]-a[0]):''):t('noWeight')))}));
  const plot=$('plot'),all=series.flatMap(s=>s.pts.map(p=>p[1]));
  $('trendTable').hidden=!all.length;
  if(!all.length){plot.onpointermove=null;plot.replaceChildren(h('p',{class:'empty'},t('trendEmpty')));return}
  const W=plot.clientWidth||320,H=190,L=30,R=40,T=10,B=24;
  let lo=Math.min(...all)-.15,hi=Math.max(...all)+.15;const step=hi-lo<=1?.2:hi-lo<=2.5?.5:1;
  lo=Math.floor(lo/step+1e-9)*step;hi=Math.ceil(hi/step-1e-9)*step;
  const x=d=>L+(d-from)/(range-1)*(W-L-R),y=v=>T+(hi-v)/(hi-lo)*(H-T-B),f=n=>n.toFixed(1);
  let g='';
  for(let v=lo;v<=hi+1e-9;v+=step)g+=`<line class="grid" x1="${L}" x2="${W-R}" y1="${f(y(v))}" y2="${f(y(v))}"/><text x="${L-6}" y="${f(y(v)+4)}" text-anchor="end">${num(v,1)}</text>`;
  [[from,'start'],[from+Math.floor(range/2),'middle'],[today,'end']].forEach(([d,a])=>g+=`<text x="${f(x(d))}" y="${H-5}" text-anchor="${a}">${d===today?t('today'):dayText(d)}</text>`);
  g+=`<line class="cross" id="cross" y1="${T}" y2="${H-B}" visibility="hidden"/>`;
  // End labels carry the latest value; nudged apart when two lines finish close together.
  const ends=series.filter(s=>s.pts.length).map(s=>({s,y:y(s.pts[s.pts.length-1][1])})).sort((a,b)=>a.y-b.y);
  for(let i=1;i<ends.length;i++)if(ends[i].y-ends[i-1].y<13)ends[i].y=ends[i-1].y+13;
  for(const s of series){if(!s.pts.length)continue;
    g+=`<path stroke="${s.color}" d="${s.pts.map((p,i)=>(i?'L':'M')+f(x(p[0]))+' '+f(y(p[1]))).join('')}"/>`;
    const dots=s.pts.length<=14?s.pts:[s.pts[s.pts.length-1]];
    for(const p of dots)g+=`<circle cx="${f(x(p[0]))}" cy="${f(y(p[1]))}" r="4" fill="${s.color}"/>`}
  for(const e of ends){const p=e.s.pts[e.s.pts.length-1];g+=`<text class="end" x="${f(x(p[0])+9)}" y="${f(e.y+4)}">${num(p[1],1)}</text>`}
  plot.innerHTML=`<svg width="${W}" height="${H}" viewBox="0 0 ${W} ${H}" role="img" aria-label="${t('chart')}">${g}</svg>`;
  const tip=h('div',{class:'tip',hidden:''});plot.append(tip);const cross=plot.querySelector('#cross');
  const daysWithData=[...new Set(series.flatMap(s=>s.pts.map(p=>p[0])))].sort((a,b)=>a-b);
  plot.onpointermove=e=>{const px=e.clientX-plot.getBoundingClientRect().left,want=from+(px-L)/(W-L-R)*(range-1);
    const d=daysWithData.reduce((a,b)=>Math.abs(b-want)<Math.abs(a-want)?b:a);
    cross.setAttribute('x1',f(x(d)));cross.setAttribute('x2',f(x(d)));cross.setAttribute('visibility','visible');
    tip.replaceChildren(h('b',{},d===today?t('Today'):dayText(d)),...series.map(s=>{const p=s.pts.find(p=>p[0]===d);return p?h('div',{style:'--c:'+s.color},h('i'),s.name+' '+num(p[1],1)+' kg'):null}).filter(Boolean));
    tip.hidden=false;const tw=tip.offsetWidth;tip.style.left=Math.max(0,Math.min(W-tw,x(d)+(x(d)>W/2?-tw-10:10)))+'px'};
  plot.onpointerleave=()=>{tip.hidden=true;cross.setAttribute('visibility','hidden')};
  $('trendRows').replaceChildren(h('table',{},h('tr',{},h('th',{},t('day')),series.map(s=>h('th',{},s.name))),
    daysWithData.slice().reverse().map(d=>h('tr',{},h('td',{},dayText(d)),series.map(s=>{const p=s.pts.find(p=>p[0]===d);return h('td',{},p?num(p[1],1)+' kg':'—')})))));
}
document.querySelectorAll('[data-range]').forEach(b=>b.onclick=()=>{range=+b.dataset.range;document.querySelectorAll('[data-range]').forEach(o=>o.setAttribute('aria-pressed',String(o===b)));drawTrend()});
addEventListener('resize',()=>{if(S){drawTrend();drawHist()}});
async function refresh(){
  try{const r=await fetch('/api/home',{cache:'no-store'});if(!r.ok)throw 0;const s=await r.json();token=s.token;S=s;
    const key=JSON.stringify([s.mcu,s.config,s.bin,s.visits,Math.floor(Date.now()/60000)]);if(key!==lastKey){lastKey=key;render()}}
  catch(e){lastKey='';setStatus('',t('unreachable'));$('clean').disabled=true}
}
function openVisit(v){
  const cats=S.config.cats;$('visitInfo').textContent=[v.t?dayLabel(v.t)+t('at',clock(v.t)):null,v.g?kg(v.g):null,v.s?dur(v.s):null].filter(Boolean).join(' · ')||t('visitNoData');
  const set=async cat=>{$('dlgVisit').close();const r=await api('/api/visit',{id:v.id,cat});toast(r.message);refresh()};
  $('visitChoices').replaceChildren(...cats.map((c,i)=>h('button',{class:'choice',onclick:()=>set(i)},avatar(c.color,'sm'),c.name)),
    h('button',{class:'choice',onclick:()=>set(-1)},avatar(null,'sm'),t('dontKnow')),
    h('button',{class:'btn danger',onclick:()=>set('delete')},t('deleteVisit')));
  $('dlgVisit').showModal();
}
function drawEditors(){
  const box=$('catEditors');box.replaceChildren();
  draft.cats.forEach((c,i)=>{
    box.append(h('div',{class:'cat-edit'},
      h('div',{class:'row'},avatar(c.color,'sm'),
        h('div',{class:'field grow'},h('input',{value:c.name,placeholder:t('name'),maxlength:16,required:'',oninput:e=>c.name=e.target.value})),
        h('div',{class:'field kg'},h('input',{type:'number',value:c.kg,min:.5,max:20,step:.1,required:'',inputmode:'decimal',placeholder:'4.0',oninput:e=>{c.kg=e.target.value;hint()}}),h('span',{},'kg'))),
      h('div',{class:'swatches'},COLORS.map((col,k)=>h('button',{type:'button',class:'swatch',style:'--c:'+col,'aria-pressed':String(c.color===k),'aria-label':t('colour',k+1),onclick:()=>{c.color=k;drawEditors()}})),
        h('button',{type:'button',class:'btn danger',onclick:()=>{draft.cats.splice(i,1);drawEditors()}},t('remove')))))});
  $('addCat').hidden=draft.cats.length>=4;hint();
}
function hint(){const w=draft.cats.map(c=>parseFloat(c.kg)).filter(x=>x>0);let close=false;for(let i=0;i<w.length;i++)for(let j=i+1;j<w.length;j++)if(Math.abs(w[i]-w[j])<.3)close=true;$('closeHint').hidden=!close}
function openSettings(){
  if(!S)return;const c=S.config;draft={cats:c.cats.map((x,i)=>({name:x.name,kg:(x.weight_g/1000).toFixed(1),shown:(x.weight_g/1000).toFixed(1),grams:x.weight_g,color:x.color,from:i}))};
  if(!draft.cats.length)draft.cats.push({name:'',kg:'',color:0},{name:'',kg:'',color:1});
  $('optLimit').value=c.bin_limit_visits;$('optTol').value=(c.tolerance_g/1000).toFixed(1);
  const m=S.mcu,on=!!m.ready;$('mcuNote').hidden=on;
  $('mAuto').checked=!!m.auto;$('mAuto').disabled=!on||m.auto==null;
  $('mWait').value=m.wait_min??'';$('mWait').disabled=!on||m.wait_min==null;
  $('mOdor').checked=!!m.odor;$('mOdor').disabled=!on||m.odor==null;
  const y=S.sync||{};$('sOn').checked=!!y.enabled;$('sUrl').value=y.url||'';$('sKey').value='';$('sKey').placeholder=t(y.key_set?'keyKeep':'keyNew');$('sStatus').textContent=syncText();
  drawEditors();$('dlgSettings').showModal();
}
$('gear').onclick=openSettings;
$('addCat').onclick=()=>{draft.cats.push({name:'',kg:'',color:draft.cats.length%6});drawEditors()};
$('formSettings').onsubmit=async e=>{e.preventDefault();
  const r=await api('/api/config',{cats:draft.cats.map(c=>({name:c.name.trim(),weight_g:c.kg===c.shown?c.grams:Math.round(parseFloat(c.kg)*1000),color:c.color,from:c.from??-1})),
    bin_limit_visits:+$('optLimit').value,tolerance_g:Math.round(parseFloat($('optTol').value)*1000)},true);
  toast(r.message);if(!r.ok)return;
  // Settings kept by the litter box itself: send only what changed.
  const m=S.mcu,w=[],auto=$('mAuto'),wait=$('mWait'),odor=$('mOdor');
  if(!auto.disabled&&auto.checked!==!!m.auto)w.push({path:'/api/command',data:{action:auto.checked?'auto_on':'auto_off'}});
  if(!wait.disabled&&wait.value!==''&&+wait.value!==m.wait_min)w.push({path:'/api/value',data:{dp:117,value:+wait.value}});
  if(!odor.disabled&&odor.checked!==!!m.odor)w.push({path:'/api/command',data:{action:odor.checked?'odor_on':'odor_off'}});
  const y=S.sync||{},sOn=$('sOn').checked,sUrl=$('sUrl').value.trim().replace(/\/+$/,''),sKey=$('sKey').value.trim();
  if(sOn!==!!y.enabled||sUrl!==(y.url||'')||sKey){const r2=await api('/api/sync',{enabled:sOn?1:0,url:sUrl,key:sKey});toast(r2.message);if(!r2.ok)return;lastKey=''}
  $('dlgSettings').close();
  if(w.length){toast(t('sending'));const sent=await mcuWrites(w);await sleep(1500);await refresh();
    const s=S.mcu,kept=(auto.disabled||auto.checked===!!s.auto)&&(wait.disabled||wait.value===''||+wait.value===s.wait_min)&&(odor.disabled||odor.checked===!!s.odor);
    toast(!sent.ok?sent.message:kept?t('confirmed'):t('notConfirmed'))}
  else refresh()};
// Both are recorded here even with the litter box offline; the MCU is told only when it is ready.
async function maintenance(path,action,done,title,text,ok){
  const on=S.mcu.ready&&!S.mcu.presence&&!S.mcu.fault&&!S.mcu.lock;
  if(!await confirmBox(title,text+(on?t('drum'):t('notReady')),ok))return;
  busy=true;$('bag').disabled=$('litter').disabled=$('clean').disabled=true;
  const r=await api(path);
  if(r.ok&&on){const w=await mcuWrites([{path:'/api/command',data:{action}}]);toast(w.ok?done:t('refused',w.message))}else toast(r.message);
  busy=false;lastKey='';refresh();
}
$('bag').onclick=()=>maintenance('/api/bin/reset','bag',t('bagDone'),t('bag'),t('bagText'),t('bagOk'));
$('litter').onclick=()=>maintenance('/api/litter','level',t('litterDone'),t('litter'),t('litterText'),t('litterOk'));
$('clean').onclick=async()=>{
  if(!await confirmBox(t('cleanAsk'),t('cleanText'),t('cleanNow')))return;
  busy=true;$('clean').disabled=true;
  const r=await mcuWrites([{path:'/api/command',data:{action:'clean'}}]);toast(r.ok?t('cleanSent'):r.message);
  busy=false;lastKey='';refresh();
};
applyLang();setStatus('',t('connecting'));
refresh();setInterval(refresh,5000);document.addEventListener('visibilitychange',()=>{if(!document.hidden)refresh()});
</script></body></html>)HTML";
