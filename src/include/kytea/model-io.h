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

#ifndef MODEL_IO_H__ 
#define MODEL_IO_H__ 

#include <kytea/dictionary.h>
#include <kytea/general-io.h>
#include <kytea/feature-vector.h>
#include <vector>

#if DISABLE_QUANTIZE
#   define MODEL_IO_VERSION "0.4.0NQ"
#else
#   define MODEL_IO_VERSION "0.4.0"
#endif

namespace kytea {

class FeatureLookup;
class KyteaModel;
class KyteaLM;
class ModelTagEntry;
class ProbTagEntry;

class ModelIO : public GeneralIO {

public:

    typedef char Format;
    const static Format FORMAT_BINARY = 'B';
    const static Format FORMAT_TEXT = 'T';
    const static Format FORMAT_UNKNOWN = 'U';

    int numTags_;

public:

    ModelIO(StringUtil* util) : GeneralIO(util) { }
    ModelIO(StringUtil* util, const char* file, bool out, bool bin) : GeneralIO(util,file,out,bin) { }
    ModelIO(StringUtil* util, std::iostream & str, bool out, bool bin) : GeneralIO(util,str,out,bin) { }

    virtual ~ModelIO() { }

    static ModelIO* createIO(const char* file, Format form, bool output, KyteaConfig & config);
    static ModelIO* createIO(std::iostream & str, Format form, bool output, KyteaConfig & config);

    virtual void writeConfig(const KyteaConfig & conf) = 0;
    virtual void writeModel(const KyteaModel * mod) = 0;
    virtual void writeWordList(const std::vector<KyteaString> & list) = 0;
    virtual void writeLM(const KyteaLM * mod) = 0;
    virtual void writeFeatVec(const FeatVec * vec) = 0;

    virtual void readConfig(KyteaConfig & conf) = 0;
    virtual KyteaModel * readModel() = 0;
    virtual std::vector<KyteaString> readWordList() = 0;
    virtual KyteaLM * readLM() = 0;
    virtual FeatVec * readFeatVec() = 0;

    // These must be explicitly expanded because templated virtuals are not allowed
    virtual void writeModelDictionary(const Dictionary<ModelTagEntry> * dict) = 0;
    virtual void writeProbDictionary(const Dictionary<ProbTagEntry> * dict) = 0;
    virtual void writeVectorDictionary(const Dictionary<FeatVec > * dict) = 0;
    virtual Dictionary<ModelTagEntry> * readModelDictionary() = 0;
    virtual Dictionary<ProbTagEntry> * readProbDictionary() = 0;
    virtual Dictionary<FeatVec > * readVectorDictionary() = 0;

    virtual void writeFeatureLookup(const FeatureLookup * featLookup) = 0;
    virtual FeatureLookup * readFeatureLookup() = 0;

};

}

#endif
