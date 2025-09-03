# WeightAlign: Near-Duplicate Text Alignment under Weighted Jaccard Similarity

This repository contains the reference implementation for the paper “Near-Duplicate Text Alignment under Weighted Jaccard Similarity.”

## Overview
WeightAlign targets weighted Jaccard similarity and proposes efficient compact window indexing with fast querying. The implementation includes the baseline alignment method (AllAlign, Single-column), the monotonic algorithm (this paper), and an independent query engine.

## Repository Structure
- CMakeLists.txt: build configuration
- src/
  - builder/: builders (Abstract/AllAlign/Monotonic/SingleColumn)
  - Query.hpp, query_main.cpp: query engine and CLI entrypoint
  - util/: hashing, TF/IDF, IO, compressed-window utilities
- paper.pdf: the paper; LICENSE: MIT

## Environment & Build
- Requirements: C++17, CMake ≥ 3.16, GCC 9+/Clang 12+
- Build
  ```bash
  mkdir build && cd build
  cmake ..
  make -j
  ```
  Binaries: `build` (index builder) and `query` (query engine).

## Command Line Parameters

### build (Index Building)

```
Usage: build -f <data.bin> -i <index.data> -k <hash_count> [options]

Required:
  -f <file>     Binary document data file
  -i <file>     Output index file path
  -k <num>      Number of hash functions

Optional:
  -n <num>          Limit number of documents (0=all)
  -l <num>          Document length limit (0=no limit)
  -t <strategy>     TF weighting: raw, log, boolean, augmented, square
  -I <file>         Load IDF weights from file (enables DOUBLE)
  -v <num>          Vocabulary size (default: 50257 for GPT-2)
  -B <builder>      Builder: allalign (default), monotonic, single
  -a <0|1>          Monotonic active-key optimization (monotonic only; default 1)
  -s <binary|linear>Monotonic search strategy (monotonic only; default binary)

Notes:
- Type selection: INT only for raw and no IDF; otherwise DOUBLE (CWS hashing).
- Unknown TF strategy values error out with a helpful message.
```

### query (Querying)

```
Usage: query -i <index.data> -f <query.txt> [options]

Required:
  -i <file>     Index file (created by build)
  -f <file>     Query tokens file (space-separated IDs)

Optional:
  -t <num>      Matching threshold 0.0-1.0 (default: 0.8)

Examples:
- Raw/INT, AllAlign: `./build/build -f sample/sample_docs.bin -i basic.data -k 16 -v 2000 -B allalign`
- TF-IDF/DOUBLE, Monotonic binary active: `./build/build -f sample/sample_docs.bin -i tfidf.data -k 16 -v 2000 -I sample/sample_idf.txt -B monotonic -a 1 -s binary`
- Query: `./build/query -i tfidf.data -f sample/query1.txt -t 0.6`
```

## Citation
If this work is useful, please cite the paper (replace with actual metadata):
```bibtex
@inproceedings{optalign-weighted-jaccard,
  title     = {Near-Duplicate Text Alignment under Weighted Jaccard Similarity},
  author    = {<Author(s)>},
  booktitle = {<Venue>},
  year      = {<Year>}
}
```

## Acknowledgments
- Questions or reproduction suggestions are welcome via Issues/PRs. Thanks for community feedback and contributions.
