#pragma once
#include <algorithm>
#include <cstdio>
#include <fstream>

template<typename WeightType>
class CW {
public:
    int T, a, b, c, d;
    WeightType v;

    CW() {}

    CW(int _T, WeightType _v, int _a, int _b, int _c, int _d) 
        : T(_T), v(_v), a(_a), b(_b), c(_c), d(_d) {}

    CW(const CW& tmp) : T(tmp.T), v(tmp.v), a(tmp.a), b(tmp.b), c(tmp.c), d(tmp.d) {}

    void display() const {
        if constexpr (std::is_same_v<WeightType, int>) {
            printf("(document id: %d, hash: %d, a: %d, b: %d, c: %d, d: %d)\n", 
                   T, v, a, b, c, d);
        } else {
            printf("(document id: %d, hash: %.6f, a: %d, b: %d, c: %d, d: %d)\n", 
                   T, v, a, b, c, d);
        }
    }

    void saveToFile(std::ofstream& file) const {
        file.write(reinterpret_cast<const char*>(&T), sizeof(T));
        file.write(reinterpret_cast<const char*>(&a), sizeof(a));
        file.write(reinterpret_cast<const char*>(&b), sizeof(b));
        file.write(reinterpret_cast<const char*>(&c), sizeof(c));
        file.write(reinterpret_cast<const char*>(&d), sizeof(d));
        file.write(reinterpret_cast<const char*>(&v), sizeof(v));
    }

    void loadFromFile(std::ifstream& file) {
        file.read(reinterpret_cast<char*>(&T), sizeof(T));
        file.read(reinterpret_cast<char*>(&a), sizeof(a));
        file.read(reinterpret_cast<char*>(&b), sizeof(b));
        file.read(reinterpret_cast<char*>(&c), sizeof(c));
        file.read(reinterpret_cast<char*>(&d), sizeof(d));
        file.read(reinterpret_cast<char*>(&v), sizeof(v));
    }
};