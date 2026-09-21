#include "CurveFit.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

CurveFit::CurveFit()
    : _degree(0), _valid(false), _residual(-1.0f)
{
    memset(_coeffs, 0, sizeof(_coeffs));
}

void CurveFit::reset()
{
    memset(_coeffs, 0, sizeof(_coeffs));
    _degree = 0;
    _valid = false;
    _residual = -1.0f;
}

bool CurveFit::fit(const float *resistanceOhms, const float *temperatureKelvin,
                    int numPoints, int degree)
{
    if (degree < 1 || degree > CURVEFIT_MAX_DEGREE) return false;
    if (numPoints <= degree) return false;
    if (resistanceOhms == nullptr || temperatureKelvin == nullptr) return false;

    int n = degree + 1; // number of coefficients / normal-equation size

    // Build x = ln(R) for every sample, bail out on any non-positive
    // resistance since ln() is undefined there.
    double *x = new double[numPoints];
    double *y = new double[numPoints];
    for (int i = 0; i < numPoints; i++) {
        if (resistanceOhms[i] <= 0.0f) {
            delete[] x;
            delete[] y;
            return false;
        }
        x[i] = log((double)resistanceOhms[i]);
        y[i] = (double)temperatureKelvin[i];
    }

    // Normal equations for least-squares polynomial fit:
    //   A[j][k] = sum(x^(j+k)),  b[j] = sum(y * x^j)
    double A[CURVEFIT_MAX_COEFFS * CURVEFIT_MAX_COEFFS];
    double b[CURVEFIT_MAX_COEFFS];
    memset(A, 0, sizeof(A));
    memset(b, 0, sizeof(b));

    // Precompute powers of x per sample up to 2*degree.
    double *xp = new double[2 * degree + 1];
    for (int i = 0; i < numPoints; i++) {
        xp[0] = 1.0;
        for (int p = 1; p <= 2 * degree; p++) xp[p] = xp[p - 1] * x[i];

        for (int j = 0; j < n; j++) {
            b[j] += y[i] * xp[j];
            for (int k = 0; k < n; k++) {
                A[j * n + k] += xp[j + k];
            }
        }
    }
    delete[] xp;

    double solution[CURVEFIT_MAX_COEFFS];
    bool ok = solveLinearSystem(A, b, n, solution);
    if (!ok) {
        delete[] x;
        delete[] y;
        return false;
    }

    // Compute mean-squared residual for diagnostics.
    double sqErr = 0.0;
    for (int i = 0; i < numPoints; i++) {
        double xi = 1.0, yFit = 0.0;
        for (int j = 0; j < n; j++) {
            yFit += solution[j] * xi;
            xi *= x[i];
        }
        double err = yFit - y[i];
        sqErr += err * err;
    }

    delete[] x;
    delete[] y;

    for (int j = 0; j < CURVEFIT_MAX_COEFFS; j++) {
        _coeffs[j] = (j < n) ? (float)solution[j] : 0.0f;
    }
    _degree = degree;
    _residual = (float)(sqErr / numPoints);
    _valid = true;
    return true;
}

bool CurveFit::solveLinearSystem(double *matrix, double *rhs, int n,
                                  double *solutionOut)
{
    // Gaussian elimination with partial pivoting, matrix is row-major n*n.
    for (int col = 0; col < n; col++) {
        // Find pivot row.
        int pivotRow = col;
        double pivotVal = fabs(matrix[col * n + col]);
        for (int row = col + 1; row < n; row++) {
            double v = fabs(matrix[row * n + col]);
            if (v > pivotVal) {
                pivotVal = v;
                pivotRow = row;
            }
        }
        if (pivotVal < 1e-12) return false; // singular / near-singular

        if (pivotRow != col) {
            for (int k = 0; k < n; k++) {
                double tmp = matrix[col * n + k];
                matrix[col * n + k] = matrix[pivotRow * n + k];
                matrix[pivotRow * n + k] = tmp;
            }
            double tmp = rhs[col];
            rhs[col] = rhs[pivotRow];
            rhs[pivotRow] = tmp;
        }

        // Eliminate below.
        for (int row = col + 1; row < n; row++) {
            double factor = matrix[row * n + col] / matrix[col * n + col];
            if (factor == 0.0) continue;
            for (int k = col; k < n; k++) {
                matrix[row * n + k] -= factor * matrix[col * n + k];
            }
            rhs[row] -= factor * rhs[col];
        }
    }

    // Back substitution.
    for (int row = n - 1; row >= 0; row--) {
        double sum = rhs[row];
        for (int k = row + 1; k < n; k++) {
            sum -= matrix[row * n + k] * solutionOut[k];
        }
        solutionOut[row] = sum / matrix[row * n + row];
    }
    return true;
}

float CurveFit::evaluate(float resistanceOhms) const
{
    if (!_valid || resistanceOhms <= 0.0f) return NAN;
    double x = log((double)resistanceOhms);
    double xi = 1.0, result = 0.0;
    for (int j = 0; j <= _degree; j++) {
        result += _coeffs[j] * xi;
        xi *= x;
    }
    return (float)result;
}

size_t CurveFit::serialize(char *buffer, size_t bufferLen) const
{
    if (buffer == nullptr || bufferLen == 0) return 0;
    if (!_valid) {
        buffer[0] = '\0';
        return 0;
    }

    int written = snprintf(buffer, bufferLen, "%d", _degree);
    if (written < 0 || (size_t)written >= bufferLen) return 0;

    for (int j = 0; j <= _degree; j++) {
        int n = snprintf(buffer + written, bufferLen - written, ",%.9g", _coeffs[j]);
        if (n < 0 || (size_t)(written + n) >= bufferLen) return 0;
        written += n;
    }
    return (size_t)written;
}

bool CurveFit::deserialize(const char *buffer)
{
    if (buffer == nullptr) return false;

    int degree = -1;
    int consumed = 0;
    if (sscanf(buffer, "%d%n", &degree, &consumed) != 1) return false;
    if (degree < 1 || degree > CURVEFIT_MAX_DEGREE) return false;

    const char *p = buffer + consumed;
    float coeffs[CURVEFIT_MAX_COEFFS];
    for (int j = 0; j <= degree; j++) {
        while (*p == ',' || *p == ' ') p++;
        char *endPtr = nullptr;
        coeffs[j] = strtof(p, &endPtr);
        if (endPtr == p) return false; // no number parsed
        p = endPtr;
    }

    for (int j = 0; j < CURVEFIT_MAX_COEFFS; j++) {
        _coeffs[j] = (j <= degree) ? coeffs[j] : 0.0f;
    }
    _degree = degree;
    _valid = true;
    _residual = -1.0f; // unknown after reload; not recomputed
    return true;
}