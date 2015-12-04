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
package org.apache.ignite.internal.processors.odbc.protocol;

import org.apache.ignite.IgniteCheckedException;
import org.apache.ignite.internal.GridKernalContext;
import org.apache.ignite.internal.portable.*;
import org.apache.ignite.internal.portable.streams.PortableHeapInputStream;
import org.apache.ignite.internal.portable.streams.PortableHeapOutputStream;
import org.apache.ignite.internal.portable.streams.PortableInputStream;
import org.apache.ignite.internal.processors.cache.portable.CacheObjectBinaryProcessorImpl;
import org.apache.ignite.internal.processors.odbc.GridOdbcColumnMeta;
import org.apache.ignite.internal.processors.odbc.request.*;
import org.apache.ignite.internal.processors.odbc.response.*;
import org.apache.ignite.internal.util.nio.GridNioParser;
import org.apache.ignite.internal.util.nio.GridNioSession;
import org.jetbrains.annotations.Nullable;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Collection;


/**
 * ODBC protocol parser.
 */
public class GridOdbcParser implements GridNioParser {

    /** Initial output stream capacity. */
    private static final int INIT_CAP = 1024;

    /** Length in bytes of the remaining message part. */
    int leftToReceive = 0;

    /** Already received bytes of current message. */
    ByteBuffer currentMessage = null;

    /** Context. */
    protected final GridKernalContext ctx;

    /** Marshaller. */
    private final GridPortableMarshaller marsh;

    GridOdbcParser(GridKernalContext context) {
        ctx = context;

        CacheObjectBinaryProcessorImpl cacheObjProc = (CacheObjectBinaryProcessorImpl)ctx.cacheObjects();

        marsh = cacheObjProc.marshaller();
    }

    /**
     * Process data chunk and try to construct new message using stored and freshly received data.
     * @param buf Fresh data buffer.
     * @return Instance of the {@link BinaryReaderExImpl} positioned to read from the beginning of the message on
     * success and null otherwise.
     */
    private BinaryRawReaderEx tryConstructMessage(ByteBuffer buf) {
        if (leftToReceive != 0) {
            // Still receiving message
            int toConsume = Math.min(leftToReceive, buf.remaining());

            currentMessage.put(buf.array(), buf.arrayOffset(), toConsume);
            leftToReceive -= toConsume;

            buf.position(buf.position() + toConsume);

            if (leftToReceive != 0)
                return null;

            PortableInputStream stream = new PortableHeapInputStream(currentMessage.array());

            BinaryReaderExImpl reader = new BinaryReaderExImpl(null, stream, null);

            currentMessage = null;

            return reader;
        }

        // Receiving new message
        // Getting message length. It's in the first four bytes of the message.
        PortableInputStream stream = new PortableHeapInputStream(buf.array());

        BinaryReaderExImpl reader = new BinaryReaderExImpl(null, stream, null);

        int messageLen = reader.readInt();
        buf.getInt();

        int remaining = buf.remaining();

        if (messageLen > remaining) {
            leftToReceive = messageLen - remaining;

            currentMessage = ByteBuffer.allocate(messageLen);
            currentMessage.put(buf);

            return null;
        }

        buf.position(buf.position() + messageLen);

        return reader;
    }

    /** {@inheritDoc} */
    @Nullable @Override public GridOdbcRequest decode(GridNioSession ses, ByteBuffer buf) throws IOException,
            IgniteCheckedException {
        BinaryRawReaderEx messageReader = tryConstructMessage(buf);

        return messageReader == null ? null : readRequest(ses, messageReader);
    }

    /** {@inheritDoc} */
    @Override public ByteBuffer encode(GridNioSession ses, Object msg) throws IOException, IgniteCheckedException {
        assert msg != null;
        assert msg instanceof GridOdbcResponse;

        System.out.println("Encoding query processing result");

        BinaryRawWriterEx writer = marsh.writer(new PortableHeapOutputStream(INIT_CAP));

        // Reserving space for the message length.
        int msgLenPos = writer.reserveInt();

        writeResponse(ses, writer, (GridOdbcResponse)msg);

        int msgLenWithHdr = writer.out().position() - msgLenPos;

        int msgLen = msgLenWithHdr - 4;

        writer.writeInt(msgLenPos, msgLen);

        ByteBuffer buf = ByteBuffer.allocate(msgLenWithHdr);

        buf.put(writer.out().array(), msgLenPos, msgLenWithHdr);

        buf.flip();

        return buf;
    }

    /**
     * Read ODBC request from the raw data using provided {@link BinaryReaderExImpl} instance.
     * @param ses Current session.
     * @param reader Reader positioned to read the request.
     * @return Instance of the {@link GridOdbcRequest}.
     * @throws IOException if the type of the request is unknown to the parser.
     */
    private GridOdbcRequest readRequest(GridNioSession ses, BinaryRawReaderEx reader) throws IOException {
        GridOdbcRequest res;

        byte cmd = reader.readByte();

        switch (cmd) {
            case GridOdbcRequest.EXECUTE_SQL_QUERY: {
                String cache = reader.readString();
                String sql = reader.readString();
                int argsNum = reader.readInt();

                //TODO: Implement parameters reading.

                System.out.println("Message EXECUTE_SQL_QUERY:");
                System.out.println("cache: " + cache);
                System.out.println("query: " + sql);
                System.out.println("argsNum: " + argsNum);

                res = new QueryExecuteRequest(cache, sql);
                break;
            }

            case GridOdbcRequest.FETCH_SQL_QUERY: {
                long queryId = reader.readLong();
                int pageSize = reader.readInt();

                System.out.println("Message FETCH_SQL_QUERY:");
                System.out.println("queryId: " + queryId);
                System.out.println("pageSize: " + pageSize);

                res = new QueryFetchRequest(queryId, pageSize);
                break;
            }

            case GridOdbcRequest.CLOSE_SQL_QUERY: {
                long queryId = reader.readLong();

                System.out.println("Message CLOSE_SQL_QUERY:");
                System.out.println("queryId: " + queryId);

                res = new QueryCloseRequest(queryId);
                break;
            }

            case GridOdbcRequest.GET_COLUMNS_META: {
                String cache = reader.readString();
                String table = reader.readString();
                String column = reader.readString();

                System.out.println("Message GET_COLUMNS_META:");
                System.out.println("cache: " + cache);
                System.out.println("table: " + table);
                System.out.println("column: " + column);

                res = new QueryGetColumnsMetaRequest(cache, table, column);
                break;
            }

            default:
                throw new IOException("Failed to parse incoming packet (unknown command type) [ses=" + ses +
                        ", cmd=[" + Byte.toString(cmd) + ']');
        }

        return res;
    }

    /**
     * Write ODBC response using provided {@link BinaryRawWriterEx} instance.
     * @param ses Current session.
     * @param writer Writer.
     * @param rsp ODBC response that should be written.
     * @throws IOException if the type of the response is unknown to the parser.
     */
    private void writeResponse(GridNioSession ses, BinaryRawWriterEx writer, GridOdbcResponse rsp) throws IOException {
        // Writing status
        writer.writeByte((byte)rsp.getSuccessStatus());

        if (rsp.getSuccessStatus() != GridOdbcResponse.STATUS_SUCCESS) {
            writer.writeString(rsp.getError());

            return;
        }

        Object res0 = rsp.getResponse();

        if (res0 instanceof QueryExecuteResult) {
            QueryExecuteResult res = (QueryExecuteResult) res0;

            System.out.println("Resulting query ID: " + res.getQueryId());

            writer.writeLong(res.getQueryId());

            Collection<GridOdbcColumnMeta> metas = res.getColumnsMetadata();

            assert metas != null;

            writer.writeInt(metas.size());

            for (GridOdbcColumnMeta meta : metas)
                meta.writeBinary(writer, marsh.context());

        } else if (res0 instanceof QueryFetchResult) {
            QueryFetchResult res = (QueryFetchResult) res0;

            System.out.println("Resulting query ID: " + res.getQueryId());

            writer.writeLong(res.getQueryId());

            Collection<?> items0 = res.getItems();

            assert items0 != null;

            Collection<Collection<Object>> items = (Collection<Collection<Object>>)items0;

            writer.writeBoolean(res.getLast());

            writer.writeInt(items.size());

            for (Collection<Object> row : items) {
                if (row != null) {
                    writer.writeInt(row.size());

                    for (Object obj : row) {
                        if (obj != null)
                            writer.writeObjectDetached(obj);
                    }
                }
            }
        } else if (res0 instanceof QueryCloseResult) {
            QueryCloseResult res = (QueryCloseResult) res0;

            System.out.println("Resulting query ID: " + res.getQueryId());

            writer.writeLong(res.getQueryId());

        } else if (res0 instanceof QueryGetColumnsMetaResult) {
            QueryGetColumnsMetaResult res = (QueryGetColumnsMetaResult) res0;

            Collection<GridOdbcColumnMeta> columnsMeta = res.getMeta();

            assert columnsMeta != null;

            writer.writeInt(columnsMeta.size());

            for (GridOdbcColumnMeta columnMeta : columnsMeta)
                columnMeta.writeBinary(writer, marsh.context());

        } else {
            throw new IOException("Failed to serialize response packet (unknown response type) [ses=" + ses + "]");
        }
    }
}
