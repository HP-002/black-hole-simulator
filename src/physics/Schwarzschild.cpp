#include "physics/Schwarzschild.hpp"

#include <cmath>

namespace {

struct State {
    glm::dvec2 pos;
    glm::dvec2 vel;
};

State offset(const State& s, const State& d, double t) {
    return {s.pos + d.pos * t, s.vel + d.vel * t};
}

State derivative(const State& s, double strength) {
    const double r2 = glm::dot(s.pos, s.pos);
    const double r5 = r2 * r2 * std::sqrt(r2);
    return {s.vel, -strength * s.pos / r5};
}

State rk4Step(const State& s, double strength, double dt) {
    const State k1 = derivative(s, strength);
    const State k2 = derivative(offset(s, k1, dt / 2.0), strength);
    const State k3 = derivative(offset(s, k2, dt / 2.0), strength);
    const State k4 = derivative(offset(s, k3, dt), strength);
    return {s.pos + (k1.pos + 2.0 * k2.pos + 2.0 * k3.pos + k4.pos) * dt / 6.0,
            s.vel + (k1.vel + 2.0 * k2.vel + 2.0 * k3.vel + k4.vel) * dt / 6.0};
}

} // namespace

Schwarzschild::Schwarzschild(double rs) : rs_(rs) {}

double Schwarzschild::criticalImpactParameter() const {
    return 1.5 * std::sqrt(3.0) * rs_;
}

TracedRay Schwarzschild::trace(const glm::dvec2& origin,
                               const glm::dvec2& direction,
                               const TraceSettings& settings) const {
    State s{origin, glm::normalize(direction)};
    const double h = s.pos.x * s.vel.y - s.pos.y * s.vel.x;
    const double strength = 1.5 * rs_ * h * h;

    TracedRay ray;
    ray.path.push_back(s.pos);
    for (int step = 0;; ++step) {
        const double r = glm::length(s.pos);
        if (r <= rs_) {
            ray.fate = RayFate::Captured;
            return ray;
        }
        if (r >= settings.escapeRadius && glm::dot(s.pos, s.vel) > 0.0) {
            ray.fate = RayFate::Escaped;
            return ray;
        }
        if (step == settings.maxSteps) {
            return ray;
        }
        const double dt = settings.stepFraction * r / glm::length(s.vel);
        s = rk4Step(s, strength, dt);
        ray.path.push_back(s.pos);
    }
}
