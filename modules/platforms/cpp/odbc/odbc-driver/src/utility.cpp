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

#include "ignite/odbc/utility.h"

#ifdef _WIN32
#   define _WINSOCKAPI_
#   include <windows.h>
#   undef min
#endif //_WIN32

#include <sqlext.h>
#include <odbcinst.h>

namespace ignite
{
    namespace utility
    {
        size_t CopyStringToBuffer(const std::string& str, char* buf, size_t buflen)
        {
            if (!buf || !buflen)
                return 0;

            size_t bytesToCopy = std::min(str.size(), static_cast<size_t>(buflen - 1));

            memcpy(buf, str.data(), bytesToCopy);
            buf[bytesToCopy] = 0;

            return bytesToCopy;
        }

        void ReadString(ignite::impl::binary::BinaryReaderImpl& reader, std::string& str)
        {
            int32_t strLen = reader.ReadString(0, 0);
            if (!strLen)
            {
                str.clear();

                char dummy;

                reader.ReadString(&dummy, sizeof(dummy));
            }
            else
            {
                str.resize(strLen);

                reader.ReadString(&str[0], static_cast<int32_t>(str.size()));
            }
        }

        void WriteString(ignite::impl::binary::BinaryWriterImpl& writer, const std::string & str)
        {
            writer.WriteString(str.data(), static_cast<int32_t>(str.size()));
        }

        std::string SqlStringToString(const unsigned char* sqlStr, int32_t sqlStrLen)
        {
            std::string res;

            const char* sqlStrC = reinterpret_cast<const char*>(sqlStr);

            if (!sqlStr || !sqlStrLen)
                return res;

            if (sqlStrLen == SQL_NTS)
                res.assign(sqlStrC);
            else
                res.assign(sqlStrC, sqlStrLen);

            return res;
        }
    }
}

