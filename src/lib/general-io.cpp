#include <kytea/general-io.h>
#include <kytea/kytea-util.h>
#include <kytea/kytea-string.h>
#include <fstream>
#include <stdint.h>

using namespace std;
using namespace kytea;

void GeneralIO::openFile(const char* file, bool out, bool bin) {
    fstream::openmode mode = (out?fstream::out:fstream::in);
    if(bin) mode = mode | fstream::binary;
    fstream * str = new fstream(file, mode);
    if(str->fail()) 
        THROW_ERROR("Couldn't open file '"<<file<<"' for "<<(out?"output":"input"));
    setStream(*str, out, bin);
    owns_ = true;
}

void GeneralIO::setStream(iostream & str, bool out, bool bin) {
    if(str_ && owns_)
        delete str_;
    str_ = &str;
    str_->precision(DECIMAL_PRECISION);
    bin_ = bin;
    out_ = out;
    owns_ = false;
}

void GeneralIO::writeString(const KyteaString & str) {
    writeBinary((uint32_t)str.length());
    for(unsigned i = 0; i < str.length(); i++)
        writeBinary(str[i]);
}

// read
template <class T>
T GeneralIO::readBinary() {
    T v;
    str_->read(reinterpret_cast<char *>(&v),sizeof(T));
    return v;
} 

// Template instantiations
template bool GeneralIO::readBinary<bool>();
template char GeneralIO::readBinary<char>();
template short GeneralIO::readBinary<short>();
template int GeneralIO::readBinary<int>();
template double GeneralIO::readBinary<double>();
template unsigned short GeneralIO::readBinary<unsigned short>();
template unsigned int GeneralIO::readBinary<unsigned int>();
template unsigned char GeneralIO::readBinary<unsigned char>();

std::string GeneralIO::readString() {
    std::string str;
    getline(*str_, str, (char)0);
    return str;
}
KyteaString GeneralIO::readKyteaString() {
    KyteaString ret(readBinary<uint32_t>());
    for(unsigned i = 0; i < ret.length(); i++)
        ret[i] = readBinary<KyteaChar>();
    return ret;
}
