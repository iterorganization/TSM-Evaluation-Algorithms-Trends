#include "evaluation_algorithms_trends_lib.hpp"

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace rec_piecewise {

double extractNumber(const std::string& obj, const std::string& key) {
    std::regex re("\\\"" + key + "\\\"\\s*:\\s*([-+0-9.eE]+)");
    std::smatch m;
    if (!std::regex_search(obj, m, re)) throw std::runtime_error("Missing numeric key: " + key);
    return std::stod(m[1].str());
}

std::string extractString(const std::string& obj, const std::string& key) {
    std::regex re("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch m;
    if (!std::regex_search(obj, m, re)) return "";
    return m[1].str();
}

bool extractBool(const std::string& obj, const std::string& key, bool defaultValue) {
    std::regex re("\\\"" + key + "\\\"\\s*:\\s*(true|false)");
    std::smatch m;
    if (!std::regex_search(obj, m, re)) return defaultValue;
    return m[1].str() == "true";
}

std::string extractObject(const std::string& txt, const std::string& key) {
    std::size_t keyPos = txt.find("\"" + key + "\"");
    if (keyPos == std::string::npos) {
        throw std::runtime_error("Missing object key: " + key);
    }

    std::size_t objStart = txt.find('{', keyPos);
    if (objStart == std::string::npos) {
        throw std::runtime_error("Missing object body for key: " + key);
    }

    int depth = 0;
    for (std::size_t i = objStart; i < txt.size(); ++i) {
        if (txt[i] == '{') {
            ++depth;
        } else if (txt[i] == '}') {
            --depth;
            if (depth == 0) {
                return txt.substr(objStart, i - objStart + 1);
            }
        }
    }

    throw std::runtime_error("Malformed object for key: " + key);
}

static std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

EstimatorConfig loadConfig(const std::string& sensorsPath, const std::string& runtimePath) {
    EstimatorConfig cfg;
    std::string sensorsTxt = readFile(sensorsPath);
    cfg.reference_temperature_K = extractNumber(sensorsTxt, "reference_temperature_K");
    cfg.energization_max_current_A = extractNumber(sensorsTxt, "energization_max_current_A");

    std::string runtimeTxt = readFile(runtimePath);
    std::string modelObj = extractObject(runtimeTxt, "model");
    cfg.reference_temperature_K = extractNumber(modelObj, "reference_temperature_K");
    cfg.cooldown_target_temperature_K = extractNumber(modelObj, "cooldown_target_temperature_K");
    cfg.energization_max_current_A = extractNumber(modelObj, "energization_max_current_A");
    cfg.clamp_inputs = extractBool(modelObj, "clamp_inputs", true);

    std::string kalmanObj = extractObject(runtimeTxt, "kalman");
    std::string defaultSensorParamsObj = extractObject(kalmanObj, "default_sensor_params");
    cfg.default_sensor_params.q_y = extractNumber(defaultSensorParamsObj, "q_y");
    cfg.default_sensor_params.q_bias = extractNumber(defaultSensorParamsObj, "q_bias");
    cfg.default_sensor_params.q_bias_rate = extractNumber(defaultSensorParamsObj, "q_bias_rate");
    cfg.default_sensor_params.r_meas = extractNumber(defaultSensorParamsObj, "r_meas");

    std::string runtimeObj = extractObject(runtimeTxt, "runtime");
    cfg.default_dt_s = extractNumber(runtimeObj, "default_dt_s");
    std::string covarianceObj = extractObject(runtimeObj, "initial_covariance_diagonal");
    cfg.initial_covariance_diagonal.p00 = extractNumber(covarianceObj, "p00");
    cfg.initial_covariance_diagonal.p11 = extractNumber(covarianceObj, "p11");
    cfg.initial_covariance_diagonal.p22 = extractNumber(covarianceObj, "p22");

    std::size_t arrStart = sensorsTxt.find("\"sensors\"");
    if (arrStart == std::string::npos) throw std::runtime_error("Missing 'sensors' key in config");
    arrStart = sensorsTxt.find('[', arrStart);
    if (arrStart == std::string::npos) throw std::runtime_error("Missing sensors array in config");

    std::size_t pos = arrStart + 1;
    while (pos < sensorsTxt.size()) {
        std::size_t objStart = sensorsTxt.find('{', pos);
        std::size_t arrEnd = sensorsTxt.find(']', pos);
        if (objStart == std::string::npos || (arrEnd != std::string::npos && arrEnd < objStart)) break;

        int depth = 0;
        std::size_t objEnd = objStart;
        for (std::size_t i = objStart; i < sensorsTxt.size(); ++i) {
            if (sensorsTxt[i] == '{') {
                ++depth;
            } else if (sensorsTxt[i] == '}') {
                --depth;
                if (depth == 0) {
                    objEnd = i;
                    break;
                }
            }
        }
        if (depth != 0) throw std::runtime_error("Malformed sensor object in config");

        std::string obj = sensorsTxt.substr(objStart, objEnd - objStart + 1);
        SensorConfig s;
        s.id = extractString(obj, "id");
        s.tag = extractString(obj, "tag");
        s.description = extractString(obj, "description");
        s.unit = extractString(obj, "unit");
        s.cooldown_target_4K_0kA = extractNumber(obj, "cooldown_target_4K_0kA");
        s.energization_target_4K_34kA = extractNumber(obj, "energization_target_4K_34kA");
        cfg.sensors.push_back(s);

        pos = objEnd + 1;
    }

    if (cfg.sensors.empty()) throw std::runtime_error("No sensors loaded from config");
    return cfg;
}

std::vector<std::pair<std::string, double>> loadInitialMeasurements(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open file: " + path);

    std::vector<std::pair<std::string, double>> out;
    std::string line;
    std::size_t lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty()) continue;
        if (line.rfind("#", 0) == 0) continue;

        std::istringstream ss(line);
        std::string id;
        std::string valueToken;
        if (!std::getline(ss, id, ',')) continue;
        if (!std::getline(ss, valueToken)) {
            throw std::runtime_error("Malformed initial measurement at line " + std::to_string(lineNo));
        }

        if (id == "id" || id == "tag") continue;
        out.emplace_back(id, std::stod(valueToken));
    }

    return out;
}

void initialize(std::map<std::string, SensorEstimator>& estimators,
                const std::vector<std::pair<std::string, double>>& initialValues,
                double temperature_K,
                double current_A) {
    for (const auto& [id, measured_value] : initialValues) {
        auto it = estimators.find(id);
        if (it != estimators.end()) {
            double normalized =
                it->second.referenceInitialMeasurement(measured_value, temperature_K, current_A);
            it->second.setInitialMeasurement(normalized);
        }
    }
}

std::vector<HistorySample> loadHistorySamples(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open file: " + path);

    std::vector<HistorySample> out;
    std::string line;
    std::size_t lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty()) continue;
        if (line.rfind("#", 0) == 0) continue;

        std::istringstream ss(line);
        std::string tempToken;
        std::string idToken;
        std::string measToken;

        if (!std::getline(ss, tempToken, ',')) continue;
        if (!std::getline(ss, idToken, ',')) throw std::runtime_error("Missing sensor id at line " + std::to_string(lineNo));
        if (!std::getline(ss, measToken)) throw std::runtime_error("Missing measured value at line " + std::to_string(lineNo));

        if (tempToken == "temperature_K") continue;

        HistorySample h;
        h.temperature_K = std::stod(tempToken);
        h.sensor_id = idToken;
        h.measured_value = std::stod(measToken);
        out.push_back(h);
    }

    return out;
}

SensorEstimator::SensorEstimator(const SensorConfig& cfg, double initial_measurement, const EstimatorConfig& global, const SensorParams& params)
    : cfg_(cfg), y0_(initial_measurement), global_(global), params_(params) {
    raw_estimate_ = initial_measurement;
    x_.y = initial_measurement;
    resetCovariance();
}

void SensorEstimator::setInitialMeasurement(double y0) {
    y0_ = y0;
    raw_estimate_ = y0;
    x_ = {};
    x_.y = y0;
    resetCovariance();
}

void SensorEstimator::step(OperationMode mode, const InputSample& u) {
    predict(mode, u);
    update(u.measured_value);
}

double SensorEstimator::rawEstimate() const { return raw_estimate_; }

double SensorEstimator::estimate() const { return x_.y; }

double SensorEstimator::bias() const { return x_.bias; }

double SensorEstimator::biasRate() const { return x_.bias_rate; }

double SensorEstimator::projectedEstimate(OperationMode mode, double target_temperature_K, double target_current_A) const {
    double target_physics = (mode == OperationMode::Cooldown)
        ? cooldownPhysics(target_temperature_K)
        : energizationPhysics(target_temperature_K, target_current_A);

    // Keep the filter's learned correction and transfer it to the requested final state.
    double learned_correction = x_.y - raw_estimate_;
    return target_physics + learned_correction;
}

double SensorEstimator::projectedCooldownFinal() const {
    return projectedEstimate(OperationMode::Cooldown, global_.cooldown_target_temperature_K, 0.0);
}

double SensorEstimator::projectedEnergizationFinal() const {
    return projectedEstimate(
        OperationMode::Energization,
        global_.cooldown_target_temperature_K,
        global_.energization_max_current_A);
}

double SensorEstimator::referenceInitialMeasurement(double measured_value, double temperature_K, double current_A) const {
    double thermal_alpha = (global_.reference_temperature_K - temperature_K) /
        (global_.reference_temperature_K - global_.cooldown_target_temperature_K);
    double current_beta = current_A / global_.energization_max_current_A;
    if (global_.clamp_inputs) {
        thermal_alpha = std::clamp(thermal_alpha, 0.0, 1.0);
        current_beta = std::clamp(current_beta, 0.0, 1.0);
    }

    double cooldown_delta = cfg_.cooldown_target_4K_0kA;
    double energization_extra = cfg_.energization_target_4K_34kA - cfg_.cooldown_target_4K_0kA;
    double model_offset = thermal_alpha * cooldown_delta + energization_extra * current_beta * current_beta;
    return measured_value - model_offset;
}

const std::string& SensorEstimator::id() const { return cfg_.id; }

const std::string& SensorEstimator::tag() const { return cfg_.tag; }

const std::string& SensorEstimator::unit() const { return cfg_.unit; }

double SensorEstimator::cooldownPhysics(double T) const {
    double alpha = (global_.reference_temperature_K - T) /
        (global_.reference_temperature_K - global_.cooldown_target_temperature_K);
    if (global_.clamp_inputs) {
        alpha = std::clamp(alpha, 0.0, 1.0);
    }
    double delta = cfg_.cooldown_target_4K_0kA;
    return y0_ + alpha * delta;
}

double SensorEstimator::energizationPhysics(double T, double I) const {
    double thermal_alpha = (global_.reference_temperature_K - T) /
        (global_.reference_temperature_K - global_.cooldown_target_temperature_K);
    double current_beta = I / global_.energization_max_current_A;
    if (global_.clamp_inputs) {
        thermal_alpha = std::clamp(thermal_alpha, 0.0, 1.0);
        current_beta = std::clamp(current_beta, 0.0, 1.0);
    }
    double cooldown_delta = cfg_.cooldown_target_4K_0kA;
    double energization_extra = cfg_.energization_target_4K_34kA - cfg_.cooldown_target_4K_0kA;
    return y0_ + thermal_alpha * cooldown_delta + energization_extra * current_beta * current_beta;
}

void SensorEstimator::predict(OperationMode mode, const InputSample& u) {
    double dt_s = u.dt_s > 0.0 ? u.dt_s : global_.default_dt_s;
    x_.bias += dt_s * x_.bias_rate;

    double y_phys = (mode == OperationMode::Cooldown)
        ? cooldownPhysics(u.temperature_K)
        : energizationPhysics(u.temperature_K, u.current_A);

    raw_estimate_ = y_phys;
    x_.y = y_phys + x_.bias;

    // Linearized transition for x=[y,bias,bias_rate] with
    // y_k = y_phys + bias_{k-1} + dt*bias_rate_{k-1}
    // bias_k = bias_{k-1} + dt*bias_rate_{k-1}
    // bias_rate_k = bias_rate_{k-1}
    const double A[3][3] = {
        {0.0, 1.0, dt_s},
        {0.0, 1.0, dt_s},
        {0.0, 0.0, 1.0},
    };

    const double P[3][3] = {
        {P_.p00, P_.p01, P_.p02},
        {P_.p10, P_.p11, P_.p12},
        {P_.p20, P_.p21, P_.p22},
    };

    double AP[3][3] = {};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                AP[i][j] += A[i][k] * P[k][j];
            }
        }
    }

    double APA_t[3][3] = {};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                APA_t[i][j] += AP[i][k] * A[j][k];
            }
        }
    }

    APA_t[0][0] += params_.q_y;
    APA_t[1][1] += params_.q_bias;
    APA_t[2][2] += params_.q_bias_rate;

    P_ = {
        APA_t[0][0], APA_t[0][1], APA_t[0][2],
        APA_t[1][0], APA_t[1][1], APA_t[1][2],
        APA_t[2][0], APA_t[2][1], APA_t[2][2],
    };
}

void SensorEstimator::update(double z) {
    double innovation = z - x_.y;
    double S = P_.p00 + params_.r_meas;
    double K0 = P_.p00 / S;
    double K1 = P_.p10 / S;
    double K2 = P_.p20 / S;

    x_.y += K0 * innovation;
    x_.bias += K1 * innovation;
    x_.bias_rate += K2 * innovation;

    Covariance Pn;
    Pn.p00 = (1.0 - K0) * P_.p00;
    Pn.p01 = (1.0 - K0) * P_.p01;
    Pn.p02 = (1.0 - K0) * P_.p02;
    Pn.p10 = P_.p10 - K1 * P_.p00;
    Pn.p11 = P_.p11 - K1 * P_.p01;
    Pn.p12 = P_.p12 - K1 * P_.p02;
    Pn.p20 = P_.p20 - K2 * P_.p00;
    Pn.p21 = P_.p21 - K2 * P_.p01;
    Pn.p22 = P_.p22 - K2 * P_.p02;
    P_ = Pn;
}

void SensorEstimator::resetCovariance() {
    P_ = {};
    P_.p00 = global_.initial_covariance_diagonal.p00;
    P_.p11 = global_.initial_covariance_diagonal.p11;
    P_.p22 = global_.initial_covariance_diagonal.p22;
}

}  // namespace rec_piecewise
