// Unit tests for physics/Kerr

#include "physics/Kerr.hpp"
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

bool near(double value, double expected, double tolerance) {
    return std::abs(value - expected) <= tolerance;
}

// Camera on +z looking at the hole: right +x, up +y, forward -z
CameraFrame frameOnZ(const Kerr& hole, double distance) {
    return hole.cameraFrame(
        glm::dvec3(0.0, 0.0, distance), glm::dvec3(1.0, 0.0, 0.0),
        glm::dvec3(0.0, 1.0, 0.0), glm::dvec3(0.0, 0.0, -1.0));
}

void constants() {
    const Kerr still(2.0, 0.0); // M = 1
    check(near(still.horizonRadius(), 2.0, 1e-12), "no spin: horizon at rs");
    check(near(still.photonOrbitRadius(true), 3.0, 1e-12) &&
              near(still.photonOrbitRadius(false), 3.0, 1e-12),
          "no spin: photon orbit at 3M");
    check(near(still.iscoRadius(true), 6.0, 1e-12) &&
              near(still.iscoRadius(false), 6.0, 1e-12),
          "no spin: ISCO at 6M");

    const Kerr extreme(2.0, 1.0);
    check(near(extreme.horizonRadius(), 1.0, 1e-12), "a = M: horizon at M");
    check(near(extreme.photonOrbitRadius(true), 1.0, 1e-9) &&
              near(extreme.photonOrbitRadius(false), 4.0, 1e-9),
          "a = M: photon orbits at M and 4M");
    check(near(extreme.iscoRadius(true), 1.0, 1e-9) &&
              near(extreme.iscoRadius(false), 9.0, 1e-9),
          "a = M: ISCO at M and 9M");

    check(near(Kerr(2.0, 0.998).iscoRadius(true), 1.2370, 1e-4),
          "a = 0.998 M: prograde ISCO at 1.237M (Thorne limit)");
    check(near(Kerr(2.0, -0.5).iscoRadius(true),
               Kerr(2.0, 0.5).iscoRadius(false), 1e-12),
          "negative spin: prograde is retrograde of the flipped hole");
}

void gradientMatchesHamiltonian() {
    const Kerr hole(2.0, 0.9);
    const glm::dvec3 p(3.0, 1.5, -4.0);
    const glm::dvec3 q(0.3, -0.7, 0.5);
    glm::dvec3 dPos;
    glm::dvec3 dMom;
    hole.derivative(p, q, dPos, dMom);

    const double h = 1e-6;
    double worst = 0.0;
    for (int i = 0; i < 3; ++i) {
        glm::dvec3 e(0.0);
        e[i] = h;
        const double dHdx =
            (hole.hamiltonian(p + e, q) - hole.hamiltonian(p - e, q)) / (2 * h);
        const double dHdp =
            (hole.hamiltonian(p, q + e) - hole.hamiltonian(p, q - e)) / (2 * h);
        worst = std::fmax(worst, std::abs(dMom[i] + dHdx));
        worst = std::fmax(worst, std::abs(dPos[i] - dHdp));
    }
    check(worst < 1e-8, "derivatives are Hamilton's equations for H");
}

void cameraFrameIsOrthonormal() {
    for (double spin : {0.0, 0.6, -0.998}) {
        const Kerr hole(2.0, spin);
        // Second point: inside the ergosphere (r < 2M on the equator) when
        // the hole spins
        const double inner = 0.5 * (hole.horizonRadius() + 2.0) + 0.1;
        for (const glm::dvec3& pos :
             {glm::dvec3(0.5, 3.0, 12.0), glm::dvec3(inner, 0.0, 0.3)}) {
            const glm::dvec3 forward = glm::normalize(-pos);
            const glm::dvec3 right =
                glm::normalize(glm::cross(forward, glm::dvec3(0.0, 1.0, 0.0)));
            const glm::dvec3 up = glm::cross(right, forward);
            const CameraFrame frame = hole.cameraFrame(pos, right, up, forward);
            check(frame.valid, "camera outside the horizon has a frame");

            const glm::dvec4 axes[4] = {frame.time, frame.right, frame.up,
                                        frame.forward};
            double worst = 0.0;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    const double eta = i != j ? 0.0 : (i == 0 ? -1.0 : 1.0);
                    worst = std::fmax(
                        worst, std::abs(hole.inverseDot(pos, axes[i], axes[j]) -
                                        eta));
                }
            }
            check(worst < 1e-9, "camera frame is orthonormal");
            check(near(hole.angularMomentum(pos, glm::dvec3(frame.time)), 0.0,
                       1e-12),
                  "camera has zero angular momentum");

            const glm::dvec3 view = glm::normalize(glm::dvec3(0.3, -0.2, 1.0));
            const Photon photon = hole.photon(frame, pos, view);
            check(near(hole.hamiltonian(pos, photon.momentum), 0.0, 1e-12),
                  "camera photons are null: H = 0");
        }
    }
    check(!Kerr(2.0, 0.5)
               .cameraFrame(glm::dvec3(0.0, 0.0, 1.5), glm::dvec3(1, 0, 0),
                            glm::dvec3(0, 1, 0), glm::dvec3(0, 0, -1))
               .valid,
          "no frame inside the horizon");
}

void noSpinMatchesSchwarzschild() {
    const Kerr kerr(1.0, 0.0);
    const Schwarzschild hole(1.0);
    const double r = 12.0;
    const glm::dvec3 pos(0.0, 0.0, r);
    const CameraFrame frame = frameOnZ(kerr, r);

    TraceSettings settings;
    settings.escapeRadius = 200.0;
    for (double alpha : {0.1, 0.25, 0.5, 1.2}) {
        const glm::dvec3 view(std::sin(alpha) * 0.6, std::sin(alpha) * 0.8,
                              std::cos(alpha));
        const glm::dvec3 local(view.x, view.y, -view.z);
        const glm::dvec3 expected = hole.coordinateDirection(pos, local);

        const Photon photon = kerr.photon(frame, pos, view);
        glm::dvec3 dPos;
        glm::dvec3 dMom;
        kerr.derivative(pos, photon.momentum, dPos, dMom);
        check(glm::length(glm::normalize(dPos) - expected) < 1e-12,
              "no spin: camera matches the hovering observer");
        check(near(photon.cameraEnergy, 1.0 / std::sqrt(1.0 - 1.0 / r), 1e-12),
              "no spin: camera sees the hovering-observer blueshift");

        const TracedRay3 a = kerr.trace(photon, settings);
        const TracedRay3 b = hole.trace(pos, expected, settings);
        check(a.fate == b.fate, "no spin: same fate as Schwarzschild");
        if (a.fate == RayFate::Escaped && b.fate == RayFate::Escaped) {
            const auto lastStep = [](const TracedRay3& ray) {
                const std::size_t n = ray.path.size();
                return glm::normalize(ray.path[n - 1] - ray.path[n - 2]);
            };
            const glm::dvec3 da = lastStep(a);
            const glm::dvec3 db = lastStep(b);
            check(glm::length(da - db) < 1e-5,
                  "no spin: same escape direction as Schwarzschild");
        }
    }

    // Shadow edge: sin(alpha) = (b_crit / r) sqrt(1 - rs / r)
    const double edge = std::asin(hole.criticalImpactParameter() / r *
                                  std::sqrt(1.0 - 1.0 / r));
    const auto fateAt = [&](double alpha) {
        const glm::dvec3 view(std::sin(alpha), 0.0, std::cos(alpha));
        return kerr.trace(kerr.photon(frame, pos, view)).fate;
    };
    check(fateAt(0.99 * edge) == RayFate::Captured &&
              fateAt(1.01 * edge) == RayFate::Escaped,
          "no spin: shadow edge where the hovering-observer formula says");
}

void conservedAlongRay() {
    const Kerr hole(2.0, 0.9);
    const glm::dvec3 pos(3.0, 5.0, 20.0);
    const glm::dvec3 forward = glm::normalize(-pos);
    const glm::dvec3 right =
        glm::normalize(glm::cross(forward, glm::dvec3(0.0, 1.0, 0.0)));
    const glm::dvec3 up = glm::cross(right, forward);
    const CameraFrame frame = hole.cameraFrame(pos, right, up, forward);
    // Swings close past the hole, out of the equator
    const Photon photon =
        hole.photon(frame, pos, glm::normalize(glm::dvec3(0.25, 0.1875, 1.0)));

    const double l0 = hole.angularMomentum(pos, photon.momentum);
    const double q0 = hole.carterConstant(pos, photon.momentum);

    // Replay the trace, checking at every step
    TraceSettings settings;
    settings.escapeRadius = 60.0;
    glm::dvec3 p = photon.position;
    glm::dvec3 q = photon.momentum;
    double worstH = 0.0;
    double worstL = 0.0;
    double worstQ = 0.0;
    double closest = 1e9;
    for (int step = 0; step < 20000; ++step) {
        glm::dvec3 k1p, k1q, k2p, k2q, k3p, k3q, k4p, k4q;
        hole.derivative(p, q, k1p, k1q);
        if (glm::length(p) > 60.0 && glm::dot(p, k1p) > 0.0) {
            break;
        }
        const double dt = 0.01 * glm::length(p) / glm::length(k1p);
        hole.derivative(p + k1p * dt / 2.0, q + k1q * dt / 2.0, k2p, k2q);
        hole.derivative(p + k2p * dt / 2.0, q + k2q * dt / 2.0, k3p, k3q);
        hole.derivative(p + k3p * dt, q + k3q * dt, k4p, k4q);
        p += (k1p + 2.0 * k2p + 2.0 * k3p + k4p) * dt / 6.0;
        q += (k1q + 2.0 * k2q + 2.0 * k3q + k4q) * dt / 6.0;
        closest = std::fmin(closest, hole.radius(p));
        worstH = std::fmax(worstH, std::abs(hole.hamiltonian(p, q)));
        worstL = std::fmax(worstL, std::abs(hole.angularMomentum(p, q) - l0));
        worstQ = std::fmax(worstQ, std::abs(hole.carterConstant(p, q) - q0));
    }
    check(closest < 8.0 && closest > hole.horizonRadius(),
          "test ray passes close and escapes");
    check(worstH < 1e-8, "H stays 0 along the ray");
    check(worstL < 1e-8 * std::fmax(1.0, std::abs(l0)), "L is conserved");
    check(worstQ < 1e-7 * std::fmax(1.0, std::abs(q0)),
          "Carter constant is conserved");
}

void equatorialShadowEdges() {
    // Far camera on the equator. Light passing on the left (x < 0) moves
    // +z there: with the spin (prograde), it can come closer before
    // capture. Critical b = -+a + 6M cos(acos(-+a / M) / 3).
    const double spin = 0.9;
    const Kerr hole(2.0, spin); // M = 1
    const double distance = 1e4;
    const glm::dvec3 pos(0.0, 0.0, distance);
    const CameraFrame frame = frameOnZ(hole, distance);
    const double bPro = -spin + 6.0 * std::cos(std::acos(-spin) / 3.0);
    const double bRetro = spin + 6.0 * std::cos(std::acos(spin) / 3.0);

    TraceSettings settings;
    settings.escapeRadius = 2.0 * distance;
    const auto fate = [&](double b, double side) {
        // b = r sin(alpha) / sqrt(1 - 2M / r) for a far static observer
        const double s = b / distance * std::sqrt(1.0 - 2.0 / distance);
        const glm::dvec3 view(side * s, 0.0, std::sqrt(1.0 - s * s));
        return hole.trace(hole.photon(frame, pos, view), settings).fate;
    };
    check(fate(0.99 * bPro, -1.0) == RayFate::Captured &&
              fate(1.01 * bPro, -1.0) == RayFate::Escaped,
          "prograde shadow edge at b = 2.84M (a = 0.9)");
    check(fate(0.99 * bRetro, 1.0) == RayFate::Captured &&
              fate(1.01 * bRetro, 1.0) == RayFate::Escaped,
          "retrograde shadow edge at b = 6.83M (a = 0.9)");
}

void capturedRaysCrossTheHorizon() {
    // Near-extreme spin: the trace must not wind round outside the horizon
    const Kerr hole(2.0, 0.998);
    const double r = 20.0;
    const glm::dvec3 pos(0.0, 0.0, r);
    const CameraFrame frame = frameOnZ(hole, r);
    TraceSettings settings;
    settings.maxSteps = 2000;
    for (double x : {0.0, 0.04, -0.04}) {
        const TracedRay3 ray = hole.trace(
            hole.photon(frame, pos, glm::normalize(glm::dvec3(x, 0.05, 1.0))),
            settings);
        check(ray.fate == RayFate::Captured, "ray at the center is captured");
        check(ray.path.size() < 1000, "capture takes few steps");
    }
}

void negativeEnergyRaysInTheErgosphere() {
    // Equator, r = 1.56M: between the horizon (1.44M) and the ergosphere
    // (2M). Looking with the spin, the time-reversed photon can have E < 0
    const Kerr hole(2.0, 0.9);
    const glm::dvec3 pos(0.0, 0.0, std::sqrt(1.56 * 1.56 + 0.81));
    const CameraFrame frame = frameOnZ(hole, pos.z);
    int negative = 0;
    double worstTurn = 0.0;
    bool escapedWithNegativeEnergy = false;
    glm::dvec3 previous(0.0);
    for (int i = 0; i <= 360; ++i) {
        const double angle = 2.0 * 3.14159265358979323846 * i / 360.0;
        const glm::dvec3 view(std::sin(angle), 0.0, std::cos(angle));
        const Photon photon = hole.photon(frame, pos, view);
        if (photon.direction < 0.0) {
            ++negative;
        }
        TraceSettings settings;
        settings.maxSteps = 4;
        const TracedRay3 ray = hole.trace(photon, settings);
        check(ray.path.size() > 1, "ergosphere rays take a step");
        if (ray.path.size() < 2) {
            continue;
        }
        if (photon.direction < 0.0) {
            settings.maxSteps = 5000;
            escapedWithNegativeEnergy |=
                hole.trace(photon, settings).fate == RayFate::Escaped;
        }
        // The ray's first step turns smoothly with the view, also where
        // E changes sign
        const glm::dvec3 step = glm::normalize(ray.path[1] - ray.path[0]);
        if (i > 0) {
            worstTurn = std::fmax(worstTurn, glm::length(step - previous));
        }
        previous = step;
    }
    check(negative > 0, "some camera photons have E < 0 in the ergosphere");
    check(worstTurn < 0.2, "rays don't flip where E changes sign");
    check(!escapedWithNegativeEnergy, "E < 0 rays never escape");
}

void azimuthIsBoyerLindquist() {
    // phi is constant along r and theta: g^(r phi) = g^(theta phi) = 0
    for (double spin : {0.9, -0.6, 0.998}) {
        const Kerr hole(2.0, spin);
        const glm::dvec3 pos(1.3, 0.7, -2.1);
        const auto theta = [&](const glm::dvec3& p) {
            return std::acos(p.y / hole.radius(p));
        };
        const double h = 1e-6;
        glm::dvec4 dr(0.0);
        glm::dvec4 dTheta(0.0);
        glm::dvec4 dPhi(0.0);
        for (int i = 0; i < 3; ++i) {
            glm::dvec3 e(0.0);
            e[i] = h;
            dr[i] = (hole.radius(pos + e) - hole.radius(pos - e)) / (2 * h);
            dTheta[i] = (theta(pos + e) - theta(pos - e)) / (2 * h);
            dPhi[i] = (hole.azimuth(pos + e) - hole.azimuth(pos - e)) / (2 * h);
        }
        check(near(hole.inverseDot(pos, dr, dPhi), 0.0, 1e-8) &&
                  near(hole.inverseDot(pos, dTheta, dPhi), 0.0, 1e-8),
              "azimuth is Boyer-Lindquist phi");
    }
    const Kerr still(2.0, 0.0);
    const glm::dvec3 pos(3.0, 1.0, 4.0);
    check(near(still.azimuth(pos), std::atan2(3.0, 4.0), 1e-12),
          "no spin: azimuth is the plain angle");
}

} // namespace

int main() {
    constants();
    gradientMatchesHamiltonian();
    cameraFrameIsOrthonormal();
    noSpinMatchesSchwarzschild();
    conservedAlongRay();
    equatorialShadowEdges();
    capturedRaysCrossTheHorizon();
    negativeEnergyRaysInTheErgosphere();
    azimuthIsBoyerLindquist();

    if (failures == 0) {
        std::printf("All Kerr tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
