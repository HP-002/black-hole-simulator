#include "physics/AccretionDisk.hpp"

#include "physics/Kerr.hpp"

#include <cmath>
#include <limits>

namespace {

constexpr double kPi = 3.14159265358979323846;
// Height changes are limited this far beyond the disk's radii, so a step
// can't hop over a corner of the slab
constexpr double kRadialMargin = 0.1;

// Peak of flux, from inner up: log-spaced scan, then golden-section search
template <typename Flux> double peak(const Flux& flux, double inner) {
    double best = inner;
    double bestFlux = 0.0;
    const int samples = 400;
    const double span = std::log(40.0);
    for (int i = 1; i <= samples; ++i) {
        const double r = inner * std::exp(span * i / samples);
        if (flux(r) > bestFlux) {
            bestFlux = flux(r);
            best = r;
        }
    }
    const double step = std::exp(span / samples);
    double lo = best / step;
    double hi = best * step;
    const double golden = 0.5 * (std::sqrt(5.0) - 1.0);
    for (int i = 0; i < 100; ++i) {
        const double a = hi - golden * (hi - lo);
        const double b = lo + golden * (hi - lo);
        if (flux(a) > flux(b)) {
            hi = b;
        } else {
            lo = a;
        }
    }
    return flux(0.5 * (lo + hi));
}

} // namespace

double AccretionDisk::peakFluxWithoutSpin() {
    // flux depends only on r / M: one number for every hole
    static const double scale = [] {
        AccretionDisk disk(2.0, 0.0, 1.0, 1.0, 1.0, false);
        return peak([&disk](double r) { return disk.flux(r); },
                    disk.innerRadius());
    }();
    return scale;
}

void RayTransfer::add(double source, double opticalDepth) {
    const double passed = std::exp(-opticalDepth);
    radiance += transmittance * (1.0 - passed) * source;
    transmittance *= passed;
    if (transmittance < kOpaqueTransmittance) {
        radiance += transmittance * source;
        transmittance = 0.0;
    }
}

AccretionDisk::AccretionDisk(double rs, double spin, double outerRadius,
                             double thickness, double opticalDepth)
    : AccretionDisk(rs, spin, outerRadius, thickness, opticalDepth, true) {}

AccretionDisk::AccretionDisk(double rs, double spin, double outerRadius,
                             double thickness, double opticalDepth,
                             bool scaled)
    : mass_(0.5 * rs), spin_(spin), inner_(Kerr(rs, spin).iscoRadius(true)),
      outer_(outerRadius), thickness_(thickness), opticalDepth_(opticalDepth),
      x0_(std::sqrt(inner_ / mass_)) {
    // Roots of x^3 - 3x + 2 spin (Page & Thorne 1974)
    const double third = std::acos(spin) / 3.0;
    roots_ = glm::dvec3(2.0 * std::cos(third - kPi / 3.0),
                        2.0 * std::cos(third + kPi / 3.0),
                        -2.0 * std::cos(third));
    for (int i = 0; i < 3; ++i) {
        const double xi = roots_[i];
        const double xj = roots_[(i + 1) % 3];
        const double xk = roots_[(i + 2) % 3];
        // x_i -> 0 as spin -> 0, where the term -> spin / 6 -> 0
        coefficients_[i] = std::abs(xi) < 1e-9
                               ? 0.0
                               : 3.0 * (xi - spin) * (xi - spin) /
                                     (xi * (xi - xj) * (xi - xk));
    }

    fluxScale_ = scaled ? peakFluxWithoutSpin() : 1.0;
}

bool AccretionDisk::inSlab(double r, double y) const {
    return contains(r) && std::abs(y) < kSlabHalfHeight * scaleHeight(r);
}

double AccretionDisk::absorption(double r, double y) const {
    const double h = scaleHeight(r);
    const double z = y / h;
    return opticalDepth_ * std::exp(-0.5 * z * z) / (std::sqrt(2.0 * kPi) * h);
}

double AccretionDisk::slabOpticalDepth() const {
    return opticalDepth_ * std::erf(kSlabHalfHeight / std::sqrt(2.0));
}

double AccretionDisk::maxHeightChange(double r, double y,
                                      double velocityY) const {
    const double unlimited = std::numeric_limits<double>::infinity();
    if (r < (1.0 - kRadialMargin) * inner_ ||
        r > (1.0 + kRadialMargin) * outer_) {
        return unlimited;
    }
    const double h = scaleHeight(r);
    const double outside = std::abs(y) - kSlabHalfHeight * h;
    if (outside < 0.0) {
        return kHeightStep * h;
    }
    if (y * velocityY < 0.0) {
        return outside + kEntryDepth * h;
    }
    return unlimited;
}

double AccretionDisk::angularVelocity(double r) const {
    const double sqrtM = std::sqrt(mass_);
    return sqrtM / (r * std::sqrt(r) + spin_ * mass_ * sqrtM);
}

double AccretionDisk::orbitTimeRate(double r) const {
    // Bardeen, Press & Teukolsky 1972
    const double sqrtR = std::sqrt(r);
    const double sqrtM = std::sqrt(mass_);
    const double aSqrtM = spin_ * mass_ * sqrtM;
    return (r * sqrtR + aSqrtM) /
           (std::pow(r, 0.75) *
            std::sqrt(r * sqrtR - 3.0 * mass_ * sqrtR + 2.0 * aSqrtM));
}

double AccretionDisk::flux(double r) const {
    if (r <= inner_) {
        return 0.0;
    }
    const double x = std::sqrt(r / mass_);
    double bracket = x - x0_ - 1.5 * spin_ * std::log(x / x0_);
    for (int i = 0; i < 3; ++i) {
        bracket -= coefficients_[i] *
                   std::log((x - roots_[i]) / (x0_ - roots_[i]));
    }
    const double x2 = x * x;
    return bracket / (x2 * x2 * (x2 * x - 3.0 * x + 2.0 * spin_));
}

double AccretionDisk::temperature(double r) const {
    return std::pow(flux(r) / fluxScale_, 0.25);
}

double AccretionDisk::redshiftFactor(double r, double lambda,
                                     double cameraEnergy) const {
    // E_emit = -p . u = E u^t (1 - Omega lambda)
    return cameraEnergy /
           (orbitTimeRate(r) * (1.0 - angularVelocity(r) * lambda));
}
