/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
 *
 * Distributable under the terms of either the Apache License (Version 2.0) or
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/util/StringBuffer.h"

#include <assert.h>
#include <algorithm>

#include "_NearSpansOrderedComplete.h"
#include "_NearSpansOrdered.h"
#include "SpanNearQuery.h"


CL_NS_DEF2(search, spans)

NearSpansOrderedComplete::NearSpansOrderedComplete(SpanNearQuery* spanNearQuery, CL_NS(index)::IndexReader* reader)
{
    firstTime = true;
    more = false;

    inSameDoc = false;

    matchDoc = -1;
    matchStart = -1;
    matchEnd = -1;
    iCachedEnds = cachedEnds.end();
    
    if (spanNearQuery->getClausesCount() < 2)
    {
        TCHAR* tszQry = spanNearQuery->toString();
        size_t  bBufLen = _tcslen(tszQry) + 25;
        TCHAR* tszMsg = _CL_NEWARRAY(TCHAR, bBufLen);
        _sntprintf(tszMsg, bBufLen, _T("Less than 2 clauses: %s"), tszQry);
        _CLDELETE_LCARRAY(tszQry);
        _CLTHROWT_DEL(CL_ERR_IllegalArgument, tszMsg);
    }

    maxSlop = spanNearQuery->getMaxSlop();
    minSlop = spanNearQuery->getMinSlop();

    subSpansCount = spanNearQuery->getClausesCount();
    subSpans = _CL_NEWARRAY(Spans*, subSpansCount);
    subSpansByDoc = _CL_NEWARRAY(Spans*, subSpansCount);

    SpanQuery** clauses = spanNearQuery->getClauses();
    for (size_t i = 0; i < subSpansCount; i++)
    {
        subSpans[i] = clauses[i]->getSpans(reader, true);
        subSpansByDoc[i] = subSpans[i];
    }
    clauses = NULL;

    query = spanNearQuery;
}

NearSpansOrderedComplete::~NearSpansOrderedComplete()
{
    for (size_t i = 0; i < subSpansCount; i++)
        _CLLDELETE(subSpans[i]);

    _CLDELETE_LARRAY(subSpans);
    _CLDELETE_LARRAY(subSpansByDoc);
}

bool NearSpansOrderedComplete::next()
{
    if (firstTime)
    {
        firstTime = false;
        for (size_t i = 0; i < subSpansCount; i++)
        {
            if (!subSpans[i]->next())
            {
                more = false;
                return false;
            }
        }
        lastSpanHasNext = true;
        more = true;
    }
    else if (more)
    {
        if (iCachedEnds != cachedEnds.end())
        {
            matchEnd = *iCachedEnds;
            iCachedEnds++;
            return true;
        }
        else if(subSpans[0]->next())
        {
            inSameDoc = matchDoc == subSpans[0]->doc();
        }
        else
        {
            more = false;
        }
    }
    return more && nextMatchAndCacheEnds();
}

bool NearSpansOrderedComplete::skipTo(int32_t target)
{
    if (firstTime)
    {
        firstTime = false;
        for (size_t i = 0; i < subSpansCount; i++)
        {
            if (!subSpans[i]->skipTo(target))
            {
                more = false;
                return false;
            }
        }
        lastSpanHasNext = true;
        more = true;
    }
    else if (more && (subSpans[0]->doc() < target))
    {
        if (subSpans[0]->skipTo(target))
        {
            inSameDoc = false;
        }
        else
        {
            more = false;
        }
    }
    return more && nextMatchAndCacheEnds();
}

bool NearSpansOrderedComplete::nextMatchAndCacheEnds()
{
    while (more && (inSameDoc || toSameDoc()))
    {
        if (stretchToOrder() && checkMatchAndCacheEnds())
            return true;
    }
    return false;
}

bool NearSpansOrderedComplete::toSameDoc()
{
    if (lastSpanHasNext)
    {
        sort(subSpansByDoc, subSpansByDoc + subSpansCount, spanDocCompare);
        size_t firstIndex = 0;
        int32_t maxDoc = subSpansByDoc[subSpansCount - 1]->doc();
        while (subSpansByDoc[firstIndex]->doc() != maxDoc)
        {
            if (!subSpansByDoc[firstIndex]->skipTo(maxDoc))
            {
                more = false;
                return false;
            }
            maxDoc = subSpansByDoc[firstIndex]->doc();
            if (++firstIndex == subSpansCount)
                firstIndex = 0;
        }
        lastSpanCache.clear();
        inSameDoc = true;
        return true;
    }
    return false;
}

bool NearSpansOrderedComplete::stretchToOrder()
{
    matchDoc = subSpans[0]->doc();

    // Adjust all spans except of the last one
    for (size_t i = 1; inSameDoc && (i < subSpansCount-1); i++)
    {
        while (!NearSpansOrdered::docSpansOrdered(subSpans[i - 1], subSpans[i]))
        {
            if (!subSpans[i]->next())
            {
                more = false;
                return false;
            }
            else if (matchDoc != subSpans[i]->doc())
            {
                inSameDoc = false;
                return false;
            }
        }
    }
    
    // Adjust last span including values from cache
    int32_t lastButOneStart = subSpans[subSpansCount - 2]->start();
    int32_t lastButOneEnd = subSpans[subSpansCount - 2]->end();
    while (!lastSpanCache.empty() && !NearSpansOrdered::docSpansOrdered(lastButOneStart, lastButOneEnd, lastSpanCache.top(), lastSpanCache.top2()))
    {
        lastSpanCache.pop2();
    }
    if (lastSpanCache.empty())
    {
        if (!lastSpanHasNext)
        {
            more = false;
            return false;
        }
        Spans* lastSpan = subSpans[subSpansCount - 1];
        if (lastSpan->doc() != matchDoc)
        {
            inSameDoc = false;
            return false;
        }
        while (!NearSpansOrdered::docSpansOrdered(lastButOneStart, lastButOneEnd, lastSpan->start(), lastSpan->end()))
        {
            if (!lastSpan->next())
            {
                lastSpanHasNext = false;
                more = false;
                return false;
            }
            else if (lastSpan->doc() != matchDoc)
            {
                inSameDoc = false;
                return false;
            }
        }
    }
    return true;
}

bool NearSpansOrderedComplete::checkMatchAndCacheEnds()
{
    Spans * lastSpans = subSpans[subSpansCount - 1];

    int32_t matchSlop = 0;
    int32_t nextSpanStart = lastSpanCache.empty() ? lastSpans->start() : lastSpanCache.top();
    for (int32_t i = (int32_t) subSpansCount - 2; i >= 0; i--)
    {
        int32_t spanEnd = subSpans[i]->end();
        if (spanEnd < nextSpanStart)
            matchSlop += (nextSpanStart - spanEnd);
        nextSpanStart = subSpans[i]->start();
    }

    if (matchSlop <= maxSlop)
    {
        cachedEnds.clear();
        if (lastSpanCache.empty())
            lastSpanCache.push2(lastSpans->start(), lastSpans->end());

        // Loop cached spans of the last spans and insert matching ends to cachedEnds (first position is current match, should not be in the cache)
        int32_t lastButOneEnd = subSpans[subSpansCount - 2]->end();
        int32_t lastGap = lastSpanCache.top() - lastButOneEnd;
        if (lastGap < 0)
            lastGap = 0;
        for (int32_t i = 0; i < lastSpanCache.size(); i += 2)
        {
            matchSlop -= lastGap;
            lastGap = lastSpanCache.get(i) - lastButOneEnd;
            if (lastGap < 0)
                lastGap = 0;
            matchSlop += lastGap;
            if (matchSlop <= maxSlop && minSlop <= matchSlop)
            {
                cachedEnds.insert(lastSpanCache.get(i + 1));
            }
        }

        // Check the spans for further valid end positions - the last spans are just 
        while (lastSpanHasNext && lastSpans->doc() == matchDoc)
        {
            lastSpanHasNext = lastSpans->next();
            if (lastSpanHasNext && lastSpans->doc() == matchDoc)
            {
                int32_t start = lastSpans->start();
                int32_t end = lastSpans->end();
                lastSpanCache.push2(start, end);
                matchSlop -= lastGap;
                lastGap = start - lastButOneEnd;
                if (lastGap < 0)
                    lastGap = 0;
                matchSlop += lastGap;
                if (matchSlop <= maxSlop && minSlop <= matchSlop)
                {
                    cachedEnds.insert(end);
                }
            }
        }

        iCachedEnds = cachedEnds.begin();
        if (iCachedEnds != cachedEnds.end())
        {
            matchStart = subSpans[0]->start();
            matchEnd = *iCachedEnds;
            iCachedEnds++;
            return true;
        }
    }

    more = subSpans[0]->next();
    inSameDoc = more && matchDoc == subSpans[0]->doc();
    return false;
}

TCHAR* NearSpansOrderedComplete::toString() const
{
    CL_NS(util)::StringBuffer buffer;
    TCHAR* tszQuery = query->toString();

    buffer.append(_T("NearSpansOrderedComplete("));
    buffer.append(tszQuery);
    buffer.append(_T(")@"));
    if (firstTime)
        buffer.append(_T("START"));
    else if (more)
    {
        buffer.appendInt(doc());
        buffer.append(_T(":"));
        buffer.appendInt(start());
        buffer.append(_T("-"));
        buffer.appendInt(end());
    }
    else
        buffer.append(_T("END"));

    _CLDELETE_ARRAY(tszQuery);

    return buffer.toString();
}

CL_NS_END2
