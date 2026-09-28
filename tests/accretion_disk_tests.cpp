// Unit tests for physics/AccretionDisk and photon impact parameters

#include "physics/AccretionDisk.hpp"
#include "physics/Schwarzschild.hpp"

#include <cmath>
#include <cstdio>

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

void impactParameterMatchesObserverFormula() {
    // b = r sin(alpha) / sqrt(1 - rs / r) for a hovering observer
    const Schwarzschild hole(1.0);
    const double r = 12.0;
    const double alpha = 0.3;
    const glm::dvec3 pos(0.0, 0.0, r);
    const glm::dvec3 local(std::sin(alpha), 0.0, -std::cos(alpha));
    const double b =
        hole.impactParameter(pos, hole.coordinateDirection(pos, local));
    const double expected = r * std::sin(alpha) / std::sqrt(1.0 - 1.0 / r);
    check(std::abs(b - expected) < 1e-12,
          "impact parameter matches the hovering-observer formula");
}

void impactParameterFarAway() {
    const Schwarzschild hole(1.0);
    const double b = hole.impactParameter(glm::dvec3(-1e6, 3.0, 0.0),
                                          glm::dvec3(1.0, 0.0, 0.0));
    check(std::abs(b - 3.0) < 1e-6, "far away, b is the offset of the line");
    check(hole.impactParameter(glm::dvec3(5.0, 0.0, 0.0),
                               glm::dvec3(-1.0, 0.0, 0.0)) == 0.0,
          "radial ray has b = 0");
}

void temperatureProfile() {
    const AccretionDisk disk(1.0, 3.0, 12.0);
    const double peak = 49.0 / 36.0 * 3.0;
    check(disk.temperature(3.0) == 0.0, "no emission at the inner edge");
    check(std::abs(disk.temperature(peak) - 1.0) < 1e-12,
          "peak temperature 1 at 49/36 r_in");
    check(disk.temperature(peak * 0.9) < 1.0 &&
              disk.temperature(peak * 1.1) < 1.0,
          "temperature falls on both sides of the peak");
    check(disk.temperature(6.0) > disk.temperature(9.0) &&
              disk.temperature(9.0) > disk.temperature(12.0),
          "temperature falls outward");
}

void redshiftLimits() {
    const AccretionDisk disk(1.0, 3.0, 12.0);
    const double far = 1e12;

    check(std::abs(disk.redshiftFactor(6.0, 0.0, far) - std::sqrt(0.75)) < 1e-9,
          "sideways emission: g = sqrt(1 - 1.5 rs / r)");
    // Observer at 4 rs: blueshift 1 / sqrt(0.75) cancels the emitter's
    check(std::abs(disk.redshiftFactor(6.0, 0.0, 4.0) - 1.0) < 1e-9,
          "hovering observer sees a blueshift of 1 / sqrt(1 - rs / r)");

    const double b = 5.0;
    check(disk.redshiftFactor(6.0, b, far) > 1.0 &&
              disk.redshiftFactor(6.0, -b, far) < std::sqrt(0.75),
          "approaching side blueshifted, receding side redshifted");
}

void weakFieldDoppler() {
    // Far out: special-relativistic Doppler 1 / (gamma (1 -+ beta))
    const double r = 1e4;
    const AccretionDisk disk(1.0, 3.0, 2.0 * r);
    const double beta = std::sqrt(0.5 / r);
    const double gamma = 1.0 / std::sqrt(1.0 - beta * beta);
    const double toward = 1.0 / (gamma * (1.0 - beta));
    const double away = 1.0 / (gamma * (1.0 + beta));
    check(std::abs(disk.redshiftFactor(r, r, 1e12) / toward - 1.0) < 2e-4 &&
              std::abs(disk.redshiftFactor(r, -r, 1e12) / away - 1.0) < 2e-4,
          "weak field reduces to special-relativistic Doppler");
}

} // namespace

int main() {
    impactParameterMatchesObserverFormula();
    impactParameterFarAway();
    temperatureProfile();
    redshiftLimits();
    weakFieldDoppler();

    if (failures == 0) {
        std::printf("All accretion disk tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
