#pragma once
#include <chrono>
#include <random>
#include <vector>
#include <fstream>
#include <unordered_set>
#include <sstream>
#include <iostream>
#include <cmath>
#include <limits>
#include "tf_strategy.hpp"

using namespace std;

template<typename WeightType>
class Hasher {
private:
    int k, tokenNum;
    // Global seed to derive all per-hash randomness deterministically
    uint64_t seed_ = 0;
    mt19937 mt_rand;
    
    // IDF support
    std::vector<double> idf;
    bool use_idf;
    
    // TF strategy
    TFMode tf_mode;
    
    
    // No separate advanced flag; precision derives from (tf_mode, use_idf)

public:
    static const int p = 998244353;

    Hasher(int k, int tokenNum = 50257) 
        : k(k), tokenNum(tokenNum), mt_rand(0), use_idf(false), tf_mode(TFMode::RAW) {
        idf.resize(tokenNum, 1.0);
        if constexpr (std::is_same_v<WeightType, double>) {
            setupAdvancedMode();
        }
    }

    void setSeed(uint64_t s) { seed_ = s; }

    void setupAdvancedMode() {
        // No-op; DOUBLE mode implies CWS hash in eval.
    }

    void loadIDF(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open IDF file: " + filepath);
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::istringstream iss(line);
            std::string token_str, idf_str;
            
            if (std::getline(iss, token_str, '\t') && std::getline(iss, idf_str)) {
                try {
                    int token = std::stoi(token_str);
                    double idf_val = std::stod(idf_str);
                    if (token >= 0 && token < tokenNum) {
                        idf[token] = idf_val;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Parse error: " << line << std::endl;
                }
            }
        }
        use_idf = true;
    }

    void calculateIDF(const std::vector<std::vector<int>>& docs) {
        std::vector<int> doc_freq(tokenNum, 0);
        
        for (const auto& doc : docs) {
            std::unordered_set<int> unique_tokens(doc.begin(), doc.end());
            for (int token : unique_tokens) {
                if (token >= 0 && token < tokenNum) {
                    doc_freq[token]++;
                }
            }
        }
        
        for (int i = 0; i < tokenNum; i++) {
            if (doc_freq[i] > 0) {
                idf[i] = std::log(static_cast<double>(docs.size()) / doc_freq[i]);
            } else {
                idf[i] = 0.0;
            }
        }
        use_idf = true;
    }

    // Consistent Weighted Sampling (Ioffe, 2010) using C++ standard distributions
    inline double cws_hash(int hid, int token, double w) {
        if (w <= 0.0) return std::numeric_limits<double>::infinity();
        // Deterministic RNG seeded by global seed_ and (hid, token)
        uint64_t seed = seed_ ^ ((static_cast<uint64_t>(hid) << 32) ^ static_cast<uint64_t>(token));
        std::mt19937_64 eng(seed);
        std::gamma_distribution<double> gamma(2.0, 1.0);      // shape k=2, scale theta=1
        std::uniform_real_distribution<double> uni(0.0, 1.0); // [0,1)

        double r = gamma(eng);
        double c = gamma(eng);
        double beta = uni(eng);
        if (r <= 0.0) r = std::numeric_limits<double>::min();
        if (beta <= 0.0) beta = std::numeric_limits<double>::min();
        if (beta >= 1.0) beta = std::nextafter(1.0, 0.0);

        double logw = std::log(w);
        double t = std::floor(logw / r + beta);
        double y = std::exp(r * (t - beta));
        return c / (y * std::exp(r));
    }

    WeightType eval(int hid, int token, WeightType weight) {
        if constexpr (std::is_same_v<WeightType, int>) {
            // Derive linear hash coefficients (a,b,c) deterministically from seed_ and hid
            std::mt19937 eng(static_cast<uint32_t>(seed_ ^ static_cast<uint64_t>(hid)));
            std::uniform_int_distribution<int> distA(1, p - 1);
            std::uniform_int_distribution<int> distB(1, p - 1);
            std::uniform_int_distribution<int> distC(0, p - 1);
            int a = distA(eng), b = distB(eng), c = distC(eng);
            return ( (1LL * token * a + 1LL * weight * b + c) % p );
        } else {
            double final_weight = use_idf ? (static_cast<double>(weight) * idf[token]) : static_cast<double>(weight);
            return cws_hash(hid, token, final_weight);
        }
    }

    // No explicit HF stored; coefficients are derived from seed_ on the fly.

    bool isIDFEnabled() const { return use_idf; }
    
    void setTFMode(TFMode mode) { tf_mode = mode; }
    TFMode getTFMode() const { return tf_mode; }
    
    std::string getModeInfo() const {
        std::string info = "Hasher Mode: ";
        if constexpr (std::is_same_v<WeightType, int>) info += "INT_OPTIMIZED"; else info += "DOUBLE_PRECISION_CWS";
        info += "\nIDF Enabled: " + std::string(use_idf ? "Yes" : "No");
        
        info += "\nTF Strategy: ";
        switch (tf_mode) {
            case TFMode::RAW: info += "raw"; break;
            case TFMode::LOG_NORMALIZED: info += "log"; break;
            case TFMode::BOOLEAN: info += "boolean"; break;
            case TFMode::AUGMENTED: info += "augmented"; break;
            case TFMode::SQUARE: info += "square"; break;
            default: info += "unknown"; break;
        }
        
        return info;
    }

    void saveToFile(std::ofstream& file) const {
        // Save basic parameters
        file.write(reinterpret_cast<const char*>(&k), sizeof(k));
        file.write(reinterpret_cast<const char*>(&tokenNum), sizeof(tokenNum));
        file.write(reinterpret_cast<const char*>(&use_idf), sizeof(use_idf));
        file.write(reinterpret_cast<const char*>(&tf_mode), sizeof(tf_mode));
        file.write(reinterpret_cast<const char*>(&seed_), sizeof(seed_));
        
        // Save IDF data if enabled
        if (use_idf) {
            for (double val : idf) {
                file.write(reinterpret_cast<const char*>(&val), sizeof(val));
            }
        }
    }

    void loadFromFile(std::ifstream& file) {
        // Load basic parameters
        file.read(reinterpret_cast<char*>(&k), sizeof(k));
        file.read(reinterpret_cast<char*>(&tokenNum), sizeof(tokenNum));
        file.read(reinterpret_cast<char*>(&use_idf), sizeof(use_idf));
        file.read(reinterpret_cast<char*>(&tf_mode), sizeof(tf_mode));
        file.read(reinterpret_cast<char*>(&seed_), sizeof(seed_));
        
        // Resize vectors
        if (use_idf) {
            idf.resize(tokenNum);
        }
        
        // Load IDF data if enabled
        if (use_idf) {
            for (double& val : idf) {
                file.read(reinterpret_cast<char*>(&val), sizeof(val));
            }
        }
    }
};
