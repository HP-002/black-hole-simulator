# Physics notes

The physics behind the simulator, and where it lives in the code. Units are
geometric (G = c = 1), so the mass M sets every scale. The code works in
terms of the Schwarzschild radius:

    rs = 2M

Positions are in scene units. The 3D view keeps the disk radii and the camera
speed as multiples of rs, so the Up/Down keys rescale the whole system.

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

**Cartesian form** (`Schwarzschild.cpp`, `tracer.comp`). Take the parameter
so that r² dφ/dλ = h, with h = |x × dx/dλ| constant. Then Binet's identity
turns the orbit equation into a central "force":

    d²x/dλ² = -1.5 rs h² x / r⁵

This is integrated in 3D with RK4. Because the force is central, h is
conserved and the ray stays in its plane automatically, so no plane or angle
bookkeeping is needed. The step is a fixed fraction of r (0.05 on the GPU,
0.01 on the CPU reference): small steps near the hole, big ones far out.

A ray ends when it:

- falls inside rs (captured: black),
- moves outward beyond the escape radius, max(50 rs, 2 × camera distance)
  (it samples the star field in its final direction), or
- runs out of steps (500 on the GPU). It is then orbiting near the photon
  sphere and is treated as captured.

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

## 4. The accretion disk

A thin, opaque disk in the plane y = 0, from 3 rs (ISCO) to 12 rs. A ray
shows the disk the first time it crosses the plane inside those radii. Rays
that cross outside them go on, which is how the far side of the disk appears
lensed over the top of the shadow.

**Temperature** (`AccretionDisk::temperature`). This is the thin-disk
(Shakura–Sunyaev) profile with no torque at the inner edge:

    T(r) ∝ r^(-3/4) (1 - √(r_in / r))^(1/4)

It is zero at r_in and peaks at r = (49/36) r_in. The code normalizes the
peak to 1. It then maps it to 4000 K for the color, which is artistic: a real
stellar-mass disk peaks in X-rays and would look white-blue.

**Redshift.** g = E_observed / E_emitted combines three effects:

- the gas's orbital Doppler shift,
- gravitational redshift climbing out from the gas's radius,
- a blueshift falling to the observer's radius.

The gas moves on a circular orbit with 4-velocity u = u^t (1, 0, 0, Ω) and
u^t = 1/√(1 - 1.5 rs/r). The photon's energy E and its angular momentum about
the disk axis, L_y = λE, are conserved. With E_emit = -p·u:

    g = √(1 - 1.5 rs/r) / ( √(1 - rs/r_obs) (1 - Ω λ) )

λ is the photon's impact parameter times the disk-axis part of its orbit
direction. The sign is flipped because rays are traced backwards from the
camera. Gas moving toward the camera has Ωλ > 0, so g > 1.

**Brightness.** A blackbody at temperature T that is shifted by g looks like
a blackbody at gT. Its total intensity scales as g⁴ (I_ν/ν³ is invariant
along a ray), so the pixel is

    blackbody(g T) × (g T)⁴

That is why the approaching side (left in the start view) is much brighter
and whiter than the receding side.

## 5. Disk animation

**Keplerian rotation** (`AccretionDisk::angularVelocity`). A circular orbit
in Schwarzschild has, in coordinate time,

    Ω = dφ/dt = √(M / r³) = √(rs / (2 r³))

This is exactly Kepler's third law. The inner disk turns much faster than the
outer disk (Ω ∝ r^(-3/2)).

**The gas pattern** (`tracer.comp`, `diskDensity`) is fractal value noise.
It is sampled on a circle in noise space, so it has no seam in angle. At each
hit it is looked up at the angle the gas had at t = 0:

    φ₀ = φ - Ω(r) t

so every radius carries its own clumps around at its own rate. The noise
scales the emitted brightness (as a density would) but not the temperature.

**Time** is coordinate time in units of rs/c, advancing at 5 rs/c per real
second:

| Radius | Orbit period | Real time |
|---|---|---|
| 3 rs (inner edge) | 2π √54 rs/c ≈ 46 rs/c | ≈ 9 s |
| 12 rs (outer edge) | ≈ 369 rs/c | ≈ 74 s |

Measuring time in rs/c means Ω·t does not jump when the mass changes. A
physically bigger hole would really turn slower.

**Winding.** Differential rotation shears any pattern into ever-tighter
spirals. Real disks have this too, and turbulence keeps making new clumps.
Ours doesn't, so after many inner orbits the inner disk becomes fine rings.

**Simplifications:**

- Every pixel sees the disk at the same instant. Light travel time
  (tens of rs/c) would really make the lensed far-side image lag behind.
- The clock is coordinate time, not the hovering camera's own time. The two
  differ by √(1 - rs/r): about 3% at 20 rs.

## 6. From radiance to pixels

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
