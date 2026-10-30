/**
 * @file piecewise_estimator_lib.hpp
 * @brief Piecewise linear estimation algorithm for sensor state prediction in cryogenic systems.
 *
 * Provides configuration structures, Kalman filter-based sensor estimation, and main algorithm
 * interface for multi-sensor fusion in superconducting magnet systems.
 *
 * @author Daniel Iglesias, IO TSM Team
 * @version 1.0
 */

#pragma once

#include <utility>
#include <string>
#include <map>
#include <vector>

namespace rec_piecewise {

/// @brief Operation mode for physical model selection.
enum class OperationMode { Cooldown, Energization };

/// @brief Configuration parameters for a single sensor.
struct SensorConfig {
    std::string id;                              ///< Unique sensor identifier
    std::string tag;                             ///< Human-readable sensor tag
    std::string description;                     ///< Sensor description
    std::string unit;                            ///< Measurement unit
    double frequency_Hz = 0.0;                   ///< Sampling frequency
    double cooldown_target_4K_0kA = 0.0;         ///< Target value at 4K, 0kA
    double energization_target_4K_34kA = 0.0;    ///< Target value at 4K, 34kA
};

/// @brief Kalman filter parameters for sensor estimation.
struct SensorParams {
    double q_y = 1e-8;            ///< Process noise covariance for measurement
    double q_bias = 1e-10;         ///< Process noise covariance for bias
    double q_bias_rate = 1e-12;    ///< Process noise covariance for bias rate
    double r_meas = 1e-6;          ///< Measurement noise covariance
};

/// @brief Global algorithm configuration.
struct EstimatorConfig {
    double reference_temperature_K = 293.15;              ///< Reference temperature (ambient)
    double cooldown_target_temperature_K = 4.0;           ///< Target cooldown temperature
    double energization_max_current_A = 34000.0;          ///< Max current during energization
    bool clamp_inputs = true;                             ///< Enable input clamping
    SensorParams default_sensor_params;                   ///< Default Kalman parameters
    struct InitialCovarianceDiagonal {                    ///< Initial state covariance
        double p00 = 1e-3;                                ///< Measurement variance
        double p11 = 1e-3;                                ///< Bias variance
        double p22 = 1e-3;                                ///< Bias rate variance
    } initial_covariance_diagonal;
    double default_dt_s = 60.0;                           ///< Default time step
    std::vector<SensorConfig> sensors;                    ///< Configured sensors
};

/// @brief Input sample for estimation step.
struct InputSample {
    double temperature_K = 293.15;    ///< Current temperature
    double current_A = 0.0;           ///< Current operating current
    double measured_value = 0.0;      ///< Measured sensor value
    double dt_s = 1.0;                ///< Time step
};

/// @brief Historical measurement sample for initialization.
struct HistorySample {
    double temperature_K = 293.15;    ///< Temperature at measurement
    std::string sensor_id;            ///< Sensor identifier
    double measured_value = 0.0;      ///< Measured value
    double current_A = 0.0;           ///< Operating current
    double dt_s = 0.0;                ///< Time step
};

/**
 * @brief Load configuration from sensor and runtime files.
 * @param sensorsPath Path to sensor configuration file.
 * @param runtimePath Path to runtime configuration file.
 * @return Populated EstimatorConfig structure.
 */
EstimatorConfig loadConfig(const std::string& sensorsPath, const std::string& runtimePath);

/**
 * @brief Load initial sensor measurements from file.
 * @param path Path to initial measurements file.
 * @return Vector of (sensor_id, value) pairs.
 */
std::vector<std::pair<std::string, double>> loadInitialMeasurements(const std::string& path);

/**
 * @brief Load historical samples from file.
 * @param path Path to history file.
 * @return Vector of historical samples.
 */
std::vector<HistorySample> loadHistorySamples(const std::string& path);

/**
 * @brief Kalman filter-based sensor state estimator.
 *
 * Maintains state (measurement, bias, bias rate) and covariance with predict-update cycle.
 * Includes physics-based models for temperature and current effects.
 */
class SensorEstimator {
public:
    /// @brief Construct sensor estimator with configuration.
    SensorEstimator(const SensorConfig& cfg, double initial_measurement, const EstimatorConfig& global, const SensorParams& params);

    /// @brief Set initial measurement value.
    void setInitialMeasurement(double y0);
    
    /// @brief Execute one estimation step.
    void step(OperationMode mode, const InputSample& u);

    /// @brief Get unbiased raw estimate.
    double rawEstimate() const;
    
    /// @brief Get bias-corrected estimate.
    double estimate() const;
    
    /// @brief Get estimated bias term.
    double bias() const;
    
    /// @brief Get estimated bias rate of change.
    double biasRate() const;
    
    /// @brief Project estimate to target conditions.
    double projectedEstimate(OperationMode mode, double target_temperature_K, double target_current_A = 0.0) const;
    
    /// @brief Project to final cooldown state (4K, 0kA).
    double projectedCooldownFinal() const;
    
    /// @brief Project to final energization state (4K, 34kA).
    double projectedEnergizationFinal() const;
    
    /// @brief Compute reference measurement at given conditions.
    double referenceInitialMeasurement(double measured_value, double temperature_K, double current_A) const;
    
    /// @brief Get sensor ID.
    const std::string& id() const;
    
    /// @brief Get sensor tag.
    const std::string& tag() const;
    
    /// @brief Get measurement unit.
    const std::string& unit() const;

private:
    /// @brief Kalman filter state vector.
    struct State {
        double y = 0.0;             ///< Measurement estimate
        double bias = 0.0;          ///< Bias estimate
        double bias_rate = 0.0;     ///< Bias rate estimate
    };

    /// @brief Symmetric covariance matrix (3x3).
    struct Covariance {
        double p00 = 1e-3, p01 = 0.0, p02 = 0.0;    ///< Row 0
        double p10 = 0.0, p11 = 1e-3, p12 = 0.0;    ///< Row 1
        double p20 = 0.0, p21 = 0.0, p22 = 1e-3;    ///< Row 2
    };

    /// @brief Compute physics model output for cooldown mode.
    double cooldownPhysics(double T) const;
    
    /// @brief Compute physics model output for energization mode.
    double energizationPhysics(double T, double I) const;
    
    /// @brief Get current sampling interval.
    double sampleDtS() const;
    
    /// @brief Kalman predict step.
    void predict(OperationMode mode, const InputSample& u);
    
    /// @brief Kalman update step.
    void update(double z);
    
    /// @brief Reset covariance to initial values.
    void resetCovariance();

    SensorConfig cfg_;                    ///< Sensor configuration
    double y0_;                           ///< Initial measurement
    EstimatorConfig global_;              ///< Global configuration
    SensorParams params_;                 ///< Kalman parameters
    double raw_estimate_ = 0.0;           ///< Raw estimate output
    State x_;                             ///< State vector
    Covariance P_;                        ///< Covariance matrix
};

/**
 * @brief Initialize all sensor estimators.
 * @param estimators Map of sensor estimators to initialize.
 * @param initialValues Initial measurement values for each sensor.
 * @param temperature_K Initial temperature.
 * @param current_A Initial current.
 */
void initialize(std::map<std::string, SensorEstimator>& estimators,
                const std::vector<std::pair<std::string, double>>& initialValues,
                double temperature_K,
                double current_A);

/**
 * @brief Main piecewise estimation algorithm managing multiple sensors.
 *
 * Orchestrates sensor configuration, initialization, and step-wise updates.
 * Provides aggregated estimates for all sensors at each time step.
 */
class piecewise_algorithm {
public:
    /// @brief Output data for a single sensor at one step.
    struct SensorStepData {
        std::string sensor_id;              ///< Sensor identifier
        double measured_value = 0.0;        ///< Measured value
        double raw_estimate = 0.0;          ///< Unbiased estimate
        double estimate = 0.0;              ///< Bias-corrected estimate
        double projected_4K_0kA = 0.0;      ///< Projected cooldown final
        double projected_4K_34kA = 0.0;     ///< Projected energization final
        double bias = 0.0;                  ///< Estimated bias
        double bias_rate = 0.0;             ///< Estimated bias rate
    };

    /// @brief Aggregated output data for all sensors at one step.
    struct StepData {
        double temperature_K = 0.0;                  ///< Temperature at step
        double current_A = 0.0;                      ///< Current at step
        std::vector<SensorStepData> sensors;         ///< Per-sensor data
    };

    /// @brief Configure algorithm with sensor and parameter definitions.
    void configure(const EstimatorConfig& cfg);
    
    /// @brief Initialize all sensors with measurement and condition values.
    void Initialize(const std::vector<std::pair<std::string, double>>& initial_values,
                    double temperature_K,
                    double current_A);
    
    /// @brief Get most recent step output.
    const StepData& getStepData() const;
    
    /// @brief Execute one estimation step with current measurements.
    const StepData& step(OperationMode mode,
                         double temperature_K,
                         double current_A,
                         const std::vector<std::pair<std::string, double>>& current_measurement);

    std::map<std::string, SensorEstimator> estimators;  ///< Sensor estimators map

private:
    EstimatorConfig cfg_;           ///< Algorithm configuration
    bool configured_ = false;       ///< Configuration state flag
    StepData step_data_;            ///< Most recent step output
};

}  // namespace rec_piecewise
