#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "cw.hpp"

using namespace std;

// load the vector<int> of a bin file and push back to docs
void loadBin(const string &binFileName, vector<int> &vec)
{
    ifstream ifs(binFileName, ios::binary);
    if (!ifs)
    {
        cout << "error open bin file" << endl;
        return;
    }
    int size;
    ifs.read((char *)&size, sizeof(int));
    vec.resize(size);
    ifs.read((char *)&vec[0], sizeof(int) * size);
    ifs.close();
}

void loadBin(const string &binFileName, vector<vector<int>> &docs)
{
    ifstream ifs(binFileName, ios::binary);
    if (!ifs)
    {
        cout << "Error open bin file" << endl;
        throw "Error open bin file";
    }
    int size;
    while (ifs.read((char *)&size, sizeof(int)))
    {
        vector<int> vec(size);
        ifs.read((char *)&vec[0], sizeof(int) * size);
        docs.emplace_back(vec);
    }
    ifs.close();
    cout << "From Binary File " << binFileName << " read " << docs.size() << " documents" << endl;
}

void loadBin(const string &binFileName, vector<vector<int>> &docs, int docnumLimit)
{
    ifstream ifs(binFileName, ios::binary);
    if (!ifs)
    {
        cout << "Error open bin file" << endl;
        throw "Error open bin file";
    }
    int size;
    while (ifs.read((char *)&size, sizeof(int)))
    {
        vector<int> vec(size);
        ifs.read((char *)&vec[0], sizeof(int) * size);
        docs.emplace_back(vec);
        if (docs.size() == docnumLimit)
            break;
    }
    ifs.close();
    cout << "From Binary File " << binFileName << " read " << docs.size() << " documents" << endl;
}

void loadBin(const string &binFileName, vector<vector<int>> &docs, int docnumLimit, int lengthLimit)
{
    ifstream ifs(binFileName, ios::binary);
    if (!ifs)
    {
        cout << "Error open bin file" << endl;
        throw "Error open bin file";
    }
    int size;
    int tokenNum = 0;
    vector<int> vec(lengthLimit);
    while (ifs.read((char *)&size, sizeof(int)))
    {
        ifs.read((char *)&vec[tokenNum], sizeof(int) * min(size, lengthLimit - tokenNum));
        ifs.seekg(sizeof(int) * (size - min(size, lengthLimit - tokenNum)), ios::cur);
        tokenNum += min(size, lengthLimit - tokenNum);
        if (tokenNum == lengthLimit)
        {
            docs.emplace_back(vec);
            tokenNum = 0;
        }
        if (docs.size() == docnumLimit)
            break;
    }
    ifs.close();

    if (docs.size() < docnumLimit)
    {
        cout << "No enough documents" << endl;
        throw "No enough documents";
    }

    cout << "From Binary File " << binFileName << " read " << docs.size() << " documents" << endl;
}

void loadSamples(const string &binFileName, vector<vector<int>> &docs, int sampleSart, int sampleNum)
{
    ifstream ifs(binFileName, ios::binary);
    if (!ifs)
    {
        cout << "Error open bin file" << endl;
        throw "Error open bin file";
    }
    int size;
    int cur = 0;
    while (ifs.read((char *)&size, sizeof(int)))
    {
        if (cur < sampleSart)
        {
            cur++;
            ifs.seekg(sizeof(int) * size, ios::cur);
            continue;
        }
        vector<int> vec(size);
        ifs.read((char *)&vec[0], sizeof(int) * size);
        docs.emplace_back(vec);
        if (docs.size() == sampleNum)
            break;
    }
    ifs.close();
    cout << "From Binary File " << binFileName << " read " << docs.size() << " documents" << endl;
}

/*
// Temporarily commented out because CW is now a template class
template<typename WeightType>
void loadCW(const string &binFileName, vector<vector<vector<CW<WeightType>>>> &cws)
{
    ifstream ifs(binFileName, ios::binary);
    if (!ifs)
    {
        cout << "Error open bin file" << endl;
        throw "Error open bin file";
    }
    int par_num, token_num, cw_num;
    ifs.read((char *)&par_num, sizeof(int));
    ifs.read((char *)&token_num, sizeof(int));
    cws.resize(par_num);
    for (int pid = 0; pid < par_num; pid++)
    {
        cws[pid].resize(token_num);
        for (int tid = 0; tid < token_num; tid++)
        {
            ifs.read((char *)&cw_num, sizeof(int));
            cws[pid][tid].resize(cw_num);
            for (int i = 0; i < cw_num; i++)
            {
                ifs.read((char *)&cws[pid][tid][i], sizeof(CW));
            }
        }
    }
}
*/

/*
template<typename WeightType>
void saveCW(const string &binFileName, vector<vector<CW<WeightType>>> &cws)
{
    ofstream ofs(binFileName, ios::binary);
    if (!ofs)
    {
        cout << "Error open bin file" << endl;
        throw "Error open bin file";
    }
    int par_num = cws.size(), cw_num;
    ofs.write((char *)&par_num, sizeof(int));
    for (int pid = 0; pid < par_num; pid++)
    {
        cw_num = cws[pid].size();
        ofs.write((char *)&cw_num, sizeof(int));
        for (int i = 0; i < cw_num; i++)
        {
            ofs.write((char *)&cws[pid][i], sizeof(CW<WeightType>));
        }
    }
}
*/

void writeBin(const vector<vector<int>> &vecs, const string &binFileName)
{
    ofstream ofs(binFileName, ios::binary);
    for (auto const &vec : vecs)
    {
        int size = vec.size();
        ofs.write((char *)&size, sizeof(int));
        for (auto const &tmp : vec)
        {
            ofs.write((char *)&tmp, sizeof(int));
        }
    }
    ofs.close();
}
