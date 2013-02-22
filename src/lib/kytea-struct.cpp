#include <kytea/kytea-struct.h>
#include <kytea/kytea-util.h>

using namespace kytea;

namespace kytea {

// Map equality checking function
template <class T>
void checkMapEqual(const KyteaStringMap<T> & a, const KyteaStringMap<T> & b) {
    if(a.size() != b.size())
        THROW_ERROR("checkMapEqual a.size() != b.size() ("<<a.size()<<", "<<b.size());
    for(typename KyteaStringMap<T>::const_iterator ait = a.begin();
        ait != a.end();
        ait++) {
        typename KyteaStringMap<T>::const_iterator bit = b.find(ait->first);
        if(bit == b.end() || ait->second != bit->second)
            THROW_ERROR("Values don't match in map");
    }
}

template void checkMapEqual(const KyteaStringMap<double> & a, const KyteaStringMap<double> & b);
template void checkMapEqual(const KyteaStringMap<unsigned int> & a, const KyteaStringMap<unsigned int> & b);

}


void KyteaSentence::refreshWS(double confidence) {
    Words newWords;
    // In order to keep track of new words, use the start and end
    int nextWord = 0, nextEnd = 0, nextStart = -1;
    if(surface.length() != 0) {
        int last = 0, i;
        for(i = 0; i <= (int)wsConfs.size(); i++) {
            double myConf = (i == (int)wsConfs.size()) ? 100.0 : wsConfs[i];
            if(myConf > confidence) {
                // Catch up to the current word
                while(nextWord < (int)words.size() && nextEnd < i+1) {
                    nextStart = nextEnd;
                    nextEnd += words[nextWord].surface.length();
                    nextWord++;
                }
                // If both the beginning and end match, use the current word
                if(last == nextStart && i+1 == nextEnd)
                    newWords.push_back(words[nextWord-1]);
                else {
                    KyteaWord w(surface.substr(last, i-last+1), norm.substr(last, i-last+1));
                    newWords.push_back(w);
                }
                // Update the start of the next word
                last = i+1;
            }
        }
    }
    words = newWords;
}
