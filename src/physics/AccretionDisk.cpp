#include "physics/AccretionDisk.hpp"

#include <cmath>

namespace {

double profile(double r, double inner) {
    const double x = inner / r;
    return std::pow(x, 0.75) * std::pow(1.0 - std::sqrt(x), 0.25);
}

} // namespace

AccretionDisk::AccretionDisk(double rs, double innerRadius, double outerRadius)
    : rs_(rs), inner_(innerRadius), outer_(outerRadius) {}

double AccretionDisk::temperature(double r) const {
    if (r <= inner_) {
        return 0.0;
    }
    // Peak at r = (49 / 36) r_in
    return profile(r, inner_) / profile(49.0 / 36.0 * inner_, inner_);
}

double AccretionDisk::redshiftFactor(double r, double lambda,
                                     double observerRadius) const {
    // Keplerian circular orbit: Omega = sqrt(M / r^3), u^t = 1/sqrt(1 - 3M/r)
    const double omega = std::sqrt(rs_ / (2.0 * r * r * r));
    const double emitter = std::sqrt(1.0 - 1.5 * rs_ / r);
    const double observer = std::sqrt(1.0 - rs_ / observerRadius);
    return emitter / (observer * (1.0 - omega * lambda));
}
