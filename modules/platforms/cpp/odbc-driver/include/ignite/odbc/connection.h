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

#ifndef _IGNITE_ODBC_DRIVER_CONNECTION
#define _IGNITE_ODBC_DRIVER_CONNECTION

#include <stdint.h>

#include <vector>

#include "ignite/odbc/socket_client.h"
#include "ignite/odbc/statement.h"
#include "ignite/odbc/connection_info.h"

namespace ignite
{
    namespace odbc
    {
        class Connection
        {
            friend class Environment;
        public:
            /**
             * Destructor.
             */
            ~Connection();

            /**
             * Get connection info.
             * @return Connection info.
             */
            const ConnectionInfo& GetInfo() const;

            /**
             * Establish connection to ODBC server.
             *
             * @param host Host.
             * @param port Port.
             * @param cache Cache name to connect to.
             * @return True on success.
             */
            bool Establish(const std::string& host, uint16_t port, const std::string& cache);
            
            /**
             * Release established connection.
             *
             * @return True on success.
             */
            bool Release();
            
            /**
             * Create statement associated with the connection.
             *
             * @return Pointer to valid instance on success and NULL on failure.
             */
            Statement* CreateStatement();

            /**
             * Send data by established connection.
             *
             * @param data Data buffer.
             * @param len Data length.
             * @return True on success.
             */
            bool Send(const int8_t* data, size_t len);

            /**
             * Receive next message.
             *
             * @param msg Buffer for message.
             * @return True on success.
             */
            bool Receive(std::vector<int8_t>& msg);

            /**
             * Get name of the assotiated cache.
             *
             * @return Cache name.
             */
            const std::string& GetCache() const
            {
                return cache;
            }

            /**
             * Synchronously send request message and receive response.
             * @param req Request message.
             * @param rsp Response message.
             * @return True on success.
             */
            template<typename ReqT, typename RspT>
            bool SyncMessage(const ReqT& req, RspT& rsp)
            {
                std::vector<int8_t> tempBuffer;

                parser.Encode(req, tempBuffer);

                bool requestSent = Send(tempBuffer.data(), tempBuffer.size());

                if (!requestSent)
                    return false;

                bool responseReceived = Receive(tempBuffer);

                if (!responseReceived)
                    return false;

                parser.Decode(rsp, tempBuffer);

                return true;
            }

        private:
            /**
             * Constructor.
             */
            Connection();

            /** Socket. */
            tcp::SocketClient socket;

            /** State flag. */
            bool connected;

            /** Cache name. */
            std::string cache;

            /** Message parser. */
            Parser parser;
        };
    }
}

#endif