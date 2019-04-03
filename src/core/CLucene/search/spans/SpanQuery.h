/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
 * 
 * Distributable under the terms of either the Apache License (Version 2.0) or 
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#ifndef _lucene_search_spans_SpanQuery_
#define _lucene_search_spans_SpanQuery_

#include "CLucene/search/Query.h"
#include "CLucene/search/spans/SpanWeight.h"
CL_CLASS_DEF2( search, spans, Spans )

CL_NS_DEF2( search, spans )

/** Base class for span-based queries. */
class CLUCENE_EXPORT SpanQuery : public CL_NS(search)::Query 
{
public:
    /** Expert: Returns the matches for this query in an index.  Used internally
     * to search for spans. */
    virtual Spans * getSpans( CL_NS(index)::IndexReader * reader, bool complete ) = 0;

    /** Returns the name of the field matched by this query.*/
    virtual const TCHAR* getField() const = 0;

    /** Returns a collection of all terms matched by this query.
    * @deprecated use extractTerms instead
    * @see Query#extractTerms(Set)
    */
//    public abstract Collection getTerms();

    Weight * _createWeight( CL_NS(search)::Searcher * searcher, CL_NS(search)::Similarity* similarity )
    {
        return _CLNEW SpanWeight( this, searcher, similarity );
    }

    /** Returns child queries or NULL */
    virtual SpanQuery ** getClauses() const = 0;

    /** Returns count of child queries */
    virtual size_t getClausesCount() const = 0;

    virtual void applyFieldRights( FieldFilter * pFilter )
    {
        size_t cnt = getClausesCount();
        SpanQuery ** clauses = getClauses();
        for( size_t i = 0; i < cnt; i++ )
        {
            clauses[ i ]->applyFieldRights( pFilter );
        }
    }
};

CL_NS_END2
#endif // _lucene_search_spans_SpanQuery_
