#pragma once
#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "../util/cw.hpp"
#include "../util/hasher.hpp"
#include "../util/tf_strategy.hpp"

using namespace std;

template<typename WeightType>
class AbstractBuilder {
protected:
    int k, tokenNum;
    const std::vector<std::vector<int>> &docs;
    std::vector<std::vector<CW<WeightType>>> cws;
    Hasher<WeightType> hasher;
    TFMode tf_mode;

public:
    AbstractBuilder(const std::vector<std::vector<int>> &docs_, int k_, int tokenNum_)
        : k(k_), tokenNum(tokenNum_), docs(docs_), cws(k_), hasher(k_, tokenNum_), tf_mode(TFMode::RAW) {}

    virtual ~AbstractBuilder() {}
    virtual void buildCW() = 0;

    void setTFMode(TFMode mode) { 
        tf_mode = mode; 
        hasher.setTFMode(mode);
    }
    void loadIDF(const std::string& file) { hasher.loadIDF(file); }
    void calculateIDF() { hasher.calculateIDF(docs); }

    WeightType calculateTF(int freq, int max_freq = 0) const {
        return TFCalculator<WeightType>::calculate(tf_mode, freq, max_freq);
    }

    const std::vector<std::vector<CW<WeightType>>> &getCWs() const {
        return cws;
    }

    long long getSize() const {
        long long sum = 0;
        for (int i = 0; i < k; i++) {
            sum += cws[i].size();
        }
        return sum;
    }

    std::string getHasherInfo() const {
        return hasher.getModeInfo();
    }

    void saveIndex(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file for writing: " + filename);
        }
        
        // Save basic parameters
        file.write(reinterpret_cast<const char*>(&k), sizeof(k));
        file.write(reinterpret_cast<const char*>(&tokenNum), sizeof(tokenNum));
        
        // Save hasher configuration (TF mode, IDF data)
        hasher.saveToFile(file);
        
        // Save CWs
        for (int hid = 0; hid < k; hid++) {
            size_t cw_count = cws[hid].size();
            file.write(reinterpret_cast<const char*>(&cw_count), sizeof(cw_count));
            for (const auto& cw : cws[hid]) {
                cw.saveToFile(file);
            }
        }
        
        file.close();
    }

    void loadIndex(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file for reading: " + filename);
        }
        
        // Load basic parameters
        file.read(reinterpret_cast<char*>(&k), sizeof(k));
        file.read(reinterpret_cast<char*>(&tokenNum), sizeof(tokenNum));
        
        // Resize CWs vector
        cws.resize(k);
        
        // Load hasher configuration
        hasher.loadFromFile(file);
        
        // Load CWs
        for (int hid = 0; hid < k; hid++) {
            size_t cw_count;
            file.read(reinterpret_cast<char*>(&cw_count), sizeof(cw_count));
            cws[hid].resize(cw_count);
            for (auto& cw : cws[hid]) {
                cw.loadFromFile(file);
            }
        }
        
        file.close();
    }

    void validation() {
        for (int tid = 0; tid < docs.size(); tid++) {
            int n = docs[tid].size();
            for (int hid = 0; hid < k; hid++) {
                for (int i = 0; i < n; i++) {
                    for (int j = i; j < n; j++) {
                        int flag = 0;
                        for (const auto& cw : cws[hid]) {
                            if (cw.T != tid) continue;
                            if (cw.a <= i && i <= cw.b && cw.c <= j && j <= cw.d) {
                                flag++;
                            }
                        }
                        if (flag == 0) {
                            cout << "uncovered : " << i << " " << j << endl;
                        } else if (flag > 1) {
                            cout << "multicover: " << i << " " << j << endl;
                        }
                    }
                }
            }
        }
    }

    void display() {
        for (int hid = 0; hid < k; hid++) {
            for (auto cw: cws[hid]) {
                cw.display();
            }
        }
    }
};