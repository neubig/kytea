#include <kytea/kytea-model.h>
#include "liblinear/linear.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace kytea;
using namespace std;

#define SIG_CUTOFF 1E-6
#define SHORT_MAX 32767


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
    // cerr << "labels.size="<<labels_.size()<<", "<<numW_<<endl;
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
            for(i = 0; i < featSize; i++)
                dec += getWeight(feat[i]-1,j);
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
    solver_ = solver;
    if(weights_.size()>0)
        weights_.clear();
    setBias(bias);
    // build the liblinear model
    struct problem   prob;
    struct parameter param;
    prob.l = xs.size();
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
    FeatVec oldNames = names_;
    names_.clear();
    ids_.clear();
    KyteaString empty;
    mapFeat(empty);
    weights_.clear();
    for(i=0; i<(int)oldNames.size()-1; i++) {
        double myMax = 0.0;
    	for(j=0; j<numW_; j++) 
            myMax = max(abs(mod_->w[i*numW_+j]),myMax);
        if(myMax>SIG_CUTOFF) {
            mapFeat(oldNames[i+1]);
            for(j = 0; j < numW_; j++)
                weights_.push_back((FeatVal)(mod_->w[i*numW_+j]/multiplier_));
        }
    }
    if(bias_>=0) {
        for(j = 0; j < numW_; j++) {
            weights_.push_back((FeatVal)(mod_->w[i*numW_+j]/multiplier_));
        }
    }

    free_and_destroy_model(&mod_);

}

void KyteaModel::setNumClasses(unsigned v) {
    if(v == 1) 
        THROW_ERROR("Trying to set the number of classes to 1");
    labels_.resize(v);
    numW_ = (v==2 && solver_ != MCSVM_CS?1:v);
}
