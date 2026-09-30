// Host tests: g++ -std=c++11 -Wall -Wextra -pedantic -Iinclude test/litter_test.cpp -o litter_test
#include "litter_logic.h"
#include <assert.h>
#include <stdio.h>

using namespace Litter;

Settings twoCats() {
  Settings s; defaults(s);
  s.catCount = 2;
  strcpy(s.cats[0].name, "Micio"); s.cats[0].weightG = 4200;
  strcpy(s.cats[1].name, "Luna"); s.cats[1].weightG = 5600;
  return s;
}
int drain(Tracker& t, uint32_t now, Event* out = nullptr) {
  int n = 0; Event e;
  while (t.poll(now, e)) { if (out) out[n] = e; n++; }
  return n;
}
int main() {
  // Cat matching
  Settings s = twoCats();
  assert(matchCat(s, 4200) == 0 && matchCat(s, 4500) == 0 && matchCat(s, 5300) == 1);
  assert(matchCat(s, 4900) == CAT_UNKNOWN);            // tie
  assert(matchCat(s, 3600) == CAT_UNKNOWN);            // beyond tolerance
  assert(matchCat(s, 0) == CAT_UNKNOWN);
  Settings none; defaults(none); assert(matchCat(none, 4200) == CAT_UNKNOWN);

  // History ring, ids, removal, rematch
  History h; memset(&h, 0, sizeof(h));
  for (int i = 0; i < MAX_VISITS + 5; i++) { Visit v{}; v.weightG = 4200; v.cat = CAT_UNKNOWN; add(h, v); }
  assert(h.count == MAX_VISITS && h.v[0].id == 6 && h.v[MAX_VISITS - 1].id == MAX_VISITS + 5);
  assert(find(h, 5) == nullptr && find(h, 6) != nullptr);
  assert(remove(h, 6) && h.count == MAX_VISITS - 1 && h.v[0].id == 7 && !remove(h, 6));
  find(h, 7)->cat = 1; find(h, 7)->flags = VISIT_MANUAL;
  rematch(s, h);
  assert(find(h, 7)->cat == 1 && find(h, 8)->cat == 0);
  s.catCount = 1; rematch(s, h);
  assert(find(h, 7)->cat == 0 && !(find(h, 7)->flags & VISIT_MANUAL)); // cat 1 removed: back to automatic

  // Sanitize rejects garbage
  Settings bad; memset(&bad, 0xff, sizeof(bad)); sanitize(bad); assert(bad.catCount == 0 && bad.binLimitVisits == 30);
  Settings old = twoCats(); old.version = 1; old.reserved = 50; old.binLimitVisits = 2000; // 1.x: 2000 g at 50 g/visit
  sanitize(old); assert(old.version == 2 && old.binLimitVisits == 40 && old.catCount == 2 && old.reserved == 0);
  History hb; memset(&hb, 0xff, sizeof(hb)); sanitize(hb); assert(hb.count == 0);

  // Weight log: daily average, late samples, ring, and following the cat over time
  static WeightLog w, scratch; memset(&w, 0xff, sizeof(w)); sanitize(w); assert(w.version == 1 && w.count[0] == 0);
  addWeight(w, 0, 100 * 86400u + 50, 4200); addWeight(w, 0, 100 * 86400u + 900, 4400);
  assert(w.count[0] == 1 && w.p[0][0].day == 100 && w.p[0][0].grams == 4300 && w.p[0][0].samples == 2);
  addWeight(w, 0, 102 * 86400u, 4500); addWeight(w, 0, 100 * 86400u, 4300); addWeight(w, 0, 101 * 86400u, 9999);
  assert(w.count[0] == 2 && w.p[0][0].samples == 3 && w.p[0][1].grams == 4500); // day 101 came too late: ignored
  addWeight(w, 0, 0, 4200); addWeight(w, -1, 103 * 86400u, 4200); assert(w.count[0] == 2);
  for (int d = 0; d < WEIGHT_DAYS + 3; d++) addWeight(w, 1, (200 + d) * 86400u, 5600);
  assert(w.count[1] == WEIGHT_DAYS && w.p[1][0].day == 203);
  uint16_t ref = 4200; for (int i = 0; i < 30; i++) ref = learn(ref, 4700);
  assert(ref > 4650 && ref <= 4700 && learn(4200, 4200) == 4200);
  { // Removing the first cat moves the second one's visits and weights to index 0
    History hr; memset(&hr, 0, sizeof(hr));
    Visit a{}; a.cat = 0; add(hr, a); Visit b{}; b.cat = 1; b.flags = VISIT_MANUAL; add(hr, b);
    int8_t from[MAX_CATS] = {1, -1, -1, -1};
    remapCats(hr, w, scratch, from, 2);
    assert(hr.v[0].cat == CAT_UNKNOWN && hr.v[1].cat == 0 && (hr.v[1].flags & VISIT_MANUAL));
    assert(w.count[0] == WEIGHT_DAYS && w.p[0][0].day == 203 && w.count[1] == 0);
  }

  Event ev[4];
  { // First count is a baseline; query responses never create visits
    Tracker t; t.onQuerySent(1000);
    t.onCount(10, 1010); t.onWeight(4200, 1011); t.onDuration(60, 1012);
    assert(drain(t, 10000) == 0 && t.haveCount && t.count == 10 && t.countDirty);
    t.onQuerySent(20000); t.onCount(10, 20010); t.onWeight(4200, 20011);
    assert(drain(t, 30000) == 0);
  }
  { // Spontaneous weight + count + duration together: one visit
    Tracker t; t.restore(true, 10);
    t.onWeight(5600, 100000); t.onCount(11, 100050); t.onDuration(75, 100100);
    assert(drain(t, 103000) == 0);
    assert(drain(t, 104001, ev) == 1 && ev[0].kind == Event::New && ev[0].n == 1 && ev[0].weightG == 5600 && ev[0].durationS == 75);
    assert(t.count == 11);
    // Same weight repeated shortly after: update, not a new visit
    t.onWeight(5500, 120000);
    assert(drain(t, 130000, ev) == 1 && ev[0].kind == Event::PatchWeight && ev[0].weightG == 5500);
  }
  { // Weight on entry, count and duration minutes later: still one visit
    Tracker t; t.restore(true, 10);
    t.onWeight(4200, 100000);
    assert(drain(t, 105000, ev) == 1 && ev[0].kind == Event::New && ev[0].weightG == 4200 && ev[0].durationS == 0);
    t.onCount(11, 200000); t.onDuration(95, 200010);
    assert(drain(t, 210000, ev) == 1 && ev[0].kind == Event::PatchDuration && ev[0].durationS == 95);
    // Next visit an hour later is a new one
    t.onWeight(5600, 3800000); t.onCount(12, 3800010);
    assert(drain(t, 3805000, ev) == 1 && ev[0].kind == Event::New && ev[0].weightG == 5600);
  }
  { // Count first, weight later
    Tracker t; t.restore(true, 10);
    t.onCount(11, 100000);
    assert(drain(t, 104500, ev) == 1 && ev[0].kind == Event::New && ev[0].weightG == 0);
    t.onWeight(4300, 130000);
    assert(drain(t, 131000, ev) == 1 && ev[0].kind == Event::PatchWeight && ev[0].weightG == 4300);
  }
  { // Visits missed while the module was off: found in the boot query
    Tracker t; t.restore(true, 10);
    t.onQuerySent(2000); t.onWeight(4200, 2010); t.onCount(13, 2011); t.onDuration(40, 2012);
    assert(drain(t, 7000, ev) == 1 && ev[0].kind == Event::New && ev[0].n == 3 && ev[0].weightG == 4200 && ev[0].durationS == 40);
  }
  { // Counter reset is a new baseline; millis wrap is harmless
    Tracker t; t.restore(true, 10);
    t.onCount(0, 5000); assert(drain(t, 20000) == 0 && t.count == 0);
    t.onWeight(4200, 0xFFFFFF00u); t.onCount(1, 0xFFFFFF80u);
    assert(drain(t, 0xFFFFFFF0u) == 0);
    assert(drain(t, 0x00001000u, ev) == 1 && ev[0].n == 1 && ev[0].weightG == 4200);
  }
  puts("litter_test: PASS");
  return 0;
}
