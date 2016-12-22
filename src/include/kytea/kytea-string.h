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

// Turn this on to make KyTea do boundary checking (useful for development)
// #define KYTEA_SAFE

#ifndef KYTEA_STRING_H__
#define KYTEA_STRING_H__

#include <vector>
#include <cstddef>
#include <algorithm>
// #include <stdexcept>
// #include <cstring>
// #include <sstream>
// #include <iostream>

namespace kytea {

typedef unsigned short KyteaChar;

class StringUtil;

// an implementation of a string, kept in memory
class KyteaStringImpl {

public:
    unsigned length_;
    unsigned count_;
    KyteaChar* chars_;

    KyteaStringImpl() : length_(0), count_(1), chars_(0) { }
    KyteaStringImpl(unsigned length);
    KyteaStringImpl(const KyteaStringImpl & impl);
    ~KyteaStringImpl() {
        if(chars_)
            delete [] chars_;
    }

    unsigned dec() { return --count_; }
    unsigned inc() { return ++count_; }

};

class KyteaString {

public:
    friend class StringUtil;
    
private:
    KyteaStringImpl* impl_;

public:

    typedef std::vector<KyteaString> Tokens;

    // ctor
    KyteaString() : impl_(0) { }
    KyteaString(const KyteaString & str) { 
        impl_ = str.impl_;
        if(impl_) impl_->inc();
    }
    KyteaString(unsigned length) { impl_ = new KyteaStringImpl(length); }
    
    // dtor
    ~KyteaString() {
        if(impl_ && !impl_->dec())
            delete impl_;
    }

    // tokenize the string using the characters in the delimiter string
    Tokens tokenize(const KyteaString & delim, bool includeDelim = false) const;

    // splice a string into the appropriate location
    void splice(const KyteaString& str, unsigned pos);

    bool contains(const KyteaChar c) const {
        if(impl_)
            for(unsigned i = 0; i < impl_->length_; i++)
                if(impl_->chars_[i] == c)
                    return true;
        return false;
    }

    // Get the substring of a KyteaString
    KyteaString substr(unsigned s) const;
    KyteaString substr(unsigned s, unsigned l) const;
    
    inline KyteaChar & operator[](int i) {
#ifdef KYTEA_SAFE
        if(impl_ == 0 || i < 0 || (unsigned)i >= impl_->length_)
            throw std::runtime_error("string index out of bounds");
#endif
        return getImpl()->chars_[i];
    }
    
    inline const KyteaChar & operator[](int i) const {
#ifdef KYTEA_SAFE
        if(impl_ == 0 || i < 0 || (unsigned)i >= impl_->length_)
            throw std::runtime_error("string index out of bounds");
#endif
        return impl_->chars_[i];
    }
    
    KyteaString & operator= (const KyteaString &str) {
        if(impl_ && !impl_->dec())
            delete impl_;
        impl_ = str.impl_;
        if(impl_) impl_->inc();
        return *this;
    }


    inline unsigned length() const {
        return (impl_?impl_->length_:0); 
    }

    // Get the hash for this value
    size_t getHash() const;

    // Get the implementation
    inline const KyteaStringImpl * getImpl() const {
        return impl_;
    } 
    KyteaStringImpl * getImpl();

    // Find if it begins with a particular string
    bool beginsWith(const KyteaString & s) const;

};

inline KyteaString operator+(const KyteaString& a, const KyteaChar& b) {
    const KyteaStringImpl * aimp = a.getImpl();
    if(aimp == 0) {
        KyteaString ret(1);
        ret[0] = b;
        return ret;
    }
    KyteaString ret(aimp->length_+1);
    ret.splice(a,0);
    ret[aimp->length_]=b;
    return ret;
}

inline KyteaString operator+(const KyteaString& a, const KyteaString& b) {
    const KyteaStringImpl * aimp = a.getImpl();
    if(aimp == 0)
        return b;
    const KyteaStringImpl * bimp = b.getImpl();
    if(bimp == 0)
        return a;
    KyteaString ret(aimp->length_+bimp->length_);
    ret.splice(a,0);
    ret.splice(b,aimp->length_);
    return ret;
}

inline bool operator<(const KyteaString & a, const KyteaString & b) {
    unsigned i;
    const unsigned al = a.length(), bl = b.length(), ml=std::min(al,bl);
    for(i = 0; i < ml; i++) {
        if(a[i] < b[i]) return true;
        else if(b[i] < a[i]) return false;
    }
    return (bl != i);
}

inline bool operator==(const KyteaString & a, const KyteaString & b) {
    unsigned i;
    const unsigned al = a.length();
    if(al!=b.length())
        return false;
    for(i = 0; i < al; i++)
        if(a[i] != b[i]) return false;
    return true;
}

inline bool operator!=(const KyteaString & a, const KyteaString & b) {
    return !(a==b);
}


// hashing using the djb2 algorithm
//  found at
//  http://www.cse.yorku.ca/~oz/hash.html
class KyteaStringHash {
public:
    size_t operator()(const KyteaString & x) const {
        return x.getHash();
    }
};

}

#endif
