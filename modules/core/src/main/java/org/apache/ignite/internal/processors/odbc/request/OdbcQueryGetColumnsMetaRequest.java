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
package org.apache.ignite.internal.processors.odbc.request;

/**
 * ODBC query get columns meta request.
 */
public class OdbcQueryGetColumnsMetaRequest extends OdbcRequest {
    /** Cache name. */
    private String cacheName;

    /** Table name. */
    private String tableName;

    /** Column name. */
    private String columnName;

    /**
     * @param cacheName Cache name.
     */
    public OdbcQueryGetColumnsMetaRequest(String cacheName, String tableName, String columnName) {
        super(GET_COLUMNS_META);

        this.cacheName = cacheName;
        this.tableName = tableName;
        this.columnName = columnName;
    }

    /**
     * @param cacheName Cache name.
     */
    public void cacheName(String cacheName) {
        this.cacheName = cacheName;
    }

    /**
     * @return Cache name.
     */
    public String cacheName() {
        return cacheName;
    }

    /**
     * @param tableName Table name.
     */
    public void tableName(String tableName) {
        this.tableName = tableName;
    }

    /**
     * @return Table name.
     */
    public String tableName() {
        return tableName;
    }

    /**
     * @param columnName Column name.
     */
    public void columnName(String columnName) {
        this.columnName = columnName;
    }

    /**
     * @return Column name.
     */
    public String columnName() {
        return columnName;
    }
}