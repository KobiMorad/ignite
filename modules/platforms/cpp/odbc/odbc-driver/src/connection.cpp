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

#include "ignite/odbc/utility.h"
#include "ignite/odbc/statement.h"
#include "ignite/odbc/connection.h"

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
        Connection::Connection() : socket(), connected(false), cache(), parser()
        {
            // No-op.
        }

        Connection::~Connection()
        {
            // No-op.
        }

        void Connection::AddStatusRecord(SqlState sqlState, const std::string & message)
        {
            diagnosticRecord.AddStatusRecord(CreateStatusRecord(sqlState, message));
        }

        const config::ConnectionInfo& Connection::GetInfo() const
        {
            // Connection info is the same for all connections now.
            static config::ConnectionInfo info;

            return info;
        }

        void Connection::GetInfo(config::ConnectionInfo::InfoType type, void* buf, short buflen, short* reslen)
        {
            IGNITE_ODBC_API_CALL(diagnosticRecord, InternalGetInfo(type, buf, buflen, reslen));
        }

        SqlResult Connection::InternalGetInfo(config::ConnectionInfo::InfoType type, void* buf, short buflen, short* reslen)
        {
            const config::ConnectionInfo& info = GetInfo();

            SqlResult res = info.GetInfo(type, buf, buflen, reslen);

            if (res != SQL_RESULT_SUCCESS)
                AddStatusRecord(SQL_STATE_HYC00_OPTIONAL_FEATURE_NOT_IMPLEMENTED, "Not implemented.");

            return res;
        }

        void Connection::Establish(const std::string& host, uint16_t port, const std::string& cache)
        {
            IGNITE_ODBC_API_CALL(diagnosticRecord, InternalEstablish(host, port, cache));
        }

        SqlResult Connection::InternalEstablish(const std::string & host, uint16_t port, const std::string & cache)
        {
            if (connected)
            {
                AddStatusRecord(SQL_STATE_08002_ALREADY_CONNECTED, "Already connected.");

                return SQL_RESULT_ERROR;
            }

            if (cache.empty())
            {
                AddStatusRecord(SQL_STATE_HY000_GENERAL_ERROR, "Cache is not specified.");

                return SQL_RESULT_ERROR;
            }

            this->cache = cache;

            connected = socket.Connect(host.c_str(), port);

            if (!connected)
            {
                AddStatusRecord(SQL_STATE_08001_CANNOT_CONNECT, "Failed to establish connection with the host.");

                return SQL_RESULT_ERROR;
            }

            return SQL_RESULT_SUCCESS;
        }

        void Connection::Release()
        {
            IGNITE_ODBC_API_CALL(diagnosticRecord, InternalRelease());
        }

        SqlResult Connection::InternalRelease()
        {
            if (!connected)
            {
                AddStatusRecord(SQL_STATE_08003_NOT_CONNECTED, "Connection is not open.");

                return SQL_RESULT_ERROR;
            }

            socket.Close();

            connected = false;

            return SQL_RESULT_SUCCESS;
        }

        Statement* Connection::CreateStatement()
        {
            Statement* statement;

            IGNITE_ODBC_API_CALL(diagnosticRecord, InternalCreateStatement(statement));

            return statement;
        }

        SqlResult Connection::InternalCreateStatement(Statement*& statement)
        {
            statement = new Statement(*this);

            if (!statement)
                AddStatusRecord(SQL_STATE_HY001_MEMORY_ALLOCATION, "Not enough memory.");
        }

        bool Connection::Send(const int8_t* data, size_t len)
        {
            if (!connected)
                return false;

            size_t sent = 0;

            while (sent != len) 
            {
                size_t res = socket.Send(data + sent, len - sent);

                if (res <= 0)
                    return false;

                sent += res;
            }

            return true;
        }

        bool Connection::Receive(std::vector<int8_t>& msg)
        {
            if (!connected)
                return false;

            msg.clear();

            OdbcProtocolHeader hdr;

            int received = socket.Receive(reinterpret_cast<int8_t*>(&hdr), sizeof(hdr));
            LOG_MSG("Received: %d\n", received);

            if (received != sizeof(hdr))
                return false;

            size_t remain = hdr.len;
            size_t receivedAtAll = 0;

            msg.resize(remain);

            while (remain)
            {
                received = socket.Receive(&msg[receivedAtAll], remain);
                LOG_MSG("Received: %d\n", received);
                LOG_MSG("remain: %d\n", remain);

                if (received <= 0)
                {
                    msg.resize(receivedAtAll);

                    return false;
                }

                remain -= static_cast<size_t>(received);
            }

            return true;
        }

        const std::string& Connection::GetCache() const
        {
            return cache;
        }

        StatusDiagnosticRecord Connection::CreateStatusRecord(SqlState sqlState,
            const std::string& message, int32_t rowNum, int32_t columnNum)
        {
            return StatusDiagnosticRecord(sqlState, message, "", "", rowNum, columnNum);
        }

        const HeaderDiagnosticRecord& Connection::GetDiagnosticRecord() const
        {
            return diagnosticRecord;
        }
    }
}

