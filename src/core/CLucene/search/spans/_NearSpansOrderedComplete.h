/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
 *
 * Distributable under the terms of either the Apache License (Version 2.0) or
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#ifndef _lucene_search_spans_NearSpansOrderedComplete_
#define _lucene_search_spans_NearSpansOrderedComplete_

CL_CLASS_DEF(index, IndexReader)
CL_CLASS_DEF2(search, spans, SpanNearQuery)
#include "Spans.h"
#include "_NearSpansUnorderedComplete.h"

CL_NS_DEF2(search, spans)

/** A Spans that is formed from the ordered subspans of a SpanNearQuery
 * where the subspans do not overlap and have a maximum slop between them.
 */
class NearSpansOrderedComplete : public Spans
{
private:
    int32_t                 maxSlop;
    int32_t                 minSlop;
    bool                    firstTime;
    bool                    more;

    // Cached ends for the current match start
    set<int32_t>            cachedEnds;
    set<int32_t>::iterator  iCachedEnds;

    // Cached positions of the last subSpan
    Int32Cache              lastSpanCache;
    bool                    lastSpanHasNext;

    /** The spans in the same order as the SpanNearQuery */
    Spans**                 subSpans;
    size_t                  subSpansCount;

    /** Indicates that all subSpans have same doc() */
    bool                    inSameDoc;

    int32_t                 matchDoc;
    int32_t                 matchStart;
    int32_t                 matchEnd;

    Spans**                 subSpansByDoc;

    SpanNearQuery*          query;

public:
    NearSpansOrderedComplete(SpanNearQuery* spanNearQuery, CL_NS(index)::IndexReader* reader );
    virtual ~NearSpansOrderedComplete();

    bool next();
    bool skipTo(int32_t target);

    int32_t doc() const { return matchDoc; }
    int32_t start() const { return matchStart; }
    int32_t end() const { return matchEnd; }

    TCHAR* toString() const;

private:
    /** Computes the next match. The first span is the possible candidate for the start position but if there 
     * is no such match it will be moved to the first match. Possible ends will be cached.
     */
    bool nextMatchAndCacheEnds();

    /** Advance the subSpans to the same document */
    bool toSameDoc();

    /** Order the subSpans within the same document by advancing all later spans after the previous one.
     *  After this method (if true):
     *  - all spans in order
     *  - last span with the smallest possible end (here we should cache later ends)
     *  - the match span can be shrinked by moving the starting spans towards the end span - shrinkToAfterShortestMatch
     */
    bool stretchToOrder();

    /** The subSpans are ordered in the same doc, so there is a possible match.
     * Compute the slop and return the first match while caching other possible ends 
     */
    bool checkMatchAndCacheEnds();
};

CL_NS_END2
#endif // _lucene_search_spans_NearSpansOrderedComplete_
