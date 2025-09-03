#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <utility>
#include "../util/cw.hpp"
#include "../util/hasher.hpp"
#include "../util/splay.hpp"
#include "AbstractBuilder.hpp"

enum class SearchStrategy {
    BINARY_SEARCH,
    LINEAR_SCAN
};

template<typename WeightType>
class MonotonicBuilder : public AbstractBuilder<WeightType>
{
    using Base = AbstractBuilder<WeightType>;
    using Base::k;
    using Base::tokenNum;
    using Base::docs;
    using Base::cws;
    using Base::hasher;
    using Base::calculateTF;

private:
    std::vector<int> first, freq;
    std::vector<WeightType> mini;
    bool active;
    SearchStrategy strategy;

    // Binary search specific search function
    std::pair<int, int> searchInSetBinary(SplayTree &V, const std::pair<int, int> &xy)
    {
        int x = V.searchByX(xy.first);
        int y = V.searchByY(xy.second);
        return make_pair(x, y);
    }

    // Linear scan specific search function
    std::pair<std::set<std::pair<int, int>>::iterator,
              std::set<std::pair<int, int>>::iterator>
    searchInSetLinear(std::set<std::pair<int, int>> &S, const std::pair<int, int> &xy)
    {
        std::pair<std::set<std::pair<int, int>>::iterator,
                  std::set<std::pair<int, int>>::iterator>
            ret(S.begin(), S.end());

        for (auto iter = S.begin(); iter != S.end(); ++iter)
        {
            if (iter->second <= xy.second)
            {
                ret.second = iter;
            }
            if (iter->first >= xy.first)
            {
                ret.first = iter;
                break;
            }
        }
        return ret;
    }

    void generateKeys(const int hid, const std::vector<int> &doc, std::vector<pair<int, int>> &keys)
    {
        int n = doc.size();
        for (int i = 0; i < n; i++)
        {
            freq[doc[i]] = 0;
        }
        int max_freq = 0;
        for (int i = 0; i < n; i++) {
            freq[doc[i]]++;
            max_freq = max(max_freq, freq[doc[i]]);
        }
        for (int i = 0; i < n; i++) {
            freq[doc[i]] = 0;
        }
        for (int i = 0; i < n; i++)
        {
            int token = doc[i];
            int x = ++freq[token];
            WeightType tf = calculateTF(x, max_freq);
            (void)tf;
            keys.emplace_back(token, x);
        }
        std::sort(keys.begin(), keys.end(),
                  [&](const std::pair<int, int> &lhs,
                      const std::pair<int, int> &rhs)
                  {
                      WeightType lhs_tf = calculateTF(lhs.second, max_freq);
                      WeightType rhs_tf = calculateTF(rhs.second, max_freq);
                      auto lhsVal = hasher.eval(hid, lhs.first, lhs_tf);
                      auto rhsVal = hasher.eval(hid, rhs.first, rhs_tf);
                      return lhsVal < rhsVal;
                  });
    }

    void generateActiveKeys(const int hid, const std::vector<int> &doc, std::vector<pair<int, int>> &keys)
    {
        int n = doc.size();
        for (int i = 0; i < n; i++)
        {
            freq[doc[i]] = 0;
        }
        int max_freq = 0;
        for (int i = 0; i < n; i++) {
            freq[doc[i]]++;
            max_freq = max(max_freq, freq[doc[i]]);
        }
        for (int i = 0; i < n; i++) {
            freq[doc[i]] = 0;
        }
        
        for (int i = 0; i < n; i++)
        {
            int token = doc[i];
            int x = ++freq[token];
            WeightType tf = calculateTF(x, max_freq);
            auto v = hasher.eval(hid, token, tf);
            if (x == 1 || v < mini[token])
            {
                mini[token] = v;
                keys.emplace_back(token, x);
            }
        }
        std::sort(keys.begin(), keys.end(),
                  [&](const std::pair<int, int> &lhs,
                      const std::pair<int, int> &rhs)
                  {
                      WeightType lhs_tf = calculateTF(lhs.second, max_freq);
                      WeightType rhs_tf = calculateTF(rhs.second, max_freq);
                      auto lhsVal = hasher.eval(hid, lhs.first, lhs_tf);
                      auto rhsVal = hasher.eval(hid, rhs.first, rhs_tf);
                      return lhsVal < rhsVal;
                  });
    }

    void buildCWBinarySearch()
    {
        for (int hid = 0; hid < k; hid++)
        {
            for (int doc_id = 0; doc_id < (int)docs.size(); doc_id++)
            {
                const std::vector<int> &doc = docs[doc_id];
                int n = (int)doc.size();

                std::vector<int> next(n + 1);
                std::vector<std::pair<int, int>> keys;
                SplayTree S;
                S.insert(-1, -1);
                S.insert(n, n);

                for (int i = 0; i < n; i++)
                {
                    first[doc[i]] = n;
                }
                next[n] = n;
                for (int i = n - 1; i >= 0; i--)
                {
                    next[i] = first[doc[i]];
                    first[doc[i]] = i;
                }

                keys.reserve(n);
                if (active)
                {
                    generateActiveKeys(hid, doc, keys);
                }
                else
                {
                    generateKeys(hid, doc, keys);
                }

                int max_freq = 0;
                for (int i = 0; i < n; i++) {
                    freq[doc[i]] = 0;
                }
                for (int i = 0; i < n; i++) {
                    freq[doc[i]]++;
                    max_freq = max(max_freq, freq[doc[i]]);
                }

                for (auto &tx : keys)
                {
                    int t = tx.first;
                    int x = tx.second;
                    WeightType tf = calculateTF(x, max_freq);
                    auto v = hasher.eval(hid, t, tf);

                    int keys_start, keys_end;
                    for (int j = 0; j < freq[t] - x + 1; j++)
                    {
                        if (j == 0)
                        {
                            keys_start = first[t];
                            keys_end = first[t];
                            for (int z = 1; z < x; z++)
                            {
                                keys_end = next[keys_end];
                            }
                        }
                        else
                        {
                            keys_start = next[keys_start];
                            keys_end = next[keys_end];
                        }

                        auto ret = searchInSetBinary(S, std::make_pair(keys_start, keys_end));

                        if (ret.second >= ret.first)
                        {
                            continue;
                        }

                        int a, b, c, d;
                        b = keys_start;
                        c = keys_end;

                        vector<pair<int, int>> dominated;
                        S.getRange(ret.second, ret.first, dominated);
                        for (auto iter = dominated.begin(); iter != dominated.end() - 1; ++iter)
                        {
                            a = iter->first + 1;
                            d = std::next(iter, 1)->second - 1;
                            if (iter->first <= keys_start && iter->second >= keys_end)
                            {
                                S.remove(iter->first);
                            }
                            cws[hid].emplace_back(doc_id, v, a, b, c, d);
                            c = std::next(iter, 1)->second;
                        }
                        if ((dominated.rbegin())->first <= keys_start && (dominated.rbegin())->second >= keys_end)
                        {
                            S.remove((dominated.rbegin())->first);
                        }
                        S.insert(keys_start, keys_end);
                    }
                }
            }
        }
    }

    void buildCWLinearScan()
    {
        for (int hid = 0; hid < k; hid++)
        {
            for (int doc_id = 0; doc_id < (int)docs.size(); doc_id++)
            {
                const std::vector<int> &doc = docs[doc_id];
                int n = (int)doc.size();

                std::vector<int> next(n + 1);
                std::vector<std::pair<int, int>> keys;
                std::set<std::pair<int, int>> S;
                S.insert(std::make_pair(-1, -1));
                S.insert(std::make_pair(n, n));

                for (int i = 0; i < n; i++)
                {
                    first[doc[i]] = n;
                }
                next[n] = n;
                for (int i = n - 1; i >= 0; i--)
                {
                    next[i] = first[doc[i]];
                    first[doc[i]] = i;
                }

                keys.reserve(n);
                if (active)
                {
                    generateActiveKeys(hid, doc, keys);
                }
                else
                {
                    generateKeys(hid, doc, keys);
                }

                int max_freq = 0;
                for (int i = 0; i < n; i++) {
                    freq[doc[i]] = 0;
                }
                for (int i = 0; i < n; i++) {
                    freq[doc[i]]++;
                    max_freq = max(max_freq, freq[doc[i]]);
                }

                for (auto &tx : keys)
                {
                    int t = tx.first;
                    int x = tx.second;
                    WeightType tf = calculateTF(x, max_freq);
                    auto v = hasher.eval(hid, t, tf);

                    int keys_start, keys_end;
                    for (int j = 0; j < freq[t] - x + 1; j++)
                    {
                        if (j == 0)
                        {
                            keys_start = first[t];
                            keys_end = first[t];
                            for (int z = 1; z < x; z++)
                            {
                                keys_end = next[keys_end];
                            }
                        }
                        else
                        {
                            keys_start = next[keys_start];
                            keys_end = next[keys_end];
                        }

                        auto ret = searchInSetLinear(S, std::make_pair(keys_start, keys_end));

                        if ((*(ret.second)).first >= (*(ret.first)).first)
                        {
                            continue;
                        }

                        int a, b, c, d;
                        b = keys_start;
                        c = keys_end;

                        for (auto iter = ret.second; iter != ret.first;)
                        {
                            a = iter->first + 1;
                            d = std::next(iter, 1)->second - 1;

                            if (iter->first <= keys_start && iter->second >= keys_end)
                            {
                                iter = S.erase(iter);
                            }
                            else
                            {
                                ++iter;
                            }

                            cws[hid].emplace_back(doc_id, v, a, b, c, d);
                            c = iter->second;
                        }
                        if ((ret.first)->first <= keys_start && (ret.first)->second >= keys_end)
                        {
                            S.erase(ret.first);
                        }
                        S.insert(std::make_pair(keys_start, keys_end));
                    }
                }
            }
        }
    }

public:
    MonotonicBuilder(const std::vector<std::vector<int>> &docs_,
                     int k_,
                     int tokenNum_,
                     bool active_,
                     SearchStrategy strategy_)
        : Base(docs_, k_, tokenNum_),
          first(tokenNum_),
          freq(tokenNum_),
          mini(tokenNum_),
          active(active_),
          strategy(strategy_)
    {
    }

    void buildCW() override
    {
        if (strategy == SearchStrategy::BINARY_SEARCH)
        {
            buildCWBinarySearch();
        }
        else
        {
            buildCWLinearScan();
        }
    }
};
