/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "../CheckHits.h"
#include "TestSpans.h"
#include "CLucene/search/spans/Spans.h"
#include "CLucene/search/spans/SpanNearQuery.h"
#include "CLucene/search/spans/SpanOrQuery.h"

const TCHAR * TestSpans::field = _T( "field" );
const TCHAR * TestSpans::docFields[] =
{
    _T( "w1 w2 w3 w4 w5" ),
    _T( "w1 w3 w2 w3" ),
    _T( "w1 xx w2 yy w3" ),
    _T( "w1 w3 xx w2 yy w3" ),
    _T( "u2 u2 u1" ),
    _T( "u2 xx u2 u1" ),
    _T( "u2 u2 xx u1" ),
    _T( "u2 xx u2 yy u1" ),
    _T( "u2 xx u1 u2" ),
    _T( "u2 u1 xx u2" ),
    _T( "u1 u2 xx u2" ),
    _T( "t1 t2 t1 t3 t2 t3" ),
    _T( "a1 b1 b1 b1 b1 b1" ),
    _T( "b2 b2 b2 b2 b2 a2" )
};

TestSpans::TestSpans( CuTest* tc )
{
    this->tc = tc;
    this->searcher = NULL;
    this->directory = NULL;
}

TestSpans::~TestSpans()
{
    if( searcher )
    {
        searcher->close();
        _CLDELETE( searcher );
    }

    if( directory )
    {
        directory->close();
        _CLDELETE( directory );
    }
}

void TestSpans::setUp()
{
    directory = _CLNEW RAMDirectory();
    Analyzer * analyzer = _CLNEW WhitespaceAnalyzer();
    IndexWriter * writer = _CLNEW IndexWriter( directory, analyzer, true );
    
    for( size_t i = 0; i < sizeof( docFields ) / sizeof( docFields[0] ); i++ )
    {
        Document doc;
        doc.add( * _CLNEW Field( field, docFields[ i ], Field::STORE_YES | Field::INDEX_TOKENIZED ));
        writer->addDocument( &doc );
    }
    
    writer->close();
    _CLDELETE( writer );
    _CLDELETE( analyzer );

    searcher = _CLNEW IndexSearcher( directory );
}

SpanTermQuery * TestSpans::makeSpanTermQuery( const TCHAR * text )
{
    Term * term = _CLNEW Term( field, text );
    SpanTermQuery * query = _CLNEW SpanTermQuery( term );
    _CLDECDELETE( term );
    return query;
}
  
void TestSpans::checkHits( Query * query, int32_t * results, size_t resultsCount )
{
    CheckHits::checkHits( tc, query, field, searcher, results, resultsCount );
}
  
void TestSpans::orderedSlopTest3SQ( SpanQuery * q1, SpanQuery * q2, SpanQuery * q3, int32_t slop, int32_t * expectedDocs, size_t expectedDocsCount )
{
    bool ordered = true;
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, 3 );
    clauses[ 0 ] = q1;
    clauses[ 1 ] = q2;
    clauses[ 2 ] = q3;

    SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+3, slop, INT_MIN, ordered, true );
    checkHits( snq, expectedDocs, expectedDocsCount );

    _CLLDELETE( snq );
    _CLDELETE_LARRAY( clauses );
}
  
void TestSpans::orderedSlopTest3( int32_t slop, int32_t * expectedDocs, size_t expectedDocsCount )
{
    orderedSlopTest3SQ( 
        makeSpanTermQuery( _T( "w1" )), 
        makeSpanTermQuery( _T( "w2" )), 
        makeSpanTermQuery( _T( "w3" )),
        slop,
        expectedDocs, 
        expectedDocsCount );
}
  
void TestSpans::orderedSlopTest3Equal( int32_t slop, int32_t * expectedDocs, size_t expectedDocsCount )
{
    orderedSlopTest3SQ(
        makeSpanTermQuery( _T( "w1" )),
        makeSpanTermQuery( _T( "w3" )),
        makeSpanTermQuery( _T( "w3" )),
        slop,
        expectedDocs,
        expectedDocsCount );
}
  
void TestSpans::orderedSlopTest1Equal( int32_t slop, int32_t * expectedDocs, size_t expectedDocsCount )
{
    orderedSlopTest3SQ(
        makeSpanTermQuery( _T( "u2" )),
        makeSpanTermQuery( _T( "u2" )),
        makeSpanTermQuery( _T( "u1" )),
        slop,
        expectedDocs,
        expectedDocsCount );
}

void TestSpans::testSpanNearOrdered()
{
    int32_t expectedDocs[] = { 0, 1, 2, 3 };

    orderedSlopTest3( 0, expectedDocs, 1 );
    orderedSlopTest3( 1, expectedDocs, 2 );
    orderedSlopTest3( 2, expectedDocs, 3 );
    orderedSlopTest3( 3, expectedDocs, 4 );
    orderedSlopTest3( 4, expectedDocs, 4 );
}
  
void TestSpans::testSpanNearOrderedEqual()
{
    int32_t expectedDocs[] = { 1, 3 };

    orderedSlopTest3Equal( 0, expectedDocs, 0 );
    orderedSlopTest3Equal( 1, expectedDocs, 1 );
    orderedSlopTest3Equal( 2, expectedDocs, 1 );
    orderedSlopTest3Equal( 3, expectedDocs, 2 );
}
  
void TestSpans::testSpanNearOrderedEqual1()
{
    int32_t expectedDocs[] = { 4, 5, 6, 7 };

    orderedSlopTest1Equal( 0, expectedDocs, 1 );
    orderedSlopTest1Equal( 0, expectedDocs, 1 );
    orderedSlopTest1Equal( 1, expectedDocs, 3 );
    orderedSlopTest1Equal( 2, expectedDocs, 4 );
    orderedSlopTest1Equal( 3, expectedDocs, 4 );
}

void TestSpans::testSpanNearOrderedOverlap()
{
    bool ordered = true;
    int32_t slop = 1;
    
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, 3 );
    clauses[ 0 ] = makeSpanTermQuery( _T( "t1" ));
    clauses[ 1 ] = makeSpanTermQuery( _T( "t2" ));
    clauses[ 2 ] = makeSpanTermQuery( _T( "t3" ));

    SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+3, slop, INT_MIN, ordered, true );
    Spans * spans = snq->getSpans( searcher->getReader(), false );

    assertTrueMsg( _T( "first range" ), spans->next() );
    assertEqualsMsg( _T( "first doc" ), 11, spans->doc());
    assertEqualsMsg( _T( "first start" ), 0, spans->start());
    assertEqualsMsg( _T( "first end" ), 4, spans->end());

    assertTrueMsg( _T( "second range" ), spans->next());
    assertEqualsMsg( _T( "second doc" ), 11, spans->doc());
    assertEqualsMsg( _T( "second start" ), 2, spans->start());
    assertEqualsMsg( _T( "second end" ), 6, spans->end());

    assertTrueMsg( _T( "third range" ), ! spans->next());

    _CLLDELETE( spans );
    _CLLDELETE( snq );
    _CLDELETE_LARRAY( clauses );
}

void TestSpans::orSpans( const TCHAR ** terms, size_t termsCount, Spans *& spans, Query *& query )
{
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, termsCount );
    
    for( size_t i = 0; i < termsCount; i++ ) 
        clauses[ i ] = makeSpanTermQuery( terms[ i ]);

    SpanOrQuery * soq = _CLNEW SpanOrQuery( clauses, clauses + termsCount, true );
    _CLDELETE_LARRAY( clauses );

    spans = soq->getSpans( searcher->getReader(), false );
    query = soq;
}

void TestSpans::tstNextSpans( Spans * spans, int32_t doc, int32_t start, int32_t end )
{
    assertTrueMsg( _T( "next" ), spans->next());
    assertEqualsMsg( _T( "doc" ), doc, spans->doc());
    assertEqualsMsg( _T( "start" ), start, spans->start());
    assertEqualsMsg( _T( "end" ), end, spans->end());
}

void TestSpans::testSpanOrEmpty()
{
    Spans * spans;
    Query * query;
    orSpans( NULL, 0, spans, query );
    assertTrueMsg( _T( "empty next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( query );
}

void TestSpans::testSpanOrSingle()
{
    Spans * spans;
    Query * query;
    const TCHAR* terms[] = { _T( "w5" ) };
    orSpans( terms, 1, spans, query );
    tstNextSpans( spans, 0, 4, 5 );
    assertTrueMsg( _T( "final next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( query );
}
  
void TestSpans::testSpanOrDouble()
{
    Spans * spans;
    Query * query;
    const TCHAR* terms[] = { _T( "w5" ), _T( "yy" ) };
    orSpans( terms, 2, spans, query );
    tstNextSpans( spans, 0, 4, 5 );
    tstNextSpans( spans, 2, 3, 4 );
    tstNextSpans( spans, 3, 4, 5 );
    tstNextSpans( spans, 7, 3, 4 );
    assertTrueMsg( _T( "final next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( query );
}

void TestSpans::testSpanOrDoubleSkip()
{
    Spans * spans;
    Query * query;
    const TCHAR* terms[] = { _T( "w5" ), _T( "yy" ) };
    orSpans( terms, 2, spans, query );
    assertTrueMsg( _T( "initial skipTo" ), spans->skipTo( 3 ));
    assertEqualsMsg( _T( "doc" ), 3, spans->doc() );
    assertEqualsMsg( _T( "start" ), 4, spans->start() );
    assertEqualsMsg( _T( "end" ), 5, spans->end() );
    tstNextSpans( spans, 7, 3, 4 );
    assertTrueMsg( _T( "final next" ), ! spans->next() );
    _CLLDELETE( spans );
    _CLLDELETE( query );
}

void TestSpans::testSpanOrUnused()
{
    Spans * spans;
    Query * query;
    const TCHAR* terms[] = { _T( "w5" ), _T( "unusedTerm" ), _T( "yy" ) };
    orSpans( terms, 3, spans, query );
    tstNextSpans( spans, 0, 4, 5 );
    tstNextSpans( spans, 2, 3, 4 );
    tstNextSpans( spans, 3, 4, 5 );
    tstNextSpans( spans, 7, 3, 4 );
    assertTrueMsg( _T( "final next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( query );
}

void TestSpans::testSpanOrTripleSameDoc()
{
    Spans * spans;
    Query * query;
    const TCHAR* terms[] = { _T( "t1" ), _T( "t2" ), _T( "t3" ) };
    orSpans( terms, 3, spans, query );
    tstNextSpans( spans, 11, 0, 1 );
    tstNextSpans( spans, 11, 1, 2 );
    tstNextSpans( spans, 11, 2, 3 );
    tstNextSpans( spans, 11, 3, 4 );
    tstNextSpans( spans, 11, 4, 5 );
    tstNextSpans( spans, 11, 5, 6 );
    assertTrueMsg( _T( "final next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( query );
}


void TestSpans::testSpanNearUnorderedComplete()
{
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, 2 );
    clauses[ 0 ] = makeSpanTermQuery( _T( "w2" ));
    clauses[ 1 ] = makeSpanTermQuery( _T( "w3" ));

    clauses[ 1 ] = _CLNEW SpanOrQuery( clauses, clauses+2, true );
    clauses[ 0 ] = makeSpanTermQuery( _T( "w1" ));

    SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, 10, INT_MIN, false, true );
    _CLDELETE_LARRAY( clauses );

    Spans * spans = snq->getSpans( searcher->getReader(), true );
    tstNextSpans( spans, 0, 0, 2 );
    tstNextSpans( spans, 0, 0, 3 );
    tstNextSpans( spans, 1, 0, 2 );
    tstNextSpans( spans, 1, 0, 3 );
    tstNextSpans( spans, 1, 0, 4 );
    tstNextSpans( spans, 2, 0, 3 );
    tstNextSpans( spans, 2, 0, 5 );
    tstNextSpans( spans, 3, 0, 2 );
    tstNextSpans( spans, 3, 0, 4 );
    tstNextSpans( spans, 3, 0, 6 );
    assertTrueMsg( _T( "final next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( snq );
}

void TestSpans::testSpanNearUnorderedComplete1()
{
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, 2 );
    clauses[ 0 ] = makeSpanTermQuery( _T( "w2" ));
    clauses[ 1 ] = makeSpanTermQuery( _T( "w3" ));

    clauses[ 1 ] = _CLNEW SpanOrQuery( clauses, clauses+2, true );
    clauses[ 0 ] = makeSpanTermQuery( _T( "w1" ));

    SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, 1, INT_MIN, false, true );
    _CLDELETE_LARRAY( clauses );

    Spans * spans = snq->getSpans( searcher->getReader(), true );
    tstNextSpans( spans, 0, 0, 2 );
    tstNextSpans( spans, 0, 0, 3 );
    tstNextSpans( spans, 1, 0, 2 );
    tstNextSpans( spans, 1, 0, 3 );
    tstNextSpans( spans, 2, 0, 3 );
    tstNextSpans( spans, 3, 0, 2 );
    assertTrueMsg( _T( "final next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( snq );
}

void TestSpans::testSpanNearUnorderedComplete2()
{
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, 2 );
    clauses[ 0 ] = makeSpanTermQuery( _T( "t2" ));
    clauses[ 1 ] = makeSpanTermQuery( _T( "t3" ));

    clauses[ 1 ] = _CLNEW SpanOrQuery( clauses, clauses+2, true );
    clauses[ 0 ] = makeSpanTermQuery( _T( "t1" ));

    SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, 10, INT_MIN, false, true );
    _CLDELETE_LARRAY( clauses );

    Spans * spans = snq->getSpans( searcher->getReader(), true );
    tstNextSpans( spans, 11, 0, 2 );
    tstNextSpans( spans, 11, 0, 4 );
    tstNextSpans( spans, 11, 0, 5 );
    tstNextSpans( spans, 11, 0, 6 );
    tstNextSpans( spans, 11, 1, 3 );
    tstNextSpans( spans, 11, 2, 4 );
    tstNextSpans( spans, 11, 2, 5 );
    tstNextSpans( spans, 11, 2, 6 );
    assertTrueMsg( _T( "final next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( snq );
}

void TestSpans::testSpanNearUnorderedComplete3()
{
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, 2 );
    clauses[ 0 ] = makeSpanTermQuery( _T( "t1" ));
    clauses[ 1 ] = makeSpanTermQuery( _T( "t2" ));

    clauses[ 1 ] = _CLNEW SpanOrQuery( clauses, clauses+2, true );
    clauses[ 0 ] = makeSpanTermQuery( _T( "t1" ));

    SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, 10, INT_MIN, false, true );
    _CLDELETE_LARRAY( clauses );

    Spans * spans = snq->getSpans( searcher->getReader(), true );
    tstNextSpans( spans, 11, 0, 1 );
    tstNextSpans( spans, 11, 0, 2 );
    tstNextSpans( spans, 11, 0, 3 );
    tstNextSpans( spans, 11, 0, 5 );
    tstNextSpans( spans, 11, 1, 3 );
    tstNextSpans( spans, 11, 2, 3 );
    tstNextSpans( spans, 11, 2, 5 );
    assertTrueMsg( _T( "final next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( snq );
}

void TestSpans::testSpanNearUnorderedComplete4()
{
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, 2 );
    clauses[ 0 ] = makeSpanTermQuery( _T( "a1" ));
    clauses[ 1 ] = makeSpanTermQuery( _T( "b1" ));

    {
        SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, 1, 1, false, false );
        Spans * spans = snq->getSpans( searcher->getReader(), true );
        tstNextSpans( spans, 12, 0, 3 );
        assertTrueMsg( _T( "final next" ), ! spans->next());
        _CLLDELETE( spans );
        _CLLDELETE( snq );
    }

    for( int maxSlop = 1; maxSlop < 5; maxSlop++ )
    {
        for( int minSlop = 0; minSlop <= maxSlop; minSlop++ )
        {
            SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, maxSlop, minSlop, false, minSlop == 4 );
            Spans * spans = snq->getSpans( searcher->getReader(), true );
            for( int span = minSlop+2; span <= maxSlop+2; span++ )
            {
                tstNextSpans( spans, 12, 0, span );
            }
            assertTrueMsg( _T( "final next" ), ! spans->next());
            _CLLDELETE( spans );
            _CLLDELETE( snq );
        }
    }

    _CLDELETE_LARRAY( clauses );
}
    
void TestSpans::testSpanNearUnorderedComplete5()
{
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, 2 );
    clauses[ 0 ] = makeSpanTermQuery( _T( "a2" ));
    clauses[ 1 ] = makeSpanTermQuery( _T( "b2" ));

    {
        SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, 1, 1, false, false );
        Spans * spans = snq->getSpans( searcher->getReader(), true );
        tstNextSpans( spans, 13, 3, 6 );
        assertTrueMsg( _T( "final next" ), ! spans->next());
        _CLLDELETE( spans );
        _CLLDELETE( snq );
    }

    for( int maxSlop = 1; maxSlop < 5; maxSlop++ )
    {
        for( int minSlop = 0; minSlop <= maxSlop; minSlop++ )
        {
            SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, maxSlop, minSlop, false, minSlop == 4 );
            Spans * spans = snq->getSpans( searcher->getReader(), true );
            for( int span = maxSlop+2; span >= minSlop+2; span-- )
            {
                tstNextSpans( spans, 13, 6-span, 6);
            }
            assertTrueMsg( _T( "final next" ), ! spans->next());
            _CLLDELETE( spans );
            _CLLDELETE( snq );
        }
    }

    _CLDELETE_LARRAY( clauses );
}

void TestSpans::testSpanNearOrderedComplete()
{
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, 3 );
    clauses[ 0 ] = makeSpanTermQuery( _T( "t1" ));
    clauses[ 1 ] = makeSpanTermQuery( _T( "t3" ));

    clauses[ 1 ] = _CLNEW SpanOrQuery( clauses, clauses+2, true );
    clauses[ 0 ] = makeSpanTermQuery( _T( "t2" ));

    SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, 10, INT_MIN, true, true );
    _CLDELETE_LARRAY( clauses );

    Spans * spans = snq->getSpans( searcher->getReader(), true );

    tstNextSpans(spans, 11, 1, 3);
    tstNextSpans(spans, 11, 1, 4);
    tstNextSpans(spans, 11, 1, 6);
    tstNextSpans(spans, 11, 4, 6);

    assertTrueMsg( _T( "final next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( snq );
}

void TestSpans::testSpanNearOrderedComplete1()
{
    SpanQuery ** clauses = _CL_NEWARRAY( SpanQuery *, 2 );
    clauses[ 0 ] = makeSpanTermQuery( _T( "w2" ));
    clauses[ 1 ] = makeSpanTermQuery( _T( "w3" ));

    clauses[ 1 ] = _CLNEW SpanOrQuery( clauses, clauses+2, true );
    clauses[ 0 ] = makeSpanTermQuery( _T( "w1" ));

    SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, 1, INT_MIN, true, true );
    _CLDELETE_LARRAY( clauses );

    Spans * spans = snq->getSpans( searcher->getReader(), true );

    tstNextSpans( spans, 0, 0, 2 );
    tstNextSpans( spans, 0, 0, 3 );
    tstNextSpans( spans, 1, 0, 2 );
    tstNextSpans( spans, 1, 0, 3 );
    tstNextSpans( spans, 2, 0, 3 );
    tstNextSpans( spans, 3, 0, 2 );

    assertTrueMsg( _T( "final next" ), ! spans->next());
    _CLLDELETE( spans );
    _CLLDELETE( snq );
}

void TestSpans::testSpanNearOrderedComplete2()
{
    SpanQuery** clauses = _CL_NEWARRAY(SpanQuery*, 2);
    clauses[0] = makeSpanTermQuery(_T("b1"));
    clauses[1] = makeSpanTermQuery(_T("a1"));

    SpanNearQuery* snq = _CLNEW SpanNearQuery(clauses, clauses + 2, 1, 1, true, true);
    Spans* spans = snq->getSpans(searcher->getReader(), true);
    assertTrueMsg(_T("final next"), !spans->next());
    _CLLDELETE(spans);
    _CLLDELETE(snq);
    _CLDELETE_LARRAY(clauses);
}

void TestSpans::testSpanNearOrderedComplete3()
{
    SpanQuery** clauses = _CL_NEWARRAY(SpanQuery*, 2);
    clauses[0] = makeSpanTermQuery(_T("a2"));
    clauses[1] = makeSpanTermQuery(_T("b2"));

    SpanNearQuery* snq = _CLNEW SpanNearQuery(clauses, clauses + 2, 1, 1, true, true);
    Spans* spans = snq->getSpans(searcher->getReader(), true);
    assertTrueMsg(_T("final next"), !spans->next());
    _CLLDELETE(spans);
    _CLLDELETE(snq);
    _CLDELETE_LARRAY(clauses);
}

void TestSpans::testSpanNearOrderedComplete4()
{
    SpanQuery** clauses = _CL_NEWARRAY(SpanQuery*, 2);
    clauses[0] = makeSpanTermQuery(_T("a1"));
    clauses[1] = makeSpanTermQuery(_T("b1"));

    {
        SpanNearQuery* snq = _CLNEW SpanNearQuery(clauses, clauses + 2, 1, 1, true, false);
        Spans* spans = snq->getSpans(searcher->getReader(), true);
        tstNextSpans(spans, 12, 0, 3);
        assertTrueMsg(_T("final next"), !spans->next());
        _CLLDELETE(spans);
        _CLLDELETE(snq);
    }

    for (int maxSlop = 1; maxSlop < 5; maxSlop++)
    {
        for (int minSlop = 0; minSlop <= maxSlop; minSlop++)
        {
            SpanNearQuery* snq = _CLNEW SpanNearQuery(clauses, clauses + 2, maxSlop, minSlop, true, minSlop == 4);
            Spans* spans = snq->getSpans(searcher->getReader(), true);
            for (int span = minSlop + 2; span <= maxSlop + 2; span++)
            {
                tstNextSpans(spans, 12, 0, span);
            }
            assertTrueMsg(_T("final next"), !spans->next());
            _CLLDELETE(spans);
            _CLLDELETE(snq);
        }
    }

    _CLDELETE_LARRAY(clauses);
}

void TestSpans::testSpanNearOrderedComplete5()
{
    SpanQuery** clauses = _CL_NEWARRAY(SpanQuery*, 2);
    clauses[0] = makeSpanTermQuery(_T("b2"));
    clauses[1] = makeSpanTermQuery(_T("a2"));

    {
        SpanNearQuery* snq = _CLNEW SpanNearQuery(clauses, clauses + 2, 1, 1, true, false);
        Spans* spans = snq->getSpans(searcher->getReader(), true);
        tstNextSpans(spans, 13, 3, 6);
        assertTrueMsg(_T("final next"), !spans->next());
        _CLLDELETE(spans);
        _CLLDELETE(snq);
    }

    for (int maxSlop = 1; maxSlop < 5; maxSlop++)
    {
        for (int minSlop = 0; minSlop <= maxSlop; minSlop++)
        {
            SpanNearQuery* snq = _CLNEW SpanNearQuery(clauses, clauses + 2, maxSlop, minSlop, true, minSlop == 4);
            Spans* spans = snq->getSpans(searcher->getReader(), true);
            for (int span = maxSlop + 2; span >= minSlop + 2; span--)
            {
                tstNextSpans(spans, 13, 6 - span, 6);
            }
            assertTrueMsg(_T("final next"), !spans->next());
            _CLLDELETE(spans);
            _CLLDELETE(snq);
        }
    }

    _CLDELETE_LARRAY(clauses);
}

void TestSpans::printSpans( Spans * spans )
{
    printf( "\n" );
    while( spans->next() )
    {
        printf( "[doc=%d, %d, %d]\n", spans->doc(), spans->start(), spans->end() );
    }
}
