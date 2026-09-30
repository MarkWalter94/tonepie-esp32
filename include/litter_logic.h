#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Cat identification, visit history and visit detection. Independent from
// Arduino so it can be tested on the host (test/litter_test.cpp).
namespace Litter {
constexpr int MAX_CATS = 4;
constexpr int MAX_VISITS = 64;
constexpr int8_t CAT_UNKNOWN = -1;
constexpr uint8_t VISIT_MANUAL = 1;  // assigned by hand: never re-matched
constexpr uint8_t VISIT_COUNTED = 2; // the MCU's visit counter already accounted for this visit

struct Cat { char name[24]; uint16_t weightG; uint8_t color, reserved; };
struct Settings {
  uint8_t version, catCount;
  uint16_t toleranceG, reserved, binLimitVisits; // version 1 had grams per visit and a limit in grams here
  Cat cats[MAX_CATS];
};
struct Visit {
  uint32_t epoch;      // 0 = clock not available when recorded
  uint16_t id, weightG, durationS, reserved;
  int8_t cat;
  uint8_t flags;
};
struct History { uint16_t nextId; uint16_t count; Visit v[MAX_VISITS]; }; // oldest first

inline void defaults(Settings& s) {
  memset(&s, 0, sizeof(s));
  s.version = 2; s.toleranceG = 500; s.binLimitVisits = 30;
}
inline uint16_t clampU16(uint32_t v, uint16_t lo, uint16_t hi) { return v < lo ? lo : v > hi ? hi : uint16_t(v); }
// Makes data loaded from flash safe to use whatever it contains.
inline void sanitize(Settings& s) {
  if (s.version == 1 && s.catCount <= MAX_CATS) { // grams limit / grams per visit -> visits
    s.binLimitVisits = s.binLimitVisits / (s.reserved ? s.reserved : 50); s.reserved = 0; s.version = 2;
  }
  if (s.version != 2 || s.catCount > MAX_CATS) { defaults(s); return; }
  s.toleranceG = clampU16(s.toleranceG, 100, 3000);
  s.binLimitVisits = clampU16(s.binLimitVisits, 5, 500);
  for (auto& c : s.cats) c.name[sizeof(c.name) - 1] = 0;
}
inline void sanitize(History& h) {
  if (h.count > MAX_VISITS) { memset(&h, 0, sizeof(h)); return; }
  for (uint16_t i = 0; i < h.count; i++) {
    Visit& v = h.v[i];
    if (v.cat < CAT_UNKNOWN || v.cat >= MAX_CATS) { v.cat = CAT_UNKNOWN; v.flags &= ~VISIT_MANUAL; }
  }
}

// Nearest reference weight within tolerance; a tie between two cats stays unknown.
inline int8_t matchCat(const Settings& s, uint16_t weightG) {
  if (!weightG) return CAT_UNKNOWN;
  int best = CAT_UNKNOWN; uint32_t bestD = UINT32_MAX; bool tie = false;
  for (int i = 0; i < s.catCount; i++) {
    if (!s.cats[i].weightG) continue;
    uint32_t d = weightG > s.cats[i].weightG ? weightG - s.cats[i].weightG : s.cats[i].weightG - weightG;
    if (d < bestD) { bestD = d; best = i; tie = false; }
    else if (d == bestD) tie = true;
  }
  return (best < 0 || tie || bestD > s.toleranceG) ? CAT_UNKNOWN : int8_t(best);
}
inline void rematch(const Settings& s, History& h) {
  for (uint16_t i = 0; i < h.count; i++) {
    Visit& v = h.v[i];
    if ((v.flags & VISIT_MANUAL) && v.cat < s.catCount) continue;
    v.flags &= ~VISIT_MANUAL; // automatic, or assigned to a cat that no longer exists
    // A recognised visit keeps its cat: reference weights drift, the past does not change.
    if (v.cat < 0 || v.cat >= s.catCount) v.cat = matchCat(s, v.weightG);
  }
}
// Daily average weight per cat, oldest first: feeds the trend chart.
constexpr int WEIGHT_DAYS = 90;
struct WeightPoint { uint16_t day, grams; uint8_t samples, reserved; }; // day = epoch / 86400
struct WeightLog { uint8_t version; uint8_t count[MAX_CATS]; WeightPoint p[MAX_CATS][WEIGHT_DAYS]; };
inline void sanitize(WeightLog& w) {
  bool ok = w.version == 1;
  for (int i = 0; i < MAX_CATS; i++) if (w.count[i] > WEIGHT_DAYS) ok = false;
  if (!ok) { memset(&w, 0, sizeof(w)); w.version = 1; }
}
inline void addWeight(WeightLog& w, int cat, uint32_t epoch, uint16_t grams) {
  if (cat < 0 || cat >= MAX_CATS || !epoch || !grams) return;
  uint16_t day = uint16_t(epoch / 86400);
  WeightPoint* p = w.p[cat]; uint8_t& n = w.count[cat];
  for (int i = n - 1; i >= 0 && p[i].day >= day; i--) if (p[i].day == day) {
    if (p[i].samples < 255) { p[i].grams = uint16_t((uint32_t(p[i].grams) * p[i].samples + grams) / (p[i].samples + 1)); p[i].samples++; }
    return;
  }
  if (n && p[n - 1].day > day) return; // older than the log and no entry for that day
  if (n == WEIGHT_DAYS) { memmove(p, p + 1, sizeof(WeightPoint) * (WEIGHT_DAYS - 1)); n--; }
  p[n++] = {day, grams, 1, 0};
}
// The reference weight follows the cat as it grows or slims down, so it keeps being recognised.
inline uint16_t learn(uint16_t reference, uint16_t grams) { return uint16_t((uint32_t(reference) * 4 + grams + 2) / 5); }
// Learning must not bring two cats' reference weights so close that every reading becomes a tie.
constexpr uint16_t MIN_GAP_G = 100;
inline bool learnAllowed(const Settings& s, int cat, uint16_t next) {
  for (int i = 0; i < s.catCount; i++) {
    if (i == cat || !s.cats[i].weightG) continue;
    uint16_t o = s.cats[i].weightG;
    if ((next > o ? next - o : o - next) < MIN_GAP_G) return false;
  }
  return true;
}
// After the cat list is edited: from[i] is the previous index of new cat i, or -1 for a new cat.
inline void remapCats(History& h, WeightLog& w, WeightLog& scratch, const int8_t* from, int newCount) {
  int8_t to[MAX_CATS]; for (auto& t : to) t = CAT_UNKNOWN;
  for (int i = 0; i < newCount; i++) if (from[i] >= 0) to[from[i]] = int8_t(i);
  for (uint16_t i = 0; i < h.count; i++) {
    Visit& v = h.v[i];
    if (v.cat < 0) continue;
    v.cat = v.cat < MAX_CATS ? to[v.cat] : CAT_UNKNOWN;
    if (v.cat < 0) v.flags &= ~VISIT_MANUAL;
  }
  scratch = w; memset(&w, 0, sizeof(w)); w.version = 1;
  for (int i = 0; i < newCount; i++) if (from[i] >= 0) {
    w.count[i] = scratch.count[from[i]];
    memcpy(w.p[i], scratch.p[from[i]], sizeof(w.p[i]));
  }
}
inline Visit& add(History& h, Visit v) {
  if (h.count == MAX_VISITS) { memmove(h.v, h.v + 1, sizeof(Visit) * (MAX_VISITS - 1)); h.count--; }
  if (!++h.nextId) h.nextId = 1;
  v.id = h.nextId;
  h.v[h.count] = v;
  return h.v[h.count++];
}
inline Visit* find(History& h, uint16_t id) {
  for (uint16_t i = 0; i < h.count; i++) if (h.v[i].id == id) return &h.v[i];
  return nullptr;
}
inline bool remove(History& h, uint16_t id) {
  for (uint16_t i = 0; i < h.count; i++) if (h.v[i].id == id) {
    memmove(h.v + i, h.v + i + 1, sizeof(Visit) * (h.count - i - 1)); h.count--; return true;
  }
  return false;
}

struct Event {
  enum Kind : uint8_t { New, PatchWeight, PatchDuration, PatchCount } kind;
  uint16_t n, weightG, durationS; // New: n visits, only the last one carries weight/duration
};

// Turns MCU reports into visits. Three signals describe a visit: the visit
// counter (DP7) increasing, a weight report (DP6) and a duration report (DP8).
// Their order and timing on the real unit are not verified, so any of them may
// open a visit; the others are merged if they arrive shortly before or after
// (patches refer to the last visit produced). Weight/duration repeated inside a
// query response never open a visit and are used only by a visit opened by that
// same response.
class Tracker {
public:
  static constexpr uint32_t QUERY_WINDOW_MS = 1500, SETTLE_MS = 4000, FRESH_MS = 20000,
                            LATE_MS = 900000, REPEAT_MS = 60000;
  bool haveCount = false, countDirty = false;
  uint32_t count = 0;

  // value: counter saved before a restart. A counter saved on another day must be restored as 0 by
  // the caller: the MCU resets it at midnight, so every visit counted since then is new.
  void restore(bool have, uint32_t value) { haveCount = have; count = value; }
  // The last visit recorded before a restart, so that late signals after the restart patch it
  // instead of opening a duplicate. ageMs: how long ago it was recorded.
  void restoreLast(uint32_t now, uint32_t ageMs, bool hasCount, bool hasWeight, bool hasDuration) {
    if (ageMs >= LATE_MS) return;
    lastValid = true; lastAt = now - ageMs; lastHasCount = hasCount; lastHasWeight = hasWeight; lastHasDuration = hasDuration;
  }
  // Counter value already turned into recorded visits: the one to persist.
  uint32_t committed() const { return pending && pendingCount <= count ? count - pendingCount : count; }
  void onQuerySent(uint32_t now) { queryAt = now; querySeen = true; }
  void onWeight(uint16_t grams, uint32_t now) {
    weight = grams; weightAt = now; weightSeen = true; weightFromQuery = inQuery(now);
    if (weightFromQuery || pending) return; // a pending visit reads the latest value when it settles
    if (lastValid && uint32_t(now - lastAt) < (lastHasWeight ? REPEAT_MS : LATE_MS)) {
      lastHasWeight = true; weightSeen = false; push({Event::PatchWeight, 0, grams, 0}); return;
    }
    open(now);
  }
  void onDuration(uint16_t seconds, uint32_t now) {
    duration = seconds; durationAt = now; durationSeen = true; durationFromQuery = inQuery(now);
    if (durationFromQuery || pending) return;
    if (lastValid && uint32_t(now - lastAt) < (lastHasDuration ? REPEAT_MS : LATE_MS)) {
      lastHasDuration = true; durationSeen = false; push({Event::PatchDuration, 0, 0, seconds}); return;
    }
    open(now);
  }
  void onCount(uint32_t total, uint32_t now) {
    if (!haveCount) { haveCount = true; count = total; countDirty = true; return; } // first baseline
    if (total == count) return;
    // A decrease means the MCU restarted counting (midnight): all of today's visits are new.
    uint32_t delta = total > count ? total - count : total;
    count = total; countDirty = true;
    if (!delta) return;
    if (pending) { pendingCount += delta; return; }
    if (lastValid && !lastHasCount && uint32_t(now - lastAt) < LATE_MS) {
      lastHasCount = true; push({Event::PatchCount, 0, 0, 0});
      if (!--delta) return;
    }
    open(now); pendingCount = delta;
  }
  bool poll(uint32_t now, Event& out) {
    expire(now);
    if (queued) { out = queue[0]; memmove(queue, queue + 1, sizeof(Event) * --queued); return true; }
    if (!pending || uint32_t(now - openedAt) < SETTLE_MS) return false;
    out.kind = Event::New;
    out.n = pendingCount ? (pendingCount > 1000 ? 1000 : uint16_t(pendingCount)) : 1;
    out.weightG = usable(weightSeen, weightAt, weightFromQuery) ? weight : 0;
    out.durationS = usable(durationSeen, durationAt, durationFromQuery) ? duration : 0;
    lastValid = true; lastAt = now; lastHasCount = pendingCount > 0;
    lastHasWeight = out.weightG > 0; lastHasDuration = out.durationS > 0;
    pending = false; pendingCount = 0; countDirty = true; // the committed count moved
    weightSeen = durationSeen = false; // consumed: never reused for a later visit
    return true;
  }
  bool lastCounted() const { return lastHasCount; }
private:
  bool querySeen = false, weightSeen = false, durationSeen = false, pending = false;
  bool weightFromQuery = false, durationFromQuery = false;
  bool lastValid = false, lastHasCount = false, lastHasWeight = false, lastHasDuration = false;
  uint16_t weight = 0, duration = 0;
  uint32_t queryAt = 0, weightAt = 0, durationAt = 0, openedAt = 0, lastAt = 0, pendingCount = 0;
  Event queue[4];
  size_t queued = 0;
  bool inQuery(uint32_t now) const { return querySeen && uint32_t(now - queryAt) < QUERY_WINDOW_MS; }
  // A value belongs to the visit if it came at most FRESH_MS before the visit opened, or after it; a
  // value from a query response only if the visit was opened by that same response.
  bool usable(bool seen, uint32_t at, bool fromQuery) const {
    if (!seen) return false;
    bool fresh = int32_t(at - openedAt) >= 0 || uint32_t(openedAt - at) <= FRESH_MS;
    return fresh && (!fromQuery || (querySeen && uint32_t(openedAt - queryAt) < QUERY_WINDOW_MS));
  }
  // Old timestamps are dropped before millis() wraps (49.7 days) and makes them look recent again.
  void expire(uint32_t now) {
    if (lastValid && uint32_t(now - lastAt) >= LATE_MS) lastValid = false;
    if (pending) return;
    if (querySeen && uint32_t(now - queryAt) >= QUERY_WINDOW_MS) querySeen = false;
    if (weightSeen && uint32_t(now - weightAt) > FRESH_MS) weightSeen = false;
    if (durationSeen && uint32_t(now - durationAt) > FRESH_MS) durationSeen = false;
  }
  void open(uint32_t now) { pending = true; openedAt = now; pendingCount = 0; }
  void push(const Event& e) { if (queued < 4) queue[queued++] = e; }
};
}
