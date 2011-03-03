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

#include <kytea/model-io.h>
#include <algorithm>
#include <set>

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
    if(output && form == ModelIO::FORMAT_UNKNOWN)
        throw runtime_error("A format must be specified for model output");
    else if(!output) {
        ifstream ifs(file);
        if(!ifs.good()) 
            throw runtime_error("Could not open model file");
        string line,buff1,buff2,buff3,buff4;
        getline(ifs, line);
        istringstream iss(line);
        if(!(iss >> buff1) || !(iss >> buff2) || !(iss >> buff3) || !(iss >> buff4) || 
                                  buff1 != "KyTea" || buff3.length() != 1)
            throw runtime_error("Badly formed model (header incorrect)");
        if(buff2 != MODEL_IO_VERSION) {
            ostringstream buff;
            buff << "Incompatible model version. Expected " << MODEL_IO_VERSION << ", but found " << buff2 << ".";
            throw runtime_error(buff.str());
        }
        form = buff3[0];
        config.setEncoding(buff4.c_str());
        ifs.close();
    }
    StringUtil * util = config.getStringUtil();
    if(form == ModelIO::FORMAT_TEXT)      { return new TextModelIO(util,file,output); }
    else if(form == ModelIO::FORMAT_BINARY) { return new BinaryModelIO(util,file,output); }
    else {
        throw runtime_error("Illegal model format");
    }
}

ModelIO * ModelIO::createIO(iostream & file, Format form, bool output, KyteaConfig & config) {
    StringUtil * util = config.getStringUtil();
    if(form == ModelIO::FORMAT_TEXT)      { return new TextModelIO(util,file,output); }
    else if(form == ModelIO::FORMAT_BINARY) { return new BinaryModelIO(util,file,output); }
    else {
        throw runtime_error("Illegal model format");
    }
}

void TextModelIO::writeConfig(const KyteaConfig & config) {

    *str_ << "KyTea " << MODEL_IO_VERSION << " T " << config.getEncodingString() << endl;

    if(!config.getDoWS()) *str_ << "-nows" << endl;
    if(!config.getDoPE()) *str_ << "-nope" << endl;
    if(config.getBias()<0) *str_ << "-nobias" << endl;
    *str_ << "-charw " << (int)config.getCharWindow() << endl;
    *str_ << "-charn " << (int)config.getCharN() << endl;
    *str_ << "-typew " << (int)config.getTypeWindow() << endl;
    *str_ << "-typen " << (int)config.getTypeN() << endl;
    *str_ << "-dicn "  << (int)config.getDictionaryN() << endl;
    *str_ << "-eps " << config.getEpsilon() << endl;
    *str_ << "-solver " << config.getSolverType() << endl;

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
}

void TextModelIO::writeModel(const KyteaModel * mod) {

    // print a single endl for empty models
    if(mod == 0) {
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
    const KyteaModel::FeatVec & names = mod->getNames();
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
				throw runtime_error("unknown solver type.");
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
            ostringstream err;
            err << "Unknown text in model file '" << str << "'";
			throw runtime_error(err.str());
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
            mod->setWeight(i,j,(KyteaModel::FeatVal)util_->parseFloat(buff.c_str()));
        }
	}
    mod->setNumFeatures(nr_feature);
    
    getline(*str_, str);
    if(str.length() != 0 && str != " ") {
        ostringstream err;
        err << "Bad line when expecting end of file: '" << str << "'" << endl;
        throw runtime_error(err.str());
    }

    // read models shouldn't add any additional features
    mod->setAddFeatures(false);

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
        throw runtime_error("Badly formatted first line in LM");
    }
    linestream1 >> str;
    KyteaLM* lm = new KyteaLM(util_->parseInt(str.c_str()));

    // get and check the second line
    getline(*str_, line);
    istringstream linestream2(line);
    linestream2 >> str;
    if(str != "lmvocab") throw runtime_error("Badly formatted second line in LM");
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

template <>
void TextModelIO::writeEntry(const ModelPronEntry * entry) {
    *str_ << util_->showString(entry->word) << endl;
    for(unsigned j = 0; j < entry->prons.size(); j++) {
        if(j!=0) *str_ << " ";
        *str_ << util_->showString(entry->prons[j]);
    }
    *str_ << endl;
    bool has = false;
    for(unsigned j = 0; j < 8; j++) {
        if(entry->isInDict(j)) {
            if(has) *str_ << " ";
            *str_ << (unsigned)j;
            has = true;
        }
    }
    *str_ << endl;
    writeModel(entry->pronMod);
}

template <>
void BinaryModelIO::writeEntry(const ModelPronEntry * entry) {
    writeString(entry->word);
    writeBinary((uint32_t)entry->prons.size());
    for(unsigned j = 0; j < entry->prons.size(); j++)
        writeString(entry->prons[j]);
    writeBinary((unsigned char)entry->inDict);
    writeModel(entry->pronMod);
}

template <>
void TextModelIO::writeEntry(const ProbPronEntry * entry) {
    *str_ << util_->showString(entry->word) << endl;
    for(unsigned j = 0; j < entry->prons.size(); j++) {
        if(j!=0) *str_ << " ";
        *str_ << util_->showString(entry->prons[j]);
    }
    *str_ << endl;
    for(unsigned j = 0; j < entry->probs.size(); j++) {
        if(j!=0) *str_ << " ";
        *str_ << entry->probs[j];
    }
    *str_ << endl;
}

template <>
void BinaryModelIO::writeEntry(const ProbPronEntry * entry) {
    writeString(entry->word);
    writeBinary((uint32_t)entry->prons.size());
    for(unsigned j = 0; j < entry->prons.size(); j++) {
        writeString(entry->prons[j]);
        writeBinary((double)entry->probs[j]);
    }
}


template <>
ModelPronEntry* TextModelIO::readEntry<ModelPronEntry>() {
    string line, buff;
    getline(*str_, line);
    ModelPronEntry* entry = new ModelPronEntry(util_->mapString(line));
    getline(*str_, line);
    istringstream iss(line);
    while(iss >> buff)
        entry->prons.push_back(util_->mapString(buff.c_str()));
    getline(*str_, line);
    istringstream iss2(line);
    while(iss2 >> buff) {
        entry->setInDict(util_->parseInt(buff.c_str()));
    }
    entry->pronMod = readModel();
    return entry;
}

template <>
ProbPronEntry* TextModelIO::readEntry<ProbPronEntry>() {
    string line, buff;
    getline(*str_, line);
    ProbPronEntry* entry = new ProbPronEntry(util_->mapString(line));
    getline(*str_, line);
    istringstream iss(line);
    while(iss >> buff)
        entry->prons.push_back(util_->mapString(buff.c_str()));
    getline(*str_, line);
    istringstream iss2(line);
    while(iss2 >> buff)
        entry->probs.push_back(util_->parseFloat(buff.c_str()));
    if(entry->probs.size() != entry->prons.size())
        throw runtime_error("Non-matching probability and pronunciation values");
    return entry;
}


void BinaryModelIO::writeConfig(const KyteaConfig & config) {
    *str_ << "KyTea " << MODEL_IO_VERSION << " B " << config.getEncodingString() << endl;

    writeBinary(config.getDoWS());
    writeBinary(config.getDoPE());
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
    config.setDoPE(readBinary<bool>() && config.getDoPE());
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
    
    // print the number of features+1, zero for empty models
    if(mod == 0) { 
        writeBinary((uint32_t)0);
        return;
    }

    // print the feature names
    const KyteaModel::FeatVec & names = mod->getNames();
    writeBinary((uint32_t)names.size());
    for(unsigned i = 1; i < names.size(); i++)
        writeString(names[i]);

    int i;
    int nr_feature=mod->getNumFeatures();
    int n;

    if(mod->getBias()>0)
    	n=nr_feature+1;
    else
    	n=nr_feature;
    int w_size = n;

    int nr_w = mod->getNumWeights();

    writeBinary((char)mod->getSolver());
    writeBinary((int32_t)mod->getNumClasses());
    for(i=0; i<(int)mod->getNumClasses(); i++)
        writeBinary((int32_t)mod->getLabel(i));

    writeBinary((int32_t)nr_feature);

    writeBinary(mod->getBias()>=0);
    writeBinary(mod->getMultiplier());

    for(i=0; i<w_size; i++)
    {
    	int j;
    	for(j=0; j<nr_w; j++) {
            writeBinary(mod->getWeight(i,j));
        }
    }

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

	unsigned i = readBinary<uint32_t>();
    if(i == 0)
        return 0;

	int nr_feature;
	int n;
	int nr_class;
    KyteaModel * mod = new KyteaModel();

    for( ; i > 1; i--) {
        KyteaString myStr = readKyteaString();
        mod->mapFeat(myStr);
    }
    
    mod->setSolver(readBinary<char>());
    nr_class = readBinary<int32_t>();
    mod->setNumClasses(nr_class);
	for(i=0;(int)i<nr_class;i++)
        mod->setLabel(i, readBinary<int32_t>());

    nr_feature = readBinary<int32_t>();

    mod->setBias(readBinary<bool>()?1.0:-1.0);
    mod->setMultiplier(readBinary<double>());

	mod->setNumFeatures(nr_feature);
	if(mod->getBias()>=0)
		n=nr_feature+1;
	else
		n=nr_feature;
	int w_size = n;
	int nr_w = mod->getNumWeights();

	mod->initializeWeights(w_size,nr_w);
	for(i=0; (int)i<w_size; i++) 
		for(int j=0; j<nr_w; j++) 
            mod->setWeight(i,j,readBinary<KyteaModel::FeatVal>()); 
    
    // read models shouldn't add any additional features
    mod->setAddFeatures(false);

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
ModelPronEntry* BinaryModelIO::readEntry<ModelPronEntry>() {
    ModelPronEntry* entry = new ModelPronEntry(readKyteaString());
    entry->prons.resize(readBinary<uint32_t>());
    for(unsigned j = 0; j < entry->prons.size(); j++)
        entry->prons[j] = readKyteaString();
    entry->inDict = readBinary<unsigned char>();
    entry->pronMod = readModel();
    return entry;
}

template <>
ProbPronEntry* BinaryModelIO::readEntry<ProbPronEntry>() {
    ProbPronEntry* entry = new ProbPronEntry(readKyteaString());
    unsigned mySize = readBinary<uint32_t>();
    entry->prons.resize(mySize);
    entry->probs.resize(mySize);
    for(unsigned j = 0; j < entry->prons.size(); j++) {
        entry->prons[j] = readKyteaString();
        entry->probs[j] = readBinary<double>();
    }
    return entry;
}


}
