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

#ifndef KYPE_CONFIG_H__
#define KYPE_CONFIG_H__

namespace kytea {
class KyteaConfig;
}

#include "string-util.h"
#include "corpus-io.h"
#include <cstring>
#include <cmath>

namespace kytea {

class KyteaConfig {

private:

    typedef StringUtil::Encoding Encoding;
    // must be the same as CorpusIO::Format, not used directly because of cross-dependencies
    typedef char CorpForm;
    bool onTraining_;

    unsigned debug_;            // the debugging level
                                // 0 = silent (default for running)
                                // 1 = simple progress updates (default for training)
                                // 2 = detailed progress updates
                                // 3 = everything

    StringUtil * util_;         // a std::string utility to hold the encoding, etc

    std::vector<std::string> corpora_;    // corpora to read for training
    std::vector<CorpForm> corpusFormats_; // the annotation of each corpus
    
    std::vector<std::string> dicts_;      // dictionaries to read

    std::vector<std::string> subwordDicts_; // subword dictionaries to use for unknown estimation

    std::string model_;              // model file to write/read
    char modelForm_;             // model format (ModelIO::Format)

    std::string input_, output_;     // the file to input/output
    CorpForm inputForm_, outputForm_; // the format/file to input/output to (default: stdout, full)

    bool doWS_, doTags_;
    std::vector<bool> doTag_;

    // feature options
    bool addFeat_;              // whether or not to add newly found features
    double confidence_;          // when using probability, only annotate or use values that
                                //  are at least this confident (default: 0=deterministic)
    
    //  charW: the number of characters on either side of the boundary to use (default: 3)
    //  charN:      the maximum n-gram order of characters to use (default: 3)
    //  typeW: the number of character types on either side of the boundary to use (default: 3)
    //  typeN:      the maximum n-gram order of types to use (default: 3)
    //  dictN:   all dictionary words over this are treated as equal frequency (default: 4)
    char charW_, charN_, typeW_, typeN_, dictN_;

    // unknown word arguments
    //  unkN: the n-gram length of the unknown word spelling model
    //  unkCount: the maximum number of unknown word candidates to return
    //  defTag: a default tag to use when no candidates were generated
    //  unkTag: a tag to append after every word with no tag in the dictionary
    char unkN_;
    unsigned unkCount_;
    unsigned unkBeam_;
    std::string defTag_;
    std::string unkTag_;

    // liblinear training values
    double bias_;    // the bias used for liblinear training
    double eps_;     // the termination epsilon
    double cost_;    // the cost for the SVM or LR training
    int solverType_; // the type of solver to be used

    // extra arguments, should be input/output for the analyzer
    std::vector<std::string> args_;

    // set the type of the input corpus
    void setIOFormat(const char* str, CorpForm & cf);
    
    // formatting tags
    std::string wordBound_, tagBound_, elemBound_, unkBound_, noBound_, hasBound_, skipBound_, escape_;

    // the number of tag levels
    int numTags_;
    std::vector<bool> global_;

    // check argument legality
    void ch(const char * n, const char* v);

public:

    KyteaConfig() : onTraining_(true), debug_(0), util_(0), dicts_(), 
                    modelForm_('B'), inputForm_(CORP_FORMAT_DEFAULT),
                    outputForm_(CORP_FORMAT_FULL), doWS_(true), doTags_(true),
                    addFeat_(false), confidence_(0.0), charW_(3), charN_(3), 
                    typeW_(3), typeN_(3), dictN_(4), 
                    unkN_(3), unkCount_(5), unkBeam_(50), defTag_("UNK"), unkTag_(),
                    bias_(1.0f), eps_(HUGE_VAL), cost_(1.0),
                    solverType_(1/*SVM*/),
                    wordBound_(" "), tagBound_("/"), elemBound_("&"), unkBound_(" "), 
                    noBound_("-"), hasBound_("|"), skipBound_("?"), escape_("\\"), 
                    numTags_(0) {
        setEncoding("utf8");
    }
    ~KyteaConfig() {
        if(util_)
            delete util_;
    }

    void addCorpus(const std::string & corp, CorpForm format) {
        corpora_.push_back(corp);
        corpusFormats_.push_back(format);
    }
    
    void addDictionary(const std::string & corp) {
        dicts_.push_back(corp);
    }
    
    void addSubwordDict(const std::string & corp) {
        subwordDicts_.push_back(corp);
    }

    // parse command line arguments
    void parseTrainCommandLine(int argc, char ** argv);
    void parseRunCommandLine(int argc, char ** argv);

    void printUsage();

    void printVersion();

    // parse a single argument
    //  the value argument can be null
    //  return 1 if the value was used 0 if not
    unsigned parseTrainArg(const char * n, const char * v);

    unsigned parseRunArg(const char * n, const char * v);

    // getters
    const std::vector<std::string> & getCorpusFiles() const { return corpora_; }
    const std::vector<CorpForm> & getCorpusFormats() const { return corpusFormats_; }
    const std::vector<std::string> & getDictionaryFiles() const { return dicts_; }
    const std::vector<std::string> & getSubwordDictFiles() const { return subwordDicts_; }
    const std::string & getModelFile() const { return model_; }
    const char getModelFormat() const { return modelForm_; }
    const unsigned getDebug() const { return debug_; }
    StringUtil * getStringUtil() { return util_; }
    const StringUtil * getStringUtil() const { return util_; }
    const CorpForm getInputFormat() const { return inputForm_; }
    const CorpForm getOutputFormat() const { return outputForm_; }
    
    const char getCharN() const { return charN_; }
    const char getCharWindow() const { return charW_; }
    const char getTypeN() const { return typeN_; }
    const char getTypeWindow() const { return typeW_; }
    const char getDictionaryN() const { return dictN_; }
    const char getUnkN() const { return unkN_; }
    const unsigned getUnkCount() const { return unkCount_; }
    const unsigned getUnkBeam() const { return unkBeam_; }
    const std::string & getUnkTag() const { return unkTag_; }
    const std::string & getDefaultTag() const { return defTag_; }

    const double getBias() const { return bias_; }
    const double getEpsilon() const { return eps_; }
    const double getCost() const { return cost_; }
    const int getSolverType() const { return solverType_; }
    const bool getDoWS() const { return doWS_; }
    const bool getDoTags() const { return doTags_; }
    const bool getDoTag(int i) const { return (i >= (int)doTag_.size() || doTag_[i]); }
    const char* getWordBound() const { return wordBound_.c_str(); } 
    const char* getTagBound() const { return tagBound_.c_str(); } 
    const char* getElemBound() const { return elemBound_.c_str(); } 
    const char* getUnkBound() const { return unkBound_.c_str(); } 
    const char* getNoBound() const { return noBound_.c_str(); } 
    const char* getHasBound() const { return hasBound_.c_str(); } 
    const char* getSkipBound() const { return skipBound_.c_str(); } 
    const char* getEscape() const { return escape_.c_str(); } 

    const double getConfidence() const { return confidence_; }
    const char getEncoding() const { return util_->getEncoding(); }
    const char* getEncodingString() const { return util_->getEncodingString(); }
    int getNumTags() const { return numTags_; }
    bool getGlobal(int i) const { return i < (int)global_.size() && global_[i]; }

    const std::vector<std::string> & getArguments() const { return args_; }
    
    // setters
    void setDebug(unsigned debug) { debug_ = debug; }
    void setModelFile(const char* file) { model_ = file; }
    void setModelFormat(char mf) { modelForm_ = mf; }
    void setEpsilon(double v) { eps_ = v; }
    void setCost(double v) { cost_ = v; }
    void setBias(bool v) { bias_ = (v?1.0f:-1.0f); }
    void setSolverType(int v) { solverType_ = v; }
    void setCharWindow(char v) { charW_ = v; }
    void setCharN(char v) { charN_ = v; }
    void setTypeWindow(char v) { typeW_ = v; }
    void setTypeN(char v) { typeN_ = v; }
    void setDictionaryN(char v) { dictN_ = v; }
    void setUnkN(char v) { unkN_ = v; }
    void setUnkCount(unsigned v) { unkCount_ = v; }
    void setUnkBeam(unsigned v) { unkBeam_ = v; }
    void setUnkTag(const std::string & v) { unkTag_ = v; }
    void setUnkTag(const char* v) { unkTag_ = v; }
    void setDefaultTag(const std::string & v) { defTag_ = v; }
    void setDefaultTag(const char* v) { defTag_ = v; }
    void setOnTraining(bool v) { onTraining_ = v; }
    void setDoWS(bool v) { doWS_ = v; }
    void setDoTags(bool v) { doTags_ = v; } 
    void setDoTag(int i, bool v)  { if(i >= (int)doTag_.size()) doTag_.resize(i+1,true); doTag_[i] = v; } 
    void setInputFormat(CorpForm v) { inputForm_ = v; }
    void setWordBound(const char* v) { wordBound_ = v; } 
    void setTagBound(const char* v) { tagBound_ = v; } 
    void setElemBound(const char* v) { elemBound_ = v; } 
    void setUnkBound(const char* v) { unkBound_ = v; } 
    void setNoBound(const char* v) { noBound_ = v; } 
    void setHasBound(const char* v) { hasBound_ = v; } 
    void setSkipBound(const char* v) { skipBound_ = v; } 
    void setEscape(const char* v) { escape_ = v; } 
    void setNumTags(int v) { numTags_ = v; } 
    void setGlobal(int v) { if((int)global_.size() <= v) global_.resize(v+1,false); global_[v] = true; } 


    // set the encoding of the StringUtil class and reset all the IOs
    void setEncoding(const char* str) {
        if(util_)
            delete util_;
        if(!strcmp(str,"utf8")) util_ = new StringUtilUtf8();
        else if(!strcmp(str,"euc")) util_ = new StringUtilEuc();
        else if(!strcmp(str,"sjis")) util_ = new StringUtilSjis();
        else
            THROW_ERROR("Unsupported encoding format '" << str << "'");
    }

};

}

#endif
