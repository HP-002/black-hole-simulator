#pragma once

#include <glm/glm.hpp>

#include <vector>

enum class RayFate { Captured, Escaped, Unfinished };

struct TraceSettings {
    double escapeRadius = 100.0;
    double stepFraction = 0.01; // step length / r
    int maxSteps = 20000;
};

struct TracedRay {
    std::vector<glm::dvec2> path;
    RayFate fate = RayFate::Unfinished;
};

struct TracedRay3 {
    std::vector<glm::dvec3> path;
    RayFate fate = RayFate::Unfinished;
};

// Photon paths around a non-rotating black hole.
// x'' = -1.5 rs h^2 x / r^5 (h = |x cross v|) matches u'' + u = 1.5 rs u^2
class Schwarzschild {
  public:
    explicit Schwarzschild(double rs);

    double rs() const { return rs_; }
    double photonSphereRadius() const { return 1.5 * rs_; }
    double criticalImpactParameter() const;

    // Direction seen by an observer hovering at position -> coordinate
    // direction (radial part scaled by sqrt(1 - rs / r)), r > rs
    glm::dvec3 coordinateDirection(const glm::dvec3& position,
                                   const glm::dvec3& localDirection) const;

    TracedRay trace(const glm::dvec2& origin, const glm::dvec2& direction,
                    const TraceSettings& settings = TraceSettings()) const;
    TracedRay3 trace(const glm::dvec3& origin, const glm::dvec3& direction,
                     const TraceSettings& settings = TraceSettings()) const;

  private:
    double rs_;
};
