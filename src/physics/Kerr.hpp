#pragma once

#include "physics/Schwarzschild.hpp"

#include <glm/glm.hpp>

// Camera's orthonormal frame with the index lowered: xyz spatial, w time.
// The photon from view direction n has momentum time + n_i * axis_i.
struct CameraFrame {
    glm::dvec4 time;
    glm::dvec4 right;
    glm::dvec4 up;
    glm::dvec4 forward;
    bool valid = false; // false inside the horizon: no observer can hover
};

// A ray leaving the camera, in the time-reversed hole (see Kerr)
struct Photon {
    glm::dvec3 position;
    glm::dvec3 momentum;      // covariant, energy 1 at infinity
    double cameraEnergy = 1.0; // camera's measured energy / energy at infinity
};

// Photon paths around a rotating black hole, spinning about +y.
// spin = a / M in (-1, 1); positive spins the same way as the disk.
//
// Cartesian Kerr-Schild coordinates: g = eta + f l l, with l null. Rays obey
// Hamilton's equations for H = g^(mu nu) p_mu p_nu / 2, which needs only f, l
// and their gradients: no poles, and smooth across the future horizon.
//
// Rays are traced backwards from the camera. Reversing time turns the hole's
// spin around, so a backward trace in spin a is a forward trace in spin -a.
// All frames, photons and traces below live in that reversed hole.
class Kerr {
  public:
    Kerr(double rs, double spin);

    double rs() const { return 2.0 * mass_; }
    double mass() const { return mass_; }
    double spin() const { return spin_; }
    double a() const { return spin_ * mass_; } // angular momentum / mass

    double horizonRadius() const;
    // Circular photon orbit in the equator, with or against the spin
    double photonOrbitRadius(bool prograde) const;
    // Innermost stable circular orbit (Bardeen, Press & Teukolsky 1972)
    double iscoRadius(bool prograde) const;

    // Boyer-Lindquist r of a Kerr-Schild position: the spheroid it lies on
    double radius(const glm::dvec3& position) const;

    // Zero-angular-momentum observer at position, axes as close to the given
    // (orthonormal) world axes as the curved space allows
    CameraFrame cameraFrame(const glm::dvec3& position, const glm::dvec3& right,
                            const glm::dvec3& up,
                            const glm::dvec3& forward) const;
    // view: unit direction in the camera's (right, up, forward) frame
    Photon photon(const CameraFrame& frame, const glm::dvec3& position,
                  const glm::dvec3& view) const;

    // g^(mu nu) a_mu b_nu of two covectors (xyz spatial, w time)
    double inverseDot(const glm::dvec3& position, const glm::dvec4& a,
                      const glm::dvec4& b) const;

    // Along a ray: H = 0, and L and Q stay constant
    double hamiltonian(const glm::dvec3& position,
                       const glm::dvec3& momentum) const;
    // Angular momentum about +y
    double angularMomentum(const glm::dvec3& position,
                           const glm::dvec3& momentum) const;
    // Carter constant
    double carterConstant(const glm::dvec3& position,
                          const glm::dvec3& momentum) const;
    // Hamilton's equations: d(position), d(momentum) / d(affine parameter)
    void derivative(const glm::dvec3& position, const glm::dvec3& momentum,
                    glm::dvec3& dPosition, glm::dvec3& dMomentum) const;

    // Ends inside the horizon (Captured), moving outward beyond
    // settings.escapeRadius (Escaped), or after settings.maxSteps
    TracedRay3 trace(const Photon& photon,
                     const TraceSettings& settings = TraceSettings()) const;

  private:
    double mass_;
    double spin_;
    double traced_; // a of the time-reversed hole the rays are traced in
};
