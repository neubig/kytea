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

#include <kytea/kytea-util.h>
#include <kytea/kytea-config.h>
#include <kytea/corpus-io.h>
#include <kytea/corpus-io-full.h>
#include <kytea/corpus-io-eda.h>
#include <kytea/corpus-io-tokenized.h>
#include <kytea/corpus-io-part.h>
#include <kytea/corpus-io-prob.h>
#include <kytea/corpus-io-raw.h>
#include <cmath>
#include "config.h"

#define PROB_TRUE    100.0
#define PROB_FALSE   -100.0
#define PROB_UNKNOWN 0.0

using namespace kytea;
using namespace std;

CorpusIO * CorpusIO::createIO(const char* file, Format form, const KyteaConfig & conf, bool output, StringUtil* util) {
    if(form == CORP_FORMAT_FULL)      { return new FullCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_TAGS)      { 
        FullCorpusIO * io = new FullCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape());
        io->setPrintWords(false);
        return io;
    }
    else if(form == CORP_FORMAT_TOK)      { return new TokenizedCorpusIO(util,file,output,conf.getWordBound()); }
    else if(form == CORP_FORMAT_PART) { return new PartCorpusIO(util,file,output,conf.getUnkBound(),conf.getSkipBound(),conf.getNoBound(),conf.getHasBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_PROB) { return new ProbCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_RAW)  { return new RawCorpusIO(util,file,output);  }
    else if(form == CORP_FORMAT_EDA)  { return new EdaCorpusIO(util,file,output);  }
    else
        THROW_ERROR("Illegal Output Format");
}

CorpusIO * CorpusIO::createIO(iostream & file, Format form, const KyteaConfig & conf, bool output, StringUtil* util) {
    if(form == CORP_FORMAT_FULL)      { return new FullCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_TAGS)      { 
        FullCorpusIO * io = new FullCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape());
        io->setPrintWords(false);
        return io;
    }
    else if(form == CORP_FORMAT_TOK)      { return new TokenizedCorpusIO(util,file,output,conf.getWordBound()); }
    else if(form == CORP_FORMAT_PART) { return new PartCorpusIO(util,file,output,conf.getUnkBound(),conf.getSkipBound(),conf.getNoBound(),conf.getHasBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_PROB) { return new ProbCorpusIO(util,file,output,conf.getWordBound(),conf.getTagBound(),conf.getElemBound(),conf.getEscape()); }
    else if(form == CORP_FORMAT_RAW)  { return new RawCorpusIO(util,file,output);  }
    else if(form == CORP_FORMAT_EDA)  { return new EdaCorpusIO(util,file,output);  }
    else 
        THROW_ERROR("Illegal Output Format");
}
