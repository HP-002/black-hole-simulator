// Unit tests for physics/Schwarzschild

#include "physics/Schwarzschild.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

void tiltedPlaneMatches2D() {
    const Schwarzschild hole(1.0);
    const glm::dmat3 rot(
        glm::rotate(glm::dmat4(1.0), 1.1, glm::normalize(glm::dvec3(1, 2, 3))));
    const glm::dvec2 origin(-50.0, 3.5);
    const glm::dvec2 dir(1.0, 0.2);

    const TracedRay flat = hole.trace(origin, dir);
    const TracedRay3 tilted =
        hole.trace(rot * glm::dvec3(origin, 0.0), rot * glm::dvec3(dir, 0.0));

    bool same =
        flat.fate == tilted.fate && flat.path.size() == tilted.path.size();
    for (size_t i = 0; same && i < flat.path.size(); ++i) {
        const glm::dvec3 back = glm::transpose(rot) * tilted.path[i];
        same = glm::length(back - glm::dvec3(flat.path[i], 0.0)) < 1e-9;
    }
    check(same, "3D ray in a tilted plane matches the 2D path");
}

void staticObserverShadowEdge() {
    // sin(alpha) = (b_crit / r) sqrt(1 - rs / r), alpha from the inward radial
    const Schwarzschild hole(1.0);
    const double r = 12.0;
    const double edge = std::asin(hole.criticalImpactParameter() / r *
                                  std::sqrt(1.0 - hole.rs() / r));
    const glm::dvec3 pos(0.0, 0.0, r);
    const auto fateAt = [&](double alpha) {
        const glm::dvec3 local(std::sin(alpha), 0.0, -std::cos(alpha));
        return hole.trace(pos, hole.coordinateDirection(pos, local)).fate;
    };
    check(fateAt(0.99 * edge) == RayFate::Captured,
          "ray just inside the shadow edge is captured");
    check(fateAt(1.01 * edge) == RayFate::Escaped,
          "ray just outside the shadow edge escapes");
}

void coordinateDirectionLimits() {
    const Schwarzschild hole(1.0);
    const glm::dvec3 local = glm::normalize(glm::dvec3(1.0, 2.0, -3.0));
    check(glm::length(hole.coordinateDirection(glm::dvec3(0, 0, 1e9), local) -
                      local) < 1e-8,
          "far away, coordinate and local directions agree");

    const glm::dvec3 pos(0.0, 0.0, 2.0);
    const glm::dvec3 radial(0.0, 0.0, -1.0);
    const glm::dvec3 tangent(1.0, 0.0, 0.0);
    check(glm::length(hole.coordinateDirection(pos, radial) - radial) < 1e-12 &&
              glm::length(hole.coordinateDirection(pos, tangent) - tangent) <
                  1e-12,
          "purely radial and tangential directions are unchanged");
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
    tiltedPlaneMatches2D();
    staticObserverShadowEdge();
    coordinateDirectionLimits();
    stepLimitStopsTrace();

    if (failures == 0) {
        std::printf("All Schwarzschild tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
