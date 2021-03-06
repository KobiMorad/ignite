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

package org.apache.ignite.internal.processors.query.h2.opt;

import com.vividsolutions.jts.geom.Envelope;
import com.vividsolutions.jts.geom.Geometry;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import org.h2.engine.Constants;
import org.h2.engine.Session;
import org.h2.index.Cursor;
import org.h2.index.IndexCondition;
import org.h2.index.IndexType;
import org.h2.index.SingleRowCursor;
import org.h2.index.SpatialIndex;
import org.h2.message.DbException;
import org.h2.mvstore.MVStore;
import org.h2.mvstore.rtree.MVRTreeMap;
import org.h2.mvstore.rtree.SpatialKey;
import org.h2.result.SearchRow;
import org.h2.result.SortOrder;
import org.h2.table.Column;
import org.h2.table.IndexColumn;
import org.h2.table.Table;
import org.h2.table.TableFilter;
import org.h2.value.Value;
import org.h2.value.ValueGeometry;

/**
 * Spatial index.
 */
public class GridH2SpatialIndex extends GridH2IndexBase implements SpatialIndex {
    /** */
    private final ReadWriteLock lock = new ReentrantReadWriteLock();

    /** */
    private volatile long rowCnt;

    /** */
    private long rowIds;

    /** */
    private boolean closed;

    /** */
    private final MVRTreeMap<Long> treeMap;

    /** */
    private final Map<Long, GridH2Row> idToRow = new HashMap<>();

    /** */
    private final Map<Value, Long> keyToId = new HashMap<>();

    /** */
    private final MVStore store;

    /**
     * @param tbl Table.
     * @param idxName Index name.
     * @param cols Columns.
     * @param keyCol Key column.
     * @param valCol Value column.
     */
    public GridH2SpatialIndex(Table tbl, String idxName, IndexColumn[] cols, int keyCol, int valCol) {
        super(keyCol, valCol);

        if (cols.length > 1)
            throw DbException.getUnsupportedException("can only do one column");

        if ((cols[0].sortType & SortOrder.DESCENDING) != 0)
            throw DbException.getUnsupportedException("cannot do descending");

        if ((cols[0].sortType & SortOrder.NULLS_FIRST) != 0)
            throw DbException.getUnsupportedException("cannot do nulls first");

        if ((cols[0].sortType & SortOrder.NULLS_LAST) != 0)
            throw DbException.getUnsupportedException("cannot do nulls last");

        initBaseIndex(tbl, 0, idxName, cols, IndexType.createNonUnique(false, false, true));

        table = tbl;

        if (cols[0].column.getType() != Value.GEOMETRY) {
            throw DbException.getUnsupportedException("spatial index on non-geometry column, " +
                cols[0].column.getCreateSQL());
        }

        // Index in memory
        store = MVStore.open(null);
        treeMap = store.openMap("spatialIndex", new MVRTreeMap.Builder<Long>());
    }

    /**
     * Check closed.
     */
    private void checkClosed() {
        if (closed)
            throw DbException.throwInternalError();
    }

    /** {@inheritDoc} */
    @Override public GridH2Row put(GridH2Row row) {
        Lock l = lock.writeLock();

        l.lock();

        try {
            checkClosed();

            Value key = row.getValue(keyCol);

            assert key != null;

            Long rowId = keyToId.get(key);

            if (rowId != null) {
                Long oldRowId = treeMap.remove(getEnvelope(idToRow.get(rowId), rowId));

                assert rowId.equals(oldRowId);
            }
            else {
                rowId = ++rowIds;

                keyToId.put(key, rowId);
            }

            GridH2Row old = idToRow.put(rowId, row);

            treeMap.put(getEnvelope(row, rowId), rowId);

            if (old == null)
                rowCnt++; // No replace.

            return old;
        }
        finally {
            l.unlock();
        }
    }

    /**
     * @param row Row.
     * @param rowId Row id.
     * @return Envelope.
     */
    private SpatialKey getEnvelope(SearchRow row, long rowId) {
        Value v = row.getValue(columnIds[0]);
        Geometry g = ((ValueGeometry) v.convertTo(Value.GEOMETRY)).getGeometry();
        Envelope env = g.getEnvelopeInternal();
        return new SpatialKey(rowId,
            (float) env.getMinX(), (float) env.getMaxX(),
            (float) env.getMinY(), (float) env.getMaxY());
    }

    /** {@inheritDoc} */
    @Override public GridH2Row remove(SearchRow row) {
        Lock l = lock.writeLock();

        l.lock();

        try {
            checkClosed();

            Value key = row.getValue(keyCol);

            assert key != null;

            Long rowId = keyToId.remove(key);

            assert rowId != null;

            GridH2Row oldRow = idToRow.remove(rowId);

            assert oldRow != null;

            if (!treeMap.remove(getEnvelope(row, rowId), rowId))
                throw DbException.throwInternalError("row not found");

            rowCnt--;

            return oldRow;
        }
        finally {
            l.unlock();
        }
    }

    /** {@inheritDoc} */
    @Override public void close(Session ses) {
        Lock l = lock.writeLock();

        l.lock();

        try {
            closed = true;

            store.close();
        }
        finally {
            l.unlock();
        }
    }

    /** {@inheritDoc} */
    @Override protected long getCostRangeIndex(int[] masks, long rowCnt, TableFilter filter, SortOrder sortOrder) {
        rowCnt += Constants.COST_ROW_OFFSET;
        long cost = rowCnt;
        long rows = rowCnt;

        if (masks == null)
            return cost;

        for (Column column : columns) {
            int idx = column.getColumnId();
            int mask = masks[idx];
            if ((mask & IndexCondition.SPATIAL_INTERSECTS) != 0) {
                cost = 3 + rows / 4;

                break;
            }
        }

        return cost;
    }

    /** {@inheritDoc} */
    @Override public double getCost(Session ses, int[] masks, TableFilter filter, SortOrder sortOrder) {
        return getCostRangeIndex(masks, rowCnt, filter, sortOrder);
    }

    /** {@inheritDoc} */
    @Override public Cursor find(Session ses, SearchRow first, SearchRow last) {
        Lock l = lock.readLock();

        l.lock();

        try {
            checkClosed();

            return new GridH2Cursor(rowIterator(treeMap.keySet().iterator()));
        }
        finally {
            l.unlock();
        }
    }

    /** {@inheritDoc} */
    @Override public boolean canGetFirstOrLast() {
        return true;
    }

    /**
     * @param i Spatial key iterator.
     * @return Iterator over rows.
     */
    private Iterator<GridH2Row> rowIterator(Iterator<SpatialKey> i) {
        if (!i.hasNext())
            return Collections.emptyIterator();

        List<GridH2Row> rows = new ArrayList<>();

        do {
            GridH2Row row = idToRow.get(i.next().getId());

            assert row != null;

            rows.add(row);
        }
        while (i.hasNext());

        return filter(rows.iterator());
    }

    /** {@inheritDoc} */
    @Override public Cursor findFirstOrLast(Session ses, boolean first) {
        Lock l = lock.readLock();

        l.lock();

        try {
            checkClosed();

            if (!first)
                throw DbException.throwInternalError("Spatial Index can only be fetch by ascending order");

            Iterator<GridH2Row> iter = rowIterator(treeMap.keySet().iterator());

            return new SingleRowCursor(iter.hasNext() ? iter.next() : null);
        }
        finally {
            l.unlock();
        }
    }

    /** {@inheritDoc} */
    @Override public long getRowCount(Session ses) {
        return rowCnt;
    }

    /** {@inheritDoc} */
    @Override public long getRowCountApproximation() {
        return rowCnt;
    }

    /** {@inheritDoc} */
    @Override public Cursor findByGeometry(TableFilter filter, SearchRow intersection) {
        Lock l = lock.readLock();

        l.lock();

        try {
            if (intersection == null)
                return find(filter.getSession(), null, null);

            return new GridH2Cursor(rowIterator(treeMap.findIntersectingKeys(getEnvelope(intersection, 0))));
        }
        finally {
            l.unlock();
        }
    }
}