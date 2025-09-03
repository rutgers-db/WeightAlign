#pragma once
#include <cmath>
#include <algorithm>
#include <type_traits>
#include <stdexcept>

enum class TFMode {
    RAW,
    LOG_NORMALIZED, 
    BOOLEAN,
    AUGMENTED,
    SQUARE
};

template<typename WeightType>
class TFCalculator {
public:
    static WeightType raw(int freq) {
        return static_cast<WeightType>(freq);
    }
    
    static WeightType log_normalized(int freq) {
        if constexpr (std::is_same_v<WeightType, int>) {
            throw std::logic_error("INT WeightType is only allowed with RAW TF; 'log' requires DOUBLE.");
        } else {
            return freq > 0 ? 1.0 + std::log(freq) : 0.0;
        }
    }
    
    static WeightType boolean_tf(int freq) {
        if constexpr (std::is_same_v<WeightType, int>) {
            throw std::logic_error("INT WeightType is only allowed with RAW TF; 'boolean' requires DOUBLE.");
        } else {
            return freq > 0 ? static_cast<WeightType>(1) : static_cast<WeightType>(0);
        }
    }
    
    static WeightType augmented(int freq, int max_freq_in_doc, double k = 0.5) {
        if constexpr (std::is_same_v<WeightType, int>) {
            throw std::logic_error("INT WeightType is only allowed with RAW TF; 'augmented' requires DOUBLE.");
        } else {
            return k + (1.0 - k) * static_cast<double>(freq) / max_freq_in_doc;
        }
    }
    
    static WeightType square(int freq) {
        if constexpr (std::is_same_v<WeightType, int>) {
            throw std::logic_error("INT WeightType is only allowed with RAW TF; 'square' requires DOUBLE.");
        } else {
            return static_cast<double>(freq * freq);
        }
    }

    static WeightType calculate(TFMode mode, int freq, int max_freq = 0) {
        if constexpr (std::is_same_v<WeightType, int>) {
            if (mode != TFMode::RAW) {
                throw std::logic_error("INT WeightType can only be used with RAW TF and without IDF.");
            }
        }
        switch (mode) {
            case TFMode::RAW: 
                return raw(freq);
            case TFMode::LOG_NORMALIZED: 
                return log_normalized(freq);
            case TFMode::BOOLEAN: 
                return boolean_tf(freq);
            case TFMode::AUGMENTED: 
                return augmented(freq, max_freq);
            case TFMode::SQUARE: 
                return square(freq);
            default: 
                return raw(freq);
        }
    }
};
