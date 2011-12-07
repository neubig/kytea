#include "kytea/dictionary.h"

using namespace kytea;
using namespace std;


ModelTagEntry::~ModelTagEntry() {
    for(int i = 0; i < (int)tagMods.size(); i++)
        if(tagMods[i]) 
            delete tagMods[i];
}
