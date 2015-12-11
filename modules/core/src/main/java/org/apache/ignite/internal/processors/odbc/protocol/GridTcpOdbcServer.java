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
import org.apache.ignite.IgniteLogger;
import org.apache.ignite.configuration.IgniteConfiguration;
import org.apache.ignite.configuration.OdbcConfiguration;
import org.apache.ignite.internal.GridKernalContext;
import org.apache.ignite.internal.processors.odbc.GridOdbcProtocolHandler;
import org.apache.ignite.internal.processors.odbc.request.GridOdbcRequest;
import org.apache.ignite.internal.util.nio.*;
import org.apache.ignite.internal.util.typedef.internal.U;
import org.apache.ignite.spi.IgnitePortProtocol;

import java.io.IOException;
import java.net.InetAddress;
import java.nio.ByteOrder;

/**
 * TCP server that handles communication with ODBC driver.
 */
public class GridTcpOdbcServer {

    /** Server. */
    private GridNioServer<GridOdbcRequest> srv;

    /** NIO server listener. */
    private GridNioServerListener<GridOdbcRequest> lsnr;

    /** Logger. */
    protected final IgniteLogger log;

    /** Context. */
    protected final GridKernalContext ctx;

    /** Host used by this protocol. */
    protected InetAddress host;

    /** Port used by this protocol. */
    protected int port;

    /** */
    public String name() {
        return "ODBC server";
    }

    public GridTcpOdbcServer(GridKernalContext ctx) {
        assert ctx != null;
        assert ctx.config().getConnectorConfiguration() != null;

        this.ctx = ctx;

        log = ctx.log(getClass());
    }

    @SuppressWarnings("BusyWait")
    public void start(final GridOdbcProtocolHandler hnd) throws IgniteCheckedException {
        OdbcConfiguration cfg = ctx.config().getOdbcConfiguration();

        assert cfg != null;

        lsnr = new GridTcpOdbcNioListener(log, this, ctx, hnd);

        GridNioParser parser = new GridOdbcParser(ctx);

        try {
            host = resolveOdbcTcpHost(ctx.config());

            int odbcPort = cfg.getPort();

            if (startTcpServer(host, odbcPort, lsnr, parser, cfg)) {
                port = odbcPort;

                System.out.println("ODBC Server has started on TCP port " + port);

                return;
            }

            U.warn(log, "Failed to start " + name() + " (possibly all ports in range are in use) " +
                    "[odbcPort=" + odbcPort + ", host=" + host + ']');
        }
        catch (IOException e) {
            U.warn(log, "Failed to start " + name() + " on port " + port + ": " + e.getMessage(),
                    "Failed to start " + name() + " on port " + port + ". " +
                            "Check restTcpHost configuration property.");
        }
    }

    /** */
    public void onKernalStart() {
    }

    /** */
    public void stop() {
        if (srv != null) {
            ctx.ports().deregisterPorts(getClass());

            srv.stop();
        }
    }

    /**
     * Resolves host for server using grid configuration.
     *
     * @param cfg Grid configuration.
     * @return Host address.
     * @throws IOException If failed to resolve host.
     */
    private InetAddress resolveOdbcTcpHost(IgniteConfiguration cfg) throws IOException {
        String host = cfg.getConnectorConfiguration().getHost();

        if (host == null)
            host = cfg.getLocalHost();

        return U.resolveLocalHost(host);
    }

    /**
     * Tries to start server with given parameters.
     *
     * @param hostAddr Host on which server should be bound.
     * @param port Port on which server should be bound.
     * @param lsnr Server message listener.
     * @param parser Server message parser.
     * @param cfg Configuration for other parameters.
     * @return {@code True} if server successfully started, {@code false} if port is used and
     *      server was unable to start.
     */
    private boolean startTcpServer(InetAddress hostAddr, int port, GridNioServerListener<GridOdbcRequest> lsnr,
                                   GridNioParser parser, OdbcConfiguration cfg) {
        try {
            GridNioFilter codec = new GridNioCodecFilter(parser, log, false);

            GridNioFilter[] filters;

            filters = new GridNioFilter[] { codec };

            srv = GridNioServer.<GridOdbcRequest>builder()
                    .address(hostAddr)
                    .port(port)
                    .listener(lsnr)
                    .logger(log)
                    .selectorCount(cfg.getSelectorCount())
                    .gridName(ctx.gridName())
                    .tcpNoDelay(cfg.isNoDelay())
                    .directBuffer(cfg.isDirectBuffer())
                    .byteOrder(ByteOrder.nativeOrder())
                    .socketSendBufferSize(cfg.getSendBufferSize())
                    .socketReceiveBufferSize(cfg.getReceiveBufferSize())
                    .sendQueueLimit(cfg.getSendQueueLimit())
                    .filters(filters)
                    .directMode(false)
                    .build();

            srv.idleTimeout(cfg.getIdleTimeout());

            srv.start();

            ctx.ports().registerPort(port, IgnitePortProtocol.TCP, getClass());

            return true;
        }
        catch (IgniteCheckedException e) {
            if (log.isDebugEnabled())
                log.debug("Failed to start " + name() + " on port " + port + ": " + e.getMessage());

            return false;
        }
    }
}
