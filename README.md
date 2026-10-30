@page  piecewise Evaluation algorithms for trend analysis
 
<img src="TSM-piecewise-banner.png" style="height:128px;margin-right:32px"/>

# Algorithms for Trend Analysis and Evaluation 

A C++ Kalman filter implementation for real-time estimation of mechanical sensor measurements. This version has been tested and validated for MCTF operations during TF coil **cooldown** (ambient → 4K) and **energization** (0kA → 34kA) phases, using FEM-derived target values as physics priors.

## Quick Start

### 1. Prerequisites

```bash
g++ >= 7.0 (C++17)
cmake >= 3.20
```


### 2. Build the Library

```bash
cmake -B build -S .
cmake --build build -j$(nproc)
```

This generates the library artifacts under `build/lib/`:

- `libevaluation_algorithms_trends.a` (static library)
- `libevaluation_algorithms_trends.so` (shared library)

Both libraries bundle:

- `evaluation_algorithms_trends_lib.cpp`: core algorithm (SensorEstimator, dynamics, projections, loaders)
- `evaluation_algorithms_trends_api.cpp`: high-level orchestration API (piecewise_algorithm class)


### 3. Run Tests

Tests are located in the `./tests` directory and are built together with the library. Enable tests during the CMake configuration:

```bash
cmake -B build -S . -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

**Expected output:**

```
100% tests passed, 0 tests failed out of 1
```

The test build automatically stages the required `sensors_config_example.json` and `configuration_example.json` configuration files, then compares each test's stdout against reference files in `./tests/reference/`.

## Library Architecture

All public symbols are under the `rec_piecewise` namespace.

The implementation is split into two compilation units:

- `evaluation_algorithms_trends_api.cpp`: orchestration API only (`piecewise_algorithm` methods)
- `evaluation_algorithms_trends_lib.cpp`: config/CSV loaders, initialization helpers, SensorEstimator implementation, dynamics, projections, history loading, and shared parsing helpers (`extract*`)

This split keeps the integration entry points isolated from the estimator internals.


## Inputs

### Configuration Files

**`sensors_config_example.json`** (located in `./tests/`):

```json
{
  "reference_temperature_K": 293.15,
  "energization_max_current_A": 34000.0,
  "sensors": [
    {
      "id": "Z1",
      "tag": "Z1",
      "description": "Displacement measurement",
      "unit": "mm",
      "frequency_Hz": 1.0,
      "cooldown_target_4K_0kA": 10.0,
      "energization_target_4K_34kA": 10.0
    },
    ...
  ]
}
```

**`configuration_example.json`** (model + Kalman parameters, located in `./tests/`):

```json
{
  "model": {
    "reference_temperature_K": 293.15,
    "cooldown_target_temperature_K": 4.0,
    "energization_max_current_A": 34000.0,
    "clamp_inputs": true
  },
  "kalman": {
    "default_sensor_params": {
      "q_y": 1e-9,
      "q_bias": 1e-12,
      "q_bias_rate": 1e-18,
      "r_meas": 200
    }
  },
  "runtime": {
    "default_dt_s": 60.0,
    "initial_covariance_diagonal": {
      "p00": 1e-3,
      "p11": 1e-3,
      "p22": 1e-3
    }
  }
}
```


### Runtime Inputs (`InputSample`)

| Field | Description | Units |
| :-- | :-- | :-- |
| `temperature_K` | Current coil temperature | K |
| `current_A` | Toroidal field current | A |
| `measured_value` | Raw sensor reading | sensor units |
| `dt_s` | Time step since last update | seconds |

## Outputs

**Per-step (from `SensorEstimator` after each `step()` call):**

| Method | Description |
| :-- | :-- |
| `rawEstimate()` | Physics model prediction at current T and I, without filter correction |
| `estimate()` | Filter-corrected value (physics prior + Kalman correction) |
| `bias()` | Estimated slow bias/drift offset |
| `biasRate()` | Estimated bias trend rate |
| `projectedCooldownFinal()` | Projection of current estimate to final cooldown state (4 K, 0 A) |
| `projectedEnergizationFinal()` | Projection of current estimate to final energization state (4 K, 34 kA) |
| `projectedEstimate(mode, T, I)` | Projection to any arbitrary target temperature and current |

**Low-level API:**

- `loadInitialMeasurements(path)`: CSV reader that returns `std::vector<std::pair<std::string, double>>`
- `initialize(estimators, initialValues, temperature_K, current_A)`: applies initial values to already-built estimators using reference-condition normalization

This allows external users to plug in custom readers (JSON, database, network) while reusing the same initialization entry point.

**High-level API (`piecewise_algorithm`):**

- `configure(cfg)`
- `Initialize(initial_values, temperature_K, current_A)`
- `step(mode, temperature_K, current_A, current_measurement)` returns `const StepData&`
- `getStepData()` returns the cached `const StepData&` from the most recent call to `step(...)`

`piecewise_algorithm` stores the estimator map internally and provides a compact orchestration interface for batch updates.

`StepData` contains one complete time-step snapshot:

- `temperature_K`
- `current_A`
- `sensors`: vector of per-sensor records

Each per-sensor record contains:

- `sensor_id`
- `measured_value`
- `raw_estimate`
- `estimate`
- `projected_4K_0kA`
- `projected_4K_34kA`
- `bias`
- `bias_rate`

The predict step performs full covariance propagation through the coupled state model:

- low-temperature cooldown estimates converge consistently,
- bias and bias-rate remain observable through the filter,
- and the projection CSV outputs align with the corrected estimator behavior.

The projection methods transfer the filter-learned correction $\Delta_k = \hat{y}_k - y_{\text{phys},k}$ to the requested final state:

$$\hat{y}_{\text{proj,final}} = y_{\text{phys,final}} + \Delta_k$$

This gives a best estimate of what each sensor would read at the end of the operation, conditioned on all measurements seen so far.


**Example console output (test_1):**

```
[test-1-one-step-cooldown] Initialized successfully
ZT2100 | estimate=26.400000 mm | bias=0.0
ZT2200 | estimate=26.400000 mm | bias=0.0
...
```

This basic test verifies single-step Kalman filter operation with a simple cooldown measurement.


## Methodology

### State-Space Model

Each sensor uses a **3-state Kalman filter**:

```
x_k = [ŷ_k,  b_k,  ḃ_k]ᵀ
    = [estimated value, bias, bias rate]
```

**Physics model** (piecewise):

```
Cooldown (T: 293K→4K):   ŷ(T) = y₀ + α(T) × δ_{4K,0kA}
Energization (I: 0→34kA): ŷ(T,I) = y₀ + α(T) × δ_{4K,0kA} + β(I)² × Δ_{34kA}
```

Where:

- `α(T) = (293K - T) / (293K - 4K)` (thermal progress)
- `β(I) = I / 34kA` (current progress)
- `δ_{4K,0kA}` = FEM cooldown target
- `Δ_{34kA}` = FEM energization delta

**Dynamics:**

```
b_{k+1} = b_k + dt × ḃ_k + w_bias
ḃ_{k+1} = ḃ_k + w_bias_rate
ŷ_{k+1} = physics(T,I) + b_{k+1} + w_y
```

Where:

- `w_bias` = zero-mean Gaussian process noise on the bias state, variance `q_bias` (accounts for unmodeled slow drift in the bias)
- `w_bias_rate` = zero-mean Gaussian process noise on the bias-rate state, variance `q_bias_rate` (accounts for changes in the drift trend)
- `w_y` = zero-mean Gaussian process noise on the estimated value state, variance `q_y` (accounts for imperfections in the piecewise physics model, e.g. deviations from the linear thermal/quadratic current approximation)

**Measurement:**

```
z_k = ŷ_k + v_k
```

Where:

- `z_k` = raw sensor reading at step `k`
- `v_k` = zero-mean Gaussian measurement noise, variance `r_meas` (accounts for sensor noise and quantization)


### Kalman Filter Steps

1. **Predict**: Apply physics model + bias evolution
2. **Update**: Fuse with new measurement via Kalman gain
3. **Output**: Bias-corrected sensor estimate

## Limitations

### Model Assumptions

1. **Linear thermal interpolation** between ambient and 4K
2. **Quadratic current scaling** (β(I)²) during energization
3. **Slow bias + linear trend** sufficient for 16-day drift
4. **Operation modes** (manual mode switching)

### Technical Limitations

1. **Single-threaded** (scales linearly with \#sensors)
2. **No sensor fusion** (independent estimators per sensor)
3. **No outlier rejection** (assumes Gaussian noise)
4. **Simplified physics** (no hysteresis, cross-sensor coupling)

## Tuning Parameters

| Parameter | Effect | Typical Range |
| :-- | :-- | :-- |
| `q_y` | Signal model trust | 1e-10 → 1e-6 |
| `q_bias` | Bias evolution speed | 1e-12 → 1e-8 |
| `q_bias_rate` | Trend adaptability | 1e-14 → 1e-10 |
| `r_meas` | Sensor noise trust | 1e-8 → 1e-4 |

**Tuning rule**: Higher `q_*` → faster adaptation, less physics trust. Higher `r_meas` → less sensor trust.

## Future Work

### High Priority

1. **Live measurement input** (Integration with TSM POZ app)
2. **Automatic phase detection** (T<20K+I>1kA → energization)
3. **Additional initial-value sources** (JSON/DB/API readers) reusing `initialize(estimators, initialValues, temperature_K, current_A)`

### Medium Priority

1. **Multi-sensor fusion** (cross-sensor consistency)
2. **Nonlinear EKF** (true FEM table interpolation)
3. **Outlier rejection** (innovation-based gating)
4. **REST API** (web dashboard integration)

### Low Priority

1. **GPU acceleration** (1000s of sensors)
2. **Distributed mode** (multiple coil sectors)
3. **ML parameter tuning** (auto-calibrate Q/R matrices)

## CMake Build Variants

```bash
# Default (Release with warnings)
cmake -B build -S .
cmake --build build -j$(nproc)

# Debug build (with symbols and no optimization)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DTSM_DEBUG=ON
cmake --build build -j$(nproc)

# Release build (optimized)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Clean build artifacts
rm -rf build/
```


## Example Workflow

```
1. Cooldown starts → set initial measurements at 293K
2. Feed T measurements → track cooldown progress vs FEM
3. Call projectedCooldownFinal() at each step → track end-of-cooldown projection
4. Monitor bias growth → detect sensor drift early
5. At 4K → switch to Energization mode
6. Feed T+I measurements → track quadratic current effect
7. Call projectedEnergizationFinal() at each step → track end-of-energization projection
8. Monitor innovations → flag anomalous sensors
```


***

**Author**: Daniel Iglesias
