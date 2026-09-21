#ifndef CURVE_FIT_H
#define CURVE_FIT_H

#include <stddef.h>

// Maximum polynomial degree supported. 7 coefficients (degree 6) is more
// than enough headroom for a GR-300-AA style R->T calibration curve while
// keeping the linear-system solve small and fast on an ESP32.
#define CURVEFIT_MAX_DEGREE 6
#define CURVEFIT_MAX_COEFFS (CURVEFIT_MAX_DEGREE + 1)

// Fits T = c0 + c1*x + c2*x^2 + ... + cn*x^n, where x = ln(R).
// A log-domain fit is used because cryogenic resistance-temperature
// sensors (Cernox/GR/RuOx style, like the GR-300-AA) span several
// decades of resistance, so fitting directly against R behaves poorly.
//
// This class is deliberately storage-agnostic: it does not touch
// LittleFS/SPIFFS/SD directly. main.cpp is responsible for reading and
// writing the serialized coefficient string to whatever filesystem it
// uses; that keeps this file reusable and easy to unit-test off-device.
class CurveFit {
public:
    CurveFit();

    // Computes a least-squares polynomial fit from (resistanceOhms[i],
    // temperatureKelvin[i]) pairs. Returns true on success. Fails if
    // numPoints <= degree, degree is out of range, any resistance value
    // is <= 0 (ln undefined), or the normal-equations matrix is singular.
    bool fit(const float *resistanceOhms, const float *temperatureKelvin,
             int numPoints, int degree);

    // Evaluates the fitted curve at a given resistance (ohms).
    // Returns NAN if no valid fit is loaded or resistanceOhms <= 0.
    float evaluate(float resistanceOhms) const;

    bool isValid() const { return _valid; }
    int degree() const { return _degree; }
    const float *coefficients() const { return _coeffs; }

    // Returns the mean-squared residual (in K^2) of the fit against the
    // points it was trained on. Useful for surfacing fit quality to the
    // user. Returns -1 if no valid fit is loaded.
    float lastFitResidual() const { return _residual; }

    // Serializes the fit to a compact, human-readable comma separated
    // string: "<degree>,<c0>,<c1>,...,<cn>\n" — safe to write straight to
    // a text file on LittleFS. Returns the number of characters written
    // (not including the null terminator), or 0 if bufferLen is too small.
    size_t serialize(char *buffer, size_t bufferLen) const;

    // Parses a string produced by serialize(). Returns true on success.
    bool deserialize(const char *buffer);

    // Clears any loaded fit.
    void reset();

private:
    // Solves the n x n linear system (matrix is row-major, n*n entries)
    // for x in matrix * x = rhs. Uses Gaussian elimination with partial
    // pivoting. Returns false if the matrix is (numerically) singular.
    static bool solveLinearSystem(double *matrix, double *rhs, int n,
                                   double *solutionOut);

    float _coeffs[CURVEFIT_MAX_COEFFS];
    int _degree;
    bool _valid;
    float _residual;
};

#endif // CURVE_FIT_H