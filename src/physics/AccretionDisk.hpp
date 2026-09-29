#pragma once

// Thin disk in the y = 0 plane, orbiting prograde about +y
class AccretionDisk {
  public:
    AccretionDisk(double rs, double innerRadius, double outerRadius);

    double innerRadius() const { return inner_; }
    double outerRadius() const { return outer_; }
    bool contains(double r) const { return r >= inner_ && r <= outer_; }

    // Thin-disk profile T ~ r^(-3/4) (1 - sqrt(r_in / r))^(1/4), peak 1
    double temperature(double r) const;

    // Observed / emitted photon energy. lambda = L_y / E of the photon,
    // observer hovering at observerRadius.
    double redshiftFactor(double r, double lambda, double observerRadius) const;

  private:
    double rs_;
    double inner_;
    double outer_;
};
