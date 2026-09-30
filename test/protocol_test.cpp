// Host tests: g++ -std=c++11 -Wall -Wextra -pedantic -Iinclude test/protocol_test.cpp -o protocol_test
#include "tuya_protocol.h"
#include <assert.h>
#include <vector>
#include <stdio.h>
#include <random>

struct Frame { uint8_t cmd; std::vector<uint8_t> data; };
std::vector<Frame> received;
void accept(uint8_t,uint8_t cmd,const uint8_t* p,size_t n){received.push_back({cmd,{p,p+n}});}
std::vector<uint8_t> frame(uint8_t cmd,std::vector<uint8_t> payload={}){
  std::vector<uint8_t> f(payload.size()+7);
  assert(Tuya::encode(cmd,payload.data(),payload.size(),f.data(),f.size())==f.size());return f;
}
void feed(Tuya::Parser& p,const std::vector<uint8_t>& bytes,uint32_t now=1){for(auto b:bytes)p.push(b,now);}
int main(){
  auto hb=frame(0);assert((hb==std::vector<uint8_t>{0x55,0xaa,0,0,0,0,0xff}));
  auto query=frame(8);assert(query.back()==7);
  uint8_t out[20];assert(Tuya::encode(1,nullptr,1,out,20)==0);assert(Tuya::encode(1,nullptr,0,out,6)==0);
  Tuya::Parser parser(accept);
  feed(parser,{0,0x55,0x55,0x99,0xaa});feed(parser,hb);assert(received.size()==1);
  auto bad=frame(7,{101,1,0,1,1});bad.back()^=1;feed(parser,bad);feed(parser,query);
  assert(received.size()==2 && received.back().cmd==8 && parser.badChecksum==1);
  feed(parser,{0x55,0xaa,3,7,0xff,0xff});feed(parser,hb);assert(parser.badLength==1 && received.size()==3);
  auto multi=frame(7,{101,1,0,1,1,117,2,0,4,0,0,0,5});feed(parser,multi);
  assert(Tuya::validDps(received.back().data.data(),received.back().data.size()));
  assert(Tuya::read32(received.back().data.data()+9)==5);
  // Arbitrary chunk boundaries and headers inside payload are legal.
  auto embedded=frame(7,{1,0,0,4,0x55,0xaa,0,0});for(auto b:embedded)parser.push(b,10);
  assert(received.back().data.size()==8);
  std::vector<uint8_t> maxPayload(Tuya::MAX_PAYLOAD,0x55);feed(parser,frame(7,maxPayload),20);
  assert(received.back().data.size()==Tuya::MAX_PAYLOAD);
  // Corrupt length swallows a good frame until timeout, then recovery finds it.
  size_t before=received.size();feed(parser,{0x55,0xaa,3,7,1,0},30);feed(parser,query,30);
  assert(received.size()==before);parser.tick(281);assert(received.size()==before+1 && parser.timeouts==1);
  // Multiple false headers before a good packet still recover on expiry.
  before=received.size();feed(parser,{0x55,0xaa,3,7,1,0,0x55,0xaa,3,7,1,0},300);feed(parser,hb,300);parser.tick(551);
  assert(received.size()==before+1);
  const uint8_t shortHeader[]={1,1,0};assert(!Tuya::validDps(shortHeader,sizeof(shortHeader)));
  const uint8_t shortValue[]={1,2,0,4,1};assert(!Tuya::validDps(shortValue,sizeof(shortValue)));
  const uint8_t badBool[]={1,1,0,1,2};assert(!Tuya::validDps(badBool,sizeof(badBool)));
  const uint8_t badBitmap[]={1,5,0,3,0,0,0};assert(!Tuya::validDps(badBitmap,sizeof(badBitmap)));
  const uint8_t unknown[]={1,6,0,0};assert(!Tuya::validDps(unknown,sizeof(unknown)));
  const uint8_t allTypes[]={1,0,0,0,2,1,0,1,0,3,2,0,4,255,255,255,255,4,3,0,1,'x',5,4,0,1,7,6,5,0,2,0,1};
  assert(Tuya::validDps(allTypes,sizeof(allTypes)));
  std::mt19937 random(1234);
  for(size_t i=0;i<1000000;++i)parser.push(uint8_t(random()),600+uint32_t(i/100));
  parser.tick(20000);before=received.size();feed(parser,hb,20001);assert(received.size()==before+1);
  // uint32 millis wrap-around.
  Tuya::Parser rollover(accept);rollover.push(0x55,0xfffffff0u);rollover.tick(300);assert(rollover.timeouts==1);
  puts("PASS: encode, checksum, noise, concatenation, fragmentation, maximum length, timeout recovery, TLV types, 1M-byte fuzz, millis wrap");
}
