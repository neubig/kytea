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

#ifndef STRING_UTIL_H__
#define STRING_UTIL_H__

#include <kytea/kytea-struct.h>
// #include <kytea/kytea-string.h>
// #include <iostream>
#include <sstream>
// #include <vector>
// #include <cstdlib>

namespace kytea {

// a class for turning std::strings into internal representation
class StringUtil {

public:

    // types of characters (set in the constructor)
    typedef char CharType;
    const static CharType KANJI    = 'K';
    const static CharType KATAKANA = 'T';
    const static CharType HIRAGANA = 'H';
    const static CharType ROMAJI   = 'R';
    const static CharType DIGIT    = 'D';
    const static CharType OTHER    = 'O';

    // types of encodings
    typedef char Encoding;
    const static Encoding ENCODING_UTF8    = 'W';
    const static Encoding ENCODING_EUC     = 'E';
    const static Encoding ENCODING_SJIS    = 'S';

    // A map that normalizes characters to a single representation
    GenericMap<KyteaChar,KyteaChar> * normMap_;

public:

    StringUtil() : normMap_(NULL) { }

    virtual ~StringUtil() {
        if(normMap_) delete normMap_;    
    }

    // Map a std::string to a character.
    // If `add` is true, this operation changes the private members. That means,
    // an external synchronization is required.
    virtual KyteaChar mapChar(const std::string & str, bool add = true) = 0;

    virtual std::string showChar(KyteaChar c) const = 0;

    std::string showString(const KyteaString & c) const {
        std::ostringstream buff;
        for(unsigned i = 0; i < c.length(); i++)
            buff << showChar(c[i]);
        return buff.str();
    }

    std::string showEscapedString(const KyteaString & c, const KyteaString & spec, const std::string & bs) const {
        std::ostringstream buff;
        for(unsigned i = 0; i < c.length(); i++) {
            if(spec.contains(c[i]))
                buff << bs;
            buff << showChar(c[i]);
        }
        return buff.str();
    }

    // Map an unparsed std::string to a KyteaString.
    // Note that this operation requires an external synchronization.
    virtual KyteaString mapString(const std::string & str) = 0;

    // get the type of a character
    virtual CharType findType(const std::string & str) = 0;
    virtual CharType findType(KyteaChar c) const = 0;

    // return the encoding provided by this util
    virtual Encoding getEncoding() const = 0;
    virtual const char* getEncodingString() const = 0;
    
    // transform to or from a character std::string
    virtual void unserialize(const std::string & str) = 0;
    virtual std::string serialize() const = 0;
    
    // normalization functions
    virtual GenericMap<KyteaChar,KyteaChar> * getNormMap() = 0;
    KyteaString normalize(const KyteaString & str);

    // Check that these are equal by serializing them
    void checkEqual(const StringUtil & rhs) const;

    // parse an integer or float
    int parseInt(const char* str) const;
    double parseFloat(const char* str) const;


    // get a std::string of character types
    std::string getTypeString(const KyteaString& str) const {
        std::ostringstream buff;
        for(unsigned i = 0; i < str.length(); i++)
            buff << findType(str[i]);
        return buff.str();
    }


};

// a class for parsing UTF8
class StringUtilUtf8 : public StringUtil {

private:
    
    const static char maskr6 = 63, maskr5 = 31, maskr4 = 15, maskr3 = 7,
                      maskl1 = 1 << 7, maskl2 = 3 << 6, maskl3 = 7 << 5, 
                      maskl4 = 15 << 4, maskl5 = 31 << 3;

    // variables
    StringCharMap charIds_;
    std::vector<std::string> charNames_;
    std::vector<CharType> charTypes_;

public:

    StringUtilUtf8();

    ~StringUtilUtf8() { }
    
    // map a std::string to a character
    KyteaChar mapChar(const std::string & str, bool add = true) override;
    std::string showChar(KyteaChar c) const override;

    CharType findType(KyteaChar c) const override;

    GenericMap<KyteaChar,KyteaChar> * getNormMap() override;

    bool badu(char val) const { return ((val ^ maskl1) & maskl2); }
    KyteaString mapString(const std::string & str) override;

    // find the type of a unicode character
    CharType findType(const std::string & str) override;

    Encoding getEncoding() const override { return ENCODING_UTF8; }
    const char* getEncodingString() const override { return "utf8"; }

    const std::vector<std::string> & getCharNames() const { return charNames_; }

    // transform to or from a character std::string
    void unserialize(const std::string & str) override;
    std::string serialize() const override;

};

class StringUtilEuc : public StringUtil {

const static char maskl1 = 1 << 7;
const static KyteaChar mask3len = 1 << 14;
    

public:
    StringUtilEuc() { };
    ~StringUtilEuc() { }

    KyteaChar mapChar(const std::string & str, bool add = true) override;
    std::string showChar(KyteaChar c) const override;
    
    GenericMap<KyteaChar,KyteaChar> * getNormMap() override;

    // map an unparsed std::string to a KyteaString
    KyteaString mapString(const std::string & str) override;

    // get the type of a character
    CharType findType(const std::string & str) override;
    CharType findType(KyteaChar c) const override;

    // return the encoding provided by this util
    Encoding getEncoding() const override;
    const char* getEncodingString() const override;
    
    // transform to or from a character std::string
    void unserialize(const std::string & str) override;
    std::string serialize() const override;

};

class StringUtilSjis : public StringUtil {

const static char maskl1 = 1 << 7;
const static KyteaChar mask3len = 1 << 14;
    

public:
    StringUtilSjis() { };
    ~StringUtilSjis() { }

    KyteaChar mapChar(const std::string & str, bool add = true) override;
    GenericMap<KyteaChar,KyteaChar> * getNormMap() override;

    std::string showChar(KyteaChar c) const override;
    
    // map an unparsed std::string to a KyteaString
    KyteaString mapString(const std::string & str) override;

    // get the type of a character
    CharType findType(const std::string & str) override;
    CharType findType(KyteaChar c) const override;

    // return the encoding provided by this util
    Encoding getEncoding() const override;
    const char* getEncodingString() const override;
    
    // transform to or from a character std::string
    void unserialize(const std::string & str) override;
    std::string serialize() const override;

};



}

#endif
