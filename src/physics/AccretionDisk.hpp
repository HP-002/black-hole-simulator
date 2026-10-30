#pragma once

#include <glm/glm.hpp>

// Light through the disk, front to back: each piece of the ray shines with
// its source radiance (what an opaque slab of it would show) and dims
// everything behind it by e^-opticalDepth (Beer-Lambert). Mirrors the
// march in tracer.comp.
struct RayTransfer {
    // Less light than this gets through: call the rest opaque
    static constexpr double kOpaqueTransmittance = 0.01;

    double radiance = 0.0;
    double transmittance = 1.0;

    // Once almost opaque, the light still coming through is taken to have
    // this piece's source, and transmittance drops to 0
    void add(double source, double opticalDepth);
    bool opaque() const { return transmittance == 0.0; }
};

// Gas disk about the y = 0 plane, orbiting prograde about +y, from the
// innermost stable circular orbit out to outerRadius. spin = a / M of the
// hole; negative means the hole turns against the disk. Radii are
// Boyer-Lindquist r.
//
// Vertically the gas is Gaussian with scale height h = thickness * r and
// a fixed column: the optical depth straight through the disk is
// opticalDepth at every radius, however thick. thickness -> 0 with
// opticalDepth -> infinity is the infinitely thin, opaque disk.
class AccretionDisk {
  public:
    // Gas beyond this many scale heights is ignored
    static constexpr double kSlabHalfHeight = 3.0;
    // Largest height change of one march step, in scale heights
    static constexpr double kHeightStep = 0.5;
    // A step heading into the slab stops this far in (scale heights), so
    // the next sample starts at its surface
    static constexpr double kEntryDepth = 0.01;

    // thickness = H / R > 0
    AccretionDisk(double rs, double spin, double outerRadius,
                  double thickness = 0.05, double opticalDepth = 2.0);

    double innerRadius() const { return inner_; }
    double outerRadius() const { return outer_; }
    bool contains(double r) const { return r >= inner_ && r <= outer_; }

    double thickness() const { return thickness_; }
    double opticalDepth() const { return opticalDepth_; }
    double scaleHeight(double r) const { return thickness_ * r; }
    // Inside the marched slab: radius in the disk, |y| < kSlabHalfHeight h
    bool inSlab(double r, double y) const;
    // Absorption per unit length at height y above radius r; also the
    // gas density, with the opacity folded in
    // = opticalDepth e^(-y^2 / 2h^2) / (sqrt(2 pi) h)
    double absorption(double r, double y) const;
    // Through the slab along y: opticalDepth erf(kSlabHalfHeight / sqrt 2)
    double slabOpticalDepth() const;
    // Largest height change of the next march step at height y, moving
    // with dy/ds = velocityY: kHeightStep h inside the slab, kEntryDepth h
    // past its surface when heading in, unlimited otherwise
    double maxHeightChange(double r, double y, double velocityY) const;

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
    // scaled = false: fluxScale 1, for finding the scale itself
    AccretionDisk(double rs, double spin, double outerRadius, double thickness,
                  double opticalDepth, bool scaled);
    static double peakFluxWithoutSpin();

    double mass_;
    double spin_;
    double inner_;
    double outer_;
    double thickness_;
    double opticalDepth_;
    double x0_;              // sqrt(r_in / M)
    glm::dvec3 roots_;        // of x^3 - 3x + 2 spin
    glm::dvec3 coefficients_; // 3 (x_i - a)^2 / (x_i (x_i - x_j)(x_i - x_k))
    double fluxScale_;        // peak flux of a non-spinning disk
};
