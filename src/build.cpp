#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <assert.h>
#include <stdexcept>
#include <unistd.h>
#include "./util/IO.hpp"
#include "./util/util.hpp"
#include "./util/tf_strategy.hpp"
#include "./builder/AllAlignBuilder.hpp"
#include "./builder/MonotonicBuilder.hpp"
#include "./builder/SingleColumnBuilder.hpp"

using namespace std;

template<typename WeightType>
void buildAndSaveIndex(const std::vector<std::vector<int>>& docs, int k, int tokenNum,
                       const std::string& tf_strategy, const std::string& idf_file,
                       const std::string& index_file, const std::string& builder_name,
                       bool mono_active = true, SearchStrategy mono_strategy = SearchStrategy::BINARY_SEARCH,
                       bool run_validation = false) {

    std::unique_ptr<AbstractBuilder<WeightType>> builder;
    if (builder_name == "allalign") {
        builder = std::make_unique<AllAlignBuilder<WeightType>>(docs, k, tokenNum);
    } else if (builder_name == "monotonic") {
        builder = std::make_unique<MonotonicBuilder<WeightType>>(docs, k, tokenNum, mono_active, mono_strategy);
    } else if (builder_name == "single" || builder_name == "singlecolumn") {
        builder = std::make_unique<SingleColumnBuilder<WeightType>>(docs, k, tokenNum);
    } else {
        throw std::invalid_argument("Unknown builder '" + builder_name + "'. Valid: allalign, monotonic, single");
    }
    
    // Configure TF strategy (validate upstream in main, enforce here defensively)
    TFMode tf_mode = TFMode::RAW;
    if (tf_strategy == "raw") {
        tf_mode = TFMode::RAW;
    } else if (tf_strategy == "log") {
        tf_mode = TFMode::LOG_NORMALIZED;
    } else if (tf_strategy == "boolean") {
        tf_mode = TFMode::BOOLEAN;
    } else if (tf_strategy == "augmented") {
        tf_mode = TFMode::AUGMENTED;
    } else if (tf_strategy == "square") {
        tf_mode = TFMode::SQUARE;
    } else {
        throw std::invalid_argument(
            "Unknown TF strategy '" + tf_strategy + "'. Valid: raw, log, boolean, augmented, square.");
    }
    
    builder->setTFMode(tf_mode);
    
    // Configure IDF
    if (!idf_file.empty()) {
        builder->loadIDF(idf_file);
    }
    
    // Run alignment
    auto gen_st = timerStart();
    builder->buildCW();
    cout << "Index Generation Time: " << timerCheck(gen_st) << " s" << endl;
    cout << "Index Size: " << builder->getSize() << endl;

    if (run_validation) {
        cout << "Running validation..." << endl;
        builder->validation();
        cout << "Validation done." << endl;
    }

    // builder->display();
    
    if constexpr (std::is_same_v<WeightType, int>) {
        cout << "Using INT optimization" << endl;
    } else {
        cout << "Using DOUBLE precision" << endl;
    }
    
    // Save index
    if (!index_file.empty()) {
        cout << "Saving index to: " << index_file << endl;
        builder->saveIndex(index_file);
        cout << "Index saved successfully" << endl;
    }
}

int main(int argc, char *argv[]) {
    int doc_num = 0;
    int doc_length = 0;
    int k = 64;
    string src_file;
    string index_file;
    int tokenNum = 50257;  // Default GPT-2 vocabulary size
    
    std::string tf_strategy = "raw";
    std::string idf_file = "";
    std::string builder_name = "monotonic";  // Default to monotonic
    bool mono_active = true;  // Default active=1
    SearchStrategy mono_strategy = SearchStrategy::BINARY_SEARCH;
    bool run_validation = false;

    int opt;
    while ((opt = getopt(argc, argv, "f:n:k:i:l:t:I:v:B:a:s:V")) != EOF) {
        switch (opt) {
        case 'f':
            src_file = optarg;
            break;
        case 'n':
            doc_num = stoi(optarg);
            break;
        case 'k':
            k = atoi(optarg);
            break;
        case 'i':
            index_file = optarg;
            break;
        case 'l':
            doc_length = stoi(optarg);
            break;
        case 't':
            tf_strategy = optarg;  // raw, log, boolean, augmented, square
            break;
        case 'B':
            builder_name = optarg; // allalign, monotonic, single
            break;
        case 'a':
            mono_active = std::stoi(optarg) != 0; // 1 or 0, only for monotonic
            break;
        case 's': {
            std::string v = optarg;
            if (v == "binary") mono_strategy = SearchStrategy::BINARY_SEARCH;
            else if (v == "linear") mono_strategy = SearchStrategy::LINEAR_SCAN;
            else {
                std::cerr << "Error: Unknown monotonic strategy '" << v << "'. Use binary or linear." << std::endl;
                return 1;
            }
            break;
        }
        case 'V':
            run_validation = true;
            break;
        case 'I':
            idf_file = optarg;     // Path to IDF file
            break;
        case 'v':
            tokenNum = stoi(optarg);  // Vocabulary size
            break;
        case '?':
            std::cout << "Build Index - OptAlign Index Builder" << std::endl;
            std::cout << "Usage: build -f <data.bin> -k <hash_count> [-i <index.data>] [options]" << std::endl;
            std::cout << std::endl;
            std::cout << "Required:" << std::endl;
            std::cout << "  -f <file>     Binary document data file" << std::endl;
            std::cout << "  -k <num>      Number of hash functions" << std::endl;
            std::cout << std::endl;
            std::cout << "Optional:" << std::endl;
            std::cout << "  -i <file>     Output index file path (if not specified, won't save to disk)" << std::endl;
            std::cout << "  -n <num>      Limit number of documents (0=all)" << std::endl;
            std::cout << "  -l <num>      Document length limit (0=no limit)" << std::endl;
            std::cout << "  -t <strategy> TF weighting: raw (default), log, boolean, augmented, square" << std::endl;
            std::cout << "  -B <builder>  Builder: monotonic (default), allalign, single" << std::endl;
            std::cout << "  -a <0|1>      Monotonic active-key optimization (monotonic only; default 1)" << std::endl;
            std::cout << "  -s <binary|linear> Monotonic search strategy (monotonic only; default binary)" << std::endl;
            std::cout << "  -V             Run in-memory validation after building (debug)" << std::endl;
            std::cout << "  -I <file>     Load IDF weights from file" << std::endl;
            std::cout << "  -v <num>      Vocabulary size (default: 50257 for GPT-2)" << std::endl;
            return 0;
        }
    }

    // Validate required parameters
    if (src_file.empty()) {
        std::cerr << "Error: Input file (-f) is required." << std::endl;
        std::cerr << "Usage: build -f <data.bin> -k <hash_count> [options]" << std::endl;
        return 1;
    }
    
    if (k <= 0) {
        std::cerr << "Error: Number of hash functions (-k) must be positive." << std::endl;
        return 1;
    }

    // Validate TF strategy early for clearer UX
    if (!(tf_strategy == "raw" || tf_strategy == "log" || tf_strategy == "boolean" ||
          tf_strategy == "augmented" || tf_strategy == "square")) {
        std::cerr << "Error: Unknown TF strategy '" << tf_strategy
                  << "'. Valid values: raw, log, boolean, augmented, square." << std::endl;
        std::cerr << "Hint: use -t <strategy> (e.g., -t log) or omit -t for raw." << std::endl;
        return 1;
    }

    std::cout << "Parameters Summary: \n";
    std::cout << "bin_file_path  : " << src_file << "\n";
    std::cout << "doc_num        : " << doc_num << "\n";
    std::cout << "k              : " << k << "\n";
    std::cout << "doc_length     : " << doc_length << "\n";
    std::cout << "tokenNum       : " << tokenNum << "\n";
    std::cout << "tf_strategy    : " << tf_strategy << "\n";
    std::cout << "idf_file       : " << idf_file << "\n";
    std::cout << "builder        : " << builder_name << "\n";
    if (builder_name == "monotonic") {
        std::cout << "mono_active    : " << (mono_active ? 1 : 0) << "\n";
        std::cout << "mono_strategy  : " << (mono_strategy == SearchStrategy::BINARY_SEARCH ? "binary" : "linear") << "\n";
    }
    std::cout << "------------------------------" << std::endl;

    auto load_st = timerStart();
    vector<vector<int>> docs;
    if (doc_num == 0) {
        loadBin(src_file, docs);
    } else {
        if (doc_length == 0)
            loadBin(src_file, docs, doc_num);
        else
            loadBin(src_file, docs, doc_num, doc_length);
    }
    cout << "Load Time: " << timerCheck(load_st) << " s\n";
    
    // Select weight type automatically
    bool need_double = (tf_strategy != "raw") || !idf_file.empty();
    
    if (need_double) {
        cout << "=== Running in DOUBLE mode ===" << endl;
        buildAndSaveIndex<double>(docs, k, tokenNum, tf_strategy, idf_file, index_file, builder_name, mono_active, mono_strategy, run_validation);
    } else {
        cout << "=== Running in INT mode (optimized) ===" << endl;
        buildAndSaveIndex<int>(docs, k, tokenNum, tf_strategy, idf_file, index_file, builder_name, mono_active, mono_strategy, run_validation);
    }

    return 0;
}
