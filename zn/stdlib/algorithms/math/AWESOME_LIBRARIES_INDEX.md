# Nova Math Libraries - Awesome Python Math Packages Port

This directory contains Nova implementations of popular Python math packages from the [Awesome Python Math Packages](https://github.com/VascoSch92/awesome_python_math_packages) repository.

## 📦 Implemented Libraries

### Algebra
- **`algebra/galois_fields.zn`** - Finite field arithmetic (inspired by `galois`)
  - GF(p) and GF(p^m) implementations
  - Reed-Solomon error correction
  - Cryptographic applications (GF(256), GF(2^128))

- **`algebra/quaternions.zn`** - Quaternion and Octonion algebra (inspired by `pyquaternion`, `pyoctonion`)
  - 3D rotations and orientations
  - SLERP interpolation
  - Conversion between representations (axis-angle, Euler, matrix)
  - Octonion support (Cayley-Dickson construction)

- **`algebra/polynomials.zn`** - Polynomial operations (inspired by `multivar_horner`)
  - Univariate and multivariate polynomials
  - Horner's method for efficient evaluation
  - Polynomial arithmetic and differentiation

### Analysis
- **`analysis/pde_solvers.zn`** - PDE solvers (inspired by `py-pde`, `findiff`, `sfepy`)
  - Heat equation (1D)
  - Wave equation (1D)
  - Poisson equation (2D)
  - Finite difference methods
  - Spectral methods (FFT-based)

- **`analysis/spherical_harmonics.zn`** - Spherical harmonic analysis (inspired by `SHTOOLS`)
  - Associated Legendre polynomials
  - Spherical harmonic functions Y_l^m
  - Forward and inverse spherical harmonic transforms

### Graph Theory
- **`graph_theory/core.zn`** - Graph algorithms (inspired by `graph-theory`)
  - BFS and DFS traversal
  - Dijkstra's shortest path
  - Prim's MST algorithm
  - Topological sort
  - Bipartite checking

### Number Theory
- **`number_theory/primes.zn`** - Prime number tools (inspired by `primesieve`, `primePy`)
  - Sieve of Eratosthenes
  - Segmented sieve for large ranges
  - Miller-Rabin primality testing
  - Prime factorization
  - Euler's totient function
  - nth prime computation

- **`number_theory/padic.zn`** - p-adic numbers (inspired by `pyadic`)
  - p-adic arithmetic
  - p-adic norm and valuation
  - Hensel's lemma for lifting solutions

### Geometry
- **`geometry/hyperbolic.zn`** - Hyperbolic geometry (inspired by `hyperbolic`)
  - Poincaré disk model
  - Upper half-plane model
  - Hyperbolic distance
  - Möbius transformations

### Statistics
- **`statistics/advanced.zn`** - Advanced statistical methods (inspired by `scipy.stats`)
  - Monte Carlo methods (integration, MCMC)
  - Metropolis-Hastings algorithm
  - Hamiltonian Monte Carlo
  - Advanced distributions (Beta, Gamma, Dirichlet, Student's t)
  - Hypothesis testing (t-test, chi-square, KS test)

## 🚀 Quick Start

```nova
import stdlib.math.algebra.quaternions
import stdlib.math.number_theory.primes
import stdlib.math.statistics.advanced

// Quaternion rotation
let q = Quaternion::from_axis_angle(vec![0.0, 0.0, 1.0], Math.PI/2.0)
let v_rotated = q.rotate_vector(vec![1.0, 0.0, 0.0])

// Prime numbers
let primes = Primes::sieve_of_eratosthenes(1000)
let is_prime = Primes::is_prime_miller_rabin(104729, 10)

// Monte Carlo integration
let integral = MonteCarlo::integrate(|x| x*x, 0.0, 1.0, 10000)
```

## 📚 Examples

See `examples/awesome_math_demo.zn` for comprehensive demonstrations of all libraries.

## 🔗 Original Python Packages

This implementation was inspired by these excellent Python packages:
- galois - Galois field arithmetic
- pyquaternion - Quaternion algebra
- py-pde - PDE solvers
- SHTOOLS - Spherical harmonics
- primesieve - Fast prime generation
- pyadic - p-adic numbers
- hyperbolic - Hyperbolic geometry
- scipy.stats - Statistical distributions

## 📖 Documentation

Each module contains inline documentation with:
- Mathematical background
- Usage examples
- Performance characteristics
- References to original implementations
