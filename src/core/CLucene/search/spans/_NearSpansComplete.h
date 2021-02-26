/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
 * 
 * Distributable under the terms of either the Apache License (Version 2.0) or 
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#ifndef _lucene_search_spans_NearSpansComplete_
#define _lucene_search_spans_NearSpansComplete_

CL_CLASS_DEF(index, IndexReader)
CL_CLASS_DEF2(search, spans, SpanNearQuery)
#include "CLucene/util/PriorityQueue.h"
#include "Spans.h"

CL_NS_DEF2( search, spans )

/////////////////////////////////////////////////////////////////////////////
class Int32Cache
{
private:
    int32_t *   pTop;
    int32_t *   pPush;

    int32_t *   pBuffer;
    int32_t *   pBufferEnd;

public:
    Int32Cache()                        { pTop = pPush = pBuffer = _CL_NEWARRAY( int32_t, 8 ); pBufferEnd = pBuffer+8; }
    virtual ~Int32Cache()               { _CLDELETE_LARRAY( pBuffer ); }

    void push2( int32_t value1, int32_t value2 );
    void pop2()                         { pTop+=2; if( pTop >= pBufferEnd ) pTop = pBuffer + (pTop - pBufferEnd); }

    /// Returns top elements of random elements
    int32_t top() const                 { return *pTop; }
    int32_t top2() const                { return *(pTop+1); }
    int32_t get( int32_t pos ) const    { int32_t * pValue = pTop + pos; if( pValue < pBufferEnd ) return *pValue; else return *( pBuffer + (pValue - pBufferEnd)); }

    /// Clear cache and check its size
    void clear()                        { pTop = pPush = pBuffer; }
    bool empty() const                  { return pTop == pPush; }
    size_t size() const                 { if( pPush >= pTop ) return pPush - pTop; else return pBufferEnd - pBuffer - (pTop - pPush); }

protected:
    void resizeAfterPush();
};

inline void Int32Cache::push2( int32_t value1, int32_t value2 )
{ 
    *(pPush++) = value1; 
    *(pPush++) = value2; 
    if( pPush == pBufferEnd ) 
        pPush = pBuffer; 
    if( pPush == pTop ) 
        resizeAfterPush(); 
};


/////////////////////////////////////////////////////////////////////////////
class NearSpansComplete : public Spans 
{
private:
    /////////////////////////////////////////////////////////////////////////////
    class SpansCell : public Spans
    {
    private:
        NearSpansComplete *    parentSpans;
        Spans *                         spans;
        int32_t                         length;
        int32_t                         index;
        
        Int32Cache                      cache;
        bool                            hasNext;

    public:
        SpansCell *                     nextCell;

    public:
        SpansCell( NearSpansComplete * parentSpans, Spans * spans, int32_t index );
        virtual ~SpansCell();

        bool next();
        bool skipTo( int32_t target );

        int32_t doc() const             { return cache.empty() ? spans->doc() : parentSpans->cachedDoc; }
        int32_t start() const           { return cache.empty() ? spans->start() : cache.top(); }
        int32_t end() const             { return cache.empty() ? spans->end() : cache.top2(); }

        void addEnds( int32_t currentEnd, int32_t minSlop, int32_t maxSlop, set<int32_t>& ends );
        void clearCache()               { cache.clear(); }

        TCHAR* toString() const;
        int32_t getIndex() const        { return index; }
        
    private:
        bool adjust( bool condition );

    };

    /////////////////////////////////////////////////////////////////////////////
    class CellQueue : public CL_NS(util)::PriorityQueue<SpansCell *, CL_NS(util)::Deletor::Object<SpansCell> >
    {
    public:
        CellQueue( int32_t size ) { initialize( size, false ); }    // All the span cells will be freed in ~NearSpansUnordered() while frein ordered member
        virtual ~CellQueue() {}

    protected:
        bool lessThan( SpansCell * spans1, SpansCell* spans2 );
    };

private:
    SpanNearQuery *     query;

    list<SpansCell *>   ordered;                // spans in query order
    int32_t             maxSlop;                // from query
    int32_t             minSlop;                // from query

    bool                ignoreOrder;            // flag if order of child spans matters

    SpansCell *         first;                  // linked list of spans
    SpansCell *         last;                   // sorted by doc only

    int32_t             totalLength;            // sum of current lengths

    CellQueue *         queue;                  // sorted queue of spans
    SpansCell *         max;                    // max element in queue

    bool                more;                   // true iff not done
    bool                firstTime;              // true before first next()

    int32_t             cachedDoc;
    int32_t             cachedStart;
    set<int32_t>        cachedEnds;
    set<int32_t>::iterator  iCachedEnds;

public:
    NearSpansComplete( SpanNearQuery * query, CL_NS(index)::IndexReader * reader, bool inOrder );
    virtual ~NearSpansComplete();

    bool next();
    bool skipTo( int32_t target );

    int32_t doc() const     { return cachedEnds.empty() ? (firstTime ? -1 : min()->doc()) : cachedDoc; }
    int32_t start() const   { return cachedEnds.empty() ? min()->start() : cachedStart; }
    int32_t end() const     { return cachedEnds.empty() ? max->end() : *iCachedEnds; }

    TCHAR* toString() const;

private:
    SpansCell * min() const { return queue->top(); }

    void initList( bool next );
    void addToList( SpansCell * cell );
    void firstToLast();
    void queueToList();
    void listToQueue();
    bool atMatch();
    bool inOrder();

    void prepareSpans();
    
    bool _next();
};

CL_NS_END2
#endif // _lucene_search_spans_NearSpansComplete_
