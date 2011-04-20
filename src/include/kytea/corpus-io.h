/*
* Copyright 2009, KyTea Development Team
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

namespace kytea {
class CorpusIO;
const static char CORP_FORMAT_RAW  = 0;
const static char CORP_FORMAT_FULL = 1;
const static char CORP_FORMAT_PART = 2;
const static char CORP_FORMAT_PROB = 3;
const static char CORP_FORMAT_DEFAULT = 4;
}

#include "general-io.h"
#include "kytea-struct.h"
#include "kytea-config.h"

namespace kytea {

class CorpusIO : public GeneralIO {

protected:

    std::string unkTag_;
    int numTags_;

public:

    typedef char Format;

    CorpusIO(StringUtil * util) : GeneralIO(util), unkTag_(), numTags_(0) { }
    CorpusIO(StringUtil * util, const char* file, bool out) : GeneralIO(util,file,out,false), numTags_(0) { } 
    CorpusIO(StringUtil * util, std::iostream & str, bool out) : GeneralIO(util,str,out,false), numTags_(0) { }

    int getNumTags() { return numTags_; }
    void setNumTags(int numTags) { numTags_ = numTags; }

    virtual ~CorpusIO() { }

    // create an appropriate parser based on the type
    static CorpusIO* createIO(const char* file, Format form, const KyteaConfig & conf, bool output, StringUtil* util);
    static CorpusIO* createIO(std::iostream & str, Format form, const KyteaConfig & conf, bool output, StringUtil* util);

    virtual KyteaSentence * readSentence() = 0;
    virtual void writeSentence(const KyteaSentence * sent, double conf = 0.0) = 0;

    void setUnkTag(const std::string & tag) { unkTag_ = tag; }

};

class FullCorpusIO : public CorpusIO {

protected:

    bool allTags_;
    KyteaString bounds_;

public:
    FullCorpusIO(StringUtil * util, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : CorpusIO(util), allTags_(false), bounds_(4) { 
        bounds_[0] = util_->mapChar(wordBound);
        bounds_[1] = util_->mapChar(tagBound);
        bounds_[2] = util_->mapChar(elemBound);
        bounds_[3] = util_->mapChar(escape);
    }
    FullCorpusIO(const CorpusIO & c, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : CorpusIO(c), allTags_(false), bounds_(4) { 
        bounds_[0] = util_->mapChar(wordBound);
        bounds_[1] = util_->mapChar(tagBound);
        bounds_[2] = util_->mapChar(elemBound);
        bounds_[3] = util_->mapChar(escape);
    }
    FullCorpusIO(StringUtil * util, const char* file, bool out, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : CorpusIO(util,file,out), allTags_(false), bounds_(4) { 
        bounds_[0] = util_->mapChar(wordBound);
        bounds_[1] = util_->mapChar(tagBound);
        bounds_[2] = util_->mapChar(elemBound);
        bounds_[3] = util_->mapChar(escape);
    } 
    FullCorpusIO(StringUtil * util, std::iostream & str, bool out, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : CorpusIO(util,str,out), allTags_(false), bounds_(4) { 
        bounds_[0] = util_->mapChar(wordBound);
        bounds_[1] = util_->mapChar(tagBound);
        bounds_[2] = util_->mapChar(elemBound);
        bounds_[3] = util_->mapChar(escape);
    }
    
    KyteaSentence * readSentence();
    void writeSentence(const KyteaSentence * sent, double conf = 0.0);

};

class PartCorpusIO : public CorpusIO {
    
private:

    KyteaString bounds_;

public:
    // PartCorpusIO ctr
    //  util: the string utility to use
    //  unkBound: the delimiter for when the bound is unannotated
    //  skipBound: the delimiter for when annotation of a bound has been skipped
    //  noBound: the delimiter for when no bound exists
    //  hasBound: the delimiter for when a boundary exists
    //  tagBound: the delimiter for when a boundary exists
    //  elemBound: the delimiter for when a boundary exists
    //  escape: the escape character
    PartCorpusIO(StringUtil * util, const char* unkBound = " ", const char* skipBound = "?", const char* noBound = "-", const char* hasBound = "|", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : CorpusIO(util), bounds_(7) { 
        bounds_[0] = util_->mapChar(unkBound);
        bounds_[1] = util_->mapChar(skipBound);
        bounds_[2] = util_->mapChar(noBound);
        bounds_[3] = util_->mapChar(hasBound);
        bounds_[4] = util_->mapChar(tagBound);
        bounds_[5] = util_->mapChar(elemBound);
        bounds_[6] = util_->mapChar(escape);
    }
    PartCorpusIO(const CorpusIO & c, const char* unkBound = " ", const char* skipBound = "?", const char* noBound = "-", const char* hasBound = "|", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : CorpusIO(c), bounds_(7) { 
        bounds_[0] = util_->mapChar(unkBound);
        bounds_[1] = util_->mapChar(skipBound);
        bounds_[2] = util_->mapChar(noBound);
        bounds_[3] = util_->mapChar(hasBound);
        bounds_[4] = util_->mapChar(tagBound);
        bounds_[5] = util_->mapChar(elemBound);
        bounds_[6] = util_->mapChar(escape);
    }
    PartCorpusIO(StringUtil * util, const char* file, bool out, const char* unkBound = " ", const char* skipBound = "?", const char* noBound = "-", const char* hasBound = "|", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : CorpusIO(util,file,out), bounds_(7) { 
        bounds_[0] = util_->mapChar(unkBound);
        bounds_[1] = util_->mapChar(skipBound);
        bounds_[2] = util_->mapChar(noBound);
        bounds_[3] = util_->mapChar(hasBound);
        bounds_[4] = util_->mapChar(tagBound);
        bounds_[5] = util_->mapChar(elemBound);
        bounds_[6] = util_->mapChar(escape);
    }
    PartCorpusIO(StringUtil * util, std::iostream & str, bool out, const char* unkBound = " ", const char* skipBound = "?", const char* noBound = "-", const char* hasBound = "|", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : CorpusIO(util,str,out), bounds_(7) { 
        bounds_[0] = util_->mapChar(unkBound);
        bounds_[1] = util_->mapChar(skipBound);
        bounds_[2] = util_->mapChar(noBound);
        bounds_[3] = util_->mapChar(hasBound);
        bounds_[4] = util_->mapChar(tagBound);
        bounds_[5] = util_->mapChar(elemBound);
        bounds_[6] = util_->mapChar(escape);
    }
    
    KyteaSentence * readSentence();
    void writeSentence(const KyteaSentence * sent, double conf = 0.0);

};

class ProbCorpusIO : public FullCorpusIO {

public:
    ProbCorpusIO(StringUtil * util, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : FullCorpusIO(util,wordBound,tagBound,elemBound,escape) { allTags_ = true; }
    ProbCorpusIO(const CorpusIO & c, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : FullCorpusIO(c,wordBound,tagBound,elemBound,escape) { allTags_ = true; }
    ProbCorpusIO(StringUtil * util, const char* file, bool out, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : FullCorpusIO(util,file,out,wordBound,tagBound,elemBound,escape) { allTags_ = true; } 
    ProbCorpusIO(StringUtil * util, std::iostream & str, bool out, const char* wordBound = " ", const char* tagBound = "/", const char* elemBound = "&", const char* escape = "\\") : FullCorpusIO(util,str,out,wordBound,tagBound,elemBound,escape) { allTags_ = true; }

    KyteaSentence * readSentence();
    void writeSentence(const KyteaSentence * sent, double conf = 0.0);

};

class RawCorpusIO : public CorpusIO {

public:
    RawCorpusIO(StringUtil * util) : CorpusIO(util) { }
    RawCorpusIO(const CorpusIO & c) : CorpusIO(c) { }
    RawCorpusIO(StringUtil * util, const char* file, bool out) : CorpusIO(util,file,out) { } 
    RawCorpusIO(StringUtil * util, std::iostream & str, bool out) : CorpusIO(util,str,out) { }

    KyteaSentence * readSentence();
    void writeSentence(const KyteaSentence * sent, double conf = 0.0);

};


}

#endif
