#include "CategoryClassifier.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace
{
  std::string toLowerAscii(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(),
      [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
  }

  std::string compactLower(const std::string& text)
  {
    std::string out;

    for (unsigned char ch : text)
    {
      if (std::isalnum(ch))
        out.push_back(static_cast<char>(std::tolower(ch)));
    }

    return out;
  }

  ProgramCategory classifyByGeneralMidiProgram(int program)
  {
    if (program >= 0 && program <= 7)   return ProgramCategory::Piano;
    if (program >= 8 && program <= 15)  return ProgramCategory::ChromaticPercussion;
    if (program >= 16 && program <= 20) return ProgramCategory::Organ;
    if (program >= 21 && program <= 23) return ProgramCategory::Accordion;
    if (program >= 24 && program <= 31) return ProgramCategory::Guitar;
    if (program >= 32 && program <= 39) return ProgramCategory::Bass;
    if (program >= 40 && program <= 47) return ProgramCategory::Strings;
    if (program >= 48 && program <= 55) return ProgramCategory::Ensemble;
    if (program >= 56 && program <= 63) return ProgramCategory::Brass;
    if (program >= 64 && program <= 71) return ProgramCategory::Reed;
    if (program >= 72 && program <= 79) return ProgramCategory::Pipe;
    if (program >= 80 && program <= 87) return ProgramCategory::SynthLead;
    if (program >= 88 && program <= 95) return ProgramCategory::SynthPad;
    if (program >= 96 && program <= 103) return ProgramCategory::SynthEffects;
    if (program >= 104 && program <= 111) return ProgramCategory::Ethnic;
    if (program >= 112 && program <= 119) return ProgramCategory::Percussive;
    if (program >= 120 && program <= 127) return ProgramCategory::SoundEffects;

    return ProgramCategory::Other;
  }

  bool looksLikeGeneralMidiBank(const std::string& name, int msb, int lsb)
  {
    const std::string n = toLowerAscii(name);

    if (msb == 121)
      return true;

    if (n.find("gm") != std::string::npos ||
      n.find("gm2") != std::string::npos ||
      n.find("xg") != std::string::npos ||
      n.find("gs") != std::string::npos ||
      n.find("voice/gm2") != std::string::npos)
    {
      return true;
    }

    return false;
  }

  std::string normalizeForWords(const std::string& text)
  {
    std::string out;
    bool lastWasSpace = true;

    for (unsigned char ch : text)
    {
      if (std::isalnum(ch))
      {
        out.push_back(static_cast<char>(std::tolower(ch)));
        lastWasSpace = false;
      }
      else if (!lastWasSpace)
      {
        out.push_back(' ');
        lastWasSpace = true;
      }
    }

    if (!out.empty() && out.back() == ' ')
      out.pop_back();

    return out;
  }

  bool containsRaw(const std::string& text, const std::string& word)
  {
    return text.find(word) != std::string::npos;
  }

  bool containsAnyRaw(const std::string& text, const std::vector<std::string>& words)
  {
    for (const auto& w : words)
    {
      if (containsRaw(text, w))
        return true;
    }
    return false;
  }

  bool containsPhrase(const std::string& normalizedText, const std::string& phrase)
  {
    const std::string normalizedPhrase = normalizeForWords(phrase);

    if (normalizedPhrase.empty())
      return false;

    const std::string paddedText = " " + normalizedText + " ";
    const std::string paddedPhrase = " " + normalizedPhrase + " ";

    return paddedText.find(paddedPhrase) != std::string::npos;
  }

  bool containsAnyPhrase(const std::string& normalizedText, const std::vector<std::string>& phrases)
  {
    for (const auto& p : phrases)
    {
      if (containsPhrase(normalizedText, p))
        return true;
    }
    return false;
  }

  bool startsWithAny(const std::string& text, const std::vector<std::string>& prefixes)
  {
    for (const auto& p : prefixes)
    {
      if (text.rfind(p, 0) == 0)
        return true;
    }
    return false;
  }

  std::vector<std::string> extractParenthesizedParts(const std::string& text)
  {
    std::vector<std::string> parts;

    size_t pos = 0;

    while (true)
    {
      const size_t open = text.find('(', pos);
      if (open == std::string::npos)
        break;

      const size_t close = text.find(')', open + 1);
      if (close == std::string::npos)
        break;

      if (close > open + 1)
        parts.push_back(text.substr(open + 1, close - open - 1));

      pos = close + 1;
    }

    return parts;
  }

  bool anyParenthesisContains(const std::vector<std::string>& parts, const std::vector<std::string>& phrases)
  {
    for (const auto& part : parts)
    {
      const std::string normalized = normalizeForWords(part);

      if (containsAnyPhrase(normalized, phrases))
        return true;
    }

    return false;
  }

  bool classifyFromParentheses(const std::string& originalName, ProgramCategory& category)
  {
    const auto parts = extractParenthesizedParts(originalName);

    if (parts.empty())
      return false;

    const std::string full = normalizeForWords(originalName);

    // Forte priorità a Synth se è presente insieme ad altre indicazioni.
    if (anyParenthesisContains(parts, { "synth", "syn" }))
    {
      if (containsAnyPhrase(full, { "pad" }))
        category = ProgramCategory::SynthPad;
      else if (containsAnyPhrase(full, { "fx", "sfx", "effect", "effects", "noise" }))
        category = ProgramCategory::SynthEffects;
      else
        category = ProgramCategory::SynthLead;

      return true;
    }

    if (anyParenthesisContains(parts, { "sax", "sax ensemble" }))
    {
      category = ProgramCategory::Reed;
      return true;
    }

    if (anyParenthesisContains(parts, { "brass", "brass ensemble", "horns ensemble" }))
    {
      category = ProgramCategory::Brass;
      return true;
    }

    if (anyParenthesisContains(parts, { "ensemble", "ens", "orchestra", "orch", "choir" }))
    {
      category = ProgramCategory::Ensemble;
      return true;
    }

    if (anyParenthesisContains(parts, { "electric piano", "e piano", "epiano", "tine e piano", "tine", "rhodes", "wurli", "wurlitzer" }))
    {
      category = ProgramCategory::ElectricPiano;
      return true;
    }

    if (anyParenthesisContains(parts, { "piano", "pno" }))
    {
      category = ProgramCategory::Piano;
      return true;
    }

    if (anyParenthesisContains(parts, { "woodwind", "reed", "sax", "clarinet", "oboe", "bassoon" }))
    {
      category = ProgramCategory::Reed;
      return true;
    }

    if (anyParenthesisContains(parts, { "flute", "pipe", "piccolo", "recorder", "whistle" }))
    {
      category = ProgramCategory::Pipe;
      return true;
    }

    if (anyParenthesisContains(parts, { "strings", "string", "violin", "viola", "cello", "contrabass" }))
    {
      category = ProgramCategory::Strings;
      return true;
    }

    if (anyParenthesisContains(parts, { "brass", "trumpet", "trombone", "horn", "tuba" }))
    {
      category = ProgramCategory::Brass;
      return true;
    }

    if (anyParenthesisContains(parts, { "drum", "drums", "drumsound", "percussion", "perc", "rhythm" }))
    {
      category = ProgramCategory::Percussive;
      return true;
    }

    if (anyParenthesisContains(parts, { "guitar", "gtr" }))
    {
      category = ProgramCategory::Guitar;
      return true;
    }

    if (anyParenthesisContains(parts, { "accordion", "accordeon", "accordian", "acc", "harmonica", "harmonika", "bandoneon", "bandneon", "musette" }))
    {
      category = ProgramCategory::Accordion;
      return true;
    }

    if (anyParenthesisContains(parts, { "bass" }))
    {
      category = ProgramCategory::Bass;
      return true;
    }

    return false;
  }
}

std::string removeSquareBracketedParts(std::string text)
{
  std::string out;
  int squareDepth = 0;

  for (char ch : text)
  {
    if (ch == '[')
    {
      ++squareDepth;
      continue;
    }

    if (ch == ']')
    {
      if (squareDepth > 0)
        --squareDepth;
      continue;
    }

    if (squareDepth == 0)
      out.push_back(ch);
  }

  return out;
}

std::string removeBracketedTechnicalTags(std::string text)
{
  std::string out;
  int roundDepth = 0;
  int squareDepth = 0;

  for (char ch : text)
  {
    if (ch == '(')
    {
      ++roundDepth;
      continue;
    }

    if (ch == ')')
    {
      if (roundDepth > 0)
        --roundDepth;
      continue;
    }

    if (ch == '[')
    {
      ++squareDepth;
      continue;
    }

    if (ch == ']')
    {
      if (squareDepth > 0)
        --squareDepth;
      continue;
    }

    if (roundDepth == 0 && squareDepth == 0)
      out.push_back(ch);
  }

  return out;
}

ProgramCategory classifyProgramCategory(const std::string& patchName, uint8_t msb, uint8_t lsb, uint8_t program)
{
  if (msb == 120)
    return ProgramCategory::Percussive;

  ProgramCategory categoryFromParentheses = ProgramCategory::Other;

  const std::string nameWithoutSquareBrackets = removeSquareBracketedParts(patchName);
  const std::string raw = toLowerAscii(nameWithoutSquareBrackets);
  const std::string s = normalizeForWords(nameWithoutSquareBrackets);
  const std::string compact = compactLower(nameWithoutSquareBrackets);

  if (classifyFromParentheses(patchName, categoryFromParentheses))
  {
    return categoryFromParentheses;
  }

  // INIT / INITIAL: program reali, ma non musicalmente classificabili.
  if (containsAnyPhrase(s, {
        "init",
        "initial",
        "init tone",
        "init patch",
        "init rhythm",
        "initial program",
        "initialize",
        "initialized"
    }))
  {
    return ProgramCategory::Other;
  }

  // Prefissi espliciti.
  if (startsWithAny(raw, { "pf:", "pno:", "pn:" }))
    return ProgramCategory::Piano;
  if (startsWithAny(raw, { "ep:", "cp:" }))
    return ProgramCategory::ElectricPiano;
  if (startsWithAny(raw, { "mal:" }))
    return ProgramCategory::ChromaticPercussion;
  if (startsWithAny(raw, { "org:", "or:" }))
    return ProgramCategory::Organ;
  if (startsWithAny(raw, { "acc:" }))
    return ProgramCategory::Accordion;
  if (startsWithAny(raw, { "gtr:", "gt:" }))
    return ProgramCategory::Guitar;
  if (startsWithAny(raw, { "bs:", "bass:" }))
    return ProgramCategory::Bass;
  if (startsWithAny(raw, { "str:", "strs:" }))
    return ProgramCategory::Strings;
  if (startsWithAny(raw, { "ens:", "cho:" }))
    return ProgramCategory::Ensemble;
  if (startsWithAny(raw, { "br:", "brs:" }))
    return ProgramCategory::Brass;
  if (startsWithAny(raw, { "reed:" }))
    return ProgramCategory::Reed;
  if (startsWithAny(raw, { "pipe:", "fl:" }))
    return ProgramCategory::Pipe;
  if (startsWithAny(raw, { "ld:", "lead:" }))
    return ProgramCategory::SynthLead;
  if (startsWithAny(raw, { "pd:", "pad:" }))
    return ProgramCategory::SynthPad;
  if (startsWithAny(raw, { "fx:" }))
    return ProgramCategory::SynthEffects;
  if (startsWithAny(raw, { "eth:" }))
    return ProgramCategory::Ethnic;
  if (startsWithAny(raw, { "perc:", "dr:" }))
    return ProgramCategory::Percussive;
  if (startsWithAny(raw, { "sfx:" }))
    return ProgramCategory::SoundEffects;

  // Eccezioni forti prima di "bass".
  if (containsAnyPhrase(s, { "bassoon", "contra bassoon", "contrabassoon" }))
  {
    return ProgramCategory::Reed;
  }

  if (containsAnyPhrase(s, { "bass clarinet" }) ||
      containsAnyRaw(compact, { "bassclarinet", "bsclarinet", "bssclarinet", "bclarinet" }))
  {
    return ProgramCategory::Reed;
  }

  if (containsAnyPhrase(s, { "contrabass", "contra bass", "double bass", "basses" }))
  {
    return ProgramCategory::Strings;
  }

  if (containsAnyPhrase(s, { "bass drum", "concert bass drum", "orch bass drum" }))
  {
    return ProgramCategory::Percussive;
  }

  if (containsAnyPhrase(s, { "bass and lead", "bass lead" }) ||
      containsAnyRaw(raw, { "bass&lead", "bass/lead", "bass-lead" }))
  {
    return ProgramCategory::SynthLead;
  }

  if (containsAnyPhrase(s, { "electric piano", "e piano", "epiano", "tine piano", "tine e piano", "rhodes", "wurli", "wurlitzer", "cp80", "cp 80", "dx ep" }) ||
      containsAnyRaw(raw, { "e.piano", "e.pno", "el.piano", "el.pno" }))
  {
    return ProgramCategory::ElectricPiano;
  }

  if (containsAnyPhrase(s, { "grand", "piano", "upright", "honky", "honky tonk", "clavichord", "pno", "harpsi", "harpsichord" }) ||
      containsAnyRaw(raw, { "hnky", "hnkytnk" }) ||
      containsAnyRaw(compact, { "honkytonk", }))
  {
    return ProgramCategory::Piano;
  }

  if (containsAnyPhrase(s, { "celesta", "glock", "glockenspiel", "vibe", "vibes", "vibraphone", "marimba", "xylophone", "dulcimer", "kalimba", "timpani" }) ||
      containsAnyRaw(compact, { "musicbox", "tinklebell", "tubularbell", "tubularbells", "synmallet" }))
  {
    return ProgramCategory::ChromaticPercussion;
  }

  if (containsAnyPhrase(s, { "organ", "hammond", "b3", "farfisa", "drawbar", "vox organ", "rotary", "pipe organ" }) ||
      containsAnyRaw(compact, { "voxorgan", "pipeorgan","churchorg", "churchorg1", "churchorg2", "churchorg3", "detunedor" }))
  {
    return ProgramCategory::Organ;
  }

  if (containsAnyPhrase(s, { "accordion", "accordeon", "accordian", "acc bass", "bandoneon", "bandneon", "harmonica", "harmonika", "musette" }) ||
      containsAnyRaw(raw, { "accbass" }) ||
      containsAnyRaw(compact, { "accordion", "accordeon", "accordian", "acrdion", "accbass", "bandoneon", "bandneon", "harmonica", "harmonika", "musette" }))
  {
    return ProgramCategory::Accordion;
  }

  if (containsAnyPhrase(s, { "drum", "drums", "drumkit", "drum kit", "kit", "tom", "snare", "kick", "cymbal", "conga", "bongo", "timpani", "perc", "percussion", "agogo", "taiko" }) ||
      containsAnyRaw(raw, { "drum'n'bass kit", "drumnbass kit", "drum&bass kit" }))
  {
    return ProgramCategory::Percussive;
  }

  if (containsPhrase(s, "set") &&
      containsAnyPhrase(s, { "standard", "room", "power", "brush", "jazz", "analog", "orchestra", "orch" }))
  {
    return ProgramCategory::Percussive;
  }

  if (containsAnyPhrase(s, { "sax ensemble", "sax section", "sax ens", "saxes" }))
  {
    return ProgramCategory::Reed;
  }

  if (containsAnyPhrase(s, { "orchestra hit", "orch hit", "orchestrahit" }) ||
      containsAnyRaw(raw, { "orchestrahit", "orchhit" }))
  {
    return ProgramCategory::Ensemble;
  }

  if (containsAnyPhrase(s, { "orch woodwind", "orchestra woodwind", "woodwind section" }) ||
      containsAnyRaw(raw, { "orchwoodwind", "orchwoodwinds" }))
  {
    return ProgramCategory::Reed;
  }

  if (containsAnyPhrase(s, { "orch" }))
  {
    if (containsAnyPhrase(s, { "flute", "piccolo" }))
      return ProgramCategory::Pipe;

    if (containsAnyPhrase(s, { "clarinet", "oboe", "english horn", "bassoon"}))
      return ProgramCategory::Reed;
  }

  if (containsAnyPhrase(s, { "brass ensemble", "brass section", "brass ens", "brasssection", "brass sect", "horns ensemble" }) ||
      containsAnyRaw(raw, { "brasssection" }))
  {
    return ProgramCategory::Brass;
  }

  if (containsAnyPhrase(s, { "ensemble", "orchestra", "orch", "choir", "aah", "ooh", "voices" }))
    return ProgramCategory::Ensemble;

  if (containsAnyPhrase(s, { "violin", "viola", "cello", "string", "strings", "quartet", "pizz", "pizzicato", "marcato strings", "slow strings", "harp", "harps" }) ||
      containsAnyRaw(raw, { "slowstr", "slwstr" }) ||
      containsAnyRaw(compact, { "synstrings", "synstrings1", "synstrings2", "synstrings3", "pizzicatostr", "tremolostr" }))
  {
    return ProgramCategory::Strings;
  }

  if (containsAnyPhrase(s, { "sax", "soprano sax", "alto sax", "tenor sax", "baritone sax", "clarinet", "clarinets", "oboe", "english horn", "bassoon", "bassoons", "woodwinds", "winds" }) ||
      containsAnyRaw(compact, { "englishhorn", "altosax", "tenorsax", "baritonesax" }))
  {
    return ProgramCategory::Reed;
  }

  if (containsAnyPhrase(s, { "flute", "piccolo", "recorder", "whistle", "pan flute", "shakuhachi", "ocarina", "flutes" }) ||
      containsAnyRaw(compact, { "bottleblow", "panflute" }))
  {
    return ProgramCategory::Pipe;
  }

  if (containsAnyPhrase(s, { "trumpet", "trumpets", "trombone", "trombones", "horn", "horns", "french horn", "brass", "flugel", "flugelhorn", "tuba", "mute trumpet", "brass section" }) ||
      containsAnyRaw(raw, { "frhn", "brss", "trbne", "trmbone" }) ||
      containsAnyRaw(compact, { "mutetrumpet", "frenchhorn", "frenchhorns", "synthbrass", "synthbrass1", "synthbrass2", "synthbrass3", "synthbrass4", "analogbrass", "octsynbrass", "britebone" }))
  {
    return ProgramCategory::Brass;
  }

  // Bass dopo le eccezioni, così Bassoon / Contrabass non vengono catturati.
  if (containsAnyPhrase(s, { "bass", "fretless", "finger bass", "picked bass", "pick bass", "slap bass", "upright bass", "synth bass", "acoustic bass", "electric bass" }) ||
      containsAnyRaw(raw, { "synthbass", "synbass", "fingerbass", "slapbass", "fretlessbs", "fretlessbass" }) ||
      containsAnyRaw(compact, { "acousticbs", "fingeredbs", "pickedbs", "fretlessbs", "synthbs", "beeffmbs", "clavbass", "bsstringslap" }))
  {
    return ProgramCategory::Bass;
  }

  if (containsAnyPhrase(s, { "guitar", "guitars", "gtr", "strat", "tele", "nylon", "steel guitar", "mandolin", "ukulele", "dist guitar", "overdrive guitar", "12 strings", "12 string guitar" }) ||
      containsAnyRaw(raw, { "12strings", "12strngs" }) ||
      containsAnyRaw(compact, { "steelstrgt", "nylonstrgt", "jazzgt", "cleangt", "mutedgt", "overdrivegt", "distortiongt", "gtharmonics", "gtpinch", "feedbackgt", "funkgt", "midtonegt", "distrythmgt" }))
  {
    return ProgramCategory::Guitar;
  }

  if (containsAnyPhrase(s, { "pad", "soft pad", "warm pad", "sweep pad", "halo pad", "atmos pad" }) ||
      containsAnyRaw(compact, { "softpad", "warmpad", "sweeppad", "halopad", "atmospad" }))
  {
    return ProgramCategory::SynthPad;
  }

  if (containsAnyPhrase(s, { "fx", "sfx", "effect", "effects", "atmo", "atmos", "texture", "noise", "sweep fx", "science fiction" }))
  {
    return ProgramCategory::SynthEffects;
  }

  if (containsAnyPhrase(s, { "lead", "synth", "saw", "square", "sync lead", "calliope" }) ||
      containsAnyRaw(raw, { "syncld", "caliop" }))
  {
    return ProgramCategory::SynthLead;
  }

  if (containsAnyPhrase(s, { "sitar", "banjo", "shamisen", "koto", "oud", "tabla", "tanpura", "tambura", "santur", "erhu", "shakuhachi", "kalimba", "fiddle", "shanai", "shenai", "charang", "bag pipe" }) ||
      containsAnyRaw(raw, { "shakhchi" }) ||
      containsAnyRaw(compact, { "bagpipe" }))
  {
    return ProgramCategory::Ethnic;
  }

  if (containsAnyPhrase(s, { "gun", "applause", "telephone", "bird", "rain", "thunder", "helicopter", "seashore", "explosion" }) ||
      containsAnyRaw(compact, { "gunshot", "lasergun", "carengine", "carstop", "doorcreak", "horsegallop", "telephone", "helicopter" }))
  {
    return ProgramCategory::SoundEffects;
  }

  if (looksLikeGeneralMidiBank(patchName, msb, lsb))
    return classifyByGeneralMidiProgram(program);

  return ProgramCategory::Other;
}