/*
* Copyright 2009-2020, KyTea Development Team
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

#ifndef CORPUS_IO_H__ 
#define CORPUS_IO_H__

#include <kytea/corpus-io-format.h>
#include <kytea/general-io.h>
#include <vector>

namespace kytea {

// Forward declarations
class KyteaConfig;
class StringUtil;
class KyteaSentence;

class CorpusIO : public GeneralIO {

protected:

    std::string unkTag_;
    int numTags_;
    std::vector<bool> doTag_;

public:

    CorpusIO(StringUtil * util) : GeneralIO(util), unkTag_(), numTags_(0), doTag_() { }
    CorpusIO(StringUtil * util, const char* file, bool out) : GeneralIO(util,file,out,false), numTags_(0), doTag_() { } 
    CorpusIO(StringUtil * util, std::iostream & str, bool out) : GeneralIO(util,str,out,false), numTags_(0), doTag_() { }

    int getNumTags() { return numTags_; }
    void setNumTags(int numTags) { numTags_ = numTags; }
    void setDoTag(int i, bool v) { 
        if(i >= (int)doTag_.size()) doTag_.resize(i+1,true);
        doTag_[i] = v;
    }
    bool getDoTag(int i) { return i >= (int)doTag_.size() || doTag_[i]; }

    virtual ~CorpusIO() { }

    // create an appropriate parser based on the type
    static CorpusIO* createIO(const char* file, CorpusFormat form, const KyteaConfig & conf, bool output, StringUtil* util);
    static CorpusIO* createIO(std::iostream & str, CorpusFormat form, const KyteaConfig & conf, bool output, StringUtil* util);

    virtual KyteaSentence * readSentence() = 0;
    virtual void writeSentence(const KyteaSentence * sent, double conf = 0.0) = 0;

    void setUnkTag(const std::string & tag) { unkTag_ = tag; }

};

}

#endif
