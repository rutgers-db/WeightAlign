#pragma once

#include <vector>
#include <algorithm>
#include "AbstractBuilder.hpp"

template<typename WeightType>
class AllAlignBuilder : public AbstractBuilder<WeightType> {
    using Base = AbstractBuilder<WeightType>;
    using Base::k;
    using Base::tokenNum;
    using Base::docs;
    using Base::cws;
    using Base::hasher;
    using Base::calculateTF;

private:
    vector<int> first, next, rnext, freq;
    int max_freq; // Cache max frequency to avoid recalculation

    void work(int l, int le, int r, int hid, int doc_id, const std::vector<int> &doc)
    {
        if (r < l)
        {
            return;
        }
        int a, b, c = 0, x;
        WeightType mn;
        
        // Find minimum hash value in range [l, r]
        for (int i = l; i <= r; i++)
        {
            freq[doc[i]]++;
            WeightType tf = calculateTF(freq[doc[i]], max_freq);
            WeightType v = hasher.eval(hid, doc[i], tf);
            if (c == 0 || v < mn)
            {
                mn = v;
                c = i;
                x = freq[doc[i]];
            }
        }
        
        // Reset frequency counts
        for (int i = l; i <= r; i++)
        {
            freq[doc[i]]--;
        }
        
        // Find the leftmost occurrence of the same token
        b = c;
        while (rnext[b] >= l)
        {
            b = rnext[b];
        }
        
        // Generate compressed windows
        while (c <= r)
        {
            a = std::max(rnext[b] + 1, l);
            if (le > b)
            {
                cws[hid].emplace_back(doc_id, mn, a, b, c, r);
                if (x == 1)
                {
                    work(a, b - 1, c - 1, hid, doc_id, doc);
                }
                else
                {
                    work(a, b, c - 1, hid, doc_id, doc);
                }
            }
            else
            {
                cws[hid].emplace_back(doc_id, mn, a, le, c, r);
                work(a, le, c - 1, hid, doc_id, doc);
                return;
            }
            if (next[c] > r)
            {
                work(b + 1, le, r, hid, doc_id, doc);
                return;
            }
            b = next[b];
            c = next[c];
        }
    }

public:
    AllAlignBuilder(const std::vector<std::vector<int>> &docs_, int k_, int tokenNum_)
        : Base(docs_, k_, tokenNum_),
          first(tokenNum_),
          freq(tokenNum_)
    {
    }
          
    void buildCW() override {
        for (int hid = 0; hid < k; hid++)
        {
            for (int doc_id = 0; doc_id < (int)docs.size(); doc_id++)
            {
                const std::vector<int> &doc = docs[doc_id];
                int n = (int)doc.size();
                next.reserve(n + 1);
                rnext.reserve(n + 1);

                // Calculate max frequency once per document
                max_freq = 0;
                for (int i = 0; i < n; i++) {
                    freq[doc[i]] = 0;
                }
                for (int i = 0; i < n; i++) {
                    freq[doc[i]]++;
                    max_freq = std::max(max_freq, freq[doc[i]]);
                }
                for (int i = 0; i < n; i++) {
                    freq[doc[i]] = 0;
                }

                // Build reverse next pointers
                for (int i = 0; i < n; i++)
                {
                    first[doc[i]] = -1;
                }
                for (int i = 0; i < n; i++)
                {
                    rnext[i] = first[doc[i]];
                    first[doc[i]] = i;
                }

                // Build forward next pointers  
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
                
                work(0, n - 1, n - 1, hid, doc_id, doc);
            }
        }
    }
};