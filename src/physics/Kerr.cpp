#include "physics/Kerr.hpp"

#include <cmath>

namespace {

// Kerr-Schild fields at a position, spin axis +y. For a = 0 these are
// ingoing Eddington-Finkelstein coordinates, and r is the distance.
struct Fields {
    double r;      // Boyer-Lindquist radius
    double d;      // sqrt((rho^2 - a^2)^2 + 4 a^2 y^2) = 2 r^2 - rho^2 + a^2
    double f;      // 2 M r^3 / (r^4 + a^2 y^2)
    glm::dvec3 l;  // spatial part of the null vector, l_t = 1
};

Fields fields(const glm::dvec3& p, double mass, double a) {
    Fields k;
    const double a2 = a * a;
    const double w = glm::dot(p, p) - a2;
    k.d = std::sqrt(w * w + 4.0 * a2 * p.y * p.y);
    k.r = std::sqrt(0.5 * (w + k.d));
    const double r2 = k.r * k.r;
    const double n = r2 + a2;
    // (z, x) turns into each other like (x, y) about z in the usual form
    k.l = glm::dvec3((k.r * p.x - a * p.z) / n, p.y / k.r,
                     (k.r * p.z + a * p.x) / n);
    k.f = 2.0 * mass * r2 * k.r / (r2 * r2 + a2 * p.y * p.y);
    return k;
}

// Generator of rotations about +y: d(position) / d(phi)
glm::dvec3 axial(const glm::dvec3& p) { return glm::dvec3(p.z, 0.0, -p.x); }

// 4-vectors are (xyz spatial, w time), contravariant
double dot4(const Fields& k, const glm::dvec4& u, const glm::dvec4& v) {
    const double lu = u.w + glm::dot(k.l, glm::dvec3(u));
    const double lv = v.w + glm::dot(k.l, glm::dvec3(v));
    return -u.w * v.w + glm::dot(glm::dvec3(u), glm::dvec3(v)) + k.f * lu * lv;
}

// Contravariant -> covariant
glm::dvec4 lower(const Fields& k, const glm::dvec4& v) {
    const double lv = v.w + glm::dot(k.l, glm::dvec3(v));
    return glm::dvec4(glm::dvec3(v) + k.f * lv * k.l, -v.w + k.f * lv);
}

// Symmetric positive definite m -> m^(-1/2), by Jacobi rotations
glm::dmat3 inverseSqrt(const glm::dmat3& m) {
    double a[3][3];
    double v[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            a[i][j] = m[j][i];
        }
    }
    for (int sweep = 0; sweep < 32; ++sweep) {
        const double off = a[0][1] * a[0][1] + a[0][2] * a[0][2] +
                           a[1][2] * a[1][2];
        if (off < 1e-30) {
            break;
        }
        for (int p = 0; p < 2; ++p) {
            for (int q = p + 1; q < 3; ++q) {
                if (a[p][q] == 0.0) {
                    continue;
                }
                // Rotation in the (p, q) plane that zeroes a[p][q]
                const double theta = (a[q][q] - a[p][p]) / (2.0 * a[p][q]);
                const double t =
                    (theta >= 0.0 ? 1.0 : -1.0) /
                    (std::abs(theta) + std::sqrt(theta * theta + 1.0));
                const double c = 1.0 / std::sqrt(t * t + 1.0);
                const double s = t * c;
                for (int k = 0; k < 3; ++k) { // a = a P
                    const double kp = a[k][p];
                    const double kq = a[k][q];
                    a[k][p] = c * kp - s * kq;
                    a[k][q] = s * kp + c * kq;
                }
                for (int k = 0; k < 3; ++k) { // a = P^T a
                    const double pk = a[p][k];
                    const double qk = a[q][k];
                    a[p][k] = c * pk - s * qk;
                    a[q][k] = s * pk + c * qk;
                }
                for (int k = 0; k < 3; ++k) { // v = v P
                    const double kp = v[k][p];
                    const double kq = v[k][q];
                    v[k][p] = c * kp - s * kq;
                    v[k][q] = s * kp + c * kq;
                }
            }
        }
    }
    glm::dmat3 result(0.0);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double sum = 0.0;
            for (int k = 0; k < 3; ++k) {
                sum += v[i][k] * v[j][k] / std::sqrt(a[k][k]);
            }
            result[j][i] = sum;
        }
    }
    return result;
}

} // namespace

Kerr::Kerr(double rs, double spin)
    : mass_(0.5 * rs), spin_(spin), traced_(-spin * 0.5 * rs) {}

double Kerr::horizonRadius() const {
    return mass_ * (1.0 + std::sqrt(1.0 - spin_ * spin_));
}

double Kerr::photonOrbitRadius(bool prograde) const {
    const double s = prograde ? spin_ : -spin_;
    return 2.0 * mass_ * (1.0 + std::cos(2.0 / 3.0 * std::acos(-s)));
}

double Kerr::iscoRadius(bool prograde) const {
    const double s = prograde ? spin_ : -spin_;
    const double z1 = 1.0 + std::cbrt(1.0 - s * s) *
                                (std::cbrt(1.0 + s) + std::cbrt(1.0 - s));
    const double z2 = std::sqrt(3.0 * s * s + z1 * z1);
    const double root = std::sqrt((3.0 - z1) * (3.0 + z1 + 2.0 * z2));
    return mass_ * (3.0 + z2 - (s >= 0.0 ? root : -root));
}

double Kerr::radius(const glm::dvec3& position) const {
    return fields(position, mass_, traced_).r;
}

CameraFrame Kerr::cameraFrame(const glm::dvec3& position,
                              const glm::dvec3& right, const glm::dvec3& up,
                              const glm::dvec3& forward) const {
    CameraFrame frame;
    const Fields k = fields(position, mass_, traced_);
    if (k.r <= horizonRadius()) {
        return frame;
    }

    // Zero angular momentum: u = t + omega phi, with u . phi = 0
    const glm::dvec4 t(0.0, 0.0, 0.0, 1.0);
    const glm::dvec4 phi(axial(position), 0.0);
    const double phiPhi = dot4(k, phi, phi);
    const double omega = phiPhi > 0.0 ? -dot4(k, t, phi) / phiPhi : 0.0;
    glm::dvec4 u = t + omega * phi;
    const double uu = dot4(k, u, u);
    if (uu >= 0.0) {
        return frame;
    }
    u /= std::sqrt(-uu);

    // Lengths the observer measures for coordinate directions:
    // h(v, w) = g(v, w) + (u . v) (u . w)
    glm::dmat3 h;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            glm::dvec4 vi(0.0);
            glm::dvec4 vj(0.0);
            vi[i] = 1.0;
            vj[j] = 1.0;
            h[j][i] = dot4(k, vi, vj) + dot4(k, vi, u) * dot4(k, vj, u);
        }
    }
    // h^(-1/2) sends orthonormal world axes to measured-orthonormal ones,
    // turning them as little as possible
    const glm::dmat3 toLocal = inverseSqrt(h);
    const auto axis = [&](const glm::dvec3& world) {
        const glm::dvec4 v(toLocal * world, 0.0);
        return lower(k, v + dot4(k, v, u) * u); // drop the part along u
    };
    frame.time = lower(k, u);
    frame.right = axis(right);
    frame.up = axis(up);
    frame.forward = axis(forward);
    frame.valid = true;
    return frame;
}

Photon Kerr::photon(const CameraFrame& frame, const glm::dvec3& position,
                    const glm::dvec3& view) const {
    // p = u + n: moves along n at the speed of light
    const glm::dvec4 p = frame.time + view.x * frame.right +
                         view.y * frame.up + view.z * frame.forward;
    const double energy = -p.w; // E = -p_t; rescale to E = 1
    return {position, glm::dvec3(p) / energy, 1.0 / energy};
}

double Kerr::inverseDot(const glm::dvec3& position, const glm::dvec4& a,
                        const glm::dvec4& b) const {
    // Inverse metric eta - f l l, with l^t = -1
    const Fields k = fields(position, mass_, traced_);
    const double la = -a.w + glm::dot(k.l, glm::dvec3(a));
    const double lb = -b.w + glm::dot(k.l, glm::dvec3(b));
    return -a.w * b.w + glm::dot(glm::dvec3(a), glm::dvec3(b)) - k.f * la * lb;
}

double Kerr::hamiltonian(const glm::dvec3& position,
                         const glm::dvec3& momentum) const {
    const Fields k = fields(position, mass_, traced_);
    const double s = 1.0 + glm::dot(k.l, momentum); // l^mu p_mu, E = 1
    return 0.5 * (glm::dot(momentum, momentum) - 1.0 - k.f * s * s);
}

double Kerr::angularMomentum(const glm::dvec3& position,
                             const glm::dvec3& momentum) const {
    return glm::dot(momentum, axial(position));
}

double Kerr::carterConstant(const glm::dvec3& position,
                            const glm::dvec3& momentum) const {
    // Q = p_theta^2 + cos^2(theta) (L^2 / sin^2(theta) - a^2), E = 1
    const Fields k = fields(position, mass_, traced_);
    const double cosTheta = position.y / k.r;
    const double sin2 = 1.0 - cosTheta * cosTheta;
    const double sinTheta = std::sqrt(sin2);
    // d(position) / d(theta) = (x cot, -r sin, z cot)
    const double radialPart =
        position.x * momentum.x + position.z * momentum.z;
    const double pTheta =
        cosTheta / sinTheta * radialPart - k.r * sinTheta * momentum.y;
    const double l = angularMomentum(position, momentum);
    return pTheta * pTheta +
           cosTheta * cosTheta * (l * l / sin2 - traced_ * traced_);
}

void Kerr::derivative(const glm::dvec3& p, const glm::dvec3& q,
                      glm::dvec3& dPosition, glm::dvec3& dMomentum) const {
    const double a = traced_;
    const double a2 = a * a;
    const Fields k = fields(p, mass_, a);
    const double r = k.r;
    const double r2 = r * r;
    const double n = r2 + a2;
    const double lq = glm::dot(k.l, q);
    const double s = 1.0 + lq;

    // dx/dl = dH/dp
    dPosition = q - k.f * s * k.l;

    // dp/dl = -dH/dx = s^2 grad(f) / 2 + f s grad(l . p)
    const glm::dvec3 yAxis(0.0, 1.0, 0.0);
    const glm::dvec3 gradR = (r2 * p + a2 * p.y * yAxis) / (r * k.d);
    const double m = r2 * r2 + a2 * p.y * p.y;
    const glm::dvec3 gradF = 2.0 * mass_ *
                             (r2 * (3.0 * a2 * p.y * p.y - r2 * r2) * gradR -
                              2.0 * a2 * p.y * r2 * r * yAxis) /
                             (m * m);
    // l . p = A / n + y p_y / r
    const double radialPart = p.x * q.x + p.z * q.z;
    const double numerator = r * radialPart + a * (p.x * q.z - p.z * q.x);
    const glm::dvec3 gradNumerator = radialPart * gradR +
                                     r * glm::dvec3(q.x, 0.0, q.z) +
                                     a * glm::dvec3(q.z, 0.0, -q.x);
    const glm::dvec3 gradLq = gradNumerator / n -
                              numerator * 2.0 * r * gradR / (n * n) +
                              q.y * yAxis / r - p.y * q.y * gradR / r2;
    dMomentum = 0.5 * s * s * gradF + k.f * s * gradLq;
}

namespace {

struct State {
    glm::dvec3 pos;
    glm::dvec3 mom;
};

State offset(const State& s, const State& d, double t) {
    return {s.pos + d.pos * t, s.mom + d.mom * t};
}

} // namespace

TracedRay3 Kerr::trace(const Photon& photon,
                       const TraceSettings& settings) const {
    const auto rate = [this](const State& s) {
        State d;
        derivative(s.pos, s.mom, d.pos, d.mom);
        return d;
    };
    const double horizon = horizonRadius();
    State s{photon.position, photon.momentum};

    TracedRay3 ray;
    ray.path.push_back(s.pos);
    for (int step = 0;; ++step) {
        if (radius(s.pos) <= horizon) {
            ray.fate = RayFate::Captured;
            return ray;
        }
        const State k1 = rate(s);
        const double distance = glm::length(s.pos);
        if (distance >= settings.escapeRadius &&
            glm::dot(s.pos, k1.pos) > 0.0) {
            ray.fate = RayFate::Escaped;
            return ray;
        }
        if (step == settings.maxSteps) {
            return ray;
        }
        const double dt =
            settings.stepFraction * distance / glm::length(k1.pos);
        const State k2 = rate(offset(s, k1, dt / 2.0));
        const State k3 = rate(offset(s, k2, dt / 2.0));
        const State k4 = rate(offset(s, k3, dt));
        s.pos += (k1.pos + 2.0 * k2.pos + 2.0 * k3.pos + k4.pos) * dt / 6.0;
        s.mom += (k1.mom + 2.0 * k2.mom + 2.0 * k3.mom + k4.mom) * dt / 6.0;
        ray.path.push_back(s.pos);
    }
}
