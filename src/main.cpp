#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_system.h>
#include <time.h>
#include "config.h"
#include "tuya_protocol.h"
#include "litter_logic.h"
#include "web_ui.h"
#include "web_home.h"
#include "icons.h"

// UART0/Serial uses the onboard CH340; UART1 on GPIO6/7 serves Tonepie.
HardwareSerial mcu(1);
WebServer server(80);
// Pages send "X-Tonepie-Lang: en" to get English messages; Italian otherwise.
bool english(){ return server.header("X-Tonepie-Lang")=="en"; }
struct Datapoint {
  bool used=false;
  uint8_t id=0,type=0;
  uint16_t len=0;
  uint8_t data[128]{};
  uint32_t seen=0;
};
Datapoint dps[64];
String logs[60];
size_t logHead=0,logCount=0;
uint32_t lastHeartbeat=0,lastHbTx=0,lastInitTx=0,lastNetTx=0,lastWifiTry=0;
uint32_t armSince=0,lastWrite=0,queryDue=0,badDp=0;
bool heard=false,armed=false,mdns=false,wasWifi=false,queryScheduled=false,wrote=false;
enum class Init { Heartbeat, Product, Mode, Ready };
Init initState=Init::Heartbeat;
uint8_t networkReported=255;
bool networkAck=false;
String token,commandIt="Nessun comando inviato",commandEn="No command sent";
struct Pending { bool active=false; uint8_t id=0,type=0; uint32_t value=0,since=0; } pending;
// Home page data: cats, visit history and visits since the last bag change, kept in NVS.
Preferences prefs;
Litter::Settings cats;
Litter::History history;
Litter::Tracker tracker;
Litter::WeightLog weights,weightScratch;
struct Bin { uint32_t reserved=0,since=0,visits=0; } bin; // layout kept for the data saved by 1.x
uint32_t litterAt=0; // when litter was last topped up
bool kgSeen=false,clockStarted=false,otaAllowed=false,otaOk=false,apOn=false;
String wifiSsid,wifiPassword; // NVS values set from /dev override the ones in config.h
uint32_t wifiSeen=0;
// History server (optional): the whole current state is posted to <url>/api/ingest whenever it changes.
// The POST runs in its own task so a slow or absent server never delays the MCU.
bool syncOn=false,syncDirty=true;
String syncUrl,syncKey,deviceId,syncError;
uint32_t syncLastTry=0,syncOkAt=0,syncFails=0;
struct DeletedRef { uint16_t id; uint32_t epoch; };
DeletedRef deletedRefs[16]; // visits deleted here, reported until the server has seen them
uint8_t deletedCount=0,deletedSent=0;
TaskHandle_t syncTask=nullptr;
volatile bool syncBusy=false,syncDone=false;
volatile int syncCode=0;
String syncBody,syncTarget,syncAuth; // owned by the task while syncBusy

bool online() { return heard && uint32_t(millis()-lastHeartbeat)<Config::LINK_TIMEOUT_MS; }
bool isArmed() { return armed && uint32_t(millis()-armSince)<Config::ARM_MS; }
void logLine(const String& message) {
  String line=String(millis())+" ms | "+message;
  if(line.length()>380) line=line.substring(0,380);
  logs[logHead]=line; logHead=(logHead+1)%60; if(logCount<60)++logCount;
  // USB backpressure must never stall MCU service.
  if(Serial && Serial.availableForWrite()>=int(line.length()+2)) Serial.println(line);
}
String hex(const uint8_t* p,size_t n,size_t limit=64) {
  String s; s.reserve(min(n,limit)*3+4);
  for(size_t i=0;i<n && i<limit;i++){char b[4];snprintf(b,sizeof(b),"%02X ",p[i]);s+=b;}
  if(n>limit)s+="...";
  return s;
}
void sendFrame(uint8_t cmd,const uint8_t* p=nullptr,size_t n=0) {
  uint8_t frame[Tuya::MAX_PAYLOAD+7];
  size_t count=Tuya::encode(cmd,p,n,frame,sizeof(frame));
  if(!count)return;
  mcu.write(frame,count);
  if(cmd==0x08)tracker.onQuerySent(millis());
  logLine("TX "+hex(frame,count));
}
bool clockSynced() { return time(nullptr)>1700000000; }
uint32_t epochNow() { return clockSynced()?uint32_t(time(nullptr)):0; }
void saveCats() { prefs.putBytes("cats",&cats,sizeof(cats));syncDirty=true; }
void saveVisits() { prefs.putBytes("visits",&history,sizeof(history));prefs.putBytes("bin",&bin,sizeof(bin));syncDirty=true; }
void loadStore() {
  prefs.begin("litter",false);
  Litter::defaults(cats);memset(&history,0,sizeof(history));
  if(prefs.getBytesLength("cats")==sizeof(cats))prefs.getBytes("cats",&cats,sizeof(cats));
  if(prefs.getBytesLength("visits")==sizeof(history))prefs.getBytes("visits",&history,sizeof(history));
  if(prefs.getBytesLength("bin")==sizeof(bin))prefs.getBytes("bin",&bin,sizeof(bin));
  memset(&weights,0,sizeof(weights));
  if(prefs.getBytesLength("weights")==sizeof(weights))prefs.getBytes("weights",&weights,sizeof(weights));
  Litter::sanitize(cats);Litter::sanitize(history);Litter::sanitize(weights);
  tracker.restore(prefs.isKey("count"),prefs.getUInt("count",0));
  litterAt=prefs.getUInt("litterAt",0);
  wifiSsid=prefs.getString("ssid",Config::WIFI_SSID);wifiPassword=prefs.getString("wpass",Config::WIFI_PASSWORD);
  syncOn=prefs.getBool("syncOn",false);syncUrl=prefs.getString("syncUrl","");syncKey=prefs.getString("syncKey","");
}
// A recognised weight feeds the trend chart and moves the cat's reference weight.
void learnWeight(int8_t cat,uint16_t grams,uint32_t epoch) {
  if(cat<0 || !grams)return;
  cats.cats[cat].weightG=Litter::learn(cats.cats[cat].weightG,grams);saveCats();
  if(epoch){Litter::addWeight(weights,cat,epoch,grams);prefs.putBytes("weights",&weights,sizeof(weights));}
}
// The recovery network always uses the password from config.h, never one typed later by mistake.
const char* wifiPasswordForAp() { return Config::WIFI_PASSWORD; }
void recordVisit(uint16_t weightG,uint16_t durationS) {
  Litter::Visit v{};v.epoch=epochNow();v.weightG=weightG;v.durationS=durationS;
  v.cat=Litter::matchCat(cats,weightG);
  Litter::add(history,v);bin.visits++;learnWeight(v.cat,weightG,v.epoch);
  logLine("Visita registrata: "+String(weightG)+" g, "+String(durationS)+" s, "+(v.cat<0?String("gatto non riconosciuto"):String(cats.cats[v.cat].name)));
}
// Community mapping, not verified on this unit: DP7 visits (per day, resets), DP8 duration (s),
// DP6 cat weight: grams on some Tonepie models (600-10000), kg x10 on others (6-300);
// DP134 weight in lb x10 (used only if DP6 never shows up).
void feedTracker(uint8_t id,uint32_t value) {
  uint32_t now=millis();
  if(id==7)tracker.onCount(value,now);
  else if(id==8 && value && value<=65535)tracker.onDuration(uint16_t(value),now);
  else if(id==6 && value>=5 && value<=300){kgSeen=true;tracker.onWeight(uint16_t(value*100),now);}
  else if(id==6 && value>=500 && value<=30000){kgSeen=true;tracker.onWeight(uint16_t(value),now);}
  else if(id==134 && !kgSeen && value>=10 && value<=660)tracker.onWeight(uint16_t(value*4536/100),now);
}
void serviceTracker(uint32_t now) {
  if(!clockSynced() && now<30000)return; // let NTP answer first, so visits found at boot get a time
  Litter::Event e;bool changed=false;
  while(tracker.poll(now,e)){
    changed=true;
    if(e.kind==Litter::Event::New){
      for(uint16_t i=1;i<e.n && i<=10;i++)recordVisit(0,0); // missed while offline: nothing known about them
      recordVisit(e.weightG,e.durationS);
    }else if(history.count){
      Litter::Visit& v=history.v[history.count-1];
      if(e.kind==Litter::Event::PatchDuration)v.durationS=e.durationS;
      else{v.weightG=e.weightG;if(!(v.flags&Litter::VISIT_MANUAL)){v.cat=Litter::matchCat(cats,e.weightG);learnWeight(v.cat,e.weightG,v.epoch);}}
    }
  }
  if(changed)saveVisits();
  if(tracker.countDirty){tracker.countDirty=false;prefs.putUInt("count",tracker.count);}
}
void sendBoolDP(uint8_t id,bool value) {
  uint8_t p[]={id,1,0,1,uint8_t(value)};sendFrame(0x06,p,sizeof(p));
}
void sendValueDP(uint8_t id,uint32_t value) {
  uint8_t p[]={id,2,0,4,uint8_t(value>>24),uint8_t(value>>16),uint8_t(value>>8),uint8_t(value)};
  sendFrame(0x06,p,sizeof(p));
}
Datapoint* findDp(uint8_t id) {
  for(auto& d:dps)if(d.used && d.id==id)return &d;
  return nullptr;
}
const char* labelEn(uint8_t id) {
  switch(id){
    case 22:return "Fault (bits to verify)";case 24:return "State (index to verify)";
    case 101:return "Manual clean";case 102:return "Empty";
    case 104:return "Presence (to verify)";case 105:return "Auto clean";
    case 114:return "Child lock (to verify)";case 126:return "Level litter (to verify)";case 127:return "Bag change (to verify)";case 129:return "Deodorise after clean (to verify)";
    case 117:return "Wait before cleaning (min)";case 118:return "Cleaning interval (min)";
    default:return "To verify / read only";
  }
}
const char* label(uint8_t id) {
  if(english())return labelEn(id);
  switch(id){
    case 22:return "Fault (bit da verificare)";case 24:return "Stato (indice da verificare)";
    case 101:return "Pulizia manuale";case 102:return "Svuotamento";
    case 104:return "Presenza (da verificare)";case 105:return "Auto-clean";
    case 114:return "Blocco bambini (da verificare)";case 126:return "Livella lettiera (da verificare)";case 127:return "Cambio sacchetto (da verificare)";case 129:return "Deodorante dopo pulizia (da verificare)";
    case 117:return "Attesa pulizia (min)";case 118:return "Intervallo pulizia (min)";
    default:return "Da verificare / sola lettura";
  }
}
const char* typeName(uint8_t type) {
  const char* names[]={"RAW","BOOL","VALUE","STRING","ENUM","BITMAP"};
  return type<6?names[type]:"?";
}
uint32_t number(const Datapoint& d) {
  uint32_t n=0;for(size_t i=0;i<d.len && i<4;++i)n=(n<<8)|d.data[i];return n;
}
String display(const Datapoint& d) {
  if(d.type==1)return d.data[0]?"true":"false";
  if(d.type==2)return String(int32_t(Tuya::read32(d.data)));
  if(d.type==4 || d.type==5)return String(number(d));
  // Strings are escaped by JSON and inserted as textContent in the browser.
  if(d.type==3){String s;for(size_t i=0;i<d.len && i<128;i++)s+=d.data[i]>=32&&d.data[i]<127?char(d.data[i]):'.';if(d.len>128)s+=" [truncated]";return s;}
  return hex(d.data,min(size_t(d.len),size_t(128)))+(d.len>128?" [truncated]":"");
}
void invalidate() {
  for(auto& d:dps)d.used=false;
  armed=false;networkAck=false;networkReported=255;queryScheduled=false;
  if(pending.active){pending.active=false;commandIt="MCU scollegata/riavviata: esito sconosciuto, non ripetere automaticamente";commandEn="MCU disconnected/restarted: result unknown, do not repeat automatically";}
}
void reportDps(const uint8_t* p,size_t n) {
  if(!Tuya::validDps(p,n)){++badDp;logLine("Report DP malformato: intero report ignorato");return;}
  for(size_t pos=0;pos<n;){
    uint8_t id=p[pos],type=p[pos+1];size_t len=(size_t(p[pos+2])<<8)|p[pos+3];
    Datapoint* d=findDp(id);
    if(!d)for(auto& candidate:dps)if(!candidate.used){d=&candidate;break;}
    if(d){d->used=true;d->id=id;d->type=type;d->len=len;d->seen=millis();memcpy(d->data,p+pos+4,min(len,sizeof(d->data)));
      if(pending.active && pending.id==id && pending.type==type && number(*d)==pending.value){
        pending.active=false;commandIt="DP "+String(id)+" riportato con valore richiesto; azione fisica non verificata";commandEn="DP "+String(id)+" reported with the requested value; physical action not verified";logLine(commandIt);
      }
      if(type==2)feedTracker(id,Tuya::read32(d->data));
    }else logLine("Cache DP piena: report non memorizzato");
    pos+=4+len;
  }
}
void onFrame(uint8_t version,uint8_t cmd,const uint8_t* p,size_t n) {
  char prefix[45];snprintf(prefix,sizeof(prefix),"RX v%02X cmd%02X len%u ",version,cmd,unsigned(n));logLine(String(prefix)+hex(p,n));
  // Standard MCU reports version 3. Reject module echoes and unknown dialects.
  if(version!=3){logLine("Versione MCU inattesa: ignorata (attesa 03)");return;}
  switch(cmd){
    case 0x00:
      if(n!=1 || p[0]>1)return;
      if(p[0]==0 || !online() || initState==Init::Heartbeat){
        invalidate();initState=Init::Product;lastInitTx=millis();sendFrame(0x01);
      }
      heard=true;lastHeartbeat=millis();break;
    case 0x01:
      if(initState==Init::Product && n){initState=Init::Mode;lastInitTx=millis();sendFrame(0x02);}break;
    case 0x02:
      if(initState==Init::Mode && (n==0 || n==2)){
        initState=Init::Ready;networkAck=false;
        if(n==2)logLine("Modo autonomo MCU: pin LED/reset ricevuti ma NON pilotati");
        // Network status is sent by loop(), then query follows it.
      }break;
    case 0x03:if(n==0){networkAck=true;queryScheduled=true;queryDue=millis()+100;}break;
    case 0x04:case 0x05:
      // Acknowledge physical pairing requests; credentials stay fixed.
      if((cmd==4 && n==0)||(cmd==5 && n==1)){sendFrame(cmd);armed=false;logLine("Richiesta pairing MCU: credenziali fisse, invii disabilitati");}break;
    case 0x07:reportDps(p,n);break;
    case 0x1c:{
      // Local time request: flag, year-2000, month, day, hour, minute, second, weekday (1 = Monday).
      uint8_t answer[8]{};time_t t=time(nullptr);struct tm local;
      if(clockSynced() && localtime_r(&t,&local)){
        answer[0]=1;answer[1]=uint8_t(local.tm_year-100);answer[2]=uint8_t(local.tm_mon+1);answer[3]=uint8_t(local.tm_mday);
        answer[4]=uint8_t(local.tm_hour);answer[5]=uint8_t(local.tm_min);answer[6]=uint8_t(local.tm_sec);answer[7]=uint8_t(local.tm_wday?local.tm_wday:7);
      }
      sendFrame(cmd,answer,sizeof(answer));break;}
    default:break; // No motor control, firmware update, reset or sensor overrides.
  }
}
Tuya::Parser parser(onFrame);

void reply(int code,const String& it,const String& en){
  StaticJsonDocument<512> doc;doc["message"]=english()?en:it;String body;serializeJson(doc,body);server.send(code,"application/json",body);
}
bool allowedHost(){
  String host=server.hostHeader();host.toLowerCase();
  int colon=host.indexOf(':');if(colon>=0)host=host.substring(0,colon);
  if(apOn && host==WiFi.softAPIP().toString())return true;
  return host==WiFi.localIP().toString() || host=="tonepie.local" || host=="tonepie" || host=="tonepie.fritz.box";
}
bool protect(){
  if(!allowedHost()){reply(403,"Host non ammesso","Host not allowed");return false;}
  if(server.header("X-Tonepie-Token")!=token){reply(403,"Ricarica la dashboard prima di inviare","Reload the page before sending");return false;}
  return true;
}
bool canWrite(uint8_t id,uint8_t type){
  if(!isArmed()){reply(409,"Abilita gli invii dalla dashboard","Enable sending from the dashboard first");return false;}
  if(!online() || initState!=Init::Ready){reply(409,"MCU non pronta","MCU not ready");return false;}
  if(pending.active || (wrote && uint32_t(millis()-lastWrite)<2000)){reply(429,"Attendi l'esito del comando precedente","Wait for the previous command's result");return false;}
  auto d=findDp(id);
  // DP126 (level litter) is a push button this MCU never reports: it cannot be checked against a report.
  if(id==126 && !d)return true;
  if(!d || d->type!=type || uint32_t(millis()-d->seen)>Config::DP_FRESH_MS){reply(409,"DP assente, obsoleto o tipo inatteso: esegui query e verifica la mappatura","DP missing, stale or of unexpected type: run a query and check the mapping");return false;}
  return true;
}
void writeDp(uint8_t id,uint8_t type,uint32_t value){
  if(!canWrite(id,type))return;
  // Best-effort additional vetoes only. Original MCU remains authoritative.
  if((id==101||id==102||id==126||id==127) && value){ // anything that may move the drum
    auto presence=findDp(104);auto fault=findDp(22);auto lock=findDp(114);
    if((presence && presence->type==1 && number(*presence)) ||
       (fault && fault->type==5 && number(*fault)) ||
       (lock && lock->type==1 && number(*lock))){reply(409,"Presenza, fault o blocco segnalato: comando bloccato","Presence, fault or lock reported: command blocked");return;}
  }
  pending.active=true;pending.id=id;pending.type=type;pending.value=value;pending.since=millis();
  lastWrite=millis();wrote=true;
  if(type==1)sendBoolDP(id,value!=0);else sendValueDP(id,value);
  commandIt="DP "+String(id)+" trasmesso: attesa report, esecuzione non confermata";
  commandEn="DP "+String(id)+" sent: waiting for the report, execution not confirmed";
  queryDue=millis()+500;queryScheduled=true;reply(202,commandIt,commandEn);
}
bool parseUnsigned(const String& s,uint32_t& value){
  if(s.isEmpty() || s.length()>10)return false;uint64_t n=0;
  for(size_t i=0;i<s.length();i++){if(s[i]<'0'||s[i]>'9')return false;n=n*10+(s[i]-'0');if(n>UINT32_MAX)return false;}
  value=uint32_t(n);return true;
}
void stateApi(){
  if(!allowedHost()){reply(403,"Host non ammesso","Host not allowed");return;}
  DynamicJsonDocument doc(45000);
  doc["firmware"]=Config::FIRMWARE_VERSION;doc["mcu_rx"]=Config::MCU_RX;doc["mcu_tx"]=Config::MCU_TX;
  doc["token"]=token;doc["ip"]=WiFi.localIP().toString();doc["online"]=online();doc["ready"]=online()&&initState==Init::Ready;
  doc["armed"]=isArmed();doc["pending"]=pending.active;doc["command"]=english()?commandEn:commandIt;
  doc["frames"]=parser.frames;doc["bad_checksum"]=parser.badChecksum;doc["bad_length"]=parser.badLength;
  doc["timeouts"]=parser.timeouts;doc["bad_dp"]=badDp;doc["heap"]=ESP.getFreeHeap();
  doc["ssid"]=wifiSsid;doc["recovery_ap"]=apOn;
  doc["uptime_s"]=millis()/1000;doc["reset_reason"]=int(esp_reset_reason());
  JsonArray array=doc.createNestedArray("dps");
  for(const auto& d:dps)if(d.used){auto j=array.createNestedObject();j["id"]=d.id;j["label"]=label(d.id);j["type"]=typeName(d.type);j["value"]=display(d);j["age_ms"]=uint32_t(millis()-d.seen);}
  JsonArray logArray=doc.createNestedArray("logs");
  for(size_t i=0;i<logCount;i++)logArray.add(logs[(logHead+60-logCount+i)%60]);
  if(doc.overflowed()){reply(503,"Memoria dashboard insufficiente","Not enough memory for the dashboard");return;}
  String body;serializeJson(doc,body);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",body);
}
String snapshotJson(){
  DynamicJsonDocument doc(14000);
  doc["device"]=deviceId;doc["firmware"]=Config::FIRMWARE_VERSION;doc["now"]=epochNow();
  JsonArray c=doc.createNestedArray("cats");
  for(int i=0;i<cats.catCount;i++){auto j=c.createNestedObject();j["index"]=i;j["name"]=cats.cats[i].name;j["color"]=cats.cats[i].color;j["weightG"]=cats.cats[i].weightG;}
  JsonObject b=doc.createNestedObject("bin");b["since"]=bin.since;b["visits"]=bin.visits;b["limit"]=cats.binLimitVisits;
  doc["litterAt"]=litterAt;
  JsonArray v=doc.createNestedArray("visits");
  for(int i=0;i<history.count;i++){const auto& x=history.v[i];auto j=v.createNestedObject();
    j["id"]=x.id;j["t"]=x.epoch;j["g"]=x.weightG;j["s"]=x.durationS;j["cat"]=x.cat;j["manual"]=(x.flags&Litter::VISIT_MANUAL)!=0;}
  JsonArray d=doc.createNestedArray("deleted");
  for(int i=0;i<deletedCount;i++){auto j=d.createNestedObject();j["id"]=deletedRefs[i].id;j["t"]=deletedRefs[i].epoch;}
  String out;serializeJson(doc,out);return out;
}
void syncWorker(void*){
  for(;;){
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
    int code;
    {
      WiFiClient plain;WiFiClientSecure secure;HTTPClient http;
      bool tls=syncTarget.startsWith("https://");
      if(tls)secure.setInsecure(); // home servers usually have self-signed certificates: the API key is the protection
      http.setConnectTimeout(4000);http.setTimeout(6000);
      if(tls?http.begin(secure,syncTarget):http.begin(plain,syncTarget)){
        http.addHeader("Content-Type","application/json");http.addHeader("Authorization",syncAuth);
        code=http.POST(syncBody);http.end();
      }else code=HTTPC_ERROR_CONNECTION_REFUSED;
    }
    syncBody=String();syncCode=code;syncDone=true;syncBusy=false;
  }
}
void serviceSync(uint32_t now){
  if(!syncTask || syncBusy)return;
  if(syncDone){
    syncDone=false;int code=syncCode;
    if(code>=200 && code<300){
      syncOkAt=epochNow();syncFails=0;syncError="";
      // Deletions reported in this snapshot are now on the server; newer ones stay queued.
      memmove(deletedRefs,deletedRefs+deletedSent,(deletedCount-deletedSent)*sizeof(DeletedRef));deletedCount-=deletedSent;
    }else{
      syncDirty=true;syncFails++;
      syncError=code<0?HTTPClient::errorToString(code):String("HTTP ")+code+(code==401?" (API key)":"");
      if(syncFails==1)logLine("Server storico: invio fallito, "+syncError);
    }
    deletedSent=0;
  }
  if(!syncOn || syncUrl.isEmpty() || syncKey.isEmpty() || WiFi.status()!=WL_CONNECTED || !clockSynced())return;
  // Changes go out within seconds; otherwise a heartbeat every 15 min. Failures back off up to 5 min.
  uint32_t wait=syncFails?min(300000u,15000u<<min(syncFails,uint32_t(4))):(syncDirty?3000u:900000u);
  if(syncLastTry && uint32_t(now-syncLastTry)<wait)return;
  syncLastTry=now;if(!syncLastTry)syncLastTry=1;syncDirty=false;
  syncBody=snapshotJson();deletedSent=deletedCount;
  syncTarget=syncUrl+"/api/ingest";syncAuth="Bearer "+syncKey;
  syncBusy=true;xTaskNotifyGive(syncTask);
}
// Settings of the history server from the home page. An empty key keeps the one already saved.
void syncApi(){
  if(!protect())return;
  bool on=server.arg("enabled")=="1";String url=server.arg("url"),key=server.arg("key");url.trim();key.trim();
  while(url.endsWith("/"))url.remove(url.length()-1);
  if(url.length()>120 || url.indexOf(' ')>=0 || (!url.isEmpty() && !url.startsWith("http://") && !url.startsWith("https://"))){
    reply(400,"Indirizzo non valido: deve iniziare con http:// o https://","Invalid address: it must start with http:// or https://");return;}
  if(!key.isEmpty() && (key.length()<16 || key.length()>64)){reply(400,"La chiave deve avere da 16 a 64 caratteri","The key must be 16 to 64 characters");return;}
  if(key.isEmpty())key=syncKey;
  if(on && (url.isEmpty() || key.isEmpty())){reply(400,"Per attivarlo servono indirizzo e chiave","Address and key are needed to enable it");return;}
  syncOn=on;syncUrl=url;syncKey=key;prefs.putBool("syncOn",on);prefs.putString("syncUrl",url);prefs.putString("syncKey",key);
  syncDirty=true;syncFails=0;syncLastTry=0;syncError="";syncOkAt=0;
  logLine(String("Server storico ")+(on?"attivo: "+url:"disattivato"));
  reply(200,"Impostazioni del server salvate","Server settings saved");
}
void homeApi(){
  if(!allowedHost()){reply(403,"Host non ammesso","Host not allowed");return;}
  DynamicJsonDocument doc(40000);
  doc["firmware"]=Config::FIRMWARE_VERSION;doc["token"]=token;doc["now"]=epochNow();
  JsonObject m=doc.createNestedObject("mcu");
  m["online"]=online();m["ready"]=online()&&initState==Init::Ready;m["pending"]=pending.active;
  auto presence=findDp(104);auto fault=findDp(22);auto lock=findDp(114);
  if(presence && presence->type==1)m["presence"]=number(*presence)!=0;
  if(fault && fault->type==5)m["fault"]=number(*fault);
  if(lock && lock->type==1)m["lock"]=number(*lock)!=0;
  // Settings kept by the MCU, editable from the home: absent until the MCU reports them.
  auto autoClean=findDp(105);auto wait=findDp(117);auto odor=findDp(129);
  if(autoClean && autoClean->type==1)m["auto"]=number(*autoClean)!=0;
  if(wait && wait->type==2)m["wait_min"]=number(*wait);
  if(odor && odor->type==1)m["odor"]=number(*odor)!=0;
  JsonObject c=doc.createNestedObject("config");
  c["tolerance_g"]=cats.toleranceG;c["bin_limit_visits"]=cats.binLimitVisits;
  JsonArray catArray=c.createNestedArray("cats");
  for(int i=0;i<cats.catCount;i++){auto j=catArray.createNestedObject();j["name"]=cats.cats[i].name;j["weight_g"]=cats.cats[i].weightG;j["color"]=cats.cats[i].color;
    JsonArray days=j.createNestedArray("days");JsonArray grams=j.createNestedArray("grams"); // daily averages, oldest first
    for(int k=0;k<weights.count[i];k++){days.add(weights.p[i][k].day);grams.add(weights.p[i][k].grams);}}
  doc["litter_at"]=litterAt;
  JsonObject sync=doc.createNestedObject("sync");
  sync["enabled"]=syncOn;sync["url"]=syncUrl;sync["device"]=deviceId;sync["key_set"]=!syncKey.isEmpty();
  sync["ok_at"]=syncOkAt;sync["error"]=syncError;sync["pending"]=syncOn&&(syncDirty||syncBusy);
  JsonObject b=doc.createNestedObject("bin");b["since"]=bin.since;b["visits"]=bin.visits;
  JsonArray visitArray=doc.createNestedArray("visits"); // newest first
  for(int i=int(history.count)-1;i>=0;i--){const auto& v=history.v[i];auto j=visitArray.createNestedObject();
    j["id"]=v.id;j["t"]=v.epoch;j["g"]=v.weightG;j["s"]=v.durationS;j["cat"]=v.cat;j["manual"]=(v.flags&Litter::VISIT_MANUAL)!=0;}
  if(doc.overflowed()){reply(503,"Memoria insufficiente","Not enough memory");return;}
  String body;serializeJson(doc,body);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",body);
}
// Truncates on a UTF-8 character boundary.
void copyName(char* out,size_t cap,const char* in){
  size_t n=strlen(in);
  if(n>=cap){n=cap-1;while(n && (uint8_t(in[n])&0xC0)==0x80)--n;}
  memcpy(out,in,n);out[n]=0;
}
void configApi(){
  if(!protect())return;
  StaticJsonDocument<1536> doc;
  if(deserializeJson(doc,server.arg("plain"))){reply(400,"Dati non validi","Invalid data");return;}
  JsonArray list=doc["cats"].as<JsonArray>();
  if(list.isNull() || list.size()>size_t(Litter::MAX_CATS)){reply(400,"Massimo 4 gatti","At most 4 cats");return;}
  Litter::Settings next;Litter::defaults(next);
  int8_t from[Litter::MAX_CATS];bool taken[Litter::MAX_CATS]{};
  for(JsonObject c:list){
    const char* name=c["name"]|"";uint32_t weight=c["weight_g"]|0u;
    if(!*name || weight<500 || weight>20000){reply(400,"Ogni gatto richiede un nome e un peso fra 0,5 e 20 kg","Each cat needs a name and a weight between 0.5 and 20 kg");return;}
    // "from": index this cat had before the edit, so its visits and weight history follow it.
    int previous=c["from"]|-1;
    if(previous<0 || previous>=cats.catCount || taken[previous])previous=-1;else taken[previous]=true;
    from[next.catCount]=int8_t(previous);
    Litter::Cat& cat=next.cats[next.catCount++];
    copyName(cat.name,sizeof(cat.name),name);cat.weightG=uint16_t(weight);cat.color=uint8_t((c["color"]|0u)%6);
  }
  next.toleranceG=Litter::clampU16(doc["tolerance_g"]|uint32_t(cats.toleranceG),100,3000);
  next.binLimitVisits=Litter::clampU16(doc["bin_limit_visits"]|uint32_t(cats.binLimitVisits),5,500);
  Litter::remapCats(history,weights,weightScratch,from,next.catCount);
  cats=next;Litter::rematch(cats,history);saveCats();saveVisits();prefs.putBytes("weights",&weights,sizeof(weights));
  reply(200,"Impostazioni salvate","Settings saved");
}
void visitApi(){
  if(!protect())return;
  uint32_t id;
  Litter::Visit* v=parseUnsigned(server.arg("id"),id)&&id<=65535?Litter::find(history,uint16_t(id)):nullptr;
  if(!v){reply(404,"Visita non trovata","Visit not found");return;}
  String cat=server.arg("cat");uint32_t index;
  if(cat=="delete"){
    if(deletedCount==16){memmove(deletedRefs,deletedRefs+1,15*sizeof(DeletedRef));deletedCount--;if(deletedSent)deletedSent--;}
    deletedRefs[deletedCount++]={v->id,v->epoch};
    // Only visits after the last bag change are part of the current count.
    if(!(v->epoch && bin.since && v->epoch<bin.since) && bin.visits)bin.visits--;
    Litter::remove(history,uint16_t(id));saveVisits();reply(200,"Visita eliminata","Visit deleted");return;
  }
  if(cat=="-1")v->cat=Litter::CAT_UNKNOWN;
  else if(parseUnsigned(cat,index) && index<cats.catCount){
    bool changed=v->cat!=int8_t(index);v->cat=int8_t(index);
    // A correction teaches the new weight, unless it is too far off to be a plausible reading of that cat.
    uint16_t reference=cats.cats[index].weightG;
    uint32_t distance=v->weightG>reference?v->weightG-reference:reference-v->weightG;
    if(changed && distance<=2u*cats.toleranceG)learnWeight(v->cat,v->weightG,v->epoch);
  }else{reply(400,"Gatto non valido","Invalid cat");return;}
  v->flags|=Litter::VISIT_MANUAL;saveVisits();reply(200,"Visita aggiornata","Visit updated");
}
void page(const char* html){
  if(!allowedHost()){reply(403,"Host non ammesso","Host not allowed");return;}
  server.sendHeader("X-Frame-Options","DENY");server.sendHeader("Cache-Control","no-store");server.send_P(200,"text/html; charset=utf-8",html);
}
// Icons for "Add to Home Screen": no host check, they reveal nothing.
void icon(const uint8_t* png,size_t size){
  server.sendHeader("Cache-Control","max-age=604800");server.send_P(200,"image/png",(const char*)png,size);
}
const char MANIFEST[] PROGMEM = R"J({"name":"Tonepie","short_name":"Tonepie","start_url":"/","scope":"/","display":"standalone",
"background_color":"#f4efe8","theme_color":"#f4efe8","icons":[{"src":"/icon-192.png","sizes":"192x192","type":"image/png","purpose":"any maskable"},
{"src":"/icon-512.png","sizes":"512x512","type":"image/png","purpose":"any maskable"}]})J";
void setupWeb(){
  const char* headers[]={"X-Tonepie-Token","X-Tonepie-Ota","X-Tonepie-Lang"};server.collectHeaders(headers,3);
  server.on("/",HTTP_GET,[]{page(HOME_UI);});
  server.on("/dev",HTTP_GET,[]{page(WEB_UI);}); // MCU developer page: not linked from the home
  server.on("/apple-touch-icon.png",HTTP_GET,[]{icon(ICON_180,sizeof(ICON_180));});
  server.on("/apple-touch-icon-precomposed.png",HTTP_GET,[]{icon(ICON_180,sizeof(ICON_180));});
  server.on("/icon-192.png",HTTP_GET,[]{icon(ICON_192,sizeof(ICON_192));});
  server.on("/icon-512.png",HTTP_GET,[]{icon(ICON_512,sizeof(ICON_512));});
  server.on("/manifest.webmanifest",HTTP_GET,[]{server.sendHeader("Cache-Control","max-age=86400");server.send_P(200,"application/manifest+json",MANIFEST);});
  server.on("/api/state",HTTP_GET,stateApi);
  server.on("/api/home",HTTP_GET,homeApi);
  server.on("/api/config",HTTP_POST,configApi);
  server.on("/api/visit",HTTP_POST,visitApi);
  server.on("/api/sync",HTTP_POST,syncApi);
  server.on("/api/litter",HTTP_POST,[]{if(!protect())return;litterAt=epochNow();prefs.putUInt("litterAt",litterAt);syncDirty=true;reply(200,"Aggiunta di lettiera registrata","Litter top-up recorded");});
  server.on("/api/bin/reset",HTTP_POST,[]{if(!protect())return;bin.visits=0;bin.since=epochNow();saveVisits();reply(200,"Cassetto svuotato: conteggio azzerato","Bin emptied: count reset");});
  server.on("/api/arm",HTTP_POST,[]{if(!protect())return;
    if(server.arg("enabled")!="0" && server.arg("enabled")!="1"){reply(400,"Parametro non valido","Invalid parameter");return;}
    armed=server.arg("enabled")=="1";armSince=millis();reply(200,armed?"Invii abilitati per 10 minuti; verifica i DP sulla tua revisione":"Invii disabilitati",
      armed?"Sending enabled for 10 minutes; check the DPs on your unit":"Sending disabled");});
  server.on("/api/query",HTTP_POST,[]{if(!protect())return;if(!online()){reply(409,"MCU offline","MCU offline");return;}sendFrame(8);reply(200,"Query inviata","Query sent");});
  server.on("/api/command",HTTP_POST,[]{if(!protect())return;String action=server.arg("action");
    if(action=="clean")writeDp(101,1,1);else if(action=="empty")writeDp(102,1,1);
    else if(action=="auto_on")writeDp(105,1,1);else if(action=="auto_off")writeDp(105,1,0);
    else if(action=="bag")writeDp(127,1,1);else if(action=="level")writeDp(126,1,1);
    else if(action=="odor_on")writeDp(129,1,1);else if(action=="odor_off")writeDp(129,1,0);
    else reply(400,"Comando non ammesso","Command not allowed");});
  server.on("/api/value",HTTP_POST,[]{if(!protect())return;uint32_t id,value;
    if(!parseUnsigned(server.arg("dp"),id)||!parseUnsigned(server.arg("value"),value)||
      (id!=117 && id!=118)||value>(id==117?60u:120u)){reply(400,"Intervallo valido: DP117 0-60; DP118 0-120 minuti","Valid range: DP117 0-60; DP118 0-120 minutes");return;}
    writeDp(uint8_t(id),2,value);});
  // New home Wi-Fi credentials, e.g. after changing router. Wrong ones bring the recovery network back.
  server.on("/api/wifi",HTTP_POST,[]{if(!protect())return;
    if(server.header("X-Tonepie-Ota")!=Config::OTA_PASSWORD){reply(403,"Password di aggiornamento errata","Wrong update password");return;}
    String ssid=server.arg("ssid"),password=server.arg("password");
    if(ssid.isEmpty() || ssid.length()>32 || password.length()<8 || password.length()>63){reply(400,"Nome rete fino a 32 caratteri, password da 8 a 63","Network name up to 32 characters, password 8 to 63");return;}
    prefs.putString("ssid",ssid);prefs.putString("wpass",password);
    reply(200,"Rete salvata: riavvio. Se non si collega entro 3 minuti riappare la rete Tonepie-Setup","Network saved: restarting. If it cannot connect within 3 minutes the Tonepie-Setup network comes back");delay(400);ESP.restart();});
  // Firmware update over the network: multipart upload, written straight to the spare app partition.
  server.on("/api/update",HTTP_POST,[]{
    if(!otaAllowed){reply(403,"Aggiornamento rifiutato: password o sessione non valide","Update refused: wrong password or session");return;}
    if(!otaOk){reply(500,String("Aggiornamento fallito: ")+Update.errorString(),String("Update failed: ")+Update.errorString());return;}
    reply(200,"Aggiornamento riuscito: riavvio in corso","Update done: restarting");delay(400);ESP.restart();
  },[]{
    HTTPUpload& upload=server.upload();
    if(upload.status==UPLOAD_FILE_START){
      otaAllowed=allowedHost() && server.header("X-Tonepie-Token")==token && server.header("X-Tonepie-Ota")==Config::OTA_PASSWORD;
      otaOk=false;if(!otaAllowed)return;
      armed=false;logLine("Aggiornamento firmware via rete avviato: invii disabilitati");
      otaOk=Update.begin(UPDATE_SIZE_UNKNOWN);
    }else if(upload.status==UPLOAD_FILE_WRITE){
      if(otaAllowed && otaOk && Update.write(upload.buf,upload.currentSize)!=upload.currentSize)otaOk=false;
    }else if(upload.status==UPLOAD_FILE_END){
      if(otaAllowed && otaOk)otaOk=Update.end(true); // verifies the image before it becomes bootable
    }else if(upload.status==UPLOAD_FILE_ABORTED){
      if(otaAllowed)Update.abort();otaOk=false;
    }
  });
  server.onNotFound([]{reply(404,"Risorsa non trovata","Not found");});server.begin();
}
void setup(){
  Serial.setTxBufferSize(1024);
  Serial.begin(115200); // UART0 on GPIO20/21 through onboard CH340.
  mcu.setRxBufferSize(4096);mcu.begin(Config::MCU_BAUD,SERIAL_8N1,Config::MCU_RX,Config::MCU_TX);
  char randomToken[33];snprintf(randomToken,sizeof(randomToken),"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random());token=randomToken;
  WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.setHostname(Config::HOSTNAME);WiFi.setAutoReconnect(true);
  loadStore();
  uint8_t mac[6];WiFi.macAddress(mac);char id[24];snprintf(id,sizeof(id),"tonepie-%02x%02x%02x",mac[3],mac[4],mac[5]);deviceId=id;
  xTaskCreate(syncWorker,"sync",12288,nullptr,1,&syncTask);
  WiFi.begin(wifiSsid.c_str(),wifiPassword.c_str());lastWifiTry=wifiSeen=millis();setupWeb();
  logLine(String("Tonepie ")+Config::FIRMWARE_VERSION+" / UART1 RX"+String(Config::MCU_RX)+" TX"+String(Config::MCU_TX)+" 115200 8N1 / log CH340");
  sendFrame(0);lastHbTx=millis();
}
void loop(){
  uint32_t now=millis();
  // Bound each drain so a noisy UART cannot starve Wi-Fi/watchdog service.
  for(size_t n=0;n<4096 && mcu.available();n++)parser.push(uint8_t(mcu.read()),millis());
  parser.tick(millis());now=millis();
  if(heard && !online()){heard=false;invalidate();initState=Init::Heartbeat;logLine("Heartbeat scaduto: invii disabilitati");}
  if(uint32_t(now-lastHbTx)>=(online()?5000u:1000u)){sendFrame(0);lastHbTx=now;}
  if(online() && (initState==Init::Product||initState==Init::Mode) && uint32_t(now-lastInitTx)>=2000){sendFrame(initState==Init::Product?1:2);lastInitTx=now;}
  bool connected=WiFi.status()==WL_CONNECTED;
  if(connected && !wasWifi){logLine("Wi-Fi connesso: http://"+WiFi.localIP().toString());mdns=MDNS.begin(Config::HOSTNAME);if(mdns)MDNS.addService("http","tcp",80);
    // Clock for visit times only: the MCU time request (1C) is still answered "not available".
    if(!clockStarted){clockStarted=true;configTzTime(Config::TIMEZONE,Config::NTP_PRIMARY,Config::NTP_FALLBACK);}}
  if(!connected && wasWifi){armed=false;if(mdns)MDNS.end();mdns=false;logLine("Wi-Fi scollegato");}
  wasWifi=connected;
  // Recovery network: lets a phone reach the pages (firmware update, Wi-Fi settings) when the home Wi-Fi is gone.
  if(connected){
    wifiSeen=now;
    if(apOn && !WiFi.softAPgetStationNum()){WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);apOn=false;logLine("Wi-Fi di casa tornato: rete di recupero spenta");}
  }else if(!apOn && uint32_t(now-wifiSeen)>=Config::AP_AFTER_MS){
    WiFi.mode(WIFI_AP_STA);apOn=WiFi.softAP(Config::AP_SSID,wifiPasswordForAp());wifiSeen=now;
    logLine(apOn?String("Wi-Fi di casa assente: rete di recupero ")+Config::AP_SSID+" su http://"+WiFi.softAPIP().toString():String("Rete di recupero non avviata"));
  }
  // Retrying makes the radio leave the recovery channel: not while someone is connected to it.
  if(!connected && !(apOn && WiFi.softAPgetStationNum()) && uint32_t(now-lastWifiTry)>=(apOn?60000u:15000u)){WiFi.reconnect();lastWifiTry=now;}
  // 2 = not connected. With 3 (router only) the MCU keeps its Wi-Fi LED blinking, so the
  // module reports the "connected" state a Tuya module would give once fully online.
  uint8_t network=connected?Config::NETWORK_CONNECTED:2;
  if(online() && initState==Init::Ready && (networkReported!=network || (!networkAck && uint32_t(now-lastNetTx)>=2000))){
    networkReported=network;networkAck=false;lastNetTx=now;sendFrame(3,&network,1);
    queryScheduled=true;queryDue=now+500;
  }
  if(queryScheduled && int32_t(now-queryDue)>=0){queryScheduled=false;if(online())sendFrame(8);}
  if(pending.active && uint32_t(now-pending.since)>=5000){pending.active=false;commandIt="Report atteso non ricevuto: esito sconosciuto (nessun reinvio automatico)";commandEn="Expected report not received: result unknown (no automatic resend)";logLine(commandIt);}
  if(armed && !isArmed())armed=false;
  serviceTracker(now);
  serviceSync(now);
  server.handleClient();delay(1);
}
