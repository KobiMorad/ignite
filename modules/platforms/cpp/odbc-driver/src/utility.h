/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _IGNITE_ODBC_DRIVER_UTILITY
#define _IGNITE_ODBC_DRIVER_UTILITY

extern FILE* log_file;
void logInit(const char*);

#define LOG_MSG(fmt, ...) \
    logInit("D:\\odbc.log"); \
    fprintf(log_file, "%s: " fmt, __FUNCTION__, __VA_ARGS__);

#include <string>
#include <stdint.h>

namespace ignite
{
    namespace utility
    {
        /**
         * Skip leading spaces.
         * 
         * @param begin Iterator to the beginning of the character sequence.
         * @param end Iterator to the end of the character sequence.
         * @return Iterator to first non-blanc character.
         */
        template<typename Iterator>
        Iterator SkipLeadingSpaces(Iterator begin, Iterator end)
        {
            Iterator res = begin;

            while (isspace(*res) && res != end)
                ++res;

            return res;
        }

        /**
         * Skip trailing spaces.
         * 
         * @param begin Iterator to the beginning of the character sequence.
         * @param end Iterator to the end of the character sequence.
         * @return Iterator to last non-blanc character.
         */
        template<typename Iterator>
        Iterator SkipTrailingSpaces(Iterator begin, Iterator end)
        {
            Iterator res = end - 1;

            while (isspace(*res) && res != begin - 1)
                --res;

            return res + 1;
        }

        /**
         * Remove leading and trailing spaces.
         * 
         * @param begin Iterator to the beginning of the character sequence.
         * @param end Iterator to the end of the character sequence.
         * @return String without leading and trailing spaces.
         */
        template<typename Iterator>
        std::string RemoveSurroundingSpaces(Iterator begin, Iterator end)
        {
            std::string res;

            if (begin >= end)
                return res;

            Iterator skipped_leading = SkipLeadingSpaces(begin, end);
            Iterator skipped_trailing = SkipTrailingSpaces(skipped_leading, end);

            res.reserve(skipped_trailing - skipped_leading);

            std::copy(skipped_leading, skipped_trailing, std::back_insert_iterator<std::string>(res));

            return res;
        }

        /**
         * Replace all alphabetic symbols of the string to its lowercase
         * version.
         * @param str String to be processed.
         */
        inline void IntoLower(std::string& str)
        {
            for (std::string::iterator i = str.begin(); i != str.end(); ++i)
                *i = tolower(*i);
        }
    }
}

#endif