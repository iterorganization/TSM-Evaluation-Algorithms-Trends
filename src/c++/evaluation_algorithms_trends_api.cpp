/**
 * @file evaluation_algorithms_trends_api.cpp
 * @brief Implementation of the piecewise estimation algorithm for sensor measurement processing.
 *
 * This module provides the core implementation of the piecewise algorithm class, which manages
 * sensor estimation with piecewise linear approximations. It handles sensor configuration,
 * initialization with physical parameters, and step-wise updates of sensor estimates based on
 * temperature and current measurements.
 *
 * @author Daniel Iglesias, IO TSM Team
 * @version 1.0
 */

#include "evaluation_algorithms_trends_lib.hpp"

#include <stdexcept>

namespace rec_piecewise {

/**
 * @brief Configures the piecewise algorithm with the provided estimator configuration.
 *
 * This method initializes the algorithm with sensor configuration and default parameters.
 * It resets all existing estimators and sensor data before applying the new configuration.
 * Must be called before Initialize() or step().
 *
 * @param cfg The EstimatorConfig containing sensor definitions and algorithm parameters.
 *
 * @throws None directly, but subsequent calls to Initialize() or step() will throw
 *         if this configuration method has not been called.
 *
 * @note This method must be called before any estimation operations.
 */
void piecewise_algorithm::configure(const EstimatorConfig& cfg) {
    cfg_ = cfg;
    configured_ = true;
    estimators.clear();
    step_data_.sensors.clear();
}

/**
 * @brief Initializes the piecewise algorithm with initial sensor values and physical parameters.
 *
 * This method sets up all sensor estimators with their initial measurement values and
 * initializes the algorithm state with the provided physical conditions (temperature and current).
 * The configure() method must be called before this initialization.
 *
 * @param initial_values Vector of pairs containing sensor IDs and their initial measurement values.
 * @param temperature_K Initial temperature in Kelvin.
 * @param current_A Initial current in Amperes.
 *
 * @throws std::runtime_error if configure() has not been called before initialization.
 *
 * @pre configure() must have been called to set up the algorithm configuration.
 * @post All sensor estimators are initialized and ready for step-wise updates.
 */
void piecewise_algorithm::Initialize(const std::vector<std::pair<std::string, double>>& initial_values,
                                     double temperature_K,
                                     double current_A) {
    if (!configured_) {
        throw std::runtime_error("piecewise_algorithm must be configured before Initialize");
    }

    estimators.clear();
    step_data_.sensors.clear();
//    estimators.reserve(cfg_.sensors.size()); // Not available for normal std::map
    for (const auto& s : cfg_.sensors) {
        estimators.emplace(s.id, SensorEstimator(s, 0.0, cfg_, cfg_.default_sensor_params));
    }

    initialize(estimators, initial_values, temperature_K, current_A);
}

/**
 * @brief Retrieves the current step data containing all sensor estimates and measurements.
 *
 * This method provides read-only access to the most recent step data computed during
 * the last call to step(). The returned data includes sensor measurements, estimates,
 * and projected values for different operating conditions.
 *
 * @return A const reference to the current StepData structure containing:
 *         - temperature_K: Current temperature
 *         - current_A: Current current
 *         - sensors: Vector of SensorStepData for each active sensor
 *
 * @note The returned reference is valid only until the next call to step().
 */
const piecewise_algorithm::StepData& piecewise_algorithm::getStepData() const {
    return step_data_;
}

/**
 * @brief Performs a single step of the piecewise estimation algorithm.
 *
 * This method updates the estimation for all configured sensors based on the current
 * measurement values and physical conditions. Each sensor estimator is updated independently
 * and the results are aggregated into step data. The algorithm operates in different modes
 * to handle various operational scenarios.
 *
 * @param mode The operation mode (e.g., baking, energization) for the step.
 * @param temperature_K Current temperature in Kelvin for this step.
 * @param current_A Current current in Amperes for this step.
 * @param current_measurement Vector of pairs containing sensor IDs and their measured values
 *                            for this time step.
 *
 * @return A const reference to the updated StepData containing:
 *         - temperature_K: The provided temperature
 *         - current_A: The provided current
 *         - sensors: Vector of SensorStepData with estimates, projections, and bias information
 *                    for each sensor with available measurements
 *
 * @throws std::runtime_error if configure() has not been called before execution.
 *
 * @pre configure() and Initialize() must have been called before stepping.
 * @note The returned reference is valid only until the next call to step().
 * @note Sensors not present in current_measurement are skipped but remain in the estimators map.
 */
const piecewise_algorithm::StepData& piecewise_algorithm::step(
    OperationMode mode,
    double temperature_K,
    double current_A,
    const std::vector<std::pair<std::string, double>>& current_measurement) {
    if (!configured_) {
        throw std::runtime_error("piecewise_algorithm must be configured before step");
    }

    step_data_.temperature_K = temperature_K;
    step_data_.current_A = current_A;
    step_data_.sensors.clear();
    step_data_.sensors.reserve(current_measurement.size());

    for (const auto& [sensor_id, measured_value] : current_measurement) {
        auto it = estimators.find(sensor_id);
        if (it == estimators.end()) continue;

        InputSample sample;
        sample.temperature_K = temperature_K;
        sample.current_A = current_A;
        sample.measured_value = measured_value;
        sample.dt_s = cfg_.default_dt_s;
        it->second.step(mode, sample);

        piecewise_algorithm::SensorStepData data;
        data.sensor_id = sensor_id;
        data.measured_value = measured_value;
        data.raw_estimate = it->second.rawEstimate();
        data.estimate = it->second.estimate();
        data.projected_4K_0kA = it->second.projectedCooldownFinal();
        data.projected_4K_34kA = it->second.projectedEnergizationFinal();
        data.bias = it->second.bias();
        data.bias_rate = it->second.biasRate();
        step_data_.sensors.push_back(data);
    }

    return step_data_;
}

}  // namespace rec_piecewise
