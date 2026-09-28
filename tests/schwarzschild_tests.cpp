// Unit tests for physics/Schwarzschild

#include "physics/Schwarzschild.hpp"

#include <glm/gtc/constants.hpp>

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

// Ray from far left, moving +x, passing the hole at height b
TracedRay traceFromLeft(const Schwarzschild& hole, double b,
                        const TraceSettings& settings = TraceSettings()) {
    return hole.trace(glm::dvec2(-50.0 * hole.rs(), b), glm::dvec2(1.0, 0.0),
                      settings);
}

void constants() {
    const Schwarzschild hole(2.0);
    check(std::abs(hole.photonSphereRadius() - 3.0) < 1e-12,
          "photon sphere at 1.5 rs");
    check(std::abs(hole.criticalImpactParameter() - 3.0 * std::sqrt(3.0)) <
              1e-12,
          "critical impact parameter is 1.5 sqrt(3) rs");
}

void weakFieldDeflection() {
    // Series in x = rs / b: 2x + (15 pi / 16) x^2 + (16 / 3) x^3
    const double rs = 1.0;
    const double b = 100.0;
    const double x = rs / b;
    const double expected = 2.0 * x + 15.0 * glm::pi<double>() / 16.0 * x * x +
                            16.0 / 3.0 * x * x * x;

    TraceSettings settings;
    settings.escapeRadius = 2e5;
    const TracedRay ray = Schwarzschild(rs).trace(
        glm::dvec2(-1e5, b), glm::dvec2(1.0, 0.0), settings);
    check(ray.fate == RayFate::Escaped, "distant ray escapes");

    const glm::dvec2 dir = ray.path.back() - ray.path[ray.path.size() - 2];
    const double deflection = std::atan2(-dir.y, dir.x);
    check(std::abs(deflection - expected) < 1e-6,
          "weak-field deflection matches the series to third order");
}

void criticalImpactParameterSplitsFates() {
    const Schwarzschild hole(1.0);
    const double bc = hole.criticalImpactParameter();
    check(traceFromLeft(hole, 0.99 * bc).fate == RayFate::Captured,
          "ray just inside b_crit is captured");
    check(traceFromLeft(hole, 1.01 * bc).fate == RayFate::Escaped,
          "ray just outside b_crit escapes");
}

void deflectionIsMirrorSymmetric() {
    const Schwarzschild hole(1.0);
    const TracedRay above = traceFromLeft(hole, 4.0);
    const TracedRay below = traceFromLeft(hole, -4.0);
    const glm::dvec2 a = above.path.back();
    const glm::dvec2 b = below.path.back();
    check(above.path.size() == below.path.size() &&
              std::abs(a.x - b.x) < 1e-9 && std::abs(a.y + b.y) < 1e-9,
          "rays at +b and -b are mirror images");
}

void radialRayFallsStraightIn() {
    const Schwarzschild hole(1.0);
    const TracedRay ray =
        hole.trace(glm::dvec2(10.0, 0.0), glm::dvec2(-1.0, 0.0));
    check(ray.fate == RayFate::Captured, "radial ray is captured");
    check(std::abs(ray.path.back().y) < 1e-12, "radial ray stays on its line");
}

void photonSphereOrbitHolds() {
    const Schwarzschild hole(1.0);
    const double r0 = hole.photonSphereRadius();
    TraceSettings settings;
    settings.maxSteps = 700; // just over one orbit
    const TracedRay ray =
        hole.trace(glm::dvec2(r0, 0.0), glm::dvec2(0.0, 1.0), settings);
    check(ray.fate == RayFate::Unfinished,
          "orbiting ray neither falls nor escapes");

    double worst = 0.0;
    for (const glm::dvec2& p : ray.path) {
        worst = std::fmax(worst, std::abs(glm::length(p) - r0));
    }
    check(worst < 1e-6, "tangent ray at 1.5 rs stays on the photon sphere");
}

void stepLimitStopsTrace() {
    TraceSettings settings;
    settings.maxSteps = 10;
    const TracedRay ray = traceFromLeft(Schwarzschild(1.0), 5.0, settings);
    check(ray.fate == RayFate::Unfinished && ray.path.size() == 11,
          "trace stops after maxSteps");
}

} // namespace

int main() {
    constants();
    weakFieldDeflection();
    criticalImpactParameterSplitsFates();
    deflectionIsMirrorSymmetric();
    radialRayFallsStraightIn();
    photonSphereOrbitHolds();
    stepLimitStopsTrace();

    if (failures == 0) {
        std::printf("All Schwarzschild tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
