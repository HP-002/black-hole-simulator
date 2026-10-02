#pragma once

#include <glm/glm.hpp>

// Thin disk in the y = 0 plane, orbiting prograde about +y, from the
// innermost stable circular orbit out to outerRadius. spin = a / M of the
// hole; negative means the hole turns against the disk. Radii are
// Boyer-Lindquist r.
class AccretionDisk {
  public:
    AccretionDisk(double rs, double spin, double outerRadius);

    double innerRadius() const { return inner_; }
    double outerRadius() const { return outer_; }
    bool contains(double r) const { return r >= inner_ && r <= outer_; }

    // Keplerian Omega = d(phi)/dt = sqrt(M) / (r^(3/2) + a sqrt(M)),
    // coordinate time
    double angularVelocity(double r) const;
    // u^t = dt/d(tau) of the gas on its circular orbit
    double orbitTimeRate(double r) const;

    // Novikov-Thorne (Page-Thorne) flux, without the constant factor
    // 3 Mdot / (8 pi M^2): zero at the inner edge, ~ (M / r)^3 far out
    double flux(double r) const;
    // (flux / flux scale)^(1/4): peak 1 without spin. Same accretion rate
    // for every spin: far out the disk looks the same, and spin mostly
    // heats the inner disk.
    double temperature(double r) const;

    // Observed / emitted photon energy. lambda = L_y / E of the photon,
    // cameraEnergy = energy the camera measures / energy at infinity.
    double redshiftFactor(double r, double lambda, double cameraEnergy) const;

    // For the shader's copy of flux():
    // bracket = x - x0 - 1.5 a ln(x / x0)
    //           - sum_i c_i ln((x - x_i) / (x0 - x_i))
    double fluxX0() const { return x0_; }
    glm::dvec3 fluxRoots() const { return roots_; }
    glm::dvec3 fluxCoefficients() const { return coefficients_; }
    double fluxScale() const { return fluxScale_; }

  private:
    double mass_;
    double spin_;
    double inner_;
    double outer_;
    double x0_;               // sqrt(r_in / M)
    glm::dvec3 roots_;        // of x^3 - 3x + 2 spin
    glm::dvec3 coefficients_; // 3 (x_i - a)^2 / (x_i (x_i - x_j)(x_i - x_k))
    double fluxScale_;        // peak flux of a non-spinning disk
};
