# Physics notes

The physics behind the simulator, and where it lives in the code. Units are
geometric (G = c = 1), so the mass M sets every scale. The code works in
terms of the Schwarzschild radius and the dimensionless spin:

    rs = 2M        spin = a/M,  a = J/M (angular momentum per unit mass)

Positions are in scene units. The 3D view keeps the disk radii and the camera
speed as multiples of rs, so the Up/Down keys rescale the whole system.

Sections 1–3 are the non-spinning (Schwarzschild) hole. The 2D view uses it
directly. The 3D view uses the spinning (Kerr) hole of sections 4–6, which
becomes exactly sections 1–3 at spin 0.

## 1. Spacetime

A non-rotating, uncharged black hole is described by the Schwarzschild metric:

    ds² = -(1 - rs/r) dt² + dr² / (1 - rs/r) + r² dΩ²

- `r = rs`: event horizon. Anything inside is drawn black.
- `r = 1.5 rs`: photon sphere. Light can circle the hole here, unstably.
- `r = 3 rs`: innermost stable circular orbit (ISCO). The disk starts here.

## 2. Light rays

Photons move in a plane through the hole's center, so each ray is a 2D
problem in its own plane. With u = 1/r and φ the angle in that plane, the
orbit equation is

    d²u/dφ² + u = 3M u² = 1.5 rs u²

The `3M u²` term is the whole of general relativity here. Without it the ray
is a straight line (u = sin φ / b).

**Cartesian form** (`Schwarzschild.cpp`). Take the parameter so that
r² dφ/dλ = h, with h = |x × dx/dλ| constant. Then Binet's identity turns the
orbit equation into a central "force":

    d²x/dλ² = -1.5 rs h² x / r⁵

This is integrated in 3D with RK4. Because the force is central, h is
conserved and the ray stays in its plane automatically, so no plane or angle
bookkeeping is needed. The step is a fixed fraction of r (0.01 on the CPU):
small steps near the hole, big ones far out.

A ray ends when it:

- falls inside rs (captured: black),
- moves outward beyond the escape radius, max(50 rs, 2 × camera distance)
  (it samples the star field in its final direction), or
- runs out of steps. It is then orbiting near the photon sphere and is
  treated as captured.

**Impact parameter.** b = L/E labels a ray: rays with b < b_c = (3√3/2) rs
≈ 2.598 rs fall in. From a position and a unit coordinate direction at angle
ψ from the radial:

    1 / b² = 1 / (r sin ψ)² - rs / r³

**Checks** (`schwarzschild_tests`):

- Deflection at b = 100 rs matches the series
  2x + (15π/16) x² + (16/3) x³ (x = rs/b) to 1e-6. The leading term 2rs/b
  = 4M/b is Einstein's light bending.
- b = 0.99 b_c is captured and b = 1.01 b_c escapes.
- A tangent ray at 1.5 rs stays on the photon sphere for a full orbit.
- A 3D ray in a tilted plane follows the same path as the 2D ray.
- For a hovering camera at 12 rs, the shadow edge falls where the formula in
  section 3 puts it.

## 3. The camera: a hovering observer

The camera hovers at fixed r (a static observer), firing rockets to stay in
place. Its measuring rods are stretched radially compared with the
coordinates, so a direction it sees has its radial part scaled by
√(1 - rs/r) before renormalizing (`coordinateDirection`). This aberration is
what gives the shadow its physical size:

    sin α_shadow = (b_c / r) √(1 - rs/r)

At r = 20 rs that is about 7.3°. A naive pinhole camera would give 7.5°.

## 4. A spinning black hole

A rotating hole (`Kerr.hpp`) is described by the Kerr metric. The 3D view's
hole spins about +y, the same axis as the disk. Positive spin turns the same
way as the disk; negative spin turns against it ([ and ] keys,
|spin| ≤ 0.998).

- **Horizon** at r₊ = M + √(M² − a²): from rs at spin 0 down to M at
  spin 1.
- **Frame dragging.** Near the hole, space itself is dragged round with the
  spin. Inside the **ergosphere** (r < 2M on the equator) nothing can stay
  still relative to the distant stars, not even with rockets.
- **Photon orbits** in the equator are no longer one radius. Light going
  with the spin (prograde) can circle closer in than light going against it
  (retrograde):

      r_ph = 2M (1 + cos(⅔ arccos(∓ spin)))

- **ISCO** (Bardeen, Press & Teukolsky 1972). Prograde orbits stay stable
  further in:

| spin | r₊ | prograde ISCO | retrograde ISCO |
|---|---|---|---|
| 0 | 2M | 6M (3 rs) | 6M |
| 0.6 | 1.8M | 3.83M | 7.85M |
| 0.9 | 1.44M | 2.32M | 8.72M |
| 0.998 | 1.06M | 1.24M | 8.99M |

**The shadow.** Prograde light can come closer before it is captured, so on
that side the shadow edge moves in and flattens. Retrograde light is captured
further out. The shadow becomes a "D", shifted away from the side moving
toward the camera. For a distant observer on the equator, the critical
impact parameters are

    b = ∓a + 6M cos(⅓ arccos(∓ spin))     (2M and 7M at spin 1)

## 5. Kerr light rays

**Coordinates.** The usual Boyer–Lindquist coordinates (t, r, θ, φ) are
singular on the spin axis and at the horizon. The code uses Cartesian
**Kerr–Schild** coordinates instead. There the metric is flat space plus one
extra term:

    g = η + f l ⊗ l
    f = 2M r³ / (r⁴ + a² y²)
    l = (1, (r x − a z)/(r² + a²), y/r, (r z + a x)/(r² + a²))

Here r is the Boyer–Lindquist radius. It is the spheroid the point lies on,
x² + z² = (r² + a²) sin²θ and y = r cos θ:

    r² = ½ (ρ² − a² + √((ρ² − a²)² + 4a² y²)),   ρ = |x|

l is a null vector, so the inverse metric is simply η − f l ⊗ l. At spin 0,
r = |x| and these are Eddington–Finkelstein coordinates.

**Hamilton's equations** (`Kerr::derivative`, `tracer.comp`). A ray is a
curve x(λ) with covariant momentum p, and the Hamiltonian
H = ½ g^(μν) p_μ p_ν = 0. Fix the energy E = −p_t = 1 and write
s = l^μ p_μ = 1 + l·p. Then

    dx/dλ = p − f s l
    dp/dλ = ½ s² ∇f + f s ∇(l·p)

The gradients come out in closed form from ∇r = (r² x + a² y ŷ) / (r d), with
d = 2r² − ρ² + a². This is more work per step than the Schwarzschild "force"
(about 2–3× the GPU time), but it works for any spin. It also has no special
cases at the poles.

**Tracing backwards.** Rays start at the camera and run back to their source.
Run in reverse, the Kerr spacetime looks like a hole spinning the other way
(t → −t flips a). So the code traces forwards in time around a hole with spin
−a (`Kerr::traced_`, `tracedA()`), starting with the reversed photon. This
matters in practice. Kerr–Schild coordinates are smooth across the future
horizon, so a reversed photon falls through r₊ cleanly. A real photon traced
backwards would instead wind round and round just outside the horizon (the
past horizon is at t = −∞ in these coordinates). At spin 0.998 that costs
thousands of steps per pixel.

A ray ends when its r ≤ r₊ (captured), when it moves outward beyond the
escape radius (samples the star field along dx/dλ), or after 500 steps on the
GPU (orbiting; drawn black). The step is 0.05 |x| / |dx/dλ| on the GPU and
0.01 on the CPU.

**Conserved quantities.** Along each ray, E, the angular momentum about the
axis L = p · (z, 0, −x), and Carter's constant are all constant:

    Q = p_θ² + cos²θ (L² / sin²θ − a²),   p_θ = cot θ (x p_x + z p_z) − r sin θ p_y

**Checks** (`kerr_tests`):

- The derivatives equal −∂H/∂x and ∂H/∂p (finite differences, 1e-8).
- H, L and Q stay constant to 1e-8 along a ray that swings past a spin-0.9
  hole out of the equator.
- At spin 0, the camera, each ray's fate and its escape direction match the
  Schwarzschild integrator, and the shadow edge matches section 3.
- At spin 0.9, for a distant camera on the equator, the prograde and
  retrograde shadow edges fall at b = 2.84M and 6.83M (±1%).
- At spin 0.998, rays aimed at the shadow are captured in a few hundred
  steps.
- Horizon, photon orbits and ISCO match the formulas above at spins 0, 1 and
  0.998.

The GPU tracer is a float copy of the CPU one. Its shadow, sampled on a
240×135 grid, matches the CPU reference (step 0.01) at all but 0, 3 and 7
points out of 32,400 at spins 0.6, 0.998 and −0.998. That is the same
difference the CPU shows between steps 0.05 and 0.01.

## 6. The Kerr camera: a zero-angular-momentum observer

A camera that hovers in place relative to the stars (section 3) can't exist
inside the ergosphere. Instead the camera is a **zero-angular-momentum
observer** (ZAMO), which is carried round at the frame-dragging rate

    ω = −g_tφ / g_φφ

so u ∝ ∂_t + ω ∂_φ. At spin 0 this is the hovering observer again.

The camera's axes (`Kerr::cameraFrame`) must be orthonormal for that
observer. Coordinate directions v measure lengths with

    h(v, w) = g(v, w) + (u·v)(u·w)

The frame uses h^(−1/2) applied to the world axes (right, up, forward). This
is the orthonormal frame that turns them as little as possible. At spin 0 it
is exactly section 3's radial stretch √(1 − rs/r). The frame is built on the
CPU in double precision once per frame. The shader gets the four axes with
their index lowered. A pixel's photon is then

    p = u + nᵢ eᵢ,  rescaled to E = 1

and 1/(−p_t) before rescaling is the energy the camera measures relative to
infinity. That number is the camera's own blueshift.

## 7. The accretion disk

A thin, opaque disk in the plane y = 0, from the **prograde ISCO** to 12 rs.
At spin 0 the ISCO is 3 rs. With the spin it moves in, to 0.62 rs at 0.998.
Against the spin it moves out, to 4.5 rs at −0.998. A ray shows the disk the
first time it crosses the plane inside those radii. Rays that cross outside
them go on, which is how the far side of the disk appears lensed over the
top of the shadow. In the equator, the Kerr–Schild r of a hit point is
√(x² + z² − a²).

**Temperature** (`AccretionDisk::flux`, `temperature`). The disk follows the
relativistic thin-disk model of Novikov & Thorne. Its flux comes from Page &
Thorne (1974), with x = √(r/M), x₀ = √(r_ISCO/M), and xᵢ the three roots of
x³ − 3x + 2 spin:

    F ∝ [x − x₀ − (3/2) spin ln(x/x₀) − Σᵢ cᵢ ln((x − xᵢ)/(x₀ − xᵢ))]
        / (x⁴ (x³ − 3x + 2 spin))

    cᵢ = 3 (xᵢ − spin)² / (xᵢ (xᵢ − xⱼ)(xᵢ − xₖ))

T ∝ F^(1/4). It is zero at the ISCO (no torque there) and Newtonian
(F ∝ r⁻³) far out. At spin 0 it peaks at 9.55M, a little further out than
the non-relativistic profile (8.17M) that Phase 4 used. The CPU passes x₀,
the roots and the cᵢ to the shader.

**Normalization.** Temperature 1 (4000 K for the color, which is artistic) is
the peak of a non-spinning disk. Every spin uses that same scale, as if the
gas flowed in at the same rate. Far out the disk looks the same at any spin.
Spin lets the gas orbit deeper in the potential before falling in, so it
releases more energy and heats the inner disk:

| spin | peak temperature | at |
|---|---|---|
| −0.998 | 0.73 | 14.5M |
| 0 | 1 | 9.55M |
| 0.6 | 1.44 | 5.9M |
| 0.9 | 2.23 | 3.4M |
| 0.998 | 4.32 | 1.6M |

At high spin the inner disk is hundreds of times brighter (∝ T⁴), and bloom
floods the view. Lower the exposure with Q.

**Redshift.** g = E_observed / E_emitted combines the gas's orbital Doppler
shift, the gravitational shift between the gas and the camera, and (with
spin) frame dragging. The gas moves on a circular orbit with
u = u^t (∂_t + Ω ∂_φ), where (Bardeen et al.)

    Ω = √M / (r^(3/2) + a √M)
    u^t = (r^(3/2) + a √M) / (r^(3/4) √(r^(3/2) − 3M √r + 2a √M))

The photon's energy E and its angular momentum about the disk axis,
L_y = λE, are conserved. With E_emit = −p·u:

    g = E_camera / ( u^t (1 − Ω λ) )

E_camera is the camera's blueshift from section 6. It is 1/√(1 − rs/r) for
a hovering camera at spin 0, which gives back Phase 4's
g = √(1 − 1.5 rs/r) / (√(1 − rs/r_obs) (1 − Ω λ)). λ is minus the traced
photon's L, because tracing reverses it. Gas moving toward the camera has
Ωλ > 0, so g > 1.

**Brightness.** A blackbody at temperature T that is shifted by g looks like
a blackbody at gT. Its total intensity scales as g⁴ (I_ν/ν³ is invariant
along a ray), so the pixel is

    blackbody(g T) × (g T)⁴

That is why the approaching side (left in the start view) is much brighter
and whiter than the receding side.

**Checks** (`accretion_disk_tests`): the closed-form flux has the same shape
(to 1e-5) as the Page–Thorne integral

    F ∝ −Ω' / (r (E − ΩL)²) ∫ (E − ΩL) L' dr

done numerically from the circular-orbit E, L, Ω at spins 0, 0.7, −0.5 and
0.998. The gas 4-velocity has unit length in the Boyer–Lindquist metric. The
spin-0 redshift and Doppler limits from Phase 4 still hold.

## 8. Disk animation

**Keplerian rotation** (`AccretionDisk::angularVelocity`). A circular orbit
has, in coordinate time,

    Ω = dφ/dt = √M / (r^(3/2) + a √M)

At spin 0 this is exactly Kepler's third law, Ω = √(M / r³). Spin makes
prograde orbits a little slower at the same r, but they reach further in.
The inner disk turns much faster than the outer disk (Ω ∝ r^(-3/2)).

**The gas pattern** (`tracer.comp`, `diskDensity`) is fractal value noise.
It is sampled on a circle in noise space, so it has no seam in angle. At each
hit it is looked up at the angle the gas had at t = 0:

    φ₀ = φ - Ω(r) t

so every radius carries its own clumps around at its own rate. The noise
scales the emitted brightness (as a density would) but not the temperature.
(φ here is the Kerr–Schild angle. It differs from Boyer–Lindquist φ by an
amount that depends only on r, which just twists the pattern a little.)

**Time** is coordinate time in units of rs/c, advancing at 5 rs/c per real
second. At spin 0:

| Radius | Orbit period | Real time |
|---|---|---|
| 3 rs (inner edge) | 2π √54 rs/c ≈ 46 rs/c | ≈ 9 s |
| 12 rs (outer edge) | ≈ 369 rs/c | ≈ 74 s |

At spin 0.6 the inner edge (1.9 rs) takes about 25 rs/c, or 5 s.

Measuring time in rs/c means Ω·t does not jump when the mass changes. A
physically bigger hole would really turn slower.

**Winding.** Differential rotation shears any pattern into ever-tighter
spirals. Real disks have this too, and turbulence keeps making new clumps.
Ours doesn't, so after many inner orbits the inner disk becomes fine rings.

**Simplifications:**

- Every pixel sees the disk at the same instant. Light travel time
  (tens of rs/c) would really make the lensed far-side image lag behind.
- The clock is coordinate time, not the camera's own time. The two differ by
  about 3% at 20 rs.
- No gas inside the ISCO: in reality it plunges in and glows faintly.

## 9. From radiance to pixels

This part is rendering, not physics:

1. `tracer.comp` writes linear radiance into an RGBA16F image. Values can go
   well above 1 on the bright side of the disk.
2. **Bloom** imitates light scattering in a camera lens or eye:
   - Bright-pass: keep what exceeds 1.0, with a soft knee.
   - Downsample: six half-size levels, each made with a 13-tap filter.
   - Upsample: each level adds a tent-blurred copy of the level below, back
     up to the top. Strength 0.1.
3. **Tone map**: × exposure (E/Q keys), then the ACES filmic curve. Bright
   values roll off toward white instead of clipping.
