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

#ifndef GENERAL_IO_H__ 
#define GENERAL_IO_H__ 

// #include <kytea/kytea-string.h>
// #include <kytea/string-util.h>
// #include <kytea/config.h>
#include <iostream>
#include <cstddef>
// #include <fstream>
// #include <sstream>
// #include <stdint.h>

#if DISABLE_QUANTIZE
#   define DECIMAL_PRECISION 8
#else
#   define DECIMAL_PRECISION 6
#endif

namespace kytea {

// Forward declarations
class StringUtil;
class KyteaString;

class GeneralIO {

protected:
    
    StringUtil* util_;
    std::iostream* str_;
    bool out_;
    bool bin_;
    bool owns_;
 
    // write
    template <class T>
    void writeBinary(T v) {
        str_->write(reinterpret_cast<char *>(&v), sizeof(T));
    } 
    void writeString(const char* str, size_t size) {
        str_->write(str, size+1);
    } 
    void writeString(const std::string & str) {
        str_->write(str.c_str(), str.length()+1);
    }
    void writeString(const KyteaString & str);
    
    // read
    template <class T>
    T readBinary();

    std::string readString();
    KyteaString readKyteaString();

public:

    GeneralIO(StringUtil* util) : 
        util_(util), str_(0), out_(true), 
        bin_(false), owns_(false) { }

    GeneralIO(StringUtil* util, std::iostream & str, bool out, bool bin) : 
        util_(util), str_(&str), out_(out), bin_(false), owns_(false) 
            { setStream(str, out, bin); }

    GeneralIO(StringUtil* util, const char* file, bool out, bool bin) : 
        util_(util), str_(0), bin_(false), owns_(true) 
            { openFile(file,out,bin); }
    
    ~GeneralIO() {
        if(str_ && owns_)
            delete str_;
    }

    void openFile(const char* file, bool out, bool bin);
    void setStream(std::iostream & str, bool out, bool bin);

};

}

#endif
