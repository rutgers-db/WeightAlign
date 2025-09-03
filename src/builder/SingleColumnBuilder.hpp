#pragma once

#include "AbstractBuilder.hpp"

template<typename WeightType>
class SingleColumnBuilder : public AbstractBuilder<WeightType>
{
    using Base = AbstractBuilder<WeightType>;
    using Base::k;
    using Base::tokenNum;
    using Base::docs;
    using Base::cws;
    using Base::hasher;
    using Base::calculateTF;
private:
    std::vector<int> freq;

public:
    SingleColumnBuilder(const std::vector<std::vector<int>> &docs_,
                       int k_,
                       int tokenNum_)
        : Base(docs_, k_, tokenNum_),
          freq(tokenNum_)
    {
    }

    void buildCW() override
    {
        for (int hid = 0; hid < k; hid++)
        {
            for (int doc_id = 0; doc_id < (int)docs.size(); doc_id++)
            {
                const std::vector<int> &doc = docs[doc_id];
                int n = (int)doc.size();
                // Calculate max frequency first
                int max_freq = 0;
                for (int i = 0; i < n; i++) {
                    freq[doc[i]] = 0;
                }
                for (int i = 0; i < n; i++) {
                    freq[doc[i]]++;
                    max_freq = max(max_freq, freq[doc[i]]);
                }
                
                for (int i = 0; i < n; i++)
                {
                    for (int j = i; j < n; j++)
                    {
                        freq[doc[j]] = 0;
                    }
                    int c = i;
                    WeightType tf = calculateTF(1, max_freq);
                    auto v = hasher.eval(hid, doc[i], tf);
                    ++freq[doc[i]];
                    for (int d = i; d < n - 1; d++)
                    {
                        ++freq[doc[d + 1]];
                        WeightType new_tf = calculateTF(freq[doc[d + 1]], max_freq);
                        if (hasher.eval(hid, doc[d + 1], new_tf) < v)
                        {
                            cws[hid].emplace_back(doc_id, v, i, i, c, d);
                            c = d + 1;
                            v = hasher.eval(hid, doc[d + 1], new_tf);
                        }
                    }
                    cws[hid].emplace_back(doc_id, v, i, i, c, n - 1);
                }
            }
        }
    }
};