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

using namespace std;
using namespace kytea;

// trains a pronunciation estimation model using a corpus and a dictionary
int main(int argc, const char **argv) {

#ifndef KYTEA_SAFE
    try {
#endif
        KyteaConfig * config = new KyteaConfig;
        config->setDebug(0);
        config->setOnTraining(false);
        config->parseRunCommandLine(argc, argv);

        Kytea kytea(config);
        kytea.analyze();
        return 0;
#ifndef KYTEA_SAFE
    } catch (exception &e) {
        cerr << endl;
        cerr << " KyTea Error: " << e.what() << endl;
        return 1;
    }
#endif
}
