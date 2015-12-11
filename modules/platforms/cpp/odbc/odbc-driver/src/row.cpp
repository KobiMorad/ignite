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

#include <ignite/impl/interop/interop_stream_position_guard.h>

#include "ignite/odbc/utility.h"
#include "ignite/odbc/row.h"

namespace
{
    using namespace ignite::impl::interop;
    using namespace ignite::impl::binary;

    bool GetObjectLength(InteropInputStream& stream, int32_t& len)
    {
        InteropStreamPositionGuard<InteropInputStream> guard(stream);

        int8_t hdr = stream.ReadInt8();

        if (hdr != IGNITE_HDR_FULL)
            return false;

        int8_t protoVer = stream.ReadInt8();

        if (protoVer != IGNITE_PROTO_VER)
            return false;

        // Skipping flags
        stream.ReadInt16();

        // Skipping typeId
        stream.ReadInt32();

        // Skipping hash code
        stream.ReadInt32();

        len = stream.ReadInt32();

        return true;
    }
}

namespace ignite
{
    namespace odbc
    {
        Row::Row(ignite::impl::interop::InteropUnpooledMemory& pageData) :
            size(0), pos(0), rowBeginPos(0), pageData(pageData), stream(&pageData), reader(&stream)
        {
            if (pageData.Length() >= 4)
            {
                size = stream.ReadInt32();

                rowBeginPos = stream.Position();
            }
        }

        Row::~Row()
        {
            // No-op.
        }

        int8_t Row::ReadColumnHeader()
        {
            using namespace ignite::impl::binary;

            int32_t headerPos = stream.Position();

            int8_t hdr = stream.ReadInt8();

            // Check if we need to restore position - to read complex types
            // stream should have unread header, but for primitive types it
            // should not.
            switch (hdr)
            {
                case IGNITE_TYPE_BYTE:
                case IGNITE_TYPE_SHORT:
                case IGNITE_TYPE_CHAR:
                case IGNITE_TYPE_INT:
                case IGNITE_TYPE_LONG:
                case IGNITE_TYPE_FLOAT:
                case IGNITE_TYPE_DOUBLE:
                case IGNITE_TYPE_BOOL:
                case IGNITE_HDR_NULL:
                {
                    // No-op.
                    break;
                }

                default:
                {
                    // Restoring position.
                    stream.Position(headerPos);
                    break;
                }
            }

            return hdr;
        }

        bool Row::ReadColumnToBuffer(app::ApplicationDataBuffer& dataBuf)
        {
            using namespace ignite::impl::binary;
            using namespace ignite::impl::interop;

            if (pos == size)
                return false;

            int8_t hdr = ReadColumnHeader();

            switch (hdr)
            {
                case IGNITE_TYPE_BYTE:
                {
                    dataBuf.PutInt8(reader.ReadInt8());
                    break;
                }

                case IGNITE_TYPE_SHORT:
                case IGNITE_TYPE_CHAR:
                {
                    dataBuf.PutInt16(reader.ReadInt16());
                    break;
                }

                case IGNITE_TYPE_INT:
                {
                    dataBuf.PutInt32(reader.ReadInt32());
                    break;
                }

                case IGNITE_TYPE_LONG:
                {
                    dataBuf.PutInt64(reader.ReadInt64());
                    break;
                }

                case IGNITE_TYPE_FLOAT:
                {
                    dataBuf.PutFloat(reader.ReadFloat());
                    break;
                }

                case IGNITE_TYPE_DOUBLE:
                {
                    dataBuf.PutDouble(reader.ReadDouble());
                    break;
                }

                case IGNITE_TYPE_BOOL:
                {
                    dataBuf.PutInt8(reader.ReadBool() ? 1 : 0);
                    break;
                }

                case IGNITE_TYPE_STRING:
                {
                    std::string str;
                    utility::ReadString(reader, str);

                    dataBuf.PutString(str);
                    break;
                }

                case IGNITE_TYPE_UUID:
                {
                    Guid guid = reader.ReadGuid();

                    dataBuf.PutGuid(guid);
                    break;
                }

                case IGNITE_HDR_NULL:
                {
                    // TODO: clear buffer here.
                    break;
                }

                case IGNITE_HDR_FULL:
                {
                    int32_t len;

                    if (!GetObjectLength(stream, len))
                        return false;

                    int32_t offset = stream.Position();

                    dataBuf.PutBinaryData(pageData.Data() + offset, static_cast<size_t>(len));

                    stream.Position(stream.Position() + len);

                    break;
                }

                case IGNITE_TYPE_DECIMAL:
                case IGNITE_TYPE_DATE:
                default:
                {
                    // TODO: This is a fail case. Process it somehow.
                    return false;
                }
            }

            ++pos;
            return true;
        }

        bool Row::SkipColumn()
        {
            using namespace ignite::impl::binary;
            using namespace ignite::impl::interop;

            if (pos == size)
                return false;

            int8_t hdr = ReadColumnHeader();

            switch (hdr)
            {
                case IGNITE_TYPE_BYTE:
                {
                    reader.ReadInt8();
                    break;
                }

                case IGNITE_TYPE_SHORT:
                case IGNITE_TYPE_CHAR:
                {
                    reader.ReadInt16();
                    break;
                }

                case IGNITE_TYPE_INT:
                {
                    reader.ReadInt32();
                    break;
                }

                case IGNITE_TYPE_LONG:
                {
                    reader.ReadInt64();
                    break;
                }

                case IGNITE_TYPE_FLOAT:
                {
                    reader.ReadFloat();
                    break;
                }

                case IGNITE_TYPE_DOUBLE:
                {
                    reader.ReadDouble();
                    break;
                }

                case IGNITE_TYPE_BOOL:
                {
                    reader.ReadBool();
                    break;
                }

                case IGNITE_TYPE_STRING:
                {
                    std::string str;
                    utility::ReadString(reader, str);

                    break;
                }

                case IGNITE_TYPE_UUID:
                {
                    Guid guid = reader.ReadGuid();
                    break;
                }

                case IGNITE_HDR_NULL:
                {
                    // No-op.
                    break;
                }

                case IGNITE_HDR_FULL:
                {
                    int32_t len;

                    if (!GetObjectLength(stream, len))
                        return false;

                    stream.Position(stream.Position() + len);

                    break;
                }

                case IGNITE_TYPE_DECIMAL:
                case IGNITE_TYPE_DATE:
                default:
                {
                    // TODO: This is a fail case. Process it somehow.
                    return false;
                }
            }

            ++pos;
            return true;
        }

        bool Row::MoveToNext()
        {
            for (int32_t i = pos; i < size; ++i)
            {
                if (!SkipColumn())
                    return false;
            }

            size = stream.ReadInt32();

            rowBeginPos = stream.Position();

            pos = 0;

            return true;
        }
    }
}

