// Unit tests for physics/AccretionDisk and photon impact parameters

#include "physics/AccretionDisk.hpp"
#include "physics/Schwarzschild.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <initializer_list>

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
    const AccretionDisk against(2.0, -0.999, 24.0);
    check(std::abs(against.innerRadius() - 9.0) < 0.01,
          "hole turning against the disk: inner edge near 9 M");
    check(std::isfinite(against.flux(12.0)) && against.flux(12.0) > 0.0,
          "near-extreme counter-spin: flux is finite");
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

constexpr double kPi = 3.14159265358979323846;

// Simpson's rule, n even
template <typename Fn> double integrate(const Fn& fn, double a, double b) {
    const int n = 20000;
    const double dx = (b - a) / n;
    double sum = fn(a) + fn(b);
    for (int i = 1; i < n; ++i) {
        sum += (i % 2 ? 4.0 : 2.0) * fn(a + i * dx);
    }
    return sum * dx / 3.0;
}

void verticalOpticalDepth() {
    for (double thickness : {0.01, 0.05, 0.3}) {
        const AccretionDisk disk(1.0, 0.0, 12.0, thickness, 2.5);
        const double r = 6.0;
        const double h = disk.scaleHeight(r);
        check(std::abs(h - thickness * r) < 1e-15, "h = (H / R) r");
        const auto alpha = [&](double y) { return disk.absorption(r, y); };

        // Gaussian of fixed column: rho0 sqrt(2 pi) h = column
        check(std::abs(alpha(0.0) * std::sqrt(2.0 * kPi) * h - 2.5) < 1e-12,
              "midplane absorption times sqrt(2 pi) h is the optical depth");
        check(std::abs(integrate(alpha, -12.0 * h, 12.0 * h) - 2.5) < 1e-9,
              "vertical optical depth is the same for every thickness");
        const double slab = 2.5 * std::erf(3.0 / std::sqrt(2.0));
        check(std::abs(disk.slabOpticalDepth() - slab) < 1e-12 &&
                  std::abs(integrate(alpha, -3.0 * h, 3.0 * h) - slab) < 1e-9,
              "slab |y| < 3h holds erf(3 / sqrt 2) = 99.7% of it");
    }
}

void rayTransfer() {
    RayTransfer clear;
    clear.add(5.0, 0.0);
    check(clear.radiance == 0.0 && clear.transmittance == 1.0,
          "transparent gas neither shines nor dims");

    // Two layers: the front one dims the back one
    RayTransfer two;
    two.add(1.0, 0.5);
    two.add(3.0, 0.5);
    const double t = std::exp(-0.5);
    check(std::abs(two.radiance - ((1.0 - t) + t * (1.0 - t) * 3.0)) < 1e-12 &&
              std::abs(two.transmittance - t * t) < 1e-12,
          "front to back: back layer seen through the front one");

    RayTransfer wall;
    wall.add(0.0, 1.0);
    wall.add(2.0, 4.0); // e^-5 < 1%: rest taken as opaque
    check(wall.opaque() && std::abs(wall.radiance - 2.0 * std::exp(-1.0)) <
                               1e-12,
          "almost opaque: the light still coming through has its source");
}

// March a straight line the way tracer.comp marches a ray: steps of 5% of
// the distance to the hole, shortened near the disk; one sample at the
// middle of each step. Flat space, no spin: r = |p|
template <typename Source>
RayTransfer marchLine(const AccretionDisk& disk, glm::dvec3 start,
                      glm::dvec3 direction, double length,
                      const Source& source) {
    direction = glm::normalize(direction);
    RayTransfer transfer;
    double s = 0.0;
    int steps = 0;
    while (s < length && !transfer.opaque() && ++steps < 100000) {
        const glm::dvec3 p = start + s * direction;
        double ds = 0.05 * glm::length(p);
        if (direction.y != 0.0) {
            ds = std::min(ds, disk.maxHeightChange(glm::length(p), p.y,
                                                   direction.y) /
                                  std::abs(direction.y));
        }
        const glm::dvec3 middle = p + 0.5 * ds * direction;
        const double r = glm::length(middle);
        if (disk.inSlab(r, middle.y)) {
            transfer.add(source(r), disk.absorption(r, middle.y) * ds);
        }
        s += ds;
    }
    return transfer;
}

// Straight through the disk at radius r, tilted from the vertical
RayTransfer crossDisk(const AccretionDisk& disk, double r, double tilt,
                      double source) {
    const glm::dvec3 direction(std::sin(tilt), -std::cos(tilt), 0.0);
    const double before = 3.0; // start above the slab
    const glm::dvec3 start = glm::dvec3(r, 0.0, 0.0) - before * direction;
    return marchLine(disk, start, direction, 2.0 * before,
                     [source](double) { return source; });
}

void semiTransparentSlab() {
    // Thin enough that h barely changes with height (h uses spherical r)
    const AccretionDisk disk(1.0, 0.0, 12.0, 0.01, 0.7);
    const double expected = std::exp(-disk.slabOpticalDepth());
    const RayTransfer straight = crossDisk(disk, 6.0, 0.0, 2.0);
    check(std::abs(straight.transmittance / expected - 1.0) < 1e-3,
          "straight down: transmittance e^-(vertical optical depth)");
    check(std::abs(straight.radiance - 2.0 * (1.0 - straight.transmittance)) <
              1e-12,
          "uniform source S: radiance S (1 - transmittance)");

    // Tilted, a thin slab is crossed on a path 1 / cos(tilt) longer
    const AccretionDisk thin(1.0, 0.0, 12.0, 1e-6, 0.7);
    const RayTransfer tilted = crossDisk(thin, 6.0, 1.0, 1.0);
    check(std::abs(std::log(tilted.transmittance) /
                       (-thin.slabOpticalDepth() / std::cos(1.0)) -
                   1.0) < 1e-3,
          "tilted: optical depth over cos(tilt)");
}

void thinOpaqueLimit() {
    // The Phase 5 disk: the plane's own color where the ray crosses it.
    // Off the vertical the opaque surface, ~2.5 h up, is shifted sideways
    // by ~2.5 h tan(tilt) from the crossing: converges as h -> 0
    const AccretionDisk disk(1.0, 0.3, 12.0, 1e-6, 1e4);
    const auto glow = [&disk](double r) {
        return std::pow(disk.temperature(r), 4.0);
    };
    for (double r : {disk.innerRadius() * 1.2, 4.0, 6.0, 11.0}) {
        for (double tilt : {0.0, 0.8, 1.5, 3.0}) { // 3.0: from below
            const glm::dvec3 direction(std::sin(tilt), -std::cos(tilt), 0.0);
            const glm::dvec3 start = glm::dvec3(r, 0.0, 0.0) - direction;
            const RayTransfer seen = marchLine(disk, start, direction, 2.0, glow);
            check(seen.opaque() &&
                      std::abs(seen.radiance / glow(r) - 1.0) < 1e-3,
                  "thin, opaque limit: the color of the plane at the crossing");
        }
    }
}

void marchStepsNearTheDisk() {
    const AccretionDisk disk(1.0, 0.0, 12.0, 0.05, 1.0);
    const double h = disk.scaleHeight(6.0);
    check(std::abs(disk.maxHeightChange(6.0, 0.0, 1.0) - 0.5 * h) < 1e-12,
          "inside the slab: steps of half a scale height");
    check(std::abs(disk.maxHeightChange(6.0, 5.0 * h, -1.0) - 2.01 * h) <
              1e-12,
          "heading in: land just inside");
    check(std::isinf(disk.maxHeightChange(6.0, 5.0 * h, 1.0)) &&
              std::isinf(disk.maxHeightChange(20.0, 0.0, 1.0)),
          "heading away, or past the edge: no limit");
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
    verticalOpticalDepth();
    rayTransfer();
    semiTransparentSlab();
    thinOpaqueLimit();
    marchStepsNearTheDisk();

    if (failures == 0) {
        std::printf("All accretion disk tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
