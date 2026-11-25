#include "../src/core/MidiControlBinding.h"
#include <iostream>
#include <stdexcept>
static void check(bool ok) { if (!ok) throw std::runtime_error("MIDI gesture check failed"); }
int main() {
  try {
    Morphora::MidiControlBinding control;
    control.enabled = true; control.threshold = 64; control.source = 68;
    check(control.update(0xb0,64,127)); check(!control.update(0xb0,64,127));
    check(!control.update(0xb0,64,40)); check(!control.update(0xb0,64,90));
    check(!control.update(0xb0,64,0)); check(control.update(0xb0,64,127));
    control.source = 2; control.rearm();
    check(control.update(0xe0,127,127)); check(!control.update(0xe0,127,127));
    check(!control.update(0xe0,0,64)); check(control.update(0xe0,127,127));
    control.source = 3; control.rearm(); check(control.update(0xe0,0,0));
    control.source = 1; control.rearm(); check(control.update(0xa0,60,127));
    check(!control.update(0xa0,60,100)); control.releaseNote(60);
    check(control.update(0xa0,60,127));
    control.source = 0; control.rearm(); check(control.update(0xd0,127,0));
    control.enabled = false; control.clearInput(); check(!control.update(0xd0,127,0));
    control.source = 5; check(control.companion(0xb0,33));
    check(!control.companion(0xb0,120));
    std::cout << "MIDI control gesture checks passed\n";
  } catch (const std::exception& error) { std::cerr << error.what(); return 1; }
}
