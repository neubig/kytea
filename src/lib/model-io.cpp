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

#include <kytea/kytea-config.h>
#include <kytea/kytea-util.h>
#include <kytea/kytea-model.h>
#include <kytea/kytea-lm.h>
#include <kytea/kytea-string.h>
#include <kytea/dictionary.h>
#include <kytea/feature-lookup.h>
#include <kytea/string-util.h>
#include <kytea/model-io.h>
#include <kytea/model-io-text.h>
#include <kytea/model-io-binary.h>
#include <algorithm>
#include <cstring>
#include <set>
#include <fstream>

#define BUFFER_SIZE 4096
#define NEG_INFINITY -999.0
#define NULL_STRING "<NULL>"

using namespace std;

namespace kytea {

static const char *solver_type_table[]=
{
	"L2R_LR", "L2R_L2LOSS_SVC_DUAL", "L2R_L2LOSS_SVC","L2R_L1LOSS_SVC_DUAL","MCSVM_CS","L1R_L2LOSS_SVC","L1R_LR","L2R_LR_DUAL", NULL
};

ModelIO * ModelIO::createIO(const char* file, Format form, bool output, KyteaConfig & config) {
    if(output && form == ModelIO::FORMAT_UNKNOWN) {
        THROW_ERROR("A format must be specified for model output");
    } else if(!output) {
        ifstream ifs(file);
        if(!ifs.good()) 
            THROW_ERROR("Could not open model file "<<file);
        string line,buff1,buff2,buff3,buff4;
        getline(ifs, line);
        istringstream iss(line);
        if(!(iss >> buff1) || !(iss >> buff2) || !(iss >> buff3) || !(iss >> buff4) || 
                                  buff1 != "KyTea" || buff3.length() != 1)
            THROW_ERROR("Badly formed model (header incorrect)");
        if(buff2 != MODEL_IO_VERSION)
            THROW_ERROR("Incompatible model version. Expected " << MODEL_IO_VERSION << ", but found " << buff2 << ".");
        form = buff3[0];
        config.setEncoding(buff4.c_str());
        ifs.close();
    }
    StringUtil * util = config.getStringUtil();
    if(form == ModelIO::FORMAT_TEXT)      { return new TextModelIO(util,file,output); }
    else if(form == ModelIO::FORMAT_BINARY) { return new BinaryModelIO(util,file,output); }
    else {
        THROW_ERROR("Illegal model format");
    }
}

ModelIO * ModelIO::createIO(iostream & file, Format form, bool output, KyteaConfig & config) {
    StringUtil * util = config.getStringUtil();
    if(form == ModelIO::FORMAT_TEXT)      { return new TextModelIO(util,file,output); }
    else if(form == ModelIO::FORMAT_BINARY) { return new BinaryModelIO(util,file,output); }
    else {
        THROW_ERROR("Illegal model format");
    }
}

void TextModelIO::writeConfig(const KyteaConfig & config) {

    *str_ << "KyTea " << MODEL_IO_VERSION << " T " << config.getEncodingString() << endl;

    numTags_ = (int)config.getNumTags();
    if(!config.getDoWS()) *str_ << "-nows" << endl;
    if(!config.getDoTags()) *str_ << "-notags" << endl;
    *str_ << "-numtags " << numTags_ << endl;
    if(config.getBias()<0) *str_ << "-nobias" << endl;
    *str_ << "-charw " << (int)config.getCharWindow() << endl
          << "-charn " << (int)config.getCharN() << endl
          << "-typew " << (int)config.getTypeWindow() << endl
          << "-typen " << (int)config.getTypeN() << endl
          << "-dicn "  << (int)config.getDictionaryN() << endl
          << "-eps " << config.getEpsilon() << endl
          << "-solver " << config.getSolverType() << endl << endl;

    // write the character map
    *str_ << "characters" << endl
          << config.getStringUtil()->serialize() << endl;

    *str_ << endl;

}

void TextModelIO::readConfig(KyteaConfig & config) {
    string line,s1,s2;
    getline(*str_,line); // ignore the header
    while(getline(*str_, line) && line.length() != 0) {
        istringstream iss(line);
        iss >> s1;
        iss >> s2;
        config.parseTrainArg(s1.c_str(), (s2.length()==0?0:s2.c_str()));
    }
    numTags_ = config.getNumTags();
    
    getline(*str_,line); // check the header
    if(line != "characters") THROW_ERROR("Badly formatted file, expected 'characters', got '" << line << "'");
    getline(*str_, line); // get the serialized string util
    config.getStringUtil()->unserialize(line);
    getline(*str_, line); // check the last line
}

void TextModelIO::writeModel(const KyteaModel * mod) {

    // print a single endl for empty models
    if(mod == 0 || mod->getNumClasses() < 2) {
        *str_ << endl;
        return;
    }

    int i;
    int nr_feature=mod->getNumFeatures();
    int n;

    if(mod->getBias()>=0)
    	n=nr_feature+1;
    else
    	n=nr_feature;
    int w_size = n;

    int nr_w = mod->getNumWeights();

    *str_ << "solver_type " << solver_type_table[mod->getSolver()] << endl;
    *str_ << "nr_class " << mod->getNumClasses() << endl;
    *str_ << "label";
    for(i=0; i<(int)mod->getNumClasses(); i++)
        *str_ << " " << mod->getLabel(i);    
    *str_ << endl;

    *str_ << "nr_feature " << nr_feature << endl;

    char buffer[50];

    // fix this
    sprintf(buffer, "%.16g", mod->getBias());
    *str_ << "bias " << buffer << endl;

    sprintf(buffer, "%.16g", mod->getMultiplier());
    *str_ << "mult " << buffer << endl;

    *str_ << "w" << endl;
    // print the feature names and values
    const FeatNameVec & names = mod->getNames();
    for(i=0; i<w_size; i++)
    {
    	int j;
        if(i < nr_feature)
            *str_ << util_->showString(names[i+1]) << endl;
    	for(j=0; j<nr_w; j++)
            *str_ << mod->getWeight(i,j) << " ";
        *str_ << endl;
    }

    *str_ << endl;

    writeFeatureLookup(mod->getFeatureLookup());

}

// write out a language model
void TextModelIO::writeLM(const KyteaLM * lm) {
    // print a single endl for empty models
    if(lm == 0) {
        *str_ << endl;
        return;
    }
    *str_ << "lmn " << lm->n_ << endl;
    *str_ << "lmvocab " << lm->vocabSize_ << endl;
    
    KyteaChar spaceChar = util_->mapChar(" ");
    KyteaString nullString = util_->mapString(NULL_STRING);

    // sort the set of all keys
    set<KyteaString> keys;
    for(KyteaDoubleMap::const_iterator it = lm->probs_.begin(); it != lm->probs_.end(); it++)
        keys.insert(it->first);
    for(KyteaDoubleMap::const_iterator it = lm->fallbacks_.begin(); it != lm->fallbacks_.end(); it++)
        keys.insert(it->first);
    for(set<KyteaString>::const_iterator it = keys.begin(); it != keys.end(); it++) {
        KyteaDoubleMap::const_iterator fit = const_cast<KyteaLM*>(lm)->probs_.find(*it);
        KyteaString displayString;
        if(it->length() == 0)
            displayString = nullString;
        else {
            displayString = *it;
            // remove the null characters
            for(unsigned i = 0; i < displayString.length(); i++)
                if(!displayString[i])
                    displayString[i] = spaceChar;
        }
        *str_ << (fit == lm->probs_.end() ? NEG_INFINITY : fit->second) 
            << "\t" << util_->showString(displayString);
        fit = const_cast<KyteaLM*>(lm)->fallbacks_.find(*it);
        if(fit != lm->fallbacks_.end())
            *str_ << "\t" << fit->second;
        *str_ << endl;
    }
    *str_ << endl; 
}

KyteaModel * TextModelIO::readModel() {

    // the first line either contains the feature count or empty line
    string line;
    getline(*str_, line);
    if(line.length() == 0)
        return 0;

	int i;
	int nr_feature;
	int n;
	int nr_class;
	double bias;
	double mult;
    KyteaModel * mod = new KyteaModel();

	string str;
	while(1)
	{
        *str_ >> str;
		if(strcmp(str.c_str(),"solver_type")==0) {
            *str_ >> str;
			int i;
			for(i=0;solver_type_table[i];i++) {
				if(strcmp(solver_type_table[i],str.c_str())==0) {
                    mod->setSolver(i);
					break;
				}
			}
			if(solver_type_table[i] == NULL) {
                delete mod;
				THROW_ERROR("unknown solver type.");
			}
		}
		else if(strcmp(str.c_str(),"nr_class")==0) {
            *str_ >> nr_class;
            mod->setNumClasses(nr_class);
		}
		else if(strcmp(str.c_str(),"nr_feature")==0) {
            *str_ >> nr_feature;
		}
		else if(strcmp(str.c_str(),"bias")==0) {
            *str_ >> bias;
            mod->setBias(bias);
		}
		else if(strcmp(str.c_str(),"mult")==0) {
            *str_ >> mult;
            mod->setMultiplier(mult);
		}
		else if(strcmp(str.c_str(),"w")==0) {
            // clear out the rest of the line
            getline(*str_,str);
			break;
		}
		else if(strcmp(str.c_str(),"label")==0) {
			int nr_class = mod->getNumClasses();
            int label;
			for(int i=0;i<nr_class;i++) {
				*str_ >> label;
                mod->setLabel(i,label);
            }
		}
		else {
            delete mod;
			THROW_ERROR("Unknown text in model file '" << str << "'");
		}
	}

	if(mod->getBias()>=0)
		n=nr_feature+1;
	else
		n=nr_feature;
	int w_size = n;
	int nr_w = mod->getNumWeights();

    mod->initializeWeights(w_size,nr_w);
	for(i=0; i<w_size; i++) {
		int j;
        if(i < nr_feature) {
            getline(*str_, line);
            mod->mapFeat(util_->mapString(line));
        }
        getline(*str_,str);
        istringstream iss(str);
        string buff;
		for(j=0; j<nr_w; j++) {
            iss >> buff;
            mod->setWeight(i,j,(FeatVal)util_->parseFloat(buff.c_str()));
        }
	}
    mod->setNumFeatures(nr_feature);
    
    getline(*str_, str);
    if(str.length() != 0 && str != " ")
        THROW_ERROR("Bad line when expecting end of file: '" << str << "'");

    // read models shouldn't add any additional features
    mod->setAddFeatures(false);
    mod->setFeatureLookup(readFeatureLookup());

    return mod;

}

// write out a language model
KyteaLM * TextModelIO::readLM() {
    // the first line either contains the n-gram length, or an empty line
    string line, str;
    getline(*str_, line);
    if(line.length() == 0)
        return 0;
    
    // get and check the first line
    istringstream linestream1(line);
    linestream1 >> str;
    if(str != "lmn") {
        cerr << str << endl;
        THROW_ERROR("Badly formatted first line in LM");
    }
    linestream1 >> str;
    KyteaLM* lm = new KyteaLM(util_->parseInt(str.c_str()));

    // get and check the second line
    getline(*str_, line);
    istringstream linestream2(line);
    linestream2 >> str;
    if(str != "lmvocab") THROW_ERROR("Badly formatted second line in LM");
    linestream2 >> str;
    lm->vocabSize_ = util_->parseInt(str.c_str());
    KyteaChar spaceChar = util_->mapChar(" ");

    // get and check
    double prob, fb;
    KyteaString kword;
    while(getline(*str_, line)) {
        if(line.length() == 0)
            break;
        istringstream linestream(line);
        // prob
        getline(linestream, str, '\t');
        prob = util_->parseFloat(str.c_str());
        // word
        getline(linestream, str, '\t');
        if(str == NULL_STRING) str = "";
        kword = util_->mapString(str);
        for(unsigned i = 0; i < kword.length(); i++)
            if(kword[i] == spaceChar)
                kword[i] = 0;
        // fallback
        if(getline(linestream, str, '\t')) {
            fb = util_->parseFloat(str.c_str());
            if(fb != NEG_INFINITY)
                lm->fallbacks_.insert(pair<KyteaString,double>(kword,fb)); 
        }
        if(prob != NEG_INFINITY)
            lm->probs_.insert(pair<KyteaString,double>(kword,prob)); 
    }

    return lm;
}


void TextModelIO::writeFeatVec(const vector<FeatVal> * entry) {
    int mySize = (int)(entry ? entry->size() : 0);
    for(int j = 0; j < mySize; j++) {
        if(j!=0) *str_ << " ";
        *str_ << (*entry)[j];
        // *str_ << util_->showString((*entry)[j]);
    }
    *str_ << endl;
}

template <>
void TextModelIO::writeEntry(const vector<FeatVal> * entry) {
    writeFeatVec(entry);
}

template <>
void TextModelIO::writeEntry(const ModelTagEntry * entry) {
    *str_ << util_->showString(entry->word) << endl;
    for(int i = 0; i < numTags_; i++) {
        int mySize = (int)entry->tags.size() > i ? entry->tags[i].size() : 0;
        for(int j = 0; j < mySize; j++) {
            if(j!=0) *str_ << " ";
            *str_ << util_->showString(entry->tags[i][j]);
        }
        *str_ << endl;
        for(int j = 0; j < mySize; j++) {
            if(j!=0) *str_ << " ";
            *str_ << (int)entry->tagInDicts[i][j];
        }
        *str_ << endl;
    }
    bool has = false;
    for(unsigned j = 0; j < 8; j++) {
        if(entry->isInDict(j)) {
            if(has) *str_ << " ";
            *str_ << (unsigned)j;
            has = true;
        }
    }
    *str_ << endl;
    for(int i = 0; i < numTags_; i++) {
        writeModel((int)entry->tagMods.size() > i?entry->tagMods[i]:0);
    }
}

template <>
void TextModelIO::writeEntry(const ProbTagEntry * entry) {
    *str_ << util_->showString(entry->word) << endl;
    for(int i = 0; i < numTags_; i++) {
        int mySize = (int)entry->tags.size() > i ? entry->tags[i].size() : 0;
        for(int j = 0; j < mySize; j++) {
            if(j!=0) *str_ << " ";
            *str_ << util_->showString(entry->tags[i][j]);
        }
        *str_ << endl;
    }
    for(int i = 0; i < numTags_; i++) {
        int mySize = (int)entry->probs.size() > i ? entry->probs[i].size() : 0;
        for(int j = 0; j < mySize; j++) {
            if(j!=0) *str_ << " ";
            *str_ << entry->probs[i][j];
        }
        *str_<<endl;
    }
}

void BinaryModelIO::writeFeatVec(const vector<FeatVal> * entry) {
    int mySize = (int)(entry ? entry->size() : 0);
    writeBinary((uint32_t)mySize);
    for(int j = 0; j < mySize; j++)
        writeBinary((FeatVal)(*entry)[j]);
}

template <>
void BinaryModelIO::writeEntry(const vector<FeatVal> * entry) {
    writeFeatVec(entry);
}

template <>
void BinaryModelIO::writeEntry(const ProbTagEntry * entry) {
    writeString(entry->word);
    for(int i = 0; i < numTags_; i++) {
        int mySize = (int)entry->tags.size() > i ? entry->tags[i].size() : 0;
        writeBinary((uint32_t)mySize);
        for(int j = 0; j < mySize; j++) {
            writeString(entry->tags[i][j]);
            writeBinary((double)entry->probs[i][j]);
        }
    }
}

vector<FeatVal>* TextModelIO::readFeatVec() {
    string line, buff;
    vector<FeatVal> * entry = new vector<FeatVal>;
    getline(*str_, line);
    istringstream iss(line);
    while(iss >> buff)
        entry->push_back((FeatVal)util_->parseFloat(buff.c_str()));
    return entry;
}

template <>
vector<FeatVal>* TextModelIO::readEntry<vector<FeatVal> >() {
    return readFeatVec();
}

template <>
ModelTagEntry* TextModelIO::readEntry<ModelTagEntry>() {
    string line, buff;
    getline(*str_, line);
    ModelTagEntry* entry = new ModelTagEntry(util_->mapString(line));
    entry->setNumTags(numTags_);
    for(int i = 0; i < numTags_; i++) {
        // get the tags
        getline(*str_, line);
        istringstream iss(line);
        while(iss >> buff)
            entry->tags[i].push_back(util_->mapString(buff.c_str()));
        // get which dictionaries each tag is in
        getline(*str_, line);
        istringstream iss2(line);
        while(iss2 >> buff)
            entry->tagInDicts[i].push_back(util_->parseInt(buff.c_str()));
    }
    getline(*str_, line);
    istringstream iss2(line);
    while(iss2 >> buff)
        entry->setInDict(util_->parseInt(buff.c_str()));
    for(int i = 0; i < numTags_; i++) {
        entry->tagMods[i] = readModel();
        if(entry->tagMods[i] && entry->tagMods[i]->getNumClasses() > entry->tags[i].size())
            THROW_ERROR("Model classes > tag classes ("<<entry->tagMods[i]->getNumClasses()<<", "<<entry->tags[i].size()<<") @ "<<util_->showString(entry->word));
    }
    return entry;
}

template <>
ProbTagEntry* TextModelIO::readEntry<ProbTagEntry>() {
    string line, buff;
    getline(*str_, line);
    ProbTagEntry* entry = new ProbTagEntry(util_->mapString(line));
    entry->setNumTags(numTags_);
    for(int i = 0; i < numTags_; i++) {
        getline(*str_, line);
        istringstream iss(line);
        while(iss >> buff)
            entry->tags[i].push_back(util_->mapString(buff.c_str()));
    }
    for(int i = 0; i < numTags_; i++) {
        getline(*str_, line);
        istringstream iss2(line);
        while(iss2 >> buff) 
            entry->probs[i].push_back(util_->parseFloat(buff.c_str()));
        if(entry->probs[i].size() != entry->tags[i].size())
            THROW_ERROR("Non-matching probability and tag values "<<entry->probs[i].size() << " != " << entry->tags[i].size());
    }
    return entry;
}


void BinaryModelIO::writeConfig(const KyteaConfig & config) {
    *str_ << "KyTea " << MODEL_IO_VERSION << " B " << config.getEncodingString() << endl;

    writeBinary(config.getDoWS());
    writeBinary(config.getDoTags());
    numTags_ = config.getNumTags();
    writeBinary((uint32_t)numTags_);
    writeBinary(config.getCharWindow());
    writeBinary(config.getCharN());
    writeBinary(config.getTypeWindow());
    writeBinary(config.getTypeN());
    writeBinary(config.getDictionaryN());
    writeBinary(config.getBias()<0);
    writeBinary(config.getEpsilon());
    writeBinary((char)config.getSolverType());
    
    // write the character map
    writeString(config.getStringUtil()->serialize());

}

void BinaryModelIO::readConfig(KyteaConfig & config) {
    
    string line;
    getline(*str_,line); // ignore the header

    config.setDoWS(readBinary<bool>() && config.getDoWS());
    config.setDoTags(readBinary<bool>() && config.getDoTags());
    numTags_ = readBinary<uint32_t>();
    config.setNumTags(numTags_);
    config.setCharWindow(readBinary<char>());
    config.setCharN(readBinary<char>());
    config.setTypeWindow(readBinary<char>());
    config.setTypeN(readBinary<char>());
    config.setDictionaryN(readBinary<char>());
    config.setBias(readBinary<bool>()?1.0:-1.0);
    config.setEpsilon(readBinary<double>());
    config.setSolverType(readBinary<char>());
     
    config.getStringUtil()->unserialize(readString());
    
}

void BinaryModelIO::writeModel(const KyteaModel * mod) {

    // Write the number of classes, or 0 if there is no model
    if(mod == 0 || mod->getNumClasses() < 2) { 
        writeBinary((int32_t)0);
        return;
    }

    writeBinary((int32_t)mod->getNumClasses());
    writeBinary((char)mod->getSolver());
    for(int i=0; i<(int)mod->getNumClasses(); i++)
        writeBinary((int32_t)mod->getLabel(i));
    writeBinary(mod->getBias()>=0);
    writeBinary(mod->getMultiplier());
    writeFeatureLookup(mod->getFeatureLookup());

}

void BinaryModelIO::writeLM(const KyteaLM * lm) {
    
    // print the n-gram length, 0 for empty models
    if(lm == 0) { 
        writeBinary((uint32_t)0);
        return;
    }

    writeBinary((uint32_t)lm->n_);
    writeBinary((uint32_t)lm->vocabSize_);

    // sort the keys
    set<KyteaString> keys;
    for(KyteaDoubleMap::const_iterator it = lm->probs_.begin(); it != lm->probs_.end(); it++)
        keys.insert(it->first);
    for(KyteaDoubleMap::const_iterator it = lm->fallbacks_.begin(); it != lm->fallbacks_.end(); it++)
        keys.insert(it->first);

    // print
    writeBinary((uint32_t)keys.size());
    for(set<KyteaString>::const_iterator it = keys.begin(); it != keys.end(); it++) {
        writeString(*it);
        KyteaDoubleMap::const_iterator fit = const_cast<KyteaLM*>(lm)->probs_.find(*it);
        writeBinary(fit == lm->probs_.end() ? NEG_INFINITY : fit->second); 
        fit = const_cast<KyteaLM*>(lm)->fallbacks_.find(*it);
        if(it->length() != lm->n_)
            writeBinary(fit == lm->fallbacks_.end() ? NEG_INFINITY : fit->second);
    } 

}

KyteaModel * BinaryModelIO::readModel() {

    // Read the number of classes
	int numC = readBinary<int32_t>();
    if(numC == 0) return NULL;
    KyteaModel * mod = new KyteaModel();
    mod->setAddFeatures(false);
    mod->setNumClasses(numC);
    // Read the solver
    mod->setSolver(readBinary<char>());
    // Read the class labels
	for(int i=0;(int)i<numC;i++)
        mod->setLabel(i, readBinary<int32_t>());
    // Read the bias and multiplier
    mod->setBias(readBinary<bool>()?1.0:-1.0);
    mod->setMultiplier(readBinary<double>());
    // Read the feature lookup
    mod->setFeatureLookup(readFeatureLookup());

    return mod;

}

KyteaLM* BinaryModelIO::readLM() {
    
    unsigned n = readBinary<uint32_t>();
    if(!n)
        return 0;

    KyteaLM* lm = new KyteaLM(n);
    lm->vocabSize_ = readBinary<uint32_t>();

    // sort the keys and print
    unsigned entrysize = readBinary<uint32_t>();
    while(entrysize-- != 0) {
        KyteaString str = readKyteaString();
        double prob = readBinary<double>();
        if(prob != NEG_INFINITY)
            lm->probs_.insert(pair<KyteaString,double>(str,prob));
        if(str.length() != lm->n_) {
            double fallback = readBinary<double>();
            if(fallback != NEG_INFINITY)
                lm->fallbacks_.insert(pair<KyteaString,double>(str,fallback));
        }
    } 

    return lm;

}

template <>
void BinaryModelIO::writeEntry(const ModelTagEntry * entry) {
    writeString(entry->word);
    for(int i = 0; i < numTags_; i++) {
        int mySize = (int)entry->tags.size() > i ? entry->tags[i].size() : 0;
        // if there are multiple tags, but only one existed in model training data, swap
        writeBinary((uint32_t)mySize);
        for(int j = 0; j < mySize; j++) {
            writeString(entry->tags[i][j]);
            writeBinary((unsigned char)entry->tagInDicts[i][j]);
        }
    }
    writeBinary((unsigned char)entry->inDict);
    for(int i = 0; i < numTags_; i++)
        writeModel((int)entry->tagMods.size() > i ? entry->tagMods[i] : 0);
}

vector<FeatVal>* BinaryModelIO::readFeatVec() {
    int mySize = readBinary<uint32_t>();
    vector<FeatVal> * entry = new vector<FeatVal>;
    for(int i = 0; i < mySize; i++)
        entry->push_back(readBinary<FeatVal>());
    return entry;
}

template <>
vector<FeatVal>* BinaryModelIO::readEntry<vector<FeatVal> >() {
    return readFeatVec();
}

template <>
ModelTagEntry* BinaryModelIO::readEntry<ModelTagEntry>() {
    ModelTagEntry* entry = new ModelTagEntry(readKyteaString());
    entry->setNumTags(numTags_);
    for(int i = 0; i < numTags_; i++) {
        int mySize = readBinary<uint32_t>();
        entry->tags[i].resize(mySize);
        entry->tagInDicts[i].resize(mySize);
        for(int j = 0; j < mySize; j++) {
            entry->tags[i][j] = readKyteaString();
            entry->tagInDicts[i][j] = readBinary<unsigned char>();
        }
    }
    entry->inDict = readBinary<unsigned char>();
    for(int i = 0; i < numTags_; i++)
        entry->tagMods[i] = readModel();
    return entry;
}

template <>
ProbTagEntry* BinaryModelIO::readEntry<ProbTagEntry>() {
    ProbTagEntry* entry = new ProbTagEntry(readKyteaString());
    entry->setNumTags(numTags_);
    for(int i = 0; i < numTags_; i++) {
        unsigned mySize = readBinary<uint32_t>();
        entry->tags[i].resize(mySize);
        entry->probs[i].resize(mySize);
        for(unsigned j = 0; j < mySize; j++) {
            entry->tags[i][j] = readKyteaString();
            entry->probs[i][j] = readBinary<double>();
        }
    }
    return entry;
}

void TextModelIO::writeWordList(const std::vector<KyteaString> & list) {
    for(unsigned i = 0; i < list.size(); i++) {
        if(i != 0) *str_ << " ";
        *str_ << util_->showString(list[i]);
    }
    *str_ << endl;
}

void BinaryModelIO::writeWordList(const std::vector<KyteaString> & list) {
    writeBinary((uint32_t)list.size());
    for(unsigned i = 0; i < list.size(); i++)
        writeString(list[i]);
}

vector<KyteaString> TextModelIO::readWordList() {
    string line,s;
    getline(*str_,line); // ignore the header
    istringstream iss(line);
    vector<KyteaString> ret;
    while(iss >> s)
        ret.push_back(util_->mapString(s));
    return ret;
}

vector<KyteaString> BinaryModelIO::readWordList() {
    vector<KyteaString> list(readBinary<uint32_t>());
    for(unsigned i = 0; i < list.size(); i++)
        list[i] = readKyteaString();
    return list;
}

void TextModelIO::writeFeatureLookup(const FeatureLookup* featLookup) {
    if(!featLookup) {
        *str_ << endl;
        return;
    }
    *str_ << "lookup" << endl;
    writeVectorDictionary(featLookup->getCharDict());
    writeVectorDictionary(featLookup->getTypeDict());
    writeVectorDictionary(featLookup->getSelfDict());
    writeFeatVec(featLookup->getDictVector());
    writeFeatVec(featLookup->getBiases());
    writeFeatVec(featLookup->getTagDictVector());
    writeFeatVec(featLookup->getTagUnkVector());
}

FeatureLookup * TextModelIO::readFeatureLookup() {
    string line;
    getline(*str_, line);
    if(line == "")
        return 0;
    else if (line != "lookup")
        THROW_ERROR("Poorly formatted model: expecting 'lookup' but got "<<line);
    FeatureLookup * look = new FeatureLookup;
    look->setCharDict(readVectorDictionary());
    look->setTypeDict(readVectorDictionary());
    look->setSelfDict(readVectorDictionary());
    look->setDictVector(readFeatVec());
    look->setBiases(readFeatVec());
    look->setTagDictVector(readFeatVec());
    look->setTagUnkVector(readFeatVec());
    return look;
}

void BinaryModelIO::writeFeatureLookup(const FeatureLookup * featLookup) {
    if(featLookup) {
       writeBinary<char>(1);
       writeVectorDictionary(featLookup->getCharDict());
       writeVectorDictionary(featLookup->getTypeDict());
       writeVectorDictionary(featLookup->getSelfDict());
       writeFeatVec(featLookup->getDictVector());
       writeFeatVec(featLookup->getBiases());
       writeFeatVec(featLookup->getTagDictVector());
       writeFeatVec(featLookup->getTagUnkVector());
    } else {
        writeBinary<char>(0);
    }
}

FeatureLookup * BinaryModelIO::readFeatureLookup() {
    char active = readBinary<char>();
    FeatureLookup * look = 0;
    if(active) {
        look = new FeatureLookup;
        look->setCharDict(readVectorDictionary());
        look->setTypeDict(readVectorDictionary());
        look->setSelfDict(readVectorDictionary());
        look->setDictVector(readFeatVec());
        look->setBiases(readFeatVec());
        look->setTagDictVector(readFeatVec());
        look->setTagUnkVector(readFeatVec());
    }
    return look;
}

}
