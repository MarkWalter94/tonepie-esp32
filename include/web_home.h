#pragma once
#include <Arduino.h>
// Everyday page served at "/". Self-contained: no CDN, no filesystem.
const char HOME_UI[] PROGMEM = R"HTML(<!doctype html>
<html lang="it"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#f4efe8" media="(prefers-color-scheme: light)">
<meta name="theme-color" content="#151210" media="(prefers-color-scheme: dark)">
<title>Lettiera</title>
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
@media (max-width:380px){.big strong{font-size:38px}.ring{width:100px;height:100px}.bin-main{font-size:24px}}
@media (prefers-reduced-motion:reduce){*{animation:none!important;transition:none!important}}
</style></head><body><main>
<header><div><p class="eyebrow">Tonepie Ti Pro</p><h1>Lettiera</h1></div>
<div class="head-right"><span id="status" class="pill"><i></i><span>Collegamento…</span></span>
<button class="icon-btn" id="gear" aria-label="Impostazioni"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.7 1.7 0 0 0 .3 1.8l.1.1a2 2 0 1 1-2.8 2.8l-.1-.1a1.7 1.7 0 0 0-2.8 1.2V21a2 2 0 1 1-4 0v-.1a1.7 1.7 0 0 0-2.9-1.2l-.1.1a2 2 0 1 1-2.8-2.8l.1-.1A1.7 1.7 0 0 0 3.100 14H3a2 2 0 1 1 0-4h.1a1.7 1.7 0 0 0 1.200-2.900l-.1-.1a2 2 0 1 1 2.800-2.800l.1.1a1.7 1.7 0 0 0 2.900-1.200V3a2 2 0 1 1 4 0v.1a1.7 1.7 0 0 0 2.900 1.200l.1-.1a2 2 0 1 1 2.800 2.800l-.1.1a1.7 1.7 0 0 0 1.200 2.900H21a2 2 0 1 1 0 4h-.1a1.7 1.7 0 0 0-1.500 1z"/></svg></button></div></header>
<div id="alerts"></div>
<section id="cats"></section>
<section class="card bin" id="bin" hidden>
  <div class="ring"><svg viewBox="0 0 120 120"><circle cx="60" cy="60" r="50"/><circle class="fill" id="ringFill" cx="60" cy="60" r="50" stroke-dasharray="314.16" stroke-dashoffset="314.16"/></svg><b id="ringPct">0%</b></div>
  <div><h2>CASSETTO · STIMA</h2><p class="bin-main"><b id="binG">0 g</b> <span id="binOf"></span></p><p class="muted small" id="binSub"></p></div>
</section>
<section class="card trend" id="trend" hidden><div class="section-title"><h2>Andamento peso</h2>
  <div class="seg" role="group" aria-label="Periodo"><button data-range="30" aria-pressed="true">30 gg</button><button data-range="90" aria-pressed="false">90 gg</button></div></div>
  <div class="legend" id="legend"></div><div class="plot" id="plot"></div>
  <details id="trendTable"><summary>Vedi i valori</summary><div id="trendRows"></div></details></section>
<section class="card" id="visitsCard" hidden><div class="section-title"><h2>Ultime visite</h2><span class="muted small" id="visitsHint"></span></div><div id="visits"></div></section>
<section class="tiles" id="cleanWrap" hidden>
  <button class="tile card" id="clean"><span class="ico"><svg viewBox="0 0 24 24" fill="currentColor"><path d="M12 2l1.800 5.200L19 9l-5.200 1.800L12 16l-1.800-5.200L5 9l5.200-1.800zM19 15l.9 2.600 2.600.9-2.600.9L19 22l-.9-2.600-2.600-.9 2.600-.9zM5 15l.7 2 2 .7-2 .7L5 20.500l-.7-2.100-2-.7 2-.7z"/></svg></span><b>Pulisci ora</b><small>avvia un ciclo</small></button>
  <button class="tile card" id="bag"><span class="ico"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9 7c-1-1.500-1-3 0-4h6c1 1 1 2.500 0 4M7 7h10l2 11a2.500 2.500 0 0 1-2.500 3h-9A2.500 2.500 0 0 1 5 18z"/></svg></span><b>Cambio sacchetto</b><small id="bagSub"></small></button>
  <button class="tile card" id="litter"><span class="ico"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3v8M8 7h8M3 17c2-2 4-2 6 0s4 2 6 0 4-2 6 0M3 21h18"/></svg></span><b>Aggiunta lettiera</b><small id="litterSub"></small></button>
</section>
<footer id="foot"></footer>
</main>
<dialog id="dlgSettings"><form class="sheet" method="dialog" id="formSettings">
  <h2>I tuoi gatti</h2><p>Li riconosco dal peso a ogni visita. Il peso di riferimento si aggiorna da solo, così continuo a riconoscerli anche se crescono o dimagriscono.</p>
  <div id="catEditors"></div><p class="hint" id="closeHint" hidden>Pesi molto vicini: il riconoscimento potrebbe confonderli.</p>
  <button type="button" class="btn" id="addCat">+ Aggiungi gatto</button>
  <h3 class="sub">Lettiera</h3><p class="hint" id="mcuNote" hidden>Lettiera non collegata: queste tre impostazioni non si possono cambiare ora.</p>
    <div class="opt"><label for="mAuto">Pulizia automatica<small>Dopo ogni visita svuota da sola nel cassetto</small></label><input class="switch" type="checkbox" id="mAuto"></div>
    <div class="opt"><label for="mWait">Pausa prima della pulizia<small>Minuti di attesa dopo l’uscita del gatto</small></label><div class="field"><input id="mWait" type="number" min="0" max="60" step="1" inputmode="numeric"><span>min</span></div></div>
    <div class="opt"><label for="mOdor">Deodorante automatico<small>Si attiva da solo dopo ogni pulizia</small></label><input class="switch" type="checkbox" id="mOdor"></div>
  <details><summary>Cassetto e riconoscimento</summary>
    <div class="opt"><label>Cassetto pieno a<small>Peso di escrementi oltre il quale va svuotato</small></label><div class="field"><input id="optLimit" type="number" min="100" max="20000" step="50" required><span>g</span></div></div>
    <div class="opt"><label>Peso stimato per visita<small>La lettiera non pesa gli escrementi: uso questa media</small></label><div class="field"><input id="optGrams" type="number" min="5" max="500" step="5" required><span>g</span></div></div>
    <div class="opt"><label>Tolleranza sul peso<small>Scarto massimo dal peso del gatto</small></label><div class="field"><input id="optTol" type="number" min="0.1" max="3" step="0.1" required><span>kg</span></div></div>
  </details>
  <div class="actions"><button type="button" class="btn ghost" data-close>Annulla</button><button class="btn primary" value="save">Salva</button></div>
</form></dialog>
<dialog id="dlgVisit"><div class="sheet"><h2>Chi era?</h2><p id="visitInfo"></p><div class="choices" id="visitChoices"></div>
  <div class="actions"><button class="btn ghost" data-close>Chiudi</button></div></div></dialog>
<dialog id="dlgConfirm"><div class="sheet"><h2 id="cfTitle"></h2><p id="cfText"></p>
  <div class="actions"><button class="btn ghost" data-close>Annulla</button><button class="btn primary" id="cfOk"></button></div></div></dialog>
<div id="toast" role="status" aria-live="polite"></div>
<script>
const $=id=>document.getElementById(id);
const darkQuery=matchMedia('(prefers-color-scheme: dark)');
const PALETTE={light:['#ef8a4a','#8a97ad','#4a4653','#d8ae78','#a56b46','#79a98c'],dark:['#f2955b','#9aa7bd','#a39cb0','#dcb887','#c48a65','#88ba9c']};
let COLORS=PALETTE[darkQuery.matches?'dark':'light'];
darkQuery.addEventListener('change',e=>{COLORS=PALETTE[e.matches?'dark':'light'];if(S)render()});
const CAT='<svg viewBox="0 0 48 48" fill="currentColor"><path d="M7 8c0-1.300 1.500-2 2.500-1.200L18 13.500c1.900-.6 3.900-.9 6-.9s4.100.3 6 .9l8.500-6.700C39.500 6 41 6.700 41 8v17c0 9.400-7.600 16-17 16S7 34.400 7 25z"/><circle cx="17.500" cy="25" r="2.400" fill="var(--card)"/><circle cx="30.500" cy="25" r="2.400" fill="var(--card)"/><path d="M21.600 30.500h4.800L24 33.200z" fill="var(--card)"/></svg>';
let S=null,token='',lastKey='',draft=null,busy=false,showAll=false,range=30;
function h(tag,attrs,...kids){const e=document.createElement(tag);for(const[k,v]of Object.entries(attrs||{})){if(v==null||v===false)continue;if(k==='class')e.className=v;else if(k==='html')e.innerHTML=v;else if(k.startsWith('on'))e[k]=v;else if(k==='style')e.style.cssText=v;else e.setAttribute(k,v)}for(const k of kids.flat())if(k!=null)e.append(k);return e}
const avatar=(color,cls)=>color==null?h('div',{class:'avatar q '+(cls||'')},'?'):h('div',{class:'avatar '+(cls||''),style:'--c:'+COLORS[color%6],html:CAT});
const num=(v,d)=>v.toLocaleString('it-IT',{minimumFractionDigits:d,maximumFractionDigits:d});
const kg=g=>num(g/1000,1)+' kg';
const grams=g=>g>=1000?num(g/1000,g%1000?(g%100?2:1):0)+' kg':g+' g';
const two=n=>String(n).padStart(2,'0');
const clock=t=>{const d=new Date(t*1000);return two(d.getHours())+':'+two(d.getMinutes())};
const dayStart=(off=0)=>{const d=new Date();d.setHours(0,0,0,0);d.setDate(d.getDate()-off);return d.getTime()/1000};
function ago(t){const s=Date.now()/1000-t;if(s<90)return'adesso';if(s<3600)return Math.round(s/60)+' min fa';if(t>=dayStart())return Math.round(s/3600)+' h fa';if(t>=dayStart(1))return'ieri';return Math.round((dayStart()-t)/86400+.5)+' giorni fa'}
function dayLabel(t){if(!t)return'Orario non disponibile';if(t>=dayStart())return'Oggi';if(t>=dayStart(1))return'Ieri';return new Date(t*1000).toLocaleDateString('it-IT',{weekday:'long',day:'numeric',month:'long'})}
const dur=s=>s<60?s+' s':Math.floor(s/60)+' min'+(s%60?' '+s%60+' s':'');
function toast(t){if(!t)return;const e=$('toast');e.textContent=t;e.classList.add('on');clearTimeout(toast.t);toast.t=setTimeout(()=>e.classList.remove('on'),3200)}
async function api(path,data,json){const o={method:'POST',headers:{'X-Tonepie-Token':token}};
  if(json){o.headers['Content-Type']='application/json';o.body=JSON.stringify(data)}else o.body=new URLSearchParams(data||{});
  try{const r=await fetch(path,o);const j=await r.json().catch(()=>({}));return{ok:r.ok,message:j.message||''}}catch(e){return{ok:false,message:'Lettiera non raggiungibile'}}}
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
  const m=S.mcu,cats=S.config.cats,vis=S.visits;
  if(!m.online)setStatus('','Lettiera non collegata');else if(m.fault)setStatus('bad','Anomalia');else if(m.presence)setStatus('cat','Gatto dentro');else if(m.ready)setStatus('ok','Pronta');else setStatus('','Avvio…');
  const pct=S.bin.g/S.config.bin_limit_g,al=$('alerts');al.replaceChildren();
  const warn='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.200" stroke-linecap="round" stroke-linejoin="round"><path d="M12 9v4m0 4h.01M10.300 3.900 2.500 17.500A2 2 0 0 0 4.200 20.500h15.600a2 2 0 0 0 1.700-3L13.700 3.900a2 2 0 0 0-3.400 0z"/></svg>';
  if(m.online&&m.fault)al.append(h('div',{class:'alert'},h('span',{html:warn,style:'display:flex'}),'La lettiera segnala un’anomalia (codice '+m.fault+'). Controllala.'));
  if(pct>=1)al.append(h('div',{class:'alert'},h('span',{html:warn,style:'display:flex'}),'È ora di svuotare il cassetto.'));
  else if(pct>=.8)al.append(h('div',{class:'alert warn'},h('span',{html:warn,style:'display:flex'}),'Il cassetto è quasi pieno.'));
  // cats
  const box=$('cats');box.replaceChildren();
  if(!cats.length){box.append(h('div',{class:'card welcome'},h('div',{class:'avatar',html:CAT}),h('h2',{},'Chi usa la lettiera?'),h('p',{},'Dimmi nome e peso dei tuoi gatti: li riconoscerò a ogni visita.'),h('button',{class:'btn primary',onclick:openSettings},'Aggiungi i gatti')))}
  const t0=dayStart();let maxDay=1;const days=cats.map((c,i)=>{const a=Array(7).fill(0);for(const v of vis)if(v.cat===i&&v.t){const k=Math.floor((t0+86400-v.t)/86400);if(k>=0&&k<7)a[6-k]++}maxDay=Math.max(maxDay,...a);return a});
  const letters=['D','L','M','M','G','V','S'],wd=new Date().getDay();
  cats.forEach((c,i)=>{const mine=vis.filter(v=>v.cat===i),last=mine[0],lw=mine.find(v=>v.g);
    box.append(h('article',{class:'card cat',style:'--c:'+COLORS[c.color%6]},
      h('div',{class:'cat-top'},avatar(c.color),h('div',{},h('h2',{},c.name),h('p',{class:'muted small'},lw?kg(lw.g):kg(c.weight_g)))),
      h('div',{class:'big'},h('strong',{},String(days[i][6])),h('span',{},days[i][6]===1?'visita oggi':'visite oggi')),
      h('p',{class:'last'},last?(last.t?'Ultima '+ago(last.t)+(last.t<t0&&last.t>=dayStart(1)?' alle '+clock(last.t):''):'Ultima: orario non disponibile'):'Nessuna visita ancora'),
      h('div',{class:'bars'},days[i].map((n,k)=>h('div',{class:'bar'+(k===6?' today':''),title:n+' visite'},h('i',{style:'height:'+Math.round(n/maxDay*100)+'%'}),letters[(wd+1+k)%7])))))});
  const unk=vis.filter(v=>v.cat<0&&(!v.t||v.t>=t0)).length;
  if(cats.length&&unk)box.append(h('div',{class:'card wide'},avatar(null,'sm'),h('div',{},h('b',{},unk===1?'1 visita non riconosciuta':unk+' visite non riconosciute'),h('p',{class:'muted small'},'Toccala nell’elenco per dirmi chi era.'))));
  // bin
  $('bin').hidden=false;const lvl=pct>=1?'var(--bad)':pct>=.8?'var(--warn)':'var(--ok)';
  $('ringFill').style.stroke=lvl;$('ringFill').style.strokeDashoffset=314.16*(1-Math.min(pct,1));
  $('ringPct').textContent=Math.round(pct*100)+'%';$('binG').textContent=grams(S.bin.g);$('binOf').textContent='di '+grams(S.config.bin_limit_g);
  const left=Math.max(0,Math.ceil((S.config.bin_limit_g-S.bin.g)/S.config.grams_per_visit));
  $('binSub').textContent=[S.bin.visits+(S.bin.visits===1?' visita finora':' visite finora'),S.bin.since?'sacchetto cambiato '+ago(S.bin.since):null,pct<1?'ancora circa '+left+(left===1?' visita':' visite'):null].filter(Boolean).join(' · ');
  // visits
  $('visitsCard').hidden=false;const list=$('visits');list.replaceChildren();$('visitsHint').textContent=vis.length?'tocca per correggere':'';
  if(!vis.length)list.append(h('p',{class:'empty'},'Qui compariranno le visite dei tuoi gatti.'));
  let day='';for(const v of vis.slice(0,showAll?64:10)){const d=dayLabel(v.t);if(d!==day){day=d;list.append(h('p',{class:'day'},d))}
    const c=v.cat>=0?cats[v.cat]:null;
    list.append(h('button',{class:'visit',onclick:()=>openVisit(v)},avatar(c?c.color:null,'sm'),
      h('div',{class:'who'},h('b',{},c?c.name:'Non riconosciuto'),h('span',{},[v.g?kg(v.g):null,v.s?dur(v.s):null].filter(Boolean).join(' · ')||'nessun dato')),
      v.t?h('time',{},clock(v.t)):null))}
  if(vis.length>10)list.append(h('button',{class:'btn ghost block',style:'padding:10px',onclick:()=>{showAll=!showAll;render()}},showAll?'Mostra meno':'Mostra tutte ('+vis.length+')'));
  $('cleanWrap').hidden=false;$('bag').disabled=$('litter').disabled=busy;
  const when=(t,word,never)=>!t?never:ago(t)==='adesso'?'proprio adesso':word+' '+ago(t);
  $('bagSub').textContent=when(S.bin.since,'ultimo','mai registrato');$('litterSub').textContent=when(S.litter_at,'ultima','mai registrata');
  $('clean').disabled=busy||!m.ready||!!m.presence||!!m.fault||!!m.lock||m.pending;
  $('foot').replaceChildren('Firmware '+S.firmware+' · ',h('a',{href:'/dev'},'Pagina sviluppatore'));drawTrend();
}
const dayText=d=>new Date(d*864e5).toLocaleDateString('it-IT',{day:'numeric',month:'short'});
const signed=v=>(v>0?'+':v<0?'\u2212':'')+num(Math.abs(v),1)+' kg';
function drawTrend(){
  const cats=S.config.cats,card=$('trend');card.hidden=!cats.length;if(!cats.length)return;
  const today=Math.floor(Date.now()/864e5),from=today-range+1;
  const series=cats.map(c=>({name:c.name,color:COLORS[c.color%6],pts:(c.days||[]).map((d,k)=>[d,c.grams[k]/1000]).filter(p=>p[0]>=from&&p[0]<=today)}));
  $('legend').replaceChildren(...series.map(s=>{const a=s.pts[0],b=s.pts[s.pts.length-1];
    return h('div',{style:'--c:'+s.color},h('i'),s.name,h('span',{},b?num(b[1],1)+' kg'+(s.pts.length>1?' · '+signed(b[1]-a[1])+' in '+(b[0]-a[0])+' giorni':''):'nessun dato'))}));
  const plot=$('plot'),all=series.flatMap(s=>s.pts.map(p=>p[1]));
  $('trendTable').hidden=!all.length;
  if(!all.length){plot.onpointermove=null;plot.replaceChildren(h('p',{class:'empty'},'Il grafico comparirà dopo le prime visite riconosciute.'));return}
  const W=plot.clientWidth||320,H=190,L=30,R=40,T=10,B=24;
  let lo=Math.min(...all)-.15,hi=Math.max(...all)+.15;const step=hi-lo<=1?.2:hi-lo<=2.5?.5:1;
  lo=Math.floor(lo/step+1e-9)*step;hi=Math.ceil(hi/step-1e-9)*step;
  const x=d=>L+(d-from)/(range-1)*(W-L-R),y=v=>T+(hi-v)/(hi-lo)*(H-T-B),f=n=>n.toFixed(1);
  let g='';
  for(let v=lo;v<=hi+1e-9;v+=step)g+=`<line class="grid" x1="${L}" x2="${W-R}" y1="${f(y(v))}" y2="${f(y(v))}"/><text x="${L-6}" y="${f(y(v)+4)}" text-anchor="end">${num(v,1)}</text>`;
  [[from,'start'],[from+Math.floor(range/2),'middle'],[today,'end']].forEach(([d,a])=>g+=`<text x="${f(x(d))}" y="${H-5}" text-anchor="${a}">${d===today?'oggi':dayText(d)}</text>`);
  g+=`<line class="cross" id="cross" y1="${T}" y2="${H-B}" visibility="hidden"/>`;
  // End labels carry the latest value; nudged apart when two lines finish close together.
  const ends=series.filter(s=>s.pts.length).map(s=>({s,y:y(s.pts[s.pts.length-1][1])})).sort((a,b)=>a.y-b.y);
  for(let i=1;i<ends.length;i++)if(ends[i].y-ends[i-1].y<13)ends[i].y=ends[i-1].y+13;
  for(const s of series){if(!s.pts.length)continue;
    g+=`<path stroke="${s.color}" d="${s.pts.map((p,i)=>(i?'L':'M')+f(x(p[0]))+' '+f(y(p[1]))).join('')}"/>`;
    const dots=s.pts.length<=14?s.pts:[s.pts[s.pts.length-1]];
    for(const p of dots)g+=`<circle cx="${f(x(p[0]))}" cy="${f(y(p[1]))}" r="4" fill="${s.color}"/>`}
  for(const e of ends){const p=e.s.pts[e.s.pts.length-1];g+=`<text class="end" x="${f(x(p[0])+9)}" y="${f(e.y+4)}">${num(p[1],1)}</text>`}
  plot.innerHTML=`<svg width="${W}" height="${H}" viewBox="0 0 ${W} ${H}" role="img" aria-label="Andamento del peso dei gatti in chilogrammi">${g}</svg>`;
  const tip=h('div',{class:'tip',hidden:''});plot.append(tip);const cross=plot.querySelector('#cross');
  const daysWithData=[...new Set(series.flatMap(s=>s.pts.map(p=>p[0])))].sort((a,b)=>a-b);
  plot.onpointermove=e=>{const px=e.clientX-plot.getBoundingClientRect().left,want=from+(px-L)/(W-L-R)*(range-1);
    const d=daysWithData.reduce((a,b)=>Math.abs(b-want)<Math.abs(a-want)?b:a);
    cross.setAttribute('x1',f(x(d)));cross.setAttribute('x2',f(x(d)));cross.setAttribute('visibility','visible');
    tip.replaceChildren(h('b',{},d===today?'Oggi':dayText(d)),...series.map(s=>{const p=s.pts.find(p=>p[0]===d);return p?h('div',{style:'--c:'+s.color},h('i'),s.name+' '+num(p[1],1)+' kg'):null}).filter(Boolean));
    tip.hidden=false;const tw=tip.offsetWidth;tip.style.left=Math.max(0,Math.min(W-tw,x(d)+(x(d)>W/2?-tw-10:10)))+'px'};
  plot.onpointerleave=()=>{tip.hidden=true;cross.setAttribute('visibility','hidden')};
  $('trendRows').replaceChildren(h('table',{},h('tr',{},h('th',{},'Giorno'),series.map(s=>h('th',{},s.name))),
    daysWithData.slice().reverse().map(d=>h('tr',{},h('td',{},dayText(d)),series.map(s=>{const p=s.pts.find(p=>p[0]===d);return h('td',{},p?num(p[1],1)+' kg':'—')})))));
}
document.querySelectorAll('[data-range]').forEach(b=>b.onclick=()=>{range=+b.dataset.range;document.querySelectorAll('[data-range]').forEach(o=>o.setAttribute('aria-pressed',String(o===b)));drawTrend()});
addEventListener('resize',()=>{if(S)drawTrend()});
async function refresh(){
  try{const r=await fetch('/api/home',{cache:'no-store'});if(!r.ok)throw 0;const s=await r.json();token=s.token;S=s;
    const key=JSON.stringify([s.mcu,s.config,s.bin,s.visits,Math.floor(Date.now()/60000)]);if(key!==lastKey){lastKey=key;render()}}
  catch(e){lastKey='';setStatus('','Non raggiungibile');$('clean').disabled=true}
}
function openVisit(v){
  const cats=S.config.cats;$('visitInfo').textContent=[v.t?dayLabel(v.t)+' alle '+clock(v.t):null,v.g?kg(v.g):null,v.s?dur(v.s):null].filter(Boolean).join(' · ')||'Visita senza dati';
  const set=async cat=>{$('dlgVisit').close();const r=await api('/api/visit',{id:v.id,cat});toast(r.message);refresh()};
  $('visitChoices').replaceChildren(...cats.map((c,i)=>h('button',{class:'choice',onclick:()=>set(i)},avatar(c.color,'sm'),c.name)),
    h('button',{class:'choice',onclick:()=>set(-1)},avatar(null,'sm'),'Non lo so'),
    h('button',{class:'btn danger',onclick:()=>set('delete')},'Non era una visita: elimina'));
  $('dlgVisit').showModal();
}
function drawEditors(){
  const box=$('catEditors');box.replaceChildren();
  draft.cats.forEach((c,i)=>{
    box.append(h('div',{class:'cat-edit'},
      h('div',{class:'row'},avatar(c.color,'sm'),
        h('div',{class:'field grow'},h('input',{value:c.name,placeholder:'Nome',maxlength:16,required:'',oninput:e=>c.name=e.target.value})),
        h('div',{class:'field kg'},h('input',{type:'number',value:c.kg,min:.5,max:20,step:.1,required:'',inputmode:'decimal',placeholder:'4,0',oninput:e=>{c.kg=e.target.value;hint()}}),h('span',{},'kg'))),
      h('div',{class:'swatches'},COLORS.map((col,k)=>h('button',{type:'button',class:'swatch',style:'--c:'+col,'aria-pressed':String(c.color===k),'aria-label':'Colore '+(k+1),onclick:()=>{c.color=k;drawEditors()}})),
        h('button',{type:'button',class:'btn danger',onclick:()=>{draft.cats.splice(i,1);drawEditors()}},'Rimuovi'))))});
  $('addCat').hidden=draft.cats.length>=4;hint();
}
function hint(){const w=draft.cats.map(c=>parseFloat(c.kg)).filter(x=>x>0);let close=false;for(let i=0;i<w.length;i++)for(let j=i+1;j<w.length;j++)if(Math.abs(w[i]-w[j])<.3)close=true;$('closeHint').hidden=!close}
function openSettings(){
  if(!S)return;const c=S.config;draft={cats:c.cats.map((x,i)=>({name:x.name,kg:(x.weight_g/1000).toFixed(1),shown:(x.weight_g/1000).toFixed(1),grams:x.weight_g,color:x.color,from:i}))};
  if(!draft.cats.length)draft.cats.push({name:'',kg:'',color:0},{name:'',kg:'',color:1});
  $('optLimit').value=c.bin_limit_g;$('optGrams').value=c.grams_per_visit;$('optTol').value=(c.tolerance_g/1000).toFixed(1);
  const m=S.mcu,on=!!m.ready;$('mcuNote').hidden=on;
  $('mAuto').checked=!!m.auto;$('mAuto').disabled=!on||m.auto==null;
  $('mWait').value=m.wait_min??'';$('mWait').disabled=!on||m.wait_min==null;
  $('mOdor').checked=!!m.odor;$('mOdor').disabled=!on||m.odor==null;
  drawEditors();$('dlgSettings').showModal();
}
$('gear').onclick=openSettings;
$('addCat').onclick=()=>{draft.cats.push({name:'',kg:'',color:draft.cats.length%6});drawEditors()};
$('formSettings').onsubmit=async e=>{e.preventDefault();
  const r=await api('/api/config',{cats:draft.cats.map(c=>({name:c.name.trim(),weight_g:c.kg===c.shown?c.grams:Math.round(parseFloat(c.kg)*1000),color:c.color,from:c.from??-1})),
    bin_limit_g:+$('optLimit').value,grams_per_visit:+$('optGrams').value,tolerance_g:Math.round(parseFloat($('optTol').value)*1000)},true);
  toast(r.message);if(!r.ok)return;
  // Settings kept by the litter box itself: send only what changed.
  const m=S.mcu,w=[],auto=$('mAuto'),wait=$('mWait'),odor=$('mOdor');
  if(!auto.disabled&&auto.checked!==!!m.auto)w.push({path:'/api/command',data:{action:auto.checked?'auto_on':'auto_off'}});
  if(!wait.disabled&&wait.value!==''&&+wait.value!==m.wait_min)w.push({path:'/api/value',data:{dp:117,value:+wait.value}});
  if(!odor.disabled&&odor.checked!==!!m.odor)w.push({path:'/api/command',data:{action:odor.checked?'odor_on':'odor_off'}});
  $('dlgSettings').close();
  if(w.length){toast('Invio le impostazioni alla lettiera…');const sent=await mcuWrites(w);await sleep(1500);await refresh();
    const s=S.mcu,kept=(auto.disabled||auto.checked===!!s.auto)&&(wait.disabled||wait.value===''||+wait.value===s.wait_min)&&(odor.disabled||odor.checked===!!s.odor);
    toast(!sent.ok?sent.message:kept?'La lettiera ha confermato le nuove impostazioni':'La lettiera non ha confermato: riapri le impostazioni per controllare')}
  else refresh()};
// Both are recorded here even with the litter box offline; the MCU is told only when it is ready.
async function maintenance(path,action,done,title,text,ok){
  const on=S.mcu.ready&&!S.mcu.presence&&!S.mcu.fault&&!S.mcu.lock;
  if(!await confirmBox(title,text+(on?' Il tamburo potrebbe muoversi: controlla che nessun gatto sia dentro.':' La lettiera non è pronta, quindi per ora lo registro soltanto.'),ok))return;
  busy=true;$('bag').disabled=$('litter').disabled=$('clean').disabled=true;
  const r=await api(path);
  if(r.ok&&on){const w=await mcuWrites([{path:'/api/command',data:{action}}]);toast(w.ok?done:'Registrato, ma la lettiera ha rifiutato: '+w.message)}else toast(r.message);
  busy=false;lastKey='';refresh();
}
$('bag').onclick=()=>maintenance('/api/bin/reset','bag','Stima azzerata e lettiera avvisata','Cambio sacchetto','Azzero la stima del cassetto e avviso la lettiera che il sacchetto è nuovo.','Sacchetto cambiato');
$('litter').onclick=()=>maintenance('/api/litter','level','Registrato: la lettiera livella la sabbia','Aggiunta lettiera','Registro la data e chiedo alla lettiera di livellare la sabbia nuova.','Lettiera aggiunta');
$('clean').onclick=async()=>{
  if(!await confirmBox('Avvio la pulizia?','Il tamburo ruoterà. Controlla che nessun gatto sia dentro o stia entrando.','Pulisci ora'))return;
  busy=true;$('clean').disabled=true;
  const r=await mcuWrites([{path:'/api/command',data:{action:'clean'}}]);toast(r.ok?'Pulizia richiesta alla lettiera':r.message);
  busy=false;lastKey='';refresh();
};
refresh();setInterval(refresh,5000);document.addEventListener('visibilitychange',()=>{if(!document.hidden)refresh()});
</script></body></html>)HTML";
