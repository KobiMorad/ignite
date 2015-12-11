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

#ifndef _IGNITE_ODBC_DRIVER_STATEMENT
#define _IGNITE_ODBC_DRIVER_STATEMENT

#include <stdint.h>

#include <map>
#include <memory>

#include <ignite/impl/interop/interop_output_stream.h>
#include <ignite/impl/interop/interop_input_stream.h>
#include <ignite/impl/binary/binary_writer_impl.h>

#include "ignite/odbc/meta/column_meta.h"
#include "ignite/odbc/meta/table_meta.h"
#include "ignite/odbc/query/query.h"
#include "ignite/odbc/app/application_data_buffer.h"
#include "ignite/odbc/app/parameter.h"
#include "ignite/odbc/parser.h"
#include "ignite/odbc/common_types.h"
#include "ignite/odbc/cursor.h"
#include "ignite/odbc/utility.h"

namespace ignite
{
    namespace odbc
    {
        class Connection;

        /**
         * SQL-statement abstraction. Holds SQL query user buffers data and
         * call result.
         */
        class Statement
        {
            friend class Connection;
        public:
            /**
             * Destructor.
             */
            ~Statement();

            /**
             * Bind result column to specified data buffer.
             *
             * @param columnIdx Column index.
             * @param buffer Buffer to put column data to.
             */
            void BindColumn(uint16_t columnIdx, const app::ApplicationDataBuffer& buffer);

            /**
             * Unbind specified column buffer.
             *
             * @param columnIdx Column index.
             */
            void UnbindColumn(uint16_t columnIdx);

            /**
             * Unbind all column buffers.
             */
            void UnbindAllColumns();

            /**
             * Bind parameter.
             *
             * @param paramIdx Parameter index.
             * @param param Parameter.
             */
            void BindParameter(uint16_t paramIdx, const app::Parameter& param);

            /**
             * Unbind specified parameter.
             *
             * @param paramIdx Parameter index.
             */
            void UnbindParameter(uint16_t paramIdx);

            /**
             * Unbind all parameters.
             */
            void UnbindAllParameters();

            /**
             * Prepare SQL query.
             *
             * @note Only SELECT queries are supported currently.
             * @param query SQL query.
             * @param len Query length.
             * @return True on success.
             */
            bool PrepareSqlQuery(const char* query, size_t len);

            /**
             * Execute SQL query.
             *
             * @note Only SELECT queries are supported currently.
             * @param query SQL query.
             * @param len Query length.
             * @return True on success.
             */
            bool ExecuteSqlQuery(const char* query, size_t len);

            /**
             * Execute SQL query.
             *
             * @note Only SELECT queries are supported currently.
             * @return True on success.
             */
            bool ExecuteSqlQuery();

            /**
             * Get columns metadata.
             *
             * @param schema Schema search pattern.
             * @param table Table search pattern.
             * @param column Column search pattern.
             * @return True on success.
             */
            bool ExecuteGetColumnsMetaQuery(const std::string& schema, const std::string& table,
                const std::string& column);

            /**
             * Get tables metadata.
             *
             * @param catalog Catalog search pattern.
             * @param schema Schema search pattern.
             * @param table Table search pattern.
             * @param tableType Table type search pattern.
             * @return True on success.
             */
            bool ExecuteGetTablesMetaQuery(const std::string& catalog, const std::string& schema,
                const std::string& table, const std::string& tableType);

            /**
             * Get foreign keys.
             *
             * @param primaryCatalog Primary key catalog name.
             * @param primarySchema Primary key schema name.
             * @param primaryTable Primary key table name.
             * @param foreignCatalog Foreign key catalog name.
             * @param foreignSchema Foreign key schema name.
             * @param foreignTable Foreign key table name.
             * @return True on success.
             */
            bool ExecuteGetForeignKeysQuery(const std::string& primaryCatalog, 
                const std::string& primarySchema, const std::string& primaryTable,
                const std::string& foreignCatalog, const std::string& foreignSchema,
                const std::string& foreignTable);

            /**
             * Close statement.
             *
             * @return True on success.
             */
            bool Close();

            /**
             * Fetch query result row.
             *
             * @return Operation result.
             */
            SqlResult FetchRow();

            /**
             * Get column metadata.
             *
             * @return Column metadata.
             */
            const meta::ColumnMetaVector* GetMeta() const;

            /**
             * Check if data is available.
             *
             * @return True if data is available.
             */
            bool DataAvailable() const;

            /**
             * Get column attribute.
             *
             * @param colIdx Column index.
             * @param attrId Attribute ID.
             * @param strbuf Buffer for string attribute value.
             * @param buflen String buffer size.
             * @param reslen Buffer to put resulting string length to.
             * @param numbuf Numeric value buffer.
             * @return True on success and false otherwise.
             */
            bool GetColumnAttribute(uint16_t colIdx, uint16_t attrId, char* strbuf,
                int16_t buflen, int16_t* reslen, int64_t* numbuf);

            /**
             * Get number of rows affected by the statement.
             *
             * @param rowCnt Number of rows affected by the statement.
             * @return True on success.
             */
            bool AffectedRows(int64_t& rowCnt) const;

        private:
            IGNITE_NO_COPY_ASSIGNMENT(Statement)

            /**
             * Constructor.
             * Called by friend classes.
             *
             * @param parent Connection associated with the statement.
             */
            Statement(Connection& parent);

            /** Connection associated with the statement. */
            Connection& connection;

            /** Column bindings. */
            app::ColumnBindingMap columnBindings;

            /** Parameter bindings. */
            app::ParameterBindingMap paramBindings;

            /** Underlying query. */
            std::auto_ptr<query::Query> currentQuery;
        };
    }
}

#endif