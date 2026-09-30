#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace Tuya {
constexpr size_t MAX_PAYLOAD = 1024;
inline uint8_t checksum(const uint8_t* p, size_t n) {
  uint8_t sum = 0;
  while (n--) sum += *p++;
  return sum;
}
inline size_t encode(uint8_t cmd, const uint8_t* data, size_t n, uint8_t* out, size_t cap) {
  if (n > MAX_PAYLOAD || cap < n + 7 || (n && !data)) return 0;
  out[0]=0x55; out[1]=0xaa; out[2]=0; out[3]=cmd;
  out[4]=uint8_t(n >> 8); out[5]=uint8_t(n);
  if (n) memcpy(out+6, data, n);
  out[n+6]=checksum(out,n+6);
  return n+7;
}
inline uint32_t read32(const uint8_t* p) {
  return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];
}
inline bool validDps(const uint8_t* p, size_t n) {
  size_t pos=0;
  while (pos<n) {
    if (n-pos<4) return false;
    uint8_t type=p[pos+1];
    size_t len=(size_t(p[pos+2])<<8)|p[pos+3];
    if (len>n-pos-4 || type>5) return false;
    if (type==1 && (len!=1 || p[pos+4]>1)) return false;
    if (type==2 && len!=4) return false;
    if (type==4 && len!=1) return false;
    if (type==5 && len!=1 && len!=2 && len!=4) return false;
    pos+=4+len;
  }
  return true;
}
class Parser {
public:
  using Handler = void (*)(uint8_t, uint8_t, const uint8_t*, size_t);
  uint32_t frames=0, badChecksum=0, badLength=0, timeouts=0, discarded=0;
  explicit Parser(Handler h): handler(h) {}
  void tick(uint32_t now) {
    if (used && uint32_t(now-lastByte)>250) {
      ++timeouts;
      // Recover a valid frame embedded behind a corrupt/incomplete prefix.
      while (used) { drop(1); scan(); }
    }
  }
  void push(uint8_t b, uint32_t now) {
    tick(now); lastByte=now;
    if (used==sizeof(buf)) drop(1);
    buf[used++]=b; scan();
  }
private:
  uint8_t buf[MAX_PAYLOAD+7]{};
  size_t used=0;
  uint32_t lastByte=0;
  Handler handler;
  void drop(size_t n) { memmove(buf,buf+n,used-n); used-=n; }
  void scan() {
    while (used>=2) {
      if (buf[0]!=0x55 || buf[1]!=0xaa) { ++discarded; drop(1); continue; }
      if (used<6) return;
      size_t len=(size_t(buf[4])<<8)|buf[5];
      if (len>MAX_PAYLOAD) { ++badLength; drop(1); continue; }
      if (used<len+7) return;
      if (checksum(buf,len+6)!=buf[len+6]) { ++badChecksum; drop(1); continue; }
      ++frames;
      handler(buf[2],buf[3],buf+6,len);
      drop(len+7);
    }
  }
};
}
