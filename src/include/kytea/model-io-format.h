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

#ifndef KYTEA_MODEL_IO_FORMAT_H__
#define KYTEA_MODEL_IO_FORMAT_H__

namespace kytea {

enum ModelFormat : char {
    MODEL_FORMAT_BINARY = 'B',
    MODEL_FORMAT_TEXT = 'T',
    MODEL_FORMAT_UNKNOWN = 'U',
};

}  // namespace kytea

#endif // KYTEA_MODEL_IO_FORMAT_H__
