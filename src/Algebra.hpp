// algebraic_music.hpp
// Modern C++17 implementation of Giammarco Volta's algebraic representation
// of notes and intervals.
//
// Core assumptions:
//   - C0 = 0
//   - ascending perfect fifth = +1
//   - ascending octave = +84
//   - period = 48 octaves = 48 * 84 = 4032
//
// Notes and intervals live in the same cyclic numeric domain Z_4032,
// but they are represented by distinct C++ types.
// This makes the musically meaningful operations explicit:
//
//   Note     - Note     = Interval
//   Note     + Interval = Note
//   Note     - Interval = Note
//   Interval + Interval = Interval
//   Interval - Interval = Interval
//
// while Note + Note is intentionally not defined.

#include <array>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

namespace music {

namespace detail {

static constexpr int Fifth   = 1;
static constexpr int Octave  = 84;
static constexpr int Octaves = 48;
static constexpr int Period  = Octave * Octaves; // 4032
static constexpr int ChromaticSemitoneUp = 7 * Fifth - 4 * Octave; // -329
static constexpr int DiatonicSemitoneUp  = -5 * Fifth + 3 * Octave; // 247
static constexpr int WholeToneUp         = 2 * Fifth - Octave;      // -82

static constexpr int positiveMod(int a, int m) {
    return ((a % m) + m) % m;
}

static constexpr int centeredMod(int a, int m) {
    return positiveMod(a + m / 2, m) - m / 2;
}

static constexpr int normalize(int v) {
    return positiveMod(v, Period);
}

static constexpr int floorDiv(int a, int b) {
    int q = a / b;
    int r = a % b;
    if (r != 0 && ((r > 0) != (b > 0))) {
        --q;
    }
    return q;
}

} // namespace detail

namespace pc
{
    inline int mod12(int x) {
        int r = x % 12;
        return (r < 0) ? r + 12 : r;
    }

    // Fifth-order <-> chromatic order conversion
	// Converts between fifth-based pitch class ordering and standard chromatic ordering.
	// The mapping is self-inverse because 7 is its own inverse modulo 12:
	// 7 * 7 ≡ 1 (mod 12).
    inline int reorder(int x) {
        return mod12(7 * x);
    }
}

class Interval;
class Note;

class PitchSpelling {
public:
    enum class Letter : std::uint8_t {
        C, D, E, F, G, A, B
    };

    static constexpr int fifthDegree(int value) {
        return detail::positiveMod(value, 7);
    }

    static constexpr int pitchClass(int value) {
        return detail::positiveMod(value, 12);
    }

    static constexpr int octaveMod48(int value) {
        return detail::positiveMod(detail::floorDiv(7 * detail::normalize(value), 12), detail::Octaves);
    }

    static constexpr int octaveCentered(int value) {
        return detail::centeredMod(octaveMod48(value), detail::Octaves);
    }

    static constexpr Letter letter(int value) {
        // fifthDegree: 0=C, 1=G, 2=D, 3=A, 4=E, 5=B, 6=F
        constexpr std::array<Letter, 7> table = {
            Letter::C, Letter::G, Letter::D, Letter::A,
            Letter::E, Letter::B, Letter::F
        };
        return table[static_cast<std::size_t>(fifthDegree(value))];
    }

    static constexpr int accidental(int value) {
        const int pc = pitchClass(value);
        const int natural = naturalPitchClass(letter(value));
        return centeredMod12Accidental(pc - natural);
    }

    static std::string noteName(int value) {
        std::ostringstream os;
        os << letterName(letter(value))
           << accidentalString(accidental(value))
           << octaveMod48(value);
        return os.str();
    }

    static std::string intervalName(int value) {
        // Minimal placeholder: interval naming is more delicate than note naming.
        // For now expose the signed algebraic value.
        std::ostringstream os;
        os << detail::centeredMod(detail::normalize(value), detail::Period);
        return os.str();
    }

    static std::string accidentalString(int a) {
        if (a == 0) {
            return "";
        }

        std::string s;
        if (a > 0) {
            for (int i = 0; i < a; ++i) {
                s += '#';
            }
        } else {
            for (int i = 0; i < -a; ++i) {
                s += 'b';
            }
        }
        return s;
    }

    static const char* letterName(Letter l) {
        switch (l) {
            case Letter::C: return "C";
            case Letter::D: return "D";
            case Letter::E: return "E";
            case Letter::F: return "F";
            case Letter::G: return "G";
            case Letter::A: return "A";
            case Letter::B: return "B";
        }
        return "?";
    }

    static constexpr int valueFromSpelling(
        Letter letter,
        int accidental,
        int octave)
    {
        return detail::normalize(
            naturalValueAtOctave0(letter)
            + accidental * detail::ChromaticSemitoneUp
            + octave * detail::Octave
        );
    }

private:
    static constexpr int naturalPitchClass(Letter l) {
        switch (l) {
            case Letter::C: return 0;
            case Letter::D: return 2;
            case Letter::E: return 4;
            case Letter::F: return 5;
            case Letter::G: return 7;
            case Letter::A: return 9;
            case Letter::B: return 11;
        }
        return 0;
    }

    static constexpr int centeredMod12Accidental(int x) {
        // Range [-6, +5].
        // Change this if you prefer +6 over -6 at the enharmonic boundary.
        return detail::positiveMod(x + 6, 12) - 6;
    }

    static constexpr int naturalValueAtOctave0(Letter l) {
        switch (l) {
            case Letter::C: return 0;
            case Letter::D: return detail::WholeToneUp;
            case Letter::E: return 2 * detail::WholeToneUp;
            case Letter::F: return -detail::Fifth + detail::Octave;
            case Letter::G: return detail::Fifth;
            case Letter::A: return 3 * detail::Fifth - detail::Octave;
            case Letter::B: return 5 * detail::Fifth - 2 * detail::Octave;
        }
        return 0;
    }
};

class Interval {
public:
    static constexpr int Fifth      = detail::Fifth;
    static constexpr int Octave     = detail::Octave;
    static constexpr int Octaves    = detail::Octaves;
    static constexpr int Period     = detail::Period;

    static constexpr int ChromaticSemitoneUp = detail::ChromaticSemitoneUp;
    static constexpr int DiatonicSemitoneUp  = detail::DiatonicSemitoneUp;
    static constexpr int WholeToneUp         = detail::WholeToneUp;

    constexpr Interval() : value_(0) {}
    explicit constexpr Interval(int v) : value_(detail::normalize(v)) {}

    static constexpr Interval raw(int v) {
        return Interval(v);
    }

    constexpr int value() const {
        return value_;
    }

    constexpr int signedValue() const {
        return detail::centeredMod(value_, Period);
    }

    constexpr Interval operator+(Interval rhs) const {
        return Interval(value_ + rhs.value_);
    }

    constexpr Interval operator-(Interval rhs) const {
        return Interval(value_ - rhs.value_);
    }

    constexpr Interval operator-() const {
        return Interval(-value_);
    }

    Interval& operator+=(Interval rhs) {
        value_ = detail::normalize(value_ + rhs.value_);
        return *this;
    }

    Interval& operator-=(Interval rhs) {
        value_ = detail::normalize(value_ - rhs.value_);
        return *this;
    }

    constexpr bool operator==(Interval rhs) const {
        return value_ == rhs.value_;
    }

    constexpr bool operator!=(Interval rhs) const {
        return !(*this == rhs);
    }

    std::string name() const {
        return PitchSpelling::intervalName(value_);
    }

    std::string debugString() const {
        std::ostringstream os;
        os << "Interval(" << signedValue() << ")"
           << " [value=" << value_
           << ", fifthDegree=" << PitchSpelling::fifthDegree(value_)
           << ", pitchClass=" << PitchSpelling::pitchClass(value_)
           << ", octaveCentered=" << PitchSpelling::octaveCentered(value_)
           << "]";
        return os.str();
    }

private:
    int value_;
};

class Note {
public:
    static constexpr int Period = detail::Period;

    constexpr Note() : value_(0) {}
    explicit constexpr Note(int v) : value_(detail::normalize(v)) {}

    static constexpr Note raw(int v) {
        return Note(v);
    }

    static constexpr Note fromSpelling(
        PitchSpelling::Letter letter,
        int accidental,
        int octave)
    {
        return Note(PitchSpelling::valueFromSpelling(letter, accidental, octave));
    }

    constexpr int value() const {
        return value_;
    }

    constexpr int signedValue() const {
        return detail::centeredMod(value_, Period);
    }

    constexpr Note operator+(Interval rhs) const {
        return Note(value_ + rhs.value());
    }

    constexpr Note operator-(Interval rhs) const {
        return Note(value_ - rhs.value());
    }

    constexpr Interval operator-(Note rhs) const {
        return Interval(value_ - rhs.value_);
    }

    Note& operator+=(Interval rhs) {
        value_ = detail::normalize(value_ + rhs.value());
        return *this;
    }

    Note& operator-=(Interval rhs) {
        value_ = detail::normalize(value_ - rhs.value());
        return *this;
    }

    constexpr bool operator==(Note rhs) const {
        return value_ == rhs.value_;
    }

    constexpr bool operator!=(Note rhs) const {
        return !(*this == rhs);
    }

    constexpr int fifthDegree() const {
        return PitchSpelling::fifthDegree(value_);
    }

    constexpr int pitchClass() const {
        return PitchSpelling::pitchClass(value_);
    }

    constexpr int octaveMod48() const {
        return PitchSpelling::octaveMod48(value_);
    }

    constexpr PitchSpelling::Letter letter() const {
        return PitchSpelling::letter(value_);
    }

    constexpr int accidental() const {
        return PitchSpelling::accidental(value_);
    }

    std::string name() const {
        return PitchSpelling::noteName(value_);
    }

    std::string debugString() const {
        std::ostringstream os;
        os << name()
           << " [value=" << value_
           << ", signed=" << signedValue()
           << ", fifthDegree=" << fifthDegree()
           << ", pitchClass=" << pitchClass()
           << ", octave=" << octaveMod48()
           << ", accidental=" << accidental()
           << "]";
        return os.str();
    }

private:
    int value_;
};

inline constexpr Note operator+(Interval lhs, Note rhs) {
    return rhs + lhs;
}

inline std::ostream& operator<<(std::ostream& os, Note n) {
    return os << n.name();
}

inline std::ostream& operator<<(std::ostream& os, Interval i) {
    return os << i.name();
}

// Named constructors for clarity.
inline constexpr Note C0() {
    return Note::raw(0);
}

inline constexpr Interval fifthUp() {
    return Interval::raw(Interval::Fifth);
}

inline constexpr Interval octaveUp() {
    return Interval::raw(Interval::Octave);
}

inline constexpr Interval chromaticSemitoneUp() {
    return Interval::raw(Interval::ChromaticSemitoneUp);
}

inline constexpr Interval diatonicSemitoneUp() {
    return Interval::raw(Interval::DiatonicSemitoneUp);
}

inline constexpr Interval wholeToneUp() {
    return Interval::raw(Interval::WholeToneUp);
}

} // namespace music