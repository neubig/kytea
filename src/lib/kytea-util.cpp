#include <kytea/kytea-util.h>
#include <kytea/dictionary.h>
#include <kytea/string-util.h>
#include <kytea/feature-lookup.h>
#include <kytea/kytea-model.h>
#include <kytea/kytea-lm.h>

using namespace std;
using namespace kytea;

namespace kytea {

template <class T>
void checkPointerEqual(const T* lhs, const T* rhs) {
    if(lhs == NULL) {
        if(rhs != NULL)
            THROW_ERROR("lhs == NULL, rhs != NULL");
    } else {
        if(rhs == NULL)
            THROW_ERROR("lhs != NULL, rhs == NULL");
        lhs->checkEqual(*rhs);
    }
}

template void checkPointerEqual(const StringUtil* lhs, const StringUtil* rhs);
template void checkPointerEqual(const KyteaModel* lhs, const KyteaModel* rhs);
template void checkPointerEqual(const FeatureLookup* lhs, const FeatureLookup* rhs);
template void checkPointerEqual(const Dictionary<ModelTagEntry>* lhs, const Dictionary<ModelTagEntry>* rhs);
template void checkPointerEqual(const Dictionary<ProbTagEntry>* lhs, const Dictionary<ProbTagEntry>* rhs);
template void checkPointerEqual(const Dictionary<FeatVec>* lhs, const Dictionary<FeatVec>* rhs);

// Vector equality checking function
template <class T>
void checkValueVecEqual(const std::vector<T> & a, const std::vector<T> & b) {
    if(a.size() != b.size()) THROW_ERROR("Vector sizes don't match: "<<a.size()<<" != "<<b.size());
    for(int i = 0; i < (int)a.size(); i++)
        if(a[i] != b[i]) THROW_ERROR("Vectors don't match at "<<i);
}

// Vector equality checking with null pointers
template <class T>
void checkValueVecEqual(const std::vector<T> * a, const std::vector<T> * b) {
    if((a == NULL || a->size() == 0) != (b == NULL || b->size() == 0)) {
        THROW_ERROR("only one dictVector_ is NULL");
    } else if(a != NULL) {
        checkValueVecEqual(*a, *b);
    }
}

template void checkValueVecEqual(const std::vector<unsigned int> * a, const std::vector<unsigned int> * b);
template void checkValueVecEqual(const std::vector<short> * a, const std::vector<short> * b);
template void checkValueVecEqual(const std::vector<vector<KyteaString> > * a, const std::vector<vector<KyteaString> > * b);
template void checkValueVecEqual(const std::vector<int> * a, const std::vector<int> * b);
template void checkValueVecEqual(const std::vector<KyteaString> * a, const std::vector<KyteaString> * b);

template void checkValueVecEqual(const std::vector<unsigned int> & a, const std::vector<unsigned int> & b);
template void checkValueVecEqual(const std::vector<short> & a, const std::vector<short> & b);
template void checkValueVecEqual(const std::vector<vector<KyteaString> > & a, const std::vector<vector<KyteaString> > & b);
template void checkValueVecEqual(const std::vector<int> & a, const std::vector<int> & b);
template void checkValueVecEqual(const std::vector<KyteaString> & a, const std::vector<KyteaString> & b);

// Vector equality checking function
template <class T>
void checkPointerVecEqual(const std::vector<T*> & a, const std::vector<T*> & b) {
    if(a.size() > b.size()) {
        for(int i = b.size(); i < (int)a.size(); i++)
            if(a[i] != 0)
                THROW_ERROR("Vector sizes don't match: "<<a.size()<<" != "<<b.size());
    } else if(b.size() > a.size()) {
        for(int i = a.size(); i < (int)b.size(); i++)
            if(b[i] != 0)
                THROW_ERROR("Vector sizes don't match: "<<a.size()<<" != "<<b.size());
    } else {
        for(int i = 0; i < (int)a.size(); i++)
            checkPointerEqual(a[i], b[i]);
    }
}

template void checkPointerVecEqual(const std::vector<KyteaModel*> & a, const std::vector<KyteaModel*> & b);
template void checkPointerVecEqual(const std::vector<KyteaLM*> & a, const std::vector<KyteaLM*> & b);

// Vector equality checking with null pointers
template <class T>
void checkPointerVecEqual(const std::vector<T*> * a, const std::vector<T*> * b) {
    if((a == NULL || a->size() == 0) != (b == NULL || b->size() == 0)) {
        THROW_ERROR("only one dictVector_ is NULL");
    } else if(a != NULL) {
        checkPointerVecEqual(*a, *b);
    }
}

}
