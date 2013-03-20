#ifndef FEATURE_VECTOR_H__
#define FEATURE_VECTOR_H__

#include <kytea/config.h>
#include <stdint.h>

namespace kytea {
// Define the size of the feature values and sums
#if DISABLE_QUANTIZE
    typedef double FeatVal;
    typedef double FeatSum;
#else
    typedef int16_t FeatVal;
    typedef int32_t FeatSum;
#endif
typedef std::vector<FeatVal> FeatVec;
}

#endif
