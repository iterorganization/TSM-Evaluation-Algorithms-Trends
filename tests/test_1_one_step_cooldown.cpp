#include <algorithm>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "piecewise_estimator_lib.hpp"

using namespace rec_piecewise;

int main() {
    try {
        EstimatorConfig cfg = loadConfig("sensors_config_example.json", "configuration_example.json");
        std::unordered_map<std::string, double> initialMeasurements;
        for (const auto& s : cfg.sensors) initialMeasurements[s.id] = 0.0;

        std::vector<SensorEstimator> estimators;
        estimators.reserve(cfg.sensors.size());
        for (const auto& s : cfg.sensors) {
            estimators.emplace_back(s, initialMeasurements[s.id], cfg, cfg.default_sensor_params);
        }

        InputSample cooldownSample;
        cooldownSample.temperature_K = 100.0;
        cooldownSample.current_A = 0.0;
        cooldownSample.dt_s = 60.0;

        std::cout << "[test-1-one-step-cooldown] Loaded sensors: " << cfg.sensors.size() << "\n";
        std::cout << std::fixed << std::setprecision(6);

        for (std::size_t i = 0; i < std::min<std::size_t>(5, estimators.size()); ++i) {
            cooldownSample.measured_value = estimators[i].estimate();
            estimators[i].step(OperationMode::Cooldown, cooldownSample);
            std::cout << estimators[i].tag() << " | raw_estimate=" << estimators[i].rawEstimate()
                      << " | estimate=" << estimators[i].estimate() << " "
                      << estimators[i].unit() << " | bias=" << estimators[i].bias() << "\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
