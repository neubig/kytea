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

#ifndef KYTEA_CONFIG_H__
#define KYTEA_CONFIG_H__

namespace kytea {
class KyteaConfig;
}

#include <string>
#include <vector>
#include <kytea/corpus-io-format.h>

namespace kytea {

class StringUtil;

class KyteaConfig {

private:
    bool onTraining_;

    unsigned debug_;            // the debugging level
                                // 0 = silent (default for running)
                                // 1 = simple progress updates (default for training)
                                // 2 = detailed progress updates
                                // 3 = everything

    StringUtil * util_;         // a std::string utility to hold the encoding, etc

    std::vector<std::string> corpora_;    // corpora to read for training
    std::vector<CorpusFormat> corpusFormats_; // the annotation of each corpus
    
    std::vector<std::string> dicts_;      // dictionaries to read

    std::vector<std::string> subwordDicts_; // subword dictionaries to use for unknown estimation

    std::string model_;              // model file to write/read
    char modelForm_;             // model format (ModelIO::Format)

    std::string input_, output_;     // the file to input/output
    CorpusFormat inputForm_, outputForm_; // the format/file to input/output to (default: stdout, full)

    std::string featIn_, featOut_;
    std::ostream* featStr_;

    bool doWS_, doTags_, doUnk_;
    std::vector<bool> doTag_;

    // feature options
    bool addFeat_;              // whether or not to add newly found features
    double confidence_;         // when using probability, only annotate or use values that
                                //  are at least this confident (default: 0=deterministic)
    
    //  charW: the number of characters on either side of the boundary to use (default: 3)
    //  charN:      the maximum n-gram order of characters to use (default: 3)
    //  typeW: the number of character types on either side of the boundary to use (default: 3)
    //  typeN:      the maximum n-gram order of types to use (default: 3)
    //  dictN:   all dictionary words over this are treated as equal frequency (default: 4)
    char charW_, charN_, typeW_, typeN_, dictN_;

    // unknown word arguments
    //  unkN: the n-gram length of the unknown word spelling model
    //  defTag: a default tag to use when no candidates were generated
    //  unkTag: a tag to append after every word with no tag in the dictionary
    char unkN_;
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
    void setIOFormat(const char* str, CorpusFormat & cf);
    
    // formatting tags
    std::string wordBound_, tagBound_, elemBound_, unkBound_, noBound_, hasBound_, skipBound_, escape_;

    // hard constraint on character divisions. can be used for digits, etc.
    std::string wsConstraint_;

    // the number of tag levels
    int numTags_;
    std::vector<bool> global_;
    //  tagMax: the maximum number of tags to return for a word
    unsigned tagMax_;

    // check argument legality
    void ch(const char * n, const char* v);

public:

    KyteaConfig();
    KyteaConfig(const KyteaConfig & rhs);
    ~KyteaConfig();
    void addCorpus(const std::string & corp, CorpusFormat format);
    void addDictionary(const std::string & corp);
    void addSubwordDict(const std::string & corp);

    // parse command line arguments
    void parseTrainCommandLine(int argc, const char ** argv);
    void parseRunCommandLine(int argc, const char ** argv);

    void printUsage();

    void printVersion();

    // parse a single argument
    //  the value argument can be null
    //  return 1 if the value was used 0 if not
    unsigned parseTrainArg(const char * n, const char * v);

    unsigned parseRunArg(const char * n, const char * v);

    // getters
    const std::vector<std::string> & getCorpusFiles() const { return corpora_; }
    const std::vector<CorpusFormat> & getCorpusFormats() const { return corpusFormats_; }
    const std::vector<std::string> & getDictionaryFiles() const { return dicts_; }
    const std::vector<std::string> & getSubwordDictFiles() const { return subwordDicts_; }
    const std::string & getModelFile();
    const char getModelFormat() const { return modelForm_; }
    const unsigned getDebug() const { return debug_; }
    StringUtil * getStringUtil() { return util_; }
    const StringUtil * getStringUtil() const { return util_; }
    const CorpusFormat getInputFormat() const { return inputForm_; }
    const CorpusFormat getOutputFormat() const { return outputForm_; }
    const std::string & getFeatureIn() const { return featIn_; }
    const std::string & getFeatureOut() const { return featOut_; }
    const bool getWriteFeatures() const { return featOut_.length() > 0; }
    
    const char getCharN() const { return charN_; }
    const char getCharWindow() const { return charW_; }
    const char getTypeN() const { return typeN_; }
    const char getTypeWindow() const { return typeW_; }
    const char getDictionaryN() const { return dictN_; }
    const char getUnkN() const { return unkN_; }
    const unsigned getTagMax() const { return tagMax_; }
    const unsigned getUnkBeam() const { return unkBeam_; }
    const std::string & getUnkTag() const { return unkTag_; }
    const std::string & getDefaultTag() const { return defTag_; }
    const std::string & getWsConstraint() const { return wsConstraint_; }

    const double getBias() const { return bias_; }
    const double getEpsilon() const { return eps_; }
    const double getCost() const { return cost_; }
    const int getSolverType() const { return solverType_; }
    const bool getDoWS() const { return doWS_; }
    const bool getDoUnk() const { return doUnk_; }
    const bool getDoTags() const { return doTags_; }
    const bool getDoTag(int i) const { return doTags_ && (i >= (int)doTag_.size() || doTag_[i]); }
    const char* getWordBound() const { return wordBound_.c_str(); } 
    const char* getTagBound() const { return tagBound_.c_str(); } 
    const char* getElemBound() const { return elemBound_.c_str(); } 
    const char* getUnkBound() const { return unkBound_.c_str(); } 
    const char* getNoBound() const { return noBound_.c_str(); } 
    const char* getHasBound() const { return hasBound_.c_str(); } 
    const char* getSkipBound() const { return skipBound_.c_str(); } 
    const char* getEscape() const { return escape_.c_str(); } 

    const double getConfidence() const { return confidence_; }
    const char getEncoding() const;
    const char* getEncodingString() const;
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
    void setTagMax(unsigned v) { tagMax_ = v; }
    void setUnkBeam(unsigned v) { unkBeam_ = v; }
    void setUnkTag(const std::string & v) { unkTag_ = v; }
    void setUnkTag(const char* v) { unkTag_ = v; }
    void setDefaultTag(const std::string & v) { defTag_ = v; }
    void setDefaultTag(const char* v) { defTag_ = v; }
    void setOnTraining(bool v) { onTraining_ = v; }
    void setDoWS(bool v) { doWS_ = v; }
    void setDoUnk(bool v) { doUnk_ = v; }
    void setDoTags(bool v) { doTags_ = v; } 
    void setDoTag(int i, bool v)  { 
        if(i >= (int)doTag_.size()) doTag_.resize(i+1,true); 
        doTag_[i] = v;
    } 
    void setInputFormat(CorpusFormat v) { inputForm_ = v; }
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
    void setFeatureIn(const std::string & featIn) { featIn_ = featIn; }
    void setFeatureOut(const std::string & featOut) { featOut_ = featOut; }
    void setWsConstraint(const std::string & wsConstraint) { wsConstraint_ = wsConstraint; }

    std::ostream * getFeatureOutStream();
    void closeFeatureOutStream();

    // set the encoding of the StringUtil class and reset all the IOs
    void setEncoding(const char* str);

};

}

#endif
