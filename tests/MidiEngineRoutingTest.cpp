#include "../src/midi/MidiEngine.h"
#include "IMidiOut.h"
#include "MidiMonoIn.h"
#include <QCoreApplication>
#include <iostream>
#include <cmath>
#include <stdexcept>

struct Message { uint8_t status, note, value; };
class RecordingMidiOut : public IMidiOut {
public:
  std::vector<Message> messages;
  QStringList listOutputs() const override { return {QStringLiteral("Test")}; }
  bool open(int) override { return true; }
  void close() override {}
  bool sendShort(uint8_t s, uint8_t n, uint8_t v) override {
    messages.push_back({s,n,v}); return true;
  }
  bool sendSysEx(const std::vector<uint8_t>&) override { return true; }
};
class InjectedMidiIn : public IMidiIn {
public:
  Callback callback;
  QStringList listInputs() const override { return {}; }
  bool open(int) override { return true; }
  void close() override {}
  void setCallback(Callback cb) override { callback=std::move(cb); }
  void send(int status,int number,int value) {
    MidiInEvent event; event.status=status;event.data1=number;event.data2=value;
    callback(event);
  }
};
class MidiEngineRoutingTest {
  static void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
  }
  static bool voice(const MidiEngine& e, int note, int track) {
    return e.activeMorphNotes_[note].sentNotes[track] >= 0;
  }
  static RecordingMidiOut* setup(MidiEngine& e) {
    auto output = std::make_unique<RecordingMidiOut>();
    auto* capture = output.get();
    e.midiOut_ = std::move(output);
    e.midiOutputPorts_ = {QStringLiteral("Test")};
    e.snapshot_.midiOutPort = QStringLiteral("Test");
    e.snapshot_.playMode = PlayMode::Poly;
    e.snapshot_.tracks[0].morphOutput = MorphOutputId::High;
    e.snapshot_.tracks[1].morphOutput = MorphOutputId::Low;
    e.snapshot_.tracks[2].morphOutput = MorphOutputId::Loud;
    return capture;
  }
public:
  static void run() {
    {
      MidiEngine engine;
      auto* output = setup(engine);
      engine.snapshot_.tracks[7] = TrackPresetData{};
      output->messages.clear();
      engine.resetTrackSetup(7);
      bool program = false, volume = false, pan = false, expression = false;
      for (const auto& message : output->messages) {
        check((message.status & 0x0f) == 7, "Track reset affected another MIDI channel");
        program |= message.status == 0xc7 && message.note == 0;
        volume |= message.status == 0xb7 && message.note == 7 && message.value == 100;
        pan |= message.status == 0xb7 && message.note == 10 && message.value == 64;
        expression |= message.status == 0xb7 && message.note == 11 && message.value == 127;
      }
      check(program && volume && pan && expression, "Track reset did not transmit default setup");
    }
    {
      MidiEngine engine;
      auto* output = setup(engine);
      engine.snapshot_.velCurve = {49, 100, 113, 0};
      engine.snapshot_.tracks[2].morphOutput = MorphOutputId::Loud;
      output->messages.clear();
      engine.handleIncomingNoteOn(40, 120);
      bool sentLoud = false;
      for (const auto& message : output->messages) {
        if (message.status == 0x92 && message.note == 40 && message.value == 120)
          sentLoud = true;
      }
      check(sentLoud, "Full Loud gain must send a Note On with full input velocity");
    }
    {
      MidiEngine e; auto* out=setup(e);
      for (int note : {48,52,55,60,64,67,72}) {
        out->messages.clear(); e.handleIncomingNoteOn(note,90);
        check(e.activeMorphNotes_[note].gains==e.calculateMorphOutputGains(note,90),
              "Chord order must not bypass the morph curves");
        for(auto m:out->messages) if((m.status & 0xe0)==0x80)
          check(m.note==note && (m.status&0xf0)==0x90,"New note altered an existing voice");
      }
      out->messages.clear(); e.handleIncomingNoteOff(72,0);
      for(auto m:out->messages) check((m.status&0xf0)!=0x90,"Release retriggered a held note");
      e.handleIncomingControlChange(64,127); e.handleIncomingNoteOff(67,0);
      check(e.activeMorphNotes_[67].sounding,"Sustain tail was lost");
      e.handleIncomingControlChange(64,0);
      check(!e.activeMorphNotes_[67].sounding,"Pedal release did not clear tail");
    }
    {
      auto assignments=defaultCurveCcAssignments();
      assignments[0]={1,20,100}; assignments[1]={2,4,80};
      PiecewiseCurve curve{32,100,96,0};
      setCurveParameter(curve,false,126);
      check(curve.x0==63 && curve.x1==127,"Position must stop at border and preserve width");
      const double center=curveParameter(curve,false);
      setCurveParameter(curve,true,127);
      check(curve.x1-curve.x0==1 && curveParameter(curve,false)==center,"Minimum width or fixed center violated");
      setCurveParameter(curve,false,0);
      check(curve.x0==0 && curve.x1==1,"Minimum position failed");
      setCurveParameter(curve,true,0);
      check(curve.x0==0 && curve.x1==1,"Width changed center at boundary");
      for(int first=0;first<127;++first) for(int last=first+1;last<=127;++last) {
        PiecewiseCurve original{double(first),73,double(last),19};
        auto restored=original;
        setCurveParameter(restored,true,curveParameter(original,true));
        check(std::abs(restored.x0-first)<1e-9 && std::abs(restored.x1-last)<1e-9,"Parameter round trip changed curve");
        for(int value : {0,1,64,126,127}) {
          auto changed=original;
          setCurveParameter(changed,true,value);
          check(changed.x0>=-1e-9 && changed.x1<=127+1e-9 && changed.x1-changed.x0>=1-1e-9,"Slope constraints violated");
          check(std::abs(curveParameter(changed,false)-curveParameter(original,false))<1e-9,"Slope moved center");
        }
      }
      for (int cc : {0,6,7,10,11,32,38,64,71,74,91,93,96,97,98,99,100,101,120,127})
        check(!isAssignableCurveCc(cc), "Reserved CC was assignable");
      assignments[0]={11,-5,200}; assignments[1]={2,0,0}; assignments[2]={2,0,127};
      sanitizeCurveCcAssignments(assignments);
      check(assignments[0].cc==-1 && assignments[2].cc==-1 && assignments[1].minimum==1
            && assignments[1].maximum==1, "Invalid mapping sanitization failed");
    }
    {
      MidiEngine e; auto* out=setup(e);
      for (int i=0;i<4;++i) e.snapshot_.curveControls[i].cc=i+20;
      e.handleIncomingNoteOn(60,90);
      const auto oldGains=e.activeMorphNotes_[60].gains;
      out->messages.clear();
      for (int i=0;i<4;++i) e.handleIncomingControlChange(20+i,100);
      check(e.calculateMorphOutputGains(60,90)==oldGains,"First hardware message moved curve");
      for (int i=0;i<4;++i) e.handleIncomingControlChange(20+i,110);
      check(out->messages.empty(), "Assigned CC was forwarded or changed held Poly voice");
      check(e.activeMorphNotes_[60].gains==oldGains, "Poly note contribution changed under CC");
      const auto expected=e.calculateMorphOutputGains(60,90);
      check(expected!=oldGains, "CC did not change future contributions");
      e.handleIncomingNoteOff(60,0); e.handleIncomingNoteOn(60,90);
      check(e.activeMorphNotes_[60].gains==expected, "New Poly note ignored effective curve");
      out->messages.clear(); e.handleIncomingControlChange(1,42);
      check(!out->messages.empty() && out->messages[0].note==1, "Unassigned CC no longer forwards");
      auto next=e.snapshot_; ++next.curveControlRevision; e.setSnapshot(next);
      check(e.curveControlValues_[0]==-1 && e.curveControlValues_[3]==-1,
            "Preset/reset revision leaked live CC values");
    }
    {
      MidiEngine e; auto* out=setup(e);
      e.snapshot_.playMode=PlayMode::Mono;
      e.snapshot_.curveControls[0]={20,20,100};
      e.handleIncomingNoteOn(60,90); out->messages.clear();
      e.handleIncomingControlChange(20,0);
      e.handleIncomingControlChange(20,127);
      bool expression=false;
      for(auto m:out->messages) {
        check((m.status&0xf0)==0xb0, "Mono curve CC retriggered a note");
        check(m.note!=20, "Assigned CC leaked to output");
        if(m.note==11) expression=true;
      }
      check(expression, "Mono curve CC failed to update Expression");
      check(e.activeMorphNotes_[60].gains==e.calculateMorphOutputGains(60,90),
            "Mono monitor gains did not follow CC");
    }
    {
      MidiEngine simulated, received;
      auto* output=setup(simulated); setup(received);
      simulated.snapshot_.curveControls[0]={20,30,90};
      received.snapshot_.curveControls[0]={20,30,90};
      for(int value : {0,1,64,126,127}) {
        simulated.previewCurveControlValue(0,value,simulated.snapshot_.curveControlRevision);
        received.handleIncomingControlChange(20,value);
        check(simulated.curveControlValues_==received.curveControlValues_,"Preview differs from received CC");
        check(simulated.calculateMorphOutputGains(60,90)==received.calculateMorphOutputGains(60,90),
              "Preview produces different curves from received CC");
      }
      check(output->messages.empty(),"Preview forwarded an assigned CC");
      simulated.previewCurveControlValue(1,50,simulated.snapshot_.curveControlRevision);
      check(simulated.curveControlValues_[1]==-1,"Preview activated an unassigned control");
      simulated.previewCurveControlValue(0,10,simulated.snapshot_.curveControlRevision+1);
      check(simulated.curveControlValues_[0]==127,"Stale preview changed a new mapping");
    }
    {
      MidiEngine e; setup(e);
      e.snapshot_.curveControls[1].cc=12;
      setCurveParameter(e.snapshot_.keyCurve,true,87);
      e.handleIncomingControlChange(12,1);
      check(std::abs(curveParameter(e.snapshot_.keyCurve,true)-87)<1e-9,"First value jumped");
      auto stale=e.snapshot_;
      e.handleIncomingControlChange(12,2);
      e.handleIncomingControlChange(12,3);
      e.setSnapshot(stale);
      check(std::abs(curveParameter(e.snapshot_.keyCurve,true)-89)<1e-9,"Stale UI snapshot lost queued deltas");
      e.handleIncomingControlChange(12,127);
      check(std::abs(curveParameter(e.snapshot_.keyCurve,true)-127)<1e-9,"Slope did not reach limit");
      e.handleIncomingControlChange(12,126);
      check(std::abs(curveParameter(e.snapshot_.keyCurve,true)-126)<1e-9,"Reversal at limit had dead travel");
      auto acknowledged=e.snapshot_;
      acknowledged.curveEditAcknowledged=e.curveEditSerial_;
      e.setSnapshot(acknowledged);
      check(e.pendingCurveEdits_.empty(),"Acknowledged edits retained");
      setCurveParameter(acknowledged.keyCurve,true,60);
      e.setSnapshot(acknowledged);
      e.handleIncomingControlChange(12,125);
      check(std::abs(curveParameter(e.snapshot_.keyCurve,true)-59)<1e-9,"Manual edit reset hardware reference");
    }
    {
      MidiEngine engine; auto* out=setup(engine);
      for(int slot=0;slot<4;++slot) engine.snapshot_.curveControls[slot]={7,0,127,slot+2};
      auto input=std::make_unique<InjectedMidiIn>(); auto* raw=input.get();
      MidiIn_MonoInterpreter::Configuration config;
      config.ignoreProgramChanges=true;
      config.ignoredControlChanges.set();
      config.forwardOffChannelControlChanges=true;
      MidiIn_MonoInterpreter interpreter(std::move(input),config);
      interpreter.setSourceChannel(0);
      interpreter.setCallback([&](const MidiInEvent& event) { engine.handleIncomingMidiEvent(event); });
      for(int slot=0;slot<4;++slot) raw->send(0xb0+slot+2,7,20+slot);
      for(int slot=0;slot<4;++slot) {
        const auto beforeKey=engine.snapshot_.keyCurve, beforeVel=engine.snapshot_.velCurve;
        const double before=curveParameter(slot<2 ? beforeKey : beforeVel,slot%2);
        raw->send(0xb0+slot+2,7,21+slot);
        check(std::abs(curveParameter(slot<2 ? engine.snapshot_.keyCurve : engine.snapshot_.velCurve,slot%2)-before-1)<1e-8,
              "CC7 on independent channel did not edit correct parameter");
        const auto other=slot<2 ? engine.snapshot_.velCurve : engine.snapshot_.keyCurve;
        const auto oldOther=slot<2 ? beforeVel : beforeKey;
        check(other.x0==oldOther.x0 && other.x1==oldOther.x1,"CC channel affected other curve");
      }
      raw->send(0x92,60,100);raw->send(0xb7,7,50);raw->send(0xb2,64,127);raw->send(0xb2,123,0);
      check(out->messages.empty() && !engine.sustainDown_,"Off-channel notes or unrelated CC leaked into performance");
      check(interpreter.getState().livingNotes.empty(),"Off-channel notes polluted mono state");
      raw->send(0x90,60,100);
      check(!out->messages.empty(),"Note input channel no longer plays");
      check(interpreter.getState().livingNotes.size()==1,"Note interpreter stopped tracking source notes");
      // Even bank, RPN and channel-mode CCs are valid on dedicated channels.
      for(int cc : {0,6,11,32,38,64,98,99,100,101,120,123,127}) {
        engine.snapshot_.curveControls[0].cc=cc;
        engine.curveControlValues_[0]=-1;
        raw->send(0xb2,cc,60);raw->send(0xb2,cc,61);
        check(engine.curveControlValues_[0]==61,"Reserved CC filtered on dedicated control channel");
      }
      auto mappings=defaultCurveCcAssignments();
      for(int slot=0;slot<4;++slot) mappings[slot]={7,0,127,slot+2};
      sanitizeCurveCcAssignments(mappings,0);
      for(const auto& mapping:mappings) check(mapping.cc==7,"Same CC on distinct channels rejected");
      mappings[3].channel=2;sanitizeCurveCcAssignments(mappings,0);
      check(mappings[3].cc==-1,"Duplicate channel/CC pair accepted");
      mappings[0].channel=0;sanitizeCurveCcAssignments(mappings,0);
      check(mappings[0].cc==-1,"Reserved CC accepted on note channel");
      // The shared interpreter retains its previous behavior unless opted in.
      auto legacyInput=std::make_unique<InjectedMidiIn>();auto* legacyRaw=legacyInput.get();
      MidiIn_MonoInterpreter legacy(std::move(legacyInput),{});
      int delivered=0;legacy.setCallback([&](const MidiInEvent&) { ++delivered; });
      legacyRaw->send(0xb2,7,90);check(delivered==0,"Default interpreter now accepts other channels");
    }
    std::cout << "All MIDI routing regressions passed\n";
  }
};
int main(int argc, char** argv) {
  QCoreApplication app(argc,argv);
  try { MidiEngineRoutingTest::run(); }
  catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
