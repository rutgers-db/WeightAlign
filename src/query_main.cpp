#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include "Query.hpp"
#include "util/index_utils.hpp"

using namespace std;

int main(int argc, char *argv[]) {
    string index_file;
    string query_file;
    double threshold = 0.8;

    int opt;
    while ((opt = getopt(argc, argv, "i:f:t:")) != EOF) {
        switch (opt) {
        case 'i':
            index_file = optarg;
            break;
        case 'f':
            query_file = optarg;
            break;
        case 't':
            threshold = stod(optarg);
            break;
        case '?':
            std::cout << "Query Index - OptAlign Query Engine" << std::endl;
            std::cout << "Usage: query -i <index.data> -f <query.txt> [options]" << std::endl;
            std::cout << std::endl;
            std::cout << "Required:" << std::endl;
            std::cout << "  -i <file>     Index file (created by build)" << std::endl;
            std::cout << "  -f <file>     Query tokens file (space-separated IDs)" << std::endl;
            std::cout << std::endl;
            std::cout << "Optional:" << std::endl;
            std::cout << "  -t <num>      Matching threshold 0.0-1.0 (default: 0.8)" << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  query -i index.data -f query.txt -t 0.7" << std::endl;
            std::cout << "  query -i index_tfidf.data -f query.txt -t 0.5" << std::endl;
            return 0;
        }
    }

    if (index_file.empty() || query_file.empty()) {
        std::cerr << "Error: Both index file (-i) and query file (-f) are required." << std::endl;
        return 1;
    }

    // Read query tokens from file
    std::vector<int> query_tokens;
    std::ifstream file(query_file);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open query file: " << query_file << std::endl;
        return 1;
    }
    
    int token;
    while (file >> token) {
        query_tokens.push_back(token);
    }
    file.close();

    if (query_tokens.empty()) {
        std::cerr << "Error: Query file is empty or contains no valid tokens." << std::endl;
        return 1;
    }

    std::cout << "Query parameters:" << std::endl;
    std::cout << "Index file: " << index_file << std::endl;
    std::cout << "Query file: " << query_file << std::endl;
    std::cout << "Query tokens (" << query_tokens.size() << " tokens): ";
    for (size_t i = 0; i < std::min(query_tokens.size(), size_t(10)); i++) {
        std::cout << query_tokens[i] << " ";
    }
    if (query_tokens.size() > 10) {
        std::cout << "... (" << (query_tokens.size() - 10) << " more)";
    }
    std::cout << std::endl;
    std::cout << "Threshold: " << threshold << std::endl;
    std::cout << "================================" << std::endl;

    try {
        // Read index header to infer WeightType
        std::cout << "Detecting index type..." << std::endl;
        IndexHeader header = readIndexHeader(index_file);
        
        std::cout << "Index info: TF=";
        switch (header.tf_mode) {
            case TFMode::RAW: std::cout << "raw"; break;
            case TFMode::LOG_NORMALIZED: std::cout << "log"; break;
            case TFMode::BOOLEAN: std::cout << "boolean"; break;
            case TFMode::AUGMENTED: std::cout << "augmented"; break;
            case TFMode::SQUARE: std::cout << "square"; break;
            default: std::cout << "unknown"; break;
        }
        std::cout << ", IDF=" << (header.use_idf ? "enabled" : "disabled") << std::endl;
        
        if (header.isIntType()) {
            std::cout << "Using INT precision (optimized for raw TF without IDF)" << std::endl;
            Query<int> query_engine;
            query_engine.loadIndex(index_file);
            std::cout << "Index loaded successfully. CWs=" << query_engine.getTotalCWCount() << std::endl;
            std::cout << query_engine.getHasherInfo() << std::endl;
            std::cout << "================================" << std::endl;
            query_engine.query(query_tokens, threshold);
        } else {
            std::cout << "Using DOUBLE precision (for advanced TF or IDF)" << std::endl;
            Query<double> query_engine;
            query_engine.loadIndex(index_file);
            std::cout << "Index loaded successfully. CWs=" << query_engine.getTotalCWCount() << std::endl;
            std::cout << query_engine.getHasherInfo() << std::endl;
            std::cout << "================================" << std::endl;
            query_engine.query(query_tokens, threshold);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
