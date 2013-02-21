/*
* Copyright 2009-2010, KyTea Development Team
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* 
*     http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef KYTEA_LM_H__
#define KYTEA_LM_H__

#include <kytea/kytea-struct.h>
// #include <vector>

namespace kytea {

class KyteaLM {

public:

    unsigned n_, vocabSize_;
    KyteaDoubleMap probs_;
    KyteaDoubleMap fallbacks_;

    KyteaLM(unsigned n) : n_(n), vocabSize_(10000) { }
    ~KyteaLM() { }

    // train a trigram model using Kneser-Ney smoothing
    void train(const std::vector<KyteaString> & corpus);

    // score a string with the language model
    double score(const KyteaString & str) const;

    // score a single position in the string
    double scoreSingle(const KyteaString & val, int pos);

    const KyteaDoubleMap & getProbs() const { return probs_; }
    const KyteaDoubleMap & getFallbacks() const { return fallbacks_; }

    void checkEqual(const KyteaLM & rhs) const;

};

}

#endif
