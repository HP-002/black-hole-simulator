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
    const AccretionDisk disk(1.0, 0.0, 12.0);
    check(disk.innerRadius() == 3.0, "no spin: disk starts at 3 rs");
    check(disk.temperature(3.0) == 0.0, "no emission at the inner edge");

    double peakR = 0.0;
    double peakT = 0.0;
    for (double r = 3.0; r < 12.0; r += 0.001) {
        if (disk.temperature(r) > peakT) {
            peakT = disk.temperature(r);
            peakR = r;
        }
    }
    check(peakT <= 1.0 + 1e-12 && peakT > 1.0 - 1e-6, "peak temperature 1");
    // Novikov-Thorne, a = 0: flux peaks at r = 9.55 M (Page & Thorne 1974)
    check(std::abs(peakR - 4.775) < 0.01, "no spin: peak at 9.55 M");
    check(disk.temperature(6.0) > disk.temperature(9.0) &&
              disk.temperature(9.0) > disk.temperature(12.0),
          "temperature falls outward");

    // Far out: Newtonian flux 3 M Mdot / (8 pi r^3), T ~ r^(-3/4)
    const AccretionDisk wide(1.0, 0.5, 1e8);
    const double r = 1e7;
    const double m = 0.5;
    check(std::abs(wide.flux(r) / std::pow(m / r, 3.0) - 1.0) < 2e-3,
          "far out the flux is Newtonian");
}

// Kerr circular orbits (Bardeen, Press & Teukolsky 1972), M = 1
double orbitEnergy(double r, double a) {
    const double sr = std::sqrt(r);
    return (r * sr - 2.0 * sr + a) /
           (std::pow(r, 0.75) * std::sqrt(r * sr - 3.0 * sr + 2.0 * a));
}
double orbitAngularMomentum(double r, double a) {
    const double sr = std::sqrt(r);
    return (r * r - 2.0 * a * sr + a * a) /
           (std::pow(r, 0.75) * std::sqrt(r * sr - 3.0 * sr + 2.0 * a));
}
double orbitOmega(double r, double a) { return 1.0 / (r * std::sqrt(r) + a); }

void spinHeatsTheInnerDisk() {
    // Same accretion rate: far out, the inner edge stops mattering
    const AccretionDisk still(2.0, 0.0, 24.0);
    const AccretionDisk fast(2.0, 0.9, 24.0);
    check(std::abs(fast.temperature(2000.0) / still.temperature(2000.0) - 1.0) <
              0.01,
          "far out, spin leaves the temperature alone");
    double hottest = 0.0;
    for (double r = fast.innerRadius(); r < 24.0; r += 0.01) {
        hottest = std::fmax(hottest, fast.temperature(r));
    }
    check(hottest > 1.3, "spin makes the inner disk hotter");
}

void fluxMatchesPageThorneIntegral() {
    // F = -Omega' / (r (E - Omega L)^2) * integral (E - Omega L) L' dr,
    // up to a constant: compare shapes for several spins
    for (double a : {0.0, 0.7, -0.5, 0.998}) {
        const AccretionDisk disk(2.0, a, 100.0); // M = 1
        const double rin = disk.innerRadius();
        const double h = 1e-5;
        const auto d = [h](auto fn, double r) {
            return (fn(r + h) - fn(r - h)) / (2.0 * h);
        };
        const auto lr = [a](double r) { return orbitAngularMomentum(r, a); };
        const auto om = [a](double r) { return orbitOmega(r, a); };
        const auto integrand = [&](double r) {
            return (orbitEnergy(r, a) - om(r) * lr(r)) * d(lr, r);
        };

        double ratio0 = 0.0;
        double worst = 0.0;
        double integral = 0.0;
        double prev = rin;
        for (double r : {1.2, 1.5, 2.0, 4.0, 8.0}) {
            r *= rin;
            const int n = 2000; // Simpson
            const double dr = (r - prev) / n;
            double sum = integrand(prev) + integrand(r);
            for (int i = 1; i < n; ++i) {
                sum += (i % 2 ? 4.0 : 2.0) * integrand(prev + i * dr);
            }
            integral += sum * dr / 3.0;
            prev = r;

            const double e = orbitEnergy(r, a) - om(r) * lr(r);
            const double expected = -d(om, r) / (r * e * e) * integral;
            const double ratio = disk.flux(r) / expected;
            if (ratio0 == 0.0) {
                ratio0 = ratio;
            }
            worst = std::fmax(worst, std::abs(ratio / ratio0 - 1.0));
        }
        check(worst < 1e-5, "flux matches the Page-Thorne integral");
    }
}

void spinMovesTheInnerEdge() {
    check(std::abs(AccretionDisk(2.0, 0.998, 24.0).innerRadius() - 1.237) <
              1e-3,
          "a = 0.998: disk reaches in to 1.237 M");
    check(std::abs(AccretionDisk(2.0, -1.0, 24.0).innerRadius() - 9.0) < 1e-9,
          "hole turning against the disk: inner edge at 9 M");
}

void orbitIsNormalized() {
    // u^t^2 (g_tt + 2 Omega g_tphi + Omega^2 g_phiphi) = -1, equator,
    // Boyer-Lindquist, M = 1
    for (double a : {0.0, 0.9, -0.6}) {
        const AccretionDisk disk(2.0, a, 30.0);
        for (double r : {disk.innerRadius(), 10.0, 25.0}) {
            const double gtt = -(1.0 - 2.0 / r);
            const double gtp = -2.0 * a / r;
            const double gpp = r * r + a * a + 2.0 * a * a / r;
            const double w = disk.angularVelocity(r);
            const double ut = disk.orbitTimeRate(r);
            const double norm = ut * ut * (gtt + 2.0 * w * gtp + w * w * gpp);
            check(std::abs(norm + 1.0) < 1e-9,
                  "gas 4-velocity has unit length");
        }
    }
}

void redshiftLimits() {
    const AccretionDisk disk(1.0, 0.0, 12.0);

    check(std::abs(disk.redshiftFactor(6.0, 0.0, 1.0) - std::sqrt(0.75)) < 1e-9,
          "sideways emission: g = sqrt(1 - 1.5 rs / r)");
    // Observer at 4 rs: blueshift 1 / sqrt(0.75) cancels the emitter's
    const double hovering = 1.0 / std::sqrt(1.0 - 1.0 / 4.0);
    check(std::abs(disk.redshiftFactor(6.0, 0.0, hovering) - 1.0) < 1e-9,
          "hovering observer sees a blueshift of 1 / sqrt(1 - rs / r)");

    const double b = 5.0;
    check(disk.redshiftFactor(6.0, b, 1.0) > 1.0 &&
              disk.redshiftFactor(6.0, -b, 1.0) < std::sqrt(0.75),
          "approaching side blueshifted, receding side redshifted");
}

void weakFieldDoppler() {
    // Far out: special-relativistic Doppler 1 / (gamma (1 -+ beta))
    const double r = 1e4;
    const AccretionDisk disk(1.0, 0.0, 2.0 * r);
    const double beta = std::sqrt(0.5 / r);
    const double gamma = 1.0 / std::sqrt(1.0 - beta * beta);
    const double toward = 1.0 / (gamma * (1.0 - beta));
    const double away = 1.0 / (gamma * (1.0 + beta));
    check(std::abs(disk.redshiftFactor(r, r, 1.0) / toward - 1.0) < 2e-4 &&
              std::abs(disk.redshiftFactor(r, -r, 1.0) / away - 1.0) < 2e-4,
          "weak field reduces to special-relativistic Doppler");
}

void keplerianRotation() {
    const AccretionDisk disk(1.0, 0.0, 12.0);
    // Kepler's third law: Omega^2 r^3 = M = rs / 2
    for (double r : {3.0, 6.0, 12.0}) {
        const double omega = disk.angularVelocity(r);
        check(std::abs(omega * omega * r * r * r - 0.5) < 1e-12,
              "Omega^2 r^3 = M");
    }
    check(disk.angularVelocity(3.0) > disk.angularVelocity(12.0),
          "inner disk turns faster");

    // Omega ~ 1 / rs at fixed r / rs: one pattern in units of rs / c
    const AccretionDisk big(2.0, 0.0, 24.0);
    check(std::abs(big.angularVelocity(12.0) * 2.0 - disk.angularVelocity(6.0)) <
              1e-12,
          "Omega scales as 1 / rs");

    // Spin: Omega = 1 / (r^(3/2) + a), M = 1
    const AccretionDisk spinning(2.0, 0.9, 24.0);
    check(std::abs(spinning.angularVelocity(4.0) - 1.0 / 8.9) < 1e-12,
          "Kerr: Omega = sqrt(M) / (r^(3/2) + a sqrt(M))");
}

} // namespace

int main() {
    impactParameterMatchesObserverFormula();
    impactParameterFarAway();
    temperatureProfile();
    fluxMatchesPageThorneIntegral();
    spinHeatsTheInnerDisk();
    spinMovesTheInnerEdge();
    orbitIsNormalized();
    redshiftLimits();
    weakFieldDoppler();
    keplerianRotation();

    if (failures == 0) {
        std::printf("All accretion disk tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
