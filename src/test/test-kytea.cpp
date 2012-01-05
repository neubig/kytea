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

#include <iostream>
#include <kytea/kytea-config.h>
#include <kytea/kytea.h>
#include "test-kytea.h"
#include "test-analysis.h"
#include "test-corpusio.h"
#include "test-sentence.h"

using namespace std;

int main(int argv, char **argc) {
    kytea::KyteaTest test_kytea;
    kytea::TestAnalysis test_analysis;
    kytea::TestCorpusIO test_corpusio;
    kytea::TestSentence test_sentence;
    if(!(
        test_kytea.runTest() &
        test_analysis.runTest() &
        test_sentence.runTest() &
        test_corpusio.runTest())) {
        cout << "**** FAILED!!! ****" << endl;
    } else {
        cout << "**** passed ****" << endl;
    }
}
