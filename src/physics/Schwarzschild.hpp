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

// Photon paths around a non-rotating black hole, in the orbital plane.
// x'' = -1.5 rs h^2 x / r^5 (h = |x cross v|) matches u'' + u = 1.5 rs u^2
class Schwarzschild {
  public:
    explicit Schwarzschild(double rs);

    double rs() const { return rs_; }
    double photonSphereRadius() const { return 1.5 * rs_; }
    double criticalImpactParameter() const;

    TracedRay trace(const glm::dvec2& origin, const glm::dvec2& direction,
                    const TraceSettings& settings = TraceSettings()) const;

  private:
    double rs_;
};
