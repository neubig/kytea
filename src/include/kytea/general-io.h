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

#include "kytea-string.h"
#include "string-util.h"
#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdint.h>

#if DISABLE_QUANTIZE
#   define DECIMAL_PRECISION 8
#else
#   define DECIMAL_PRECISION 6
#endif


// do sanity checks on the IO files
#define KYTEA_IO_SAFE

namespace kytea {

class GeneralIO {

protected:
    
    StringUtil* util_;
    std::iostream* str_;
    bool out_;
    bool bin_;
    bool owns_;
 
    // write
    template <class T>
    inline void writeBinary(T v) {
        str_->write(reinterpret_cast<char *>(&v), sizeof(T));
    } 
    inline void writeString(const char* str, size_t size) {
        str_->write(str, size+1);
    } 
    inline void writeString(const std::string & str) {
        str_->write(str.c_str(), str.length()+1);
    }
    inline void writeString(const KyteaString & str) {
        writeBinary((uint32_t)str.length());
        for(unsigned i = 0; i < str.length(); i++)
            writeBinary(str[i]);
    }
    
    // read
    template <class T>
    inline T readBinary() {
        T v;
        str_->read(reinterpret_cast<char *>(&v),sizeof(T));
        return v;
    } 
    inline std::string readString() {
        std::string str;
        getline(*str_, str, (char)0);
        return str;
    }
    inline KyteaString readKyteaString() {
        KyteaString ret(readBinary<uint32_t>());
        for(unsigned i = 0; i < ret.length(); i++)
            ret[i] = readBinary<KyteaChar>();
        return ret;
    }
    

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

    void openFile(const char* file, bool out, bool bin) {
        std::fstream::openmode mode = (out?std::fstream::out:std::fstream::in);
        if(bin) out = out | std::fstream::binary;
        std::fstream * str = new std::fstream(file, mode);
        if(str->fail()) {
            delete str;
            std::ostringstream buff;
            buff << "Couldn't open file '"<<file<<"' for "<<(out?"output":"input");
            throw std::runtime_error(buff.str());
        }
        setStream(*str, out, bin);
        owns_ = true;
    }

    void setStream(std::iostream & str, bool out, bool bin) {
        if(str_ && owns_)
            delete str_;
        str_ = &str;
        str_->precision(DECIMAL_PRECISION);
        bin_ = bin;
        out_ = out;
        owns_ = false;
    }

};

}

#endif
