#ifndef SEGMENTED_CALIBRATION_H
#define SEGMENTED_CALIBRATION_H

#include <stddef.h>

// Maximum number of resistance segments a calibration profile can hold.
#define CAL_MAX_SEGMENTS 8

// One piecewise segment of an R->T calibration curve. Valid for resistance
// readings in [rMin, rMax). The mapping itself is a cubic fit in the log
// domain (standard practice for cryogenic Cernox/GR/RuOx-style sensors,
// which span several decades of resistance):
//   ln(T) = c0*ln(R)^3 + c1*ln(R)^2 + c2*ln(R) + c3
struct CalSegment {
    float rMin;
    float rMax;
    float c0, c1, c2, c3;
};

// Converts resistance to temperature via a small set of user-supplied
// piecewise cubic segments, instead of an on-device least-squares fit from
// raw calibration points. The 4 coefficients and the [rMin, rMax) range of
// every segment are set directly (e.g. transcribed from a manufacturer
// data-sheet interpolation) and are fully adjustable from the web UI, then
// persisted per calibration profile. This class does no fitting of its
// own — it only stores, evaluates, and (de)serializes a segment set.
class SegmentedCalibration {
public:
    SegmentedCalibration();

    // Clears any loaded segment set.
    void reset();

    // Replaces the active segment set. Returns false (and leaves the
    // previous set untouched) if numSegments is out of [1, CAL_MAX_SEGMENTS]
    // or any segment has rMax <= rMin.
    bool setSegments(const CalSegment *segments, int numSegments);

    // Evaluates T (Kelvin) for a given resistance (ohms), using whichever
    // segment's [rMin, rMax) range contains it (first match wins). Returns
    // NAN if no segment matches, no segment set is loaded, or
    // resistanceOhms <= 0.
    float evaluate(float resistanceOhms) const;

    bool isValid() const { return _numSegments > 0; }
    int numSegments() const { return _numSegments; }
    const CalSegment *segments() const { return _segments; }

    // Serializes to a compact, human-readable text format, safe to write
    // straight to a LittleFS file:
    //   "<numSegments>\n<rMin>,<rMax>,<c0>,<c1>,<c2>,<c3>\n..."
    // Returns the number of characters written (not including the null
    // terminator), or 0 if bufferLen is too small or nothing is loaded.
    size_t serialize(char *buffer, size_t bufferLen) const;

    // Parses a string produced by serialize(). Returns true on success.
    bool deserialize(const char *buffer);

private:
    CalSegment _segments[CAL_MAX_SEGMENTS];
    int _numSegments;
};

#endif // SEGMENTED_CALIBRATION_H
