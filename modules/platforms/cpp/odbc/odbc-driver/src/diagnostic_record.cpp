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

#include <set>
#include <string>

#include "ignite/odbc/diagnostic_record.h"

namespace
{
    /** SQLSTATEs defined by Open Group and ISO call-level interface. */
    const std::string ORIGIN_ISO_9075 = "ISO 9075";

    /** ODBC-specific SQLSTATEs (all those whose SQLSTATE class is "IM"). */
    const std::string ORIGIN_ODBC_3_0 = "ODBC 3.0";

    /** SQL state unknown constant. */
    const std::string STATE_UNKNOWN = "";

    /** SQL state 01004 constant. */
    const std::string STATE_01004 = "01004";

    /** SQL state 08001 constant. */
    const std::string STATE_08001 = "08001";

    /** SQL state 08002 constant. */
    const std::string STATE_08002 = "08002";

    /** SQL state 08003 constant. */
    const std::string STATE_08003 = "08003";

    /** SQL state HY000 constant. */
    const std::string STATE_HY000 = "HY000";

    /** SQL state HY001 constant. */
    const std::string STATE_HY001 = "HY001";

    /** SQL state HYC00 constant. */
    const std::string STATE_HYC00 = "HYC00";
}


namespace ignite
{
    namespace odbc
    {
        StatusDiagnosticRecord::StatusDiagnosticRecord() :
            sqlState(SQL_STATE_UNKNOWN),
            message(),
            connectionName(),
            serverName(),
            rowNum(0),
            columnNum(0)
        {
            // No-op.
        }

        StatusDiagnosticRecord::StatusDiagnosticRecord(SqlState sqlState,
            const std::string& message, const std::string& connectionName,
            const std::string& serverName, int32_t rowNum, int32_t columnNum) :
            sqlState(sqlState),
            message(message),
            connectionName(connectionName),
            serverName(serverName),
            rowNum(rowNum),
            columnNum(columnNum)
        {
            // No-op.
        }

        StatusDiagnosticRecord::~StatusDiagnosticRecord()
        {
            // No-op.
        }

        const std::string& StatusDiagnosticRecord::GetClassOrigin() const
        {
            const std::string& state = GetSqlState();

            if (state[0] == 'I' && state[1] == 'M')
                return ORIGIN_ODBC_3_0;

            return ORIGIN_ISO_9075;
        }

        const std::string& StatusDiagnosticRecord::GetSubclassOrigin() const
        {
            static std::set<std::string> odbcSubclasses;

            if (odbcSubclasses.empty())
            {
                odbcSubclasses.insert("01S00");
                odbcSubclasses.insert("01S01");
                odbcSubclasses.insert("01S02");
                odbcSubclasses.insert("01S06");
                odbcSubclasses.insert("01S07");
                odbcSubclasses.insert("07S01");
                odbcSubclasses.insert("08S01");
                odbcSubclasses.insert("21S01");
                odbcSubclasses.insert("21S02");
                odbcSubclasses.insert("25S01");
                odbcSubclasses.insert("25S02");
                odbcSubclasses.insert("25S03");
                odbcSubclasses.insert("42S01");
                odbcSubclasses.insert("42S02");
                odbcSubclasses.insert("42S11");
                odbcSubclasses.insert("42S12");
                odbcSubclasses.insert("42S21");
                odbcSubclasses.insert("42S22");
                odbcSubclasses.insert("HY095");
                odbcSubclasses.insert("HY097");
                odbcSubclasses.insert("HY098");
                odbcSubclasses.insert("HY099");
                odbcSubclasses.insert("HY100");
                odbcSubclasses.insert("HY101");
                odbcSubclasses.insert("HY105");
                odbcSubclasses.insert("HY107");
                odbcSubclasses.insert("HY109");
                odbcSubclasses.insert("HY110");
                odbcSubclasses.insert("HY111");
                odbcSubclasses.insert("HYT00");
                odbcSubclasses.insert("HYT01");
                odbcSubclasses.insert("IM001");
                odbcSubclasses.insert("IM002");
                odbcSubclasses.insert("IM003");
                odbcSubclasses.insert("IM004");
                odbcSubclasses.insert("IM005");
                odbcSubclasses.insert("IM006");
                odbcSubclasses.insert("IM007");
                odbcSubclasses.insert("IM008");
                odbcSubclasses.insert("IM010");
                odbcSubclasses.insert("IM011");
                odbcSubclasses.insert("IM012");
            }

            const std::string& state = GetSqlState();

            if (odbcSubclasses.find(state) != odbcSubclasses.end())
                return ORIGIN_ODBC_3_0;

            return ORIGIN_ISO_9075;
        }

        const std::string& StatusDiagnosticRecord::GetMessage() const
        {
            return message;
        }

        const std::string& StatusDiagnosticRecord::GetConnectionName() const
        {
            return connectionName;
        }

        const std::string& StatusDiagnosticRecord::GetServerName() const
        {
            return serverName;
        }

        const std::string& StatusDiagnosticRecord::GetSqlState() const
        {
            switch (sqlState)
            {
                case SQL_STATE_01004_DATA_TRUNCATED:
                    return STATE_01004;

                case SQL_STATE_08001_CANNOT_CONNECT:
                    return STATE_08001;

                case SQL_STATE_08002_ALREADY_CONNECTED:
                    return STATE_08002;

                case SQL_STATE_08003_NOT_CONNECTED:
                    return STATE_08003;

                case SQL_STATE_HY000_GENERAL_ERROR:
                    return STATE_HY000;

                case SQL_STATE_HY001_MEMORY_ALLOCATION:
                    return STATE_HY001;

                case SQL_STATE_HYC00_OPTIONAL_FEATURE_NOT_IMPLEMENTED:
                    return STATE_HYC00;

                default:
                    break;
            }

            return STATE_UNKNOWN;
        }

        int32_t StatusDiagnosticRecord::GetRowNumber() const
        {
            return rowNum;
        }

        int32_t StatusDiagnosticRecord::GetColumnNumber() const
        {
            return columnNum;
        }

        HeaderDiagnosticRecord::HeaderDiagnosticRecord() :
            rowCount(0),
            dynamicFunction(),
            dynamicFunctionCode(0),
            result(SQL_RESULT_SUCCESS),
            rowsAffected(0)
        {
            // No-op.
        }

        HeaderDiagnosticRecord::~HeaderDiagnosticRecord()
        {
            // No-op.
        }

        void HeaderDiagnosticRecord::SetHeaderRecord(SqlResult result)
        {
            rowCount = 0;
            dynamicFunction.clear();
            dynamicFunctionCode = 0;
            this->result = result;
            rowsAffected = 0;
        }

        void HeaderDiagnosticRecord::AddStatusRecord(const StatusDiagnosticRecord& record)
        {
            statusRecords.push_back(record);
        }

        void HeaderDiagnosticRecord::Reset()
        {
            SetHeaderRecord(SQL_RESULT_ERROR);

            statusRecords.clear();
        }

        SqlResult HeaderDiagnosticRecord::GetOperaionResult() const
        {
            return result;
        }

        int HeaderDiagnosticRecord::GetReturnCode() const
        {
            return SqlResultToReturnCode(result);
        }

        int64_t HeaderDiagnosticRecord::GetRowCount() const
        {
            return rowCount;
        }

        const std::string & HeaderDiagnosticRecord::GetDynamicFunction() const
        {
            return dynamicFunction;
        }

        int32_t HeaderDiagnosticRecord::GetDynamicFunctionCode() const
        {
            return dynamicFunctionCode;
        }

        int32_t HeaderDiagnosticRecord::GetRowsAffected() const
        {
            return rowsAffected;
        }

        int32_t HeaderDiagnosticRecord::GetStatusRecordsNumber() const
        {
            return static_cast<int32_t>(statusRecords.size());
        }

        const StatusDiagnosticRecord& HeaderDiagnosticRecord::GetStatusRecord(int32_t idx) const
        {
            return statusRecords[idx];
        }

        bool HeaderDiagnosticRecord::IsSuccessful() const
        {
            return result == SQL_RESULT_SUCCESS || 
                   result == SQL_RESULT_SUCCESS_WITH_INFO;
        }

        SqlResult HeaderDiagnosticRecord::GetField(int32_t recNum, DiagnosticField field, app::ApplicationDataBuffer& buffer) const
        {
            if (recNum < 1 || recNum > statusRecords.size() + 1)
                return SQL_RESULT_NO_DATA;

            if (recNum == 1)
            {
                // Header record.

                switch (field)
                {
                    case IGNITE_SQL_DIAG_HEADER_CURSOR_ROW_COUNT:
                    {
                        buffer.PutInt64(GetRowCount());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_HEADER_DYNAMIC_FUNCTION:
                    {
                        buffer.PutString(GetDynamicFunction());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_HEADER_DYNAMIC_FUNCTION_CODE:
                    {
                        buffer.PutInt32(GetDynamicFunctionCode());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_HEADER_NUMBER:
                    {
                        buffer.PutInt32(GetStatusRecordsNumber());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_HEADER_RETURNCODE:
                    {
                        buffer.PutInt32(GetReturnCode());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_HEADER_ROW_COUNT:
                    {
                        buffer.PutInt64(GetRowsAffected());

                        return SQL_RESULT_SUCCESS;
                    }

                    default:
                        break;
                }
            }
            else
            {
                // Status record.
                const StatusDiagnosticRecord& record = GetStatusRecord(recNum);

                switch (field)
                {
                    case IGNITE_SQL_DIAG_STATUS_CLASS_ORIGIN:
                    {
                        buffer.PutString(record.GetClassOrigin());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_STATUS_COLUMN_NUMBER:
                    {
                        buffer.PutInt32(record.GetColumnNumber());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_STATUS_CONNECTION_NAME:
                    {
                        buffer.PutString(record.GetConnectionName());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_STATUS_MESSAGE_TEXT:
                    {
                        buffer.PutString(record.GetMessage());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_STATUS_NATIVE:
                    {
                        buffer.PutInt32(0);

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_STATUS_ROW_NUMBER:
                    {
                        buffer.PutInt64(record.GetRowNumber());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_STATUS_SERVER_NAME:
                    {
                        buffer.PutString(record.GetServerName());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_STATUS_SQLSTATE:
                    {
                        buffer.PutString(record.GetSqlState());

                        return SQL_RESULT_SUCCESS;
                    }

                    case IGNITE_SQL_DIAG_STATUS_SUBCLASS_ORIGIN:
                    {
                        buffer.PutString(record.GetSubclassOrigin());

                        return SQL_RESULT_SUCCESS;
                    }

                    default:
                        break;
                }
            }

            return SQL_RESULT_ERROR;
        }
    }
}
