#pragma once
#include <fstream>
#include <string>
#include "tf_strategy.hpp"

// Index file header information structure
struct IndexHeader {
    int k;
    int tokenNum;
    bool use_idf;
    TFMode tf_mode;
    // Infer WeightType: Raw TF + no IDF = INT, otherwise DOUBLE
    bool isIntType() const {
        return (tf_mode == TFMode::RAW) && (!use_idf);
    }
    
    bool isDoubleType() const {
        return !isIntType();
    }
};

// Read index file header (without loading full data)
inline IndexHeader readIndexHeader(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for reading: " + filename);
    }
    
    IndexHeader header;
    
    // Read basic parameters
    file.read(reinterpret_cast<char*>(&header.k), sizeof(header.k));
    file.read(reinterpret_cast<char*>(&header.tokenNum), sizeof(header.tokenNum));
    
    // Read hasher configuration (order must match hasher.hpp::loadFromFile)
    file.read(reinterpret_cast<char*>(&header.k), sizeof(header.k)); // hasher internal k
    file.read(reinterpret_cast<char*>(&header.tokenNum), sizeof(header.tokenNum)); // hasher internal tokenNum
    file.read(reinterpret_cast<char*>(&header.use_idf), sizeof(header.use_idf));
    file.read(reinterpret_cast<char*>(&header.tf_mode), sizeof(header.tf_mode));
    
    file.close();
    return header;
}
