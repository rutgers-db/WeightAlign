#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <algorithm>
#include <limits>
#include "util/cw.hpp"
#include "util/hasher.hpp"
#include "util/tf_strategy.hpp"

const double eps = 1e-5;

class Update {
public:
    int t, l, r, type, value;

    Update(int _t, int _l, int _r, int _type, int _value) 
        : t(_t), l(_l), r(_r), type(_type), value(_value) {}
        
    bool operator<(const Update &rhs) const {
        return t < rhs.t;
    }

    friend std::ostream &operator<<(std::ostream &os, const Update &update) {
        os << "t: " << update.t << ", l: " << update.l << ", r: " << update.r
           << ", type: " << update.type << ", value: " << update.value;
        return os;
    }
};

template<typename WeightType>
class Query {
private:
    int k, tokenNum;
    std::vector<std::vector<CW<WeightType>>> cws;
    Hasher<WeightType> hasher;
    
    void innerScan(std::vector<CW<WeightType>> &cws_subset, 
                   std::unordered_set<int> &ids, 
                   double threshold, 
                   std::vector<std::pair<int, int>> &ranges) {
        std::vector<std::pair<int, int>> updates;
        for (auto id : ids) {
            updates.emplace_back(cws_subset[id].a, 1);
            updates.emplace_back(cws_subset[id].b + 1, -1);
        }
        std::sort(updates.begin(), updates.end());
        
        int cnt = 0;
        for (int i = 0; i < updates.size(); i++) {
            if (i > 0 && updates[i].first != updates[i - 1].first) {
                if (cnt >= k * threshold - eps) {
                    ranges.emplace_back(updates[i - 1].first, updates[i].first - 1);
                }
            }
            cnt += updates[i].second;
        }
    }

    std::vector<std::pair<int, int>> outerScan(std::vector<CW<WeightType>> &cws_subset, 
                                              double threshold) {
        std::vector<std::pair<int, int>> results;
        std::vector<Update> updates;
        
        for (int i = 0; i < cws_subset.size(); i++) {
            CW<WeightType> &cw = cws_subset[i];
            updates.emplace_back(cw.c, 0, 0, i, 1);
            updates.emplace_back(cw.d + 1, 0, 0, i, -1);
        }
        std::sort(updates.begin(), updates.end());

        std::unordered_set<int> ids;
        int cnt = 0;
        for (int i = 0; i < updates.size(); i++) {
            if (i > 0 && updates[i].t != updates[i - 1].t) {
                if (cnt >= k * threshold - eps) {
                    std::vector<std::pair<int, int>> ranges;
                    innerScan(cws_subset, ids, threshold, ranges);
                    for (auto range : ranges) {
                        results.emplace_back(updates[i - 1].t, range.second);
                    }
                }
            }
            cnt += updates[i].value;
            if (updates[i].value > 0) {
                ids.insert(updates[i].type);
            } else {
                ids.erase(updates[i].type);
            }
        }
        return results;
    }

public:
    Query() : k(0), tokenNum(0), hasher(0, 0) {}
    
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
    
    std::vector<WeightType> getSignature(const std::vector<int> &query) {
        std::vector<WeightType> signature(k);
        
        // Calculate max frequency for TF calculation
        int max_freq = 0;
        std::unordered_map<int, int> freqMap;
        for (auto token : query) {
            freqMap[token]++;
            max_freq = std::max(max_freq, freqMap[token]);
        }
        
        // Reset frequency map
        freqMap.clear();
        
        for (int hid = 0; hid < k; hid++) {
            if constexpr (std::is_same_v<WeightType, int>) {
                signature[hid] = std::numeric_limits<int>::max();
            } else {
                signature[hid] = std::numeric_limits<double>::max();
            }
            
            for (auto token : query) {
                WeightType tf = TFCalculator<WeightType>::calculate(hasher.getTFMode(), ++freqMap[token], max_freq);
                WeightType v = hasher.eval(hid, token, tf);
                if (v < signature[hid]) {
                    signature[hid] = v;
                }
            }
            
            // Reset frequency map for next hash function
            freqMap.clear();
        }
        return signature;
    }
    
    void query(const std::vector<int>& queryTokens, double threshold) {
        std::vector<WeightType> signature = getSignature(queryTokens);
        
        std::cout << "Query signature: ";
        for (int i = 0; i < k && i < 5; i++) {  // Show first 5 hash values
            std::cout << signature[i] << " ";
        }
        if (k > 5) std::cout << "...";
        std::cout << std::endl;
        std::cout << "Finding colliding CWs..." << std::endl;
        
        std::map<int, std::vector<CW<WeightType>>> collided_cws;

        // Find colliding CWs
        for (int hid = 0; hid < k; hid++) {
            for (const auto& cw : cws[hid]) {
                if (cw.v == signature[hid]) {
                    collided_cws[cw.T].push_back(cw);
                }
            }
        }
        
        int collided_cnt = 0;
        int result_cnt = 0;
        std::cout << "Found matches in " << collided_cws.size() << " documents:" << std::endl;
        
        for (auto& doc_entry : collided_cws) {
            int doc_id = doc_entry.first;
            auto& doc_cws = doc_entry.second;
            
            auto results = outerScan(doc_cws, threshold);
            collided_cnt += doc_cws.size();
            result_cnt += results.size();
            
            if (!results.empty()) {
                std::cout << "Document " << doc_id << ": " << results.size() << " matches" << std::endl;
                for (size_t i = 0; i < std::min(results.size(), size_t(3)); i++) {
                    std::cout << "  Range: [" << results[i].first << ", " << results[i].second << "]" << std::endl;
                }
                if (results.size() > 3) {
                    std::cout << "  ..." << (results.size() - 3) << " more matches" << std::endl;
                }
            }
        }
        
        std::cout << "Total collided CWs: " << collided_cnt << std::endl;
        std::cout << "Total result ranges: " << result_cnt << std::endl;
    }
    
    long long getTotalCWCount() const {
        long long total = 0;
        for (const auto& cw_list : cws) {
            total += cw_list.size();
        }
        return total;
    }
    
    std::string getHasherInfo() const {
        return hasher.getModeInfo();
    }
};