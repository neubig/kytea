/*
 * Copyright 2020, KyTea Development Team
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

#ifndef KYTEA_CORPUS_IO_FORMAT_H__
#define KYTEA_CORPUS_IO_FORMAT_H__

namespace kytea {

enum CorpusFormat {
    CORP_FORMAT_RAW,
    CORP_FORMAT_FULL,
    CORP_FORMAT_PART,
    CORP_FORMAT_PROB,
    CORP_FORMAT_TOK,
    CORP_FORMAT_DEFAULT,
    CORP_FORMAT_EDA,
    CORP_FORMAT_TAGS,
};

} // namespace kytea

#endif // KYTEA_CORPUS_IO_FORMAT_H__
