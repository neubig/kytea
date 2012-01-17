#ifndef KYTEA_UTIL__
#define KYTEA_UTIL__

#include <iostream>
#include <sstream>
#include <vector>

namespace kytea {

#define THROW_ERROR(msg) do {                   \
    std::ostringstream oss;                     \
    oss << msg;                                 \
    throw std::runtime_error(oss.str()); }       \
  while (0);

template <class T>
inline void checkPointerEqual(const T* lhs, const T* rhs) {
    if(lhs == NULL) {
        if(rhs != NULL)
            THROW_ERROR("lhs == NULL, rhs != NULL");
    } else {
        if(rhs == NULL)
            THROW_ERROR("lhs != NULL, rhs == NULL");
        lhs->checkEqual(*rhs);
    }
}

// Vector equality checking function
template <class T>
inline void checkValueVecEqual(const std::vector<T> & a, const std::vector<T> & b) {
    if(a.size() != b.size()) THROW_ERROR("Vector sizes don't match: "<<a.size()<<" != "<<b.size());
    for(int i = 0; i < (int)a.size(); i++)
        if(a[i] != b[i]) THROW_ERROR("Vectors don't match at "<<i);
}

// Vector equality checking with null pointers
template <class T>
inline void checkValueVecEqual(const std::vector<T> * a, const std::vector<T> * b) {
    if((a == NULL || a->size() == 0) != (b == NULL || b->size() == 0)) {
        THROW_ERROR("only one dictVector_ is NULL");
    } else if(a != NULL) {
        checkValueVecEqual(*a, *b);
    }
}

// Vector equality checking function
template <class T>
inline void checkPointerVecEqual(const std::vector<T*> & a, const std::vector<T*> & b) {
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

// Vector equality checking with null pointers
template <class T>
inline void checkPointerVecEqual(const std::vector<T*> * a, const std::vector<T*> * b) {
    if((a == NULL || a->size() == 0) != (b == NULL || b->size() == 0)) {
        THROW_ERROR("only one dictVector_ is NULL");
    } else if(a != NULL) {
        checkPointerVecEqual(*a, *b);
    }
}

};

#endif // KYTEA_UTIL__
