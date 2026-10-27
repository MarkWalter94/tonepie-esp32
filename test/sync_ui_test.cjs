// Run with Node.js. Exercise the real Send now handler without a browser.
const {readFileSync} = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const source = readFileSync(require('node:path').join(__dirname, '../include/web_home.h'), 'utf8');
const script = source.match(/<script>([\s\S]*?)<\/script>/)[1];
new vm.Script(script); // syntax of the entire shipped page
const handler = script.slice(script.indexOf('let syncNowBusy=false;'), script.indexOf('async function drawHist(){'));

async function scenario(states, expected, requestId=8) {
  const messages = [], button = {}, dialog = {open:false};
  let reads=0;
  const context = vm.createContext({
    $: id => id==='syncNow'?button:dialog,
    S:{sync:{try_at:123,error:'old connection failure',result_id:7}},
    api:async()=>({ok:true,request_id:requestId}),
    sleep:async()=>{}, toast:m=>messages.push(m),
    t:(key,arg)=>arg===undefined?key:`${key}:${arg}`,
    refresh:async()=>{reads++;return states[Math.min(reads-1,states.length-1)]},
    histFail:{90:true},drawHist:{sig:'old'},lastKey:'old',drawSync:()=>{}
  });
  vm.runInContext(handler,context);
  await button.onclick();
  assert.equal(messages.at(-1),expected);
  assert.equal(vm.runInContext('syncNowBusy',context),false);
  return reads;
}

(async()=>{
  // Same timestamp for old/new requests; an old error and a lost status response
  // must not win over the eventual result of the acknowledged request.
  assert.equal(await scenario([
    {sync:{try_at:123,result_id:7,error:'old connection failure'}},null,
    {sync:{try_at:123,result_id:8,error:''}}
  ],'forcedOk'),4); // three status reads, then the final page refresh
  await scenario([{sync:{result_id:8,error:'wrong API key'}}],'forcedErr:wrong API key');
  await scenario([null],'forcedUnknown');
  await scenario([{sync:{result_id:7,error:'old connection failure'}}],'forcedUnknown');
  assert.equal(await scenario([
    {sync:{result_id:8,error:'',pending:true}},
    {sync:{result_id:9,error:'',pending:true}},
    {sync:{result_id:10,error:'',pending:false}}
  ],'forcedOk'),4); // success means the entire local backlog, not just its first batch

  // The API helper must preserve the server's request id for the handler.
  const helper=script.slice(script.indexOf('async function api('),script.indexOf('const sleep='));
  const context=vm.createContext({token:'test',LANG:'en',URLSearchParams,
    fetch:async()=>({ok:true,status:202,json:async()=>({request_id:8,message:'Sending'})})});
  vm.runInContext(helper,context);
  assert.equal((await vm.runInContext("api('/api/sync/now')",context)).request_id,8);
  console.log('Sync UI tests passed: request correlation, stale/lost status, failure, API response; full page syntax OK.');
})().catch(e=>{console.error(e);process.exitCode=1});
