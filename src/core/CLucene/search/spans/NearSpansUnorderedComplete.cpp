/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
 * 
 * Distributable under the terms of either the Apache License (Version 2.0) or 
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/util/StringBuffer.h"

#include "_NearSpansUnorderedComplete.h"
#include "_NearSpansOrdered.h"
#include "SpanNearQuery.h"

CL_NS_DEF2( search, spans )


/////////////////////////////////////////////////////////////////////////////
void Int32Cache::resizeAfterPush()
{
    size_t curSize = pBufferEnd - pBuffer;
    size_t newSize = curSize < 32 ? curSize + 8 : curSize + 32;
    int32_t * newBuffer = _CL_NEWARRAY( int32_t, newSize );
    if( pPush > pTop )
        memcpy( newBuffer, pTop, ( pPush - pTop ) * sizeof( int32_t ));
    else
    {
        size_t cnt = pBufferEnd - pTop;
        memcpy( newBuffer, pTop, cnt * sizeof( int32_t ));
        memcpy( newBuffer + cnt, pBuffer, ( pPush - pBuffer ) * sizeof( int32_t ));
    }
    _CLDELETE_LARRAY( pBuffer );
    pBuffer = pTop = newBuffer;
    pBufferEnd = pBuffer + newSize;
    pPush = pBuffer + curSize;
}

/////////////////////////////////////////////////////////////////////////////
NearSpansUnorderedComplete::SpansCell::SpansCell( NearSpansUnorderedComplete * parentSpans, Spans * spans, int32_t index )
{
    this->parentSpans = parentSpans;
    this->spans  = spans;
    this->index  = index;
    this->length = -1;
    this->cachedNext = true;
}

/////////////////////////////////////////////////////////////////////////////
NearSpansUnorderedComplete::SpansCell::~SpansCell()
{
    _CLLDELETE( spans );
}

/////////////////////////////////////////////////////////////////////////////
bool NearSpansUnorderedComplete::SpansCell::adjust( bool condition )
{
    if( length != -1 )
        parentSpans->totalLength -= length;  // subtract old length

    if( condition )
    {
        length = end() - start(); 
        parentSpans->totalLength += length;  // add new length

        if( ! parentSpans->max 
            || doc() > parentSpans->max->doc()
            || ( doc() == parentSpans->max->doc() && end() > parentSpans->max->end()))
        {
            parentSpans->max = this;
        }
    }
    parentSpans->more = condition;
    return condition;
}

/////////////////////////////////////////////////////////////////////////////
bool NearSpansUnorderedComplete::SpansCell::next()
{ 
    if( cache.empty() )
        return adjust( spans->next() );
    else
    {
        cache.pop2();
        return adjust( ! cache.empty() || cachedNext );
    }
}

/////////////////////////////////////////////////////////////////////////////
bool NearSpansUnorderedComplete::SpansCell::skipTo( int32_t target )
{ 
    if( cache.empty() || doc() < target )
    {
        cache.clear();
        return adjust( spans->skipTo( target )); 
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////
void NearSpansUnorderedComplete::SpansCell::addEnds( int32_t endMin, int32_t endMax, set<int32_t>& ends )
{
    int32_t end;

    size_t sz = cache.size();
    for( size_t pos = 1; pos < sz; pos += 2 )
    {
        end = cache.get( pos );
        if( endMin < end && end <= endMax )
            ends.insert( end );
    }

    if( cachedNext && spans->doc() == parentSpans->cachedDoc )
    {
        end = spans->end();
        while( end <= endMax )
        {
            cache.push2(spans->start(), end);
            if( spans->next() )
            {
                if( spans->doc() == parentSpans->cachedDoc )
                {
                    end = spans->end();
                    if( endMin < end && end <= endMax )
                        ends.insert( end );
                }
                else
                    break;
            }
            else
            {
                cachedNext = false;
                break;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
TCHAR* NearSpansUnorderedComplete::SpansCell::toString() const
{
    CL_NS(util)::StringBuffer buffer;
    TCHAR * tszSpans = spans->toString();

    buffer.append( tszSpans );
    buffer.append( _T( "#" ));
    buffer.appendInt( index );

    _CLDELETE_LARRAY( tszSpans );
    return buffer.toString();
}


/////////////////////////////////////////////////////////////////////////////
bool NearSpansUnorderedComplete::CellQueue::lessThan(SpansCell * spans1, SpansCell* spans2 )
{
     if( spans1->doc() == spans2->doc() )
         return NearSpansOrdered::docSpansOrdered( spans1, spans2 );
     else 
        return spans1->doc() < spans2->doc();
}


/////////////////////////////////////////////////////////////////////////////
NearSpansUnorderedComplete::NearSpansUnorderedComplete( SpanNearQuery * query, CL_NS(index)::IndexReader * reader )
{
    // this->ordered = new ArrayList();
    this->more = true;
    this->firstTime = true;

    this->max = NULL;                       // CLucene specific, SpansCell::adjust tests this member to NULL
    this->first = NULL;                     // CLucene specific
    this->last = NULL;                      // CLucene specific, addToList test this member to NULL 
    this->totalLength = 0;                  // CLucene specific

    this->query = query;
    this->maxSlop = query->getMaxSlop();
    this->minSlop = query->getMinSlop();

    SpanQuery ** clauses = query->getClauses();
    this->queue = _CLNEW CellQueue( query->getClausesCount() );
    for( size_t i = 0; i < query->getClausesCount(); i++ )
    {
        SpansCell * cell = _CLNEW SpansCell( this, clauses[ i ]->getSpans( reader, true ), i );
        ordered.push_back( cell );
    }
    clauses = NULL;

    cachedDoc = -1;
    cachedStart = -1;
    iCachedEnds = cachedEnds.end();
}

NearSpansUnorderedComplete::~NearSpansUnorderedComplete()
{
    for( list<SpansCell *>::iterator iCell = ordered.begin(); iCell != ordered.end(); iCell++ )
        _CLLDELETE( *iCell );

    _CLLDELETE( queue );
}

bool NearSpansUnorderedComplete::next()
{
    if( iCachedEnds != cachedEnds.end())
        iCachedEnds++;

    return ( iCachedEnds != cachedEnds.end() ) ? true : _next();
}

bool NearSpansUnorderedComplete::_next()
{
    if( firstTime )
    {
        initList( true );
        listToQueue();                          // initialize queue
        firstTime = false;
    } 
    else if( more )
    {
        if( min()->next() )                     // trigger further scanning
            queue->adjustTop();                 // maintain queue
        else 
            more = false;
    }

    while( more )
    {
        bool queueStale = false;
        if( min()->doc() != max->doc() )        // maintain list
        {             
            queueToList();
            queueStale = true;
        }

        // skip to doc w/ all clauses
        while( more && first->doc() < last->doc() )
        {
            more = first->skipTo( last->doc() );// skip first upto last
            firstToLast();                      // and move it to the end
            queueStale = true;
        }

        if( ! more ) 
            return false;

        // found doc w/ all clauses
        if( queueStale )                        // maintain the queue
        {
            listToQueue();
            queueStale = false;
        }

        if( atMatch() )
            return true;
      
        more = min()->next();
        if( more )
            queue->adjustTop();                 // maintain queue
    }

    return false;                               // no more matches
}

bool NearSpansUnorderedComplete::skipTo( int32_t target )
{
    if( firstTime )                             // initialize
    {
        initList( false );
        for( SpansCell * cell = first; more && cell; cell = cell->nextCell )
        {
            more = cell->skipTo( target );      // skip all
        }
        
        if( more )
            listToQueue();

        firstTime = false;
    } 
    else 
    {                                           // normal case
        while( more && min()->doc() < target )  // skip as needed
        {
            if( min()->skipTo( target )) 
                queue->adjustTop();
            else 
                more = false;
        }
    }

    return more && ( atMatch() ||  next() );
}

TCHAR* NearSpansUnorderedComplete::toString() const
{
    CL_NS(util)::StringBuffer buffer;
    TCHAR * tszQuery = query->toString();

    buffer.append( _T( "NearSpansUnorderedComplete(" ));
    buffer.append( tszQuery );
    buffer.append( _T( ")@" ));
    if( firstTime )
        buffer.append( _T( "START" ));
    else if( more )
    {
        buffer.appendInt( doc() );
        buffer.append( _T( ":" ));
        buffer.appendInt( start() );
        buffer.append( _T( "-" ));
        buffer.appendInt( end() );
    }
    else
        buffer.append( _T( "END" ));

    _CLDELETE_ARRAY( tszQuery );

    return buffer.toString();
}

void NearSpansUnorderedComplete::initList( bool next ) 
{
    for( list<SpansCell *>::iterator iCell = ordered.begin(); more && iCell != ordered.end(); iCell++ )
    {
        if( next )
            more = (*iCell)->next();            // move to first entry

        if( more )
            addToList( *iCell );                // add to list
    }
}

void NearSpansUnorderedComplete::addToList( SpansCell * cell )
{
    if( last )                                  // add next to end of list
        last->nextCell = cell;
    else
        first = cell;
    
    last = cell;
    cell->nextCell = NULL;
}

void NearSpansUnorderedComplete::firstToLast()
{
    last->nextCell = first;			            // move first to end of list
    last = first;
    first = first->nextCell;
    last->nextCell = NULL;
}

void NearSpansUnorderedComplete::queueToList()
{
    last = NULL;
    first = NULL;
    while( queue->top() ) 
        addToList( queue->pop() );
}
  
void NearSpansUnorderedComplete::listToQueue()
{
    queue->clear();                             // rebuild queue
    for( SpansCell * cell = first; cell; cell = cell->nextCell )
        queue->put( cell );                     // add to queue from list
}

bool NearSpansUnorderedComplete::atMatch() 
{
    int32_t minDoc = min()->doc();
    if( minDoc == max->doc())
    {
        int32_t minStart = min()->start();
        if( minStart > cachedStart || cachedDoc != minDoc )
        {
            int32_t matchSlop = max->end() - minStart - totalLength;
            if( matchSlop <= maxSlop && minSlop <= matchSlop )
            {
                prepareSpans();
                return true;
            }
        }
    }
    return false;
}

void NearSpansUnorderedComplete::prepareSpans() 
{
    cachedStart = min()->start();
    int32_t endMax = cachedStart + totalLength + maxSlop;
    int32_t endMin = max->end();
    bool minToo = false;

    cachedEnds.clear();
    if( cachedDoc != min()->doc() )
    {
        for( SpansCell * cell = first; cell; cell = cell->nextCell )
            cell->clearCache();
        
        cachedDoc = min()->doc();
    }

    cachedEnds.insert( endMin );
    for( SpansCell * cell = first; cell; cell = cell->nextCell )
    {
        if( cell != min())
        {
            if( cell->start() == cachedStart )
                minToo = true;

            cell->addEnds( endMin, endMax, cachedEnds );
        }
    }
    if( minToo )
    {
        min()->addEnds( endMin, endMax, cachedEnds );
    }

    iCachedEnds = cachedEnds.begin();
}

CL_NS_END2
