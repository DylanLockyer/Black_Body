#include "SegmentedCalibration.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

SegmentedCalibration::SegmentedCalibration()
    : _numSegments(0)
{
    memset(_segments, 0, sizeof(_segments));
}

void SegmentedCalibration::reset()
{
    memset(_segments, 0, sizeof(_segments));
    _numSegments = 0;
}

bool SegmentedCalibration::setSegments(const CalSegment *segments, int numSegments)
{
    if (segments == nullptr) return false;
    if (numSegments < 1 || numSegments > CAL_MAX_SEGMENTS) return false;
    for (int i = 0; i < numSegments; i++) {
        if (!(segments[i].rMax > segments[i].rMin)) return false;
    }

    memcpy(_segments, segments, numSegments * sizeof(CalSegment));
    _numSegments = numSegments;
    return true;
}

float SegmentedCalibration::evaluate(float resistanceOhms) const
{
    if (_numSegments == 0 || resistanceOhms <= 0.0f) return NAN;

    for (int i = 0; i < _numSegments; i++) {
        const CalSegment &s = _segments[i];
        if (resistanceOhms >= s.rMin && resistanceOhms < s.rMax) {
            double x = log((double)resistanceOhms);
            double lnT = ((double)s.c0 * x + (double)s.c1) * x + (double)s.c2;
            lnT = lnT * x + (double)s.c3;
            return (float)exp(lnT);
        }
    }
    return NAN; // resistance doesn't fall inside any configured segment
}

size_t SegmentedCalibration::serialize(char *buffer, size_t bufferLen) const
{
    if (buffer == nullptr || bufferLen == 0) return 0;
    if (_numSegments == 0) {
        buffer[0] = '\0';
        return 0;
    }

    int written = snprintf(buffer, bufferLen, "%d", _numSegments);
    if (written < 0 || (size_t)written >= bufferLen) return 0;

    for (int i = 0; i < _numSegments; i++) {
        const CalSegment &s = _segments[i];
        int n = snprintf(buffer + written, bufferLen - written, "\n%.9g,%.9g,%.9g,%.9g,%.9g,%.9g",
                          s.rMin, s.rMax, s.c0, s.c1, s.c2, s.c3);
        if (n < 0 || (size_t)(written + n) >= bufferLen) return 0;
        written += n;
    }
    return (size_t)written;
}

bool SegmentedCalibration::deserialize(const char *buffer)
{
    if (buffer == nullptr) return false;

    int numSegments = -1;
    int consumed = 0;
    if (sscanf(buffer, "%d%n", &numSegments, &consumed) != 1) return false;
    if (numSegments < 1 || numSegments > CAL_MAX_SEGMENTS) return false;

    const char *p = buffer + consumed;
    CalSegment segments[CAL_MAX_SEGMENTS];
    for (int i = 0; i < numSegments; i++) {
        float rMin, rMax, c0, c1, c2, c3;
        int lineConsumed = 0;
        // Leading whitespace/newlines in the format string skip over the
        // '\n' left before each segment's line.
        if (sscanf(p, " %f,%f,%f,%f,%f,%f%n", &rMin, &rMax, &c0, &c1, &c2, &c3, &lineConsumed) != 6) {
            return false;
        }
        segments[i] = { rMin, rMax, c0, c1, c2, c3 };
        p += lineConsumed;
    }

    return setSegments(segments, numSegments);
}
