#include <kytea/kytea-util.h>
#include <kytea/kytea-model.h>
#include <kytea/feature-lookup.h>
#include <kytea/string-util.h>
#include <kytea/dictionary.h>
#include "liblinear/linear.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace kytea;
using namespace std;

#define SIG_CUTOFF 1E-6
#define SHORT_MAX 32767

int KyteaModel::featuresAdded_ = 0;

template <class A, class B>
class secondmore {
public:
    bool operator() (const pair<A,B> & a, const pair<A,B> & b) {
        return b.second < a.second;
    }
};

// note: this is not safe, all features must be within the appropriate range
vector< pair<int,double> > KyteaModel::runClassifier(const vector<unsigned> & feat) {
    int i, j, featSize = feat.size();
    FeatSum dec;
    vector< pair<int,double> > ret(labels_.size());
    // for binary predictors
    if(numW_ == 1) {
        dec = (bias_>=0?getWeight(getBiasId()-1,0):0);
        for(i = 0; i < featSize; i++)
            dec += getWeight(feat[i]-1,0);
        // linear regression is probabilities
        double big = abs(dec)*multiplier_, small = 0;
        if(isProbabilistic(solver_)) {
            big = 1/(1+exp(-1*big));
            small = 1-big;
        }
        if(dec > 0) {
            ret[0] = pair<int,double>(labels_[0],big); 
            ret[1] = pair<int,double>(labels_[1],small); 
        } else {
            ret[0] = pair<int,double>(labels_[1],big); 
            ret[1] = pair<int,double>(labels_[0],small); 
        }
    }
    // for non-binary predictors 
    else {
        double sum = 0, max1 = -100000, max2 = -100000, weight;
        for(j = 0; j < numW_; j++) {
            dec = (bias_>=0?getWeight(getBiasId()-1,j):0);
            for(i = 0; i < featSize; i++) {
                dec += getWeight(feat[i]-1,j);
            }
            weight = dec*multiplier_;
            // get probability for LR
            if(isProbabilistic(solver_)) {
                weight = 1/(1+exp(-1*weight));
                sum += weight;
            }
            // save the top two values for SVM
            else if(weight > max1) {
                max2 = max1; max1 = weight;
            }
            else if(weight > max2)
                max2 = weight;
            ret[j] = pair<int,double>(labels_[j], weight);
        }
        if(isProbabilistic(solver_))
            for(j = 0; j < numW_; j++)
                ret[j].second /= sum;
        else 
            for(j = 0; j < numW_; j++)
                ret[j].second -= max2;
        sort(ret.begin(),ret.end(),secondmore<int,double>());
    }
    return ret;
}


// note: this is not safe, all features must be within the appropriate range
void KyteaModel::printClassifier(const vector<unsigned> & feat, StringUtil * util, ostream & out) {
    int i, j, featSize = feat.size();
    FeatSum weight, tot;
    vector< pair<string,double> > idxs;
    vector<double> sums(numW_,0);
    // for binary predictors
    if(numW_ == 1) {
        if(bias_>=0) {
            sums[0] = getWeight(getBiasId()-1,0);
            ostringstream buff; buff << "BIAS=" << sums[0];
            idxs.push_back(pair<string,double>(buff.str(),abs(sums[0])));
        }
        for(i = 0; i < featSize; i++) {
            weight = getWeight(feat[i]-1,0);
            ostringstream buff; buff << util->showString(showFeat(feat[i])) << "=" << weight;
            idxs.push_back(pair<string,double>(buff.str(),abs(weight)));
            sums[0] += weight;
        }
    }
    // for non-binary predictors 
    else {
        if(bias_>=0) {
            tot = 0;
            ostringstream buff; buff << "BIAS=";
            for(j = 0; j < numW_; j++) {
                weight = getWeight(getBiasId()-1,j);
                sums[j] += weight;
                tot += abs(weight);
                if(j != 0) buff << "/";
                buff << weight;
            }
            idxs.push_back(pair<string,double>(buff.str(),tot));
        }
        for(i = 0; i < featSize; i++) {
            tot = 0;
            ostringstream buff; buff << util->showString(showFeat(feat[i])) << "=";
            for(j = 0; j < numW_; j++) {
                weight = getWeight(feat[i]-1,j);
                sums[j] += weight;
                tot += abs(weight);
                if(j != 0) buff << "/";
                buff << weight;
            }
            idxs.push_back(pair<string,double>(buff.str(),tot));
        }
    }
    sort(idxs.begin(),idxs.end(),secondmore<string,double>());
    for(i = 0; i < (int)idxs.size(); i++) {
        if(i != 0) out << " ";
        out << idxs[i].first;
    }
    out << " --- TOTAL=";
    for(i = 0; i < numW_; i++) {
        if(i != 0) out << "/";
        out << sums[i];
    }
    
    
}

// allocate a LL feature node
feature_node * allocateFeatures(const vector<unsigned> & feats, int biasId, double biasVal) {
    feature_node * nodes = (feature_node*)malloc((feats.size()+(biasVal>=0?2:1))*sizeof(feature_node));
    unsigned i;
    for(i = 0; i < feats.size(); i++) {
        nodes[i].index=feats[i];
        nodes[i].value = 1;
    }
    if(biasVal >= 0) {
        nodes[i].index = biasId;
        nodes[i++].value = biasVal;
    }
    nodes[i].index = -1;
    return nodes;
}
// train the model
void KyteaModel::trainModel(const vector< vector<unsigned> > & xs, vector<int> & ys, double bias, int solver, double epsilon, double cost) {
    if(xs.size() == 0) return;
    solver_ = solver;
    if(weights_.size()>0)
        weights_.clear();
    setBias(bias);
    // build the liblinear model
    struct problem   prob;
    struct parameter param;
    prob.l = xs.size();
    // for(int i = 0; i < min(5,(int)xs.size()); i++) {
    //     cerr << "ys["<<i<<"] == "<<ys[i]<<":";
    //     for(int j = 0; j < (int)xs[i].size(); j++) {
    //         cerr << " "<<xs[i][j];
    //     }
    //     cerr << endl;
    // }
    prob.y = &ys.front();

    // allocate the feature space
    feature_node** myXs = (feature_node**)malloc(sizeof(feature_node*)*xs.size());
    int biasId = getBiasId();
    for(int i = 0; i < prob.l; i++)
        myXs[i] = allocateFeatures(xs[i], biasId, bias);
    prob.x = myXs;

    prob.bias = bias;
    prob.n = names_.size()+(bias>=0?1:0);

    param.solver_type = solver;
    param.C = cost;
    param.eps = epsilon;
    param.nr_weight = 0;
    param.weight_label = NULL;
    param.weight = NULL;
    if(param.eps == HUGE_VAL) {
    	if(param.solver_type == L2R_LR || param.solver_type == L2R_L2LOSS_SVC)
    		param.eps = 0.01;
    	else if(param.solver_type == L2R_L2LOSS_SVC_DUAL || param.solver_type == L2R_L1LOSS_SVC_DUAL || param.solver_type == MCSVM_CS || param.solver_type == L2R_LR_DUAL)
    		param.eps = 0.1;
    	else if(param.solver_type == L1R_L2LOSS_SVC || param.solver_type == L1R_LR)
    		param.eps = 0.01;
    }
    model* mod_ = train(&prob, &param);

    // free the problem
    for(int i = 0; i < prob.l; i++)
        free(myXs[i]);
    free(myXs);

    int i, j;

    // create the labels
    labels_.resize(mod_->nr_class);
    for(int i = 0; i < mod_->nr_class; i++)
        labels_[i] = mod_->label[i];

    numW_ = (labels_.size()==2 && solver_ != MCSVM_CS?1:labels_.size());
    
    // find the multiplier
#if DISABLE_QUANTIZE
    multiplier_ = 1;
#else
    const unsigned wSize = numW_*names_.size();
    multiplier_ = 0;
    double val;
    for(unsigned i = 0; i < wSize; i++) {
        val = abs(mod_->w[i]);
        if(val > multiplier_)
            multiplier_ = val;
    }
    multiplier_ /= SHORT_MAX;
#endif

    // trim values
    oldNames_ = names_;
    names_.clear();
    ids_.clear();
    KyteaString empty;
    mapFeat(empty);
    weights_.clear();
    for(i=0; i<(int)oldNames_.size()-1; i++) {
        double myMax = 0.0;
    	for(j=0; j<numW_; j++) 
            myMax = max(abs(mod_->w[i*numW_+j]),myMax);
        if(myMax>SIG_CUTOFF) {
            mapFeat(oldNames_[i+1]);
            // If the number of weights is two, push the difference
            if(numW_ == 2) {
                weights_.push_back((FeatVal)
                        ((mod_->w[i*numW_]-mod_->w[i*numW_+1])/multiplier_));
            // Otherwise, keep the number of weights as-is, and push all
            } else {
                for(j = 0; j < numW_; j++)
                    weights_.push_back((FeatVal)(mod_->w[i*numW_+j]/multiplier_));
            }
        }
    }
    if(bias_>=0) {
        // If the number of weights is two, push the difference
        if(numW_ == 2) {
            weights_.push_back((FeatVal)
                    ((mod_->w[i*numW_]-mod_->w[i*numW_+1])/multiplier_));
        // Otherwise push all
        } else {
            for(j = 0; j < numW_; j++)
                weights_.push_back((FeatVal)(mod_->w[i*numW_+j]/multiplier_));
        }
    }

    // If the number of weights was two, we've converted to one
    if(numW_ == 2) numW_ = 1;

    free_and_destroy_model(&mod_);
    // When we're done with training, no more adding features
    addFeat_ = false;

}

void KyteaModel::setNumClasses(unsigned v) {
    if(v == 1) 
        THROW_ERROR("Trying to set the number of classes to 1");
    labels_.resize(v);
    numW_ = (v==2 && solver_ != MCSVM_CS?1:v);
}

Dictionary<vector<FeatVal> > * KyteaModel::makeDictionaryFromPrefixes(const vector<KyteaString> & prefs, StringUtil* util, bool adjustPos) {
    typedef Dictionary<vector<FeatVal> >::WordMap WordMap;
    WordMap wm;
    int pos;
    for(int i = 0; i < (int)names_.size(); i++) {
        const KyteaString & str = names_[i];
        for(pos = 0; pos < (int)prefs.size() && !str.beginsWith(prefs[pos]); pos++);
        if(pos != (int)prefs.size()) {
            featuresAdded_++;
            KyteaString name = str.substr(prefs[pos].length());
            WordMap::iterator it = wm.find(name);
            if(it == wm.end()) {
                pair<WordMap::iterator, bool> p = wm.insert(WordMap::value_type(name,new vector<FeatVal>(prefs.size()*numW_)));
                it = p.first;
            }
            // If this is an n-gram dictionary, adjust the position according to
            // n-gram length, otherwise just use the location of th eprefix
            int id = (adjustPos ?
                (prefs.size()-pos-name.length())*numW_ :
                pos*numW_
            );
            for(int j = 0; j < numW_; j++) {
                // cerr << "adding for "<<util->showString(str)<<" @ "<<util->showString(name) << " ["<<id<<"]"<<"/"<<(*it->second).size()<<" == "<<getWeight(i,j)<<"/"<<weights_.size()<< " == " <<getWeight(i-1,j) * labels_[0]<<endl;
                (*it->second)[id+j] = getWeight(i-1,j) * labels_[0];
            }
        }
    }
    if(wm.size() > 0) {
        Dictionary<vector<FeatVal> > * ret = new Dictionary<vector<FeatVal> >(util);
        ret->buildIndex(wm);
        return ret;
    }
    return NULL;
}

void KyteaModel::buildFeatureLookup(StringUtil * util, int charw, int typew, int numDicts, int maxLen) {
    if(featLookup_) {
        delete featLookup_;
        featLookup_ = 0;
    }
    // Do not build the feature lookup if there are no features to use
    if(names_.size() == 0 || getNumClasses() < 2)
        return;
    featLookup_ = new FeatureLookup;
    featuresAdded_ = 0;
    // Make the character values
    vector<KyteaString> charPref, typePref, selfPref, dictPref;
    for(int i = 1-charw; i <= charw; i++) {
        ostringstream oss; oss << "X" << i;
        charPref.push_back(util->mapString(oss.str()));
    }
    featLookup_->setCharDict(makeDictionaryFromPrefixes(charPref, util, true));
    // Make the type values
    for(int i = 1-typew; i <= typew; i++) {
        ostringstream oss; oss << "T" << i;
        typePref.push_back(util->mapString(oss.str()));
    }
    featLookup_->setTypeDict(makeDictionaryFromPrefixes(typePref, util, true));
    // Make the self prefixes
    selfPref.push_back(util->mapString("SX"));
    selfPref.push_back(util->mapString("ST"));
    featLookup_->setSelfDict(makeDictionaryFromPrefixes(selfPref, util, false));
    // Get the bias feature
    int bias = getBiasId();
    if(bias != -1) {
        featuresAdded_++;
        for(int j = 0; j < numW_; j++)
            featLookup_->setBias(getWeight(bias-1, j) * labels_[0], j);
    }    
    bool prevAddFeat = addFeat_;
    addFeat_ = false;
    // Make the dictionary values
    if(numDicts*maxLen > 0) {
        vector<FeatVal> * dictFeats = new vector<FeatVal>(numDicts*maxLen*3,0);
        int id = 0;
        for(int i = 0; i < numDicts; i++) {
            for(int j = 1; j <= maxLen; j++) {
                ostringstream oss1; oss1 << "D" << i << "R" << j;
                unsigned id1 = mapFeat(util->mapString(oss1.str()));
                if(id1 != 0) {
                    (*dictFeats)[id] = getWeight(id1-1, 0) * labels_[0];
                    featuresAdded_++;
                }
                id++;
                ostringstream oss2; oss2 << "D" << i << "I" << j;
                unsigned id2 = mapFeat(util->mapString(oss2.str()));
                if(id2 != 0) {
                    featuresAdded_++;
                    (*dictFeats)[id] = getWeight(id2-1, 0) * labels_[0];
                }
                id++;
                ostringstream oss3; oss3 << "D" << i << "L" << j;
                unsigned id3 = mapFeat(util->mapString(oss3.str()));
                if(id3 != 0) {
                    featuresAdded_++;
                    (*dictFeats)[id] = getWeight(id3-1, 0) * labels_[0];
                }
                id++;
            }
        }
        featLookup_->setDictVector(dictFeats);
    }
    if(numDicts > 0) {
        // Make the tag dictionary values
        vector<FeatVal> * tagDictFeats = new vector<FeatVal>(numDicts*labels_.size()*labels_.size(),0);
        int id = 0;
        for(int i = 0; i <= numDicts; i++) {
            for(int j = 0; j < (int)labels_.size(); j++) {
                ostringstream oss1; oss1 << "D" << i << "T" << j;
                unsigned id1 = mapFeat(util->mapString(oss1.str()));
                if(id1 != 0) {
                    for(int k = 0; k < (int)labels_.size(); k++)
                        (*tagDictFeats)[id+k] = getWeight(id1-1, k) * labels_[0];
                    featuresAdded_++;
                }
                id += labels_.size();
            }
        }
        featLookup_->setTagDictVector(tagDictFeats);
    }
    // Make the unknown vector
    unsigned id1 = mapFeat(util->mapString("UNK"));
    if(id1 != 0) {
        vector<FeatVal> * tagUnkFeats = new vector<FeatVal>(labels_.size(),0);
        featuresAdded_++;
        for(int k = 0; k < (int)labels_.size(); k++)
            (*tagUnkFeats)[k] = getWeight(id1-1, k) * labels_[0];
        featLookup_->setTagUnkVector(tagUnkFeats);
    }
    addFeat_ = prevAddFeat;
    if(featuresAdded_ != (int)names_.size())
        THROW_ERROR("Did not add all the features to the feature lookup ("<<featuresAdded_<<" != "<<names_.size()<<")");
}


void KyteaModel::checkEqual(const KyteaModel & rhs) const {
    // If the features are already encoded in the feature lookup, ignore
    // the feature hash
    if(featLookup_ == NULL) {
        checkMapEqual(ids_, rhs.ids_);
        checkValueVecEqual(names_, rhs.names_);
        checkValueVecEqual(weights_, rhs.weights_);
    }
    // Ignore old names, they are not really important
    checkValueVecEqual(labels_, rhs.labels_);
    if(abs((double)(multiplier_ - rhs.multiplier_)/multiplier_) > 0.01) THROW_ERROR("multipliers don't match: "<<multiplier_ << " != " << rhs.multiplier_);
    if(bias_ != rhs.bias_) THROW_ERROR("biases don't match: "<<bias_ << " != " << rhs.bias_);
    if(solver_ != rhs.solver_) THROW_ERROR("solvers don't match: "<<solver_ << " != " << rhs.solver_);
    if(numW_ != rhs.numW_) THROW_ERROR("numWs don't match: "<<numW_ << " != " << rhs.numW_);
    if(addFeat_ != rhs.addFeat_) THROW_ERROR("addFeats don't match: "<<addFeat_ << " != " << rhs.addFeat_);
    checkPointerEqual(featLookup_, rhs.featLookup_);
}


KyteaModel::~KyteaModel() {
    if(featLookup_) delete featLookup_;
}


void KyteaModel::setNumFeatures(unsigned i) {
    if(i != getNumFeatures()) 
        THROW_ERROR("setting the number of features to a different value is not allowed ("<<i<<" != "<<getNumFeatures()<<")");
}
