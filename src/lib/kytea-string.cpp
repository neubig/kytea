#include <kytea/kytea-string.h>
#include <cstring>

using namespace kytea;
using namespace std;

// Constructors for KyteaStringImpl
KyteaStringImpl::KyteaStringImpl(unsigned length) : length_(length), count_(1) {
    chars_ = new KyteaChar[length];
}
KyteaStringImpl::KyteaStringImpl(const KyteaStringImpl & impl) : length_(impl.length_), count_(1) {
    chars_ = new KyteaChar[length_];
    memcpy(chars_, impl.chars_, sizeof(KyteaChar)*length_);
}

// tokenize the string using the characters in the delimiter string
KyteaString::Tokens KyteaString::tokenize(const KyteaString & delim, bool includeDelim) const {
    unsigned i,j,s=0;
    const unsigned l=length(),dl=delim.length();
    vector<KyteaString> ret;
    for(i = 0; i < l; i++) {
        for(j = 0; j < dl && delim[j] != impl_->chars_[i]; j++);
        if(j != dl) {
            if(s != i)
                ret.push_back(substr(s,i-s));
            if(includeDelim)
                ret.push_back(substr(i,1));
            s = i+1;
        }
    }
    if(s != i)
        ret.push_back(substr(s,i-s));
    return ret;
}

// splice a string into the appropriate location
void KyteaString::splice(const KyteaString& str, unsigned pos) {
    const unsigned l = str.length();
    if(!l) 
        return;
#ifdef KYTEA_SAFE
    if(pos+l > length())
        throw runtime_error("KyteaString splice index out of bounds");
#endif
    memcpy(impl_->chars_+pos, str.getImpl()->chars_, sizeof(KyteaChar)*l);
}

KyteaString KyteaString::substr(unsigned s) const {
    const unsigned l = length()-s;
#ifdef KYTEA_SAFE
    if(s+l > length())
        throw runtime_error("KyteaString substr index out of bounds");
#endif
    KyteaString ret(l);
    memcpy(ret.getImpl()->chars_, impl_->chars_+s, sizeof(KyteaChar)*l);
    return ret;
}


KyteaString KyteaString::substr(unsigned s, unsigned l) const {
#ifdef KYTEA_SAFE
    if(s+l > length())
        throw runtime_error("substr out of bounds");
#endif
    KyteaString ret(l);
    memcpy(ret.getImpl()->chars_, impl_->chars_+s, sizeof(KyteaChar)*l);
    return ret;
}

size_t KyteaString::getHash() const {
    size_t hash = 5381;
    if(impl_==0)
        return hash;
    const unsigned l = impl_->length_;
    const KyteaChar* cs = impl_->chars_;
    for(unsigned i = 0; i < l; i++)
        hash = ((hash << 5) + hash) + cs[i]; /* hash * 33 + x[i] */
    return hash;
}

KyteaStringImpl * KyteaString::getImpl() {
    if(impl_->count_ != 1) {
        impl_->dec();
        impl_ = new KyteaStringImpl(*impl_);
    }
    return impl_;
}
bool KyteaString::beginsWith(const KyteaString & s) const {
    if(s.length() > this->length()) return 0;
    for(int i = s.length()-1; i >= 0; i--) {
        if((*this)[i] != s[i])
            return 0;
    }
    return 1;
}
