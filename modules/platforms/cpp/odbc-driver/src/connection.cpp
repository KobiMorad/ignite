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

#include <cstring>

#include <sstream>

#include "connection.h"

// TODO: implement appropriate protocol with de-/serialisation.
namespace
{
#pragma pack(push, 1)
    struct OdbcProtocolHeader
    {
        int32_t len;
    };
#pragma pack(pop)
}

namespace ignite
{
    namespace odbc
    {
        Connection::Connection() : socket(), connected(false)
        {
            // No-op.
        }

        Connection::~Connection()
        {
            // No-op.
        }

        bool Connection::Establish(const std::string& host, uint16_t port)
        {
            if (connected)
                return false;

            connected = socket.Connect(host.c_str(), port);

            return connected;
        }

        bool Connection::Release()
        {
            if (!connected)
                return false;

            socket.Close();

            connected = false;

            return !connected;
        }

        Statement* Connection::CreateStatement()
        {
            return new Statement(*this);
        }

        bool Connection::Send(const uint8_t* data, size_t len)
        {
            if (!connected)
                return false;

            OdbcProtocolHeader hdr;

            hdr.len = static_cast<int32_t>(len);

            bool success = socket.Send(reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)) == sizeof(hdr);

            if (!success)
                return false;

            return socket.Send(data, len) == len;
        }

        bool Connection::Receive(std::vector<uint8_t>& msg)
        {
            if (!connected)
                return false;

            msg.clear();

            OdbcProtocolHeader hdr;

            bool success = socket.Receive(reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)) == sizeof(hdr);

            if (!success)
                return false;

            size_t remain = hdr.len;
            size_t receivedAtAll = 0;

            msg.resize(remain);

            while (remain)
            {
                int received = socket.Receive(&msg[receivedAtAll], remain);
                
                if (received <= 0)
                {
                    msg.resize(receivedAtAll);

                    return false;
                }

                remain -= static_cast<size_t>(received);
            }

            return true;
        }
    }
}

