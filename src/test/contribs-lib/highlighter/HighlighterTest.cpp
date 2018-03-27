/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

#include "CLucene/analysis/CachingTokenFilter.h"
#include "CLucene/highlighter/SimpleHTMLFormatter.h"
#include "CLucene/highlighter/SimpleFragmenter.h"
#include "CLucene/highlighter/SpanHighlightScorer.h"
#include "CLucene/highlighter/Highlighter.h"
#include "CLucene/util/StringBuffer.h"
#include "CLucene/search/MultiPhraseQuery.h"
#include "CLucene/search/spans/SpanNearQuery.h"
#include "CLucene/search/spans/SpanNotQuery.h"
#include "CLucene/highlighter/WeightedSpanTerm.h"

#include "HighlighterTest.h"


/////////////////////////////////////////////////////////////////////////////
HelperFormatter::HelperFormatter()
{
}

/////////////////////////////////////////////////////////////////////////////
HelperFormatter::~HelperFormatter()
{
}

/////////////////////////////////////////////////////////////////////////////
TCHAR* HelperFormatter::highlightTerm( const TCHAR* originalTermText, const TokenGroup* tokenGroup )
{
    if( tokenGroup->getTotalScore() > 0)
    {
	    StringBuffer sb;
	    sb.append( _T( "<B>" ));
	    sb.append( originalTermText );
	    sb.append( _T( "</B>" ));

        numHighlights++;
        return sb.toString();
    }

    return stringDuplicate( originalTermText );
}

void HelperFormatter::reset()
{
    numHighlights = 0;
}


/////////////////////////////////////////////////////////////////////////////
const TCHAR* HighlighterTest::texts[] = {
        _T( "Hello this is a piece of text that is very long and contains too much preamble and the meat is really here which says kennedy has been shot" ),
        _T( "This piece of text refers to Kennedy at the beginning then has a longer piece of text that is very long in the middle and finally ends with another reference to Kennedy" ),
        _T( "JFK has been shot" ),
        _T( "John Kennedy has been shot" ),
        _T( "This text has a typo in referring to Keneddy" ),
        _T( "wordx wordy wordz wordx wordy wordx worda wordb wordy wordc" ),
        _T( "y z x y z a b" ),
        _T( "wx wa wa wa wa wa" ),
        _T( "wa wa wa wa wa wy" ),
        _T( "wz wz wz wz wz wz wz" ),
        _T( "wl wl wl wr wr wr" ),
        _T( "wo we wo we wo we" ),
        _T( "vx va va va va va aaaaa" ),
        _T( "vx va va va va va bbbbb" ),
        _T( "va va va va va vy aaaaa" ),
        _T( "va va va va va vy bbbbb" ),
        _T( "vz vz vz vz vz vz vz aaaaa" ),
        _T( "vz vz vz vz vz vz vz bbbbb" ),
        NULL
};

/////////////////////////////////////////////////////////////////////////////
HighlighterTest::HighlighterTest( CuTest* tc ) 
{
    this->tc = tc;
    reader   = NULL;
    ramDir   = NULL;
    query    = NULL;
    searcher = NULL;
    hits     = NULL;
    dir      = NULL;

    setUp();
}

/////////////////////////////////////////////////////////////////////////////
HighlighterTest::~HighlighterTest()
{
    tearDown();
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::setUp()
{
    ramDir = _CLNEW RAMDirectory();
    IndexWriter * writer = _CLNEW IndexWriter( ramDir, &analyzer, true );
    for( int i = 0; texts[ i ]; i++ ) 
    {
		Document * d = _CLNEW Document();
		d->add( *_CLNEW Field( FIELD_NAME, texts[ i ], Field::STORE_YES | Field::INDEX_TOKENIZED ));
		writer->addDocument( d );
		_CLDELETE(d);
	}

    writer->optimize();
    writer->close();
    _CLDELETE( writer );

   reader = IndexReader::open( ramDir );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::tearDown()
{
    _CLDELETE( query );
    _CLDELETE( hits );

    if( searcher )
    {
        searcher->close();
        _CLDELETE( searcher );
    }

    if( reader )
    {
        reader->close();
        _CLDELETE( reader );
    }

    if( ramDir )
    {
        ramDir->close();
        _CLDECDELETE( ramDir );
    }
}

/////////////////////////////////////////////////////////////////////////////
Token * HighlighterTest::createToken( const TCHAR * term, int32_t start, int32_t offset )
{
    Token * token = _CLNEW Token( term, start, offset );
    return token;
}

/////////////////////////////////////////////////////////////////////////////
TCHAR * HighlighterTest::highlightField( Query * query, const TCHAR * fieldName, const TCHAR * text )
{
    StringReader reader( text );
    CachingTokenFilter tokenStream( analyzer.tokenStream( fieldName, &reader ), true );
    SpanHighlightScorer spanScorer;
    spanScorer.init( query, fieldName, &tokenStream );
    Highlighter highlighter( &formatter, &spanScorer );
    tokenStream.reset();
    TCHAR * rv = highlighter.getBestFragments( &tokenStream, text, 1, _T( "(FIELD TEXT TRUNCATED)" ));
    if( ! rv || ! *rv )
    {
        if( rv )
            _CLDELETE_ARRAY( rv );
        rv = STRDUP_TtoT( text );
    }

    return rv;
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::doSearching( const TCHAR * queryString )
{
    QueryParser parser( FIELD_NAME, &analyzer );
    doSearching( parser.parse( queryString ) );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::doSearching( Query * unReWrittenQuery, bool bRememberRewritten )
{
    if( searcher )
    {
        searcher->close();
        _CLDELETE( searcher );
    }
    searcher = _CLNEW IndexSearcher( ramDir );

    // for any multi-term queries to work (prefix, wildcard, range,fuzzy etc) you must use a rewritten query!
    _CLDELETE( query );
    query = unReWrittenQuery->rewrite( reader );
    _CLDELETE( hits );
    hits = searcher->search( query, NULL );

    // remember query for highlighting
    if( unReWrittenQuery != query )
    {
        if( bRememberRewritten )
        {
            _CLDELETE( unReWrittenQuery );
        }
        else
        {
            _CLDELETE( query );
            query = unReWrittenQuery;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
SpanTermQuery * HighlighterTest::createSpanTermQuery( const TCHAR * fieldName, const TCHAR * text )
{
    Term * t = _CLNEW Term( fieldName, text );
    SpanTermQuery * pQuery = _CLNEW SpanTermQuery( t );
    _CLLDECDELETE( t );
    return pQuery;
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::assertHighlightAllHits( int32_t maxFragments, int32_t fragmentSize, int32_t expectedHighlights, bool autoRewriteQuery )
{
    formatter.reset();
    for( size_t i = 0; i < hits->length(); i++ )
    {
        const TCHAR * text = hits->doc( i ).get( FIELD_NAME );
        StringReader reader( text );
        CachingTokenFilter tokenStream( analyzer.tokenStream( FIELD_NAME, &reader ), true );
        SpanHighlightScorer spanScorer( autoRewriteQuery );
        spanScorer.init( query, FIELD_NAME, &tokenStream );
        Highlighter highlighter( &formatter, &spanScorer );
        SimpleFragmenter fragmenter( fragmentSize );
        highlighter.setTextFragmenter( &fragmenter );
        tokenStream.reset();
        TCHAR * hilited = highlighter.getBestFragments( &tokenStream, text, maxFragments, _T( "..." ));
        _CLDELETE_ARRAY( hilited );
    }
    assertTrueMsg( _T( "Failed to find correct number of highlights." ), formatter.numHighlights == expectedHighlights );
}


/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testSimpleSpanHighlighter()
{
    doSearching( _T( "Kennedy" ));
    assertHighlightAllHits( 2, 40, 4 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testSimpleSpanPhraseHighlighting()
{
    doSearching( _T( "\"very long and contains\"" ));
    assertHighlightAllHits( 2, 40, 3 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testSimpleSpanPhraseHighlighting2()
{
    doSearching( _T( "\"text piece long\"~5" ));
    assertHighlightAllHits( 2, 40, 6 );
    assertTrueMsg( _T( "Failed to find correct number of highlights." ), formatter.numHighlights == 6 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testSimpleSpanPhraseHighlighting3()
{
    doSearching( _T( "\"x y z\"" ));
    assertHighlightAllHits( 2, 40, 3 );
}

/////////////////////////////////////////////////////////////////////////////
// position sensitive query added after position insensitive query
void HighlighterTest::testPosTermStdTerm()
{
    doSearching( _T( "y \"x y z\"" ));
    assertHighlightAllHits( 2, 40, 4 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testSpanMultiPhraseQueryHighlighting()
{
    MultiPhraseQuery * mpq = _CLNEW MultiPhraseQuery();
    Term * t1 = _CLNEW Term( FIELD_NAME, _T( "wordx" ));
    Term * t2 = _CLNEW Term( FIELD_NAME, _T( "wordb" ));
    Term * t3 = _CLNEW Term( FIELD_NAME, _T( "wordy" ));
    
    CL_NS(util)::ValueArray<Term *> * prgTerms = _CLNEW CL_NS(util)::ValueArray<Term *>( 2 ); 
    prgTerms->values[ 0 ] = t1;
    prgTerms->values[ 1 ] = t2;

    mpq->add( prgTerms );
    mpq->add( t3 );

    _CLDELETE( prgTerms );
    _CLDECDELETE( t3 );
    _CLDECDELETE( t2 );
    _CLDECDELETE( t1 );

    doSearching( mpq );
    assertHighlightAllHits( 2, 40, 6 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testSpanMultiPhraseQueryHighlightingWithGap()
{
    MultiPhraseQuery * mpq = _CLNEW MultiPhraseQuery();
    Term * t1 = _CLNEW Term( FIELD_NAME, _T( "wordz" ));
    Term * t2 = _CLNEW Term( FIELD_NAME, _T( "wordx" ));
    
    CL_NS(util)::ValueArray<Term *> * prgTerms1 = _CLNEW CL_NS(util)::ValueArray<Term *>( 1 ); 
    prgTerms1->values[ 0 ] = t1;

    CL_NS(util)::ValueArray<Term *> * prgTerms2 = _CLNEW CL_NS(util)::ValueArray<Term *>( 1 ); 
    prgTerms2->values[ 0 ] = t2;

    mpq->add( prgTerms1, 2 );
    mpq->add( prgTerms2, 0 );

    // This query highlights wrong terms (wrong positions as the span highlights also term closer together)
//     mpq->add( prgTerms1, 0 );
//     mpq->add( prgTerms2, 3 );

    _CLDELETE( prgTerms1 );
    _CLDELETE( prgTerms2 );
    _CLDECDELETE( t1 );
    _CLDECDELETE( t2 );

    doSearching( mpq );
    assertHighlightAllHits( 2, 40, 2 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testNearSpanSimpleQuery()
{
    SpanTermQuery * rgSubQuery[ 2 ];
    rgSubQuery[ 0 ] = createSpanTermQuery( FIELD_NAME, _T( "beginning" ));
    rgSubQuery[ 1 ] = createSpanTermQuery( FIELD_NAME, _T( "kennedy" ));
    SpanNearQuery * query = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, 3, INT_MIN, false, true );
    doSearching( query );
    assertHighlightAllHits( 2, 40, 2 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testSpanHighlighting()
{
    SpanTermQuery * rgSubQuery[ 2 ];

    rgSubQuery[ 0 ] = createSpanTermQuery( FIELD_NAME, _T( "wordx" ));
    rgSubQuery[ 1 ] = createSpanTermQuery( FIELD_NAME, _T( "wordy" ));
    SpanNearQuery * query1 = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, 1, INT_MIN, false, true );

    rgSubQuery[ 0 ] = createSpanTermQuery( FIELD_NAME, _T( "wordy" ));
    rgSubQuery[ 1 ] = createSpanTermQuery( FIELD_NAME, _T( "wordc" ));
    SpanNearQuery * query2 = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, 1, INT_MIN, false, true );

    BooleanQuery * bquery = _CLNEW BooleanQuery();
    bquery->add( query1, true, BooleanClause::SHOULD );
    bquery->add( query2, true, BooleanClause::SHOULD );

    doSearching( bquery );
    assertHighlightAllHits( 2, 40, 7 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testNearSpanMinSlopQuery()
{
    SpanTermQuery * rgSubQuery[ 8 ];
    rgSubQuery[ 0 ] = createSpanTermQuery( FIELD_NAME, _T( "wx" ));
    rgSubQuery[ 1 ] = createSpanTermQuery( FIELD_NAME, _T( "wx" ));
    rgSubQuery[ 2 ] = createSpanTermQuery( FIELD_NAME, _T( "wa" ));
    rgSubQuery[ 3 ] = createSpanTermQuery( FIELD_NAME, _T( "wa" ));
    rgSubQuery[ 4 ] = createSpanTermQuery( FIELD_NAME, _T( "wy" ));
    rgSubQuery[ 5 ] = createSpanTermQuery( FIELD_NAME, _T( "wy" ));
    rgSubQuery[ 6 ] = createSpanTermQuery( FIELD_NAME, _T( "wz" ));
    rgSubQuery[ 7 ] = createSpanTermQuery( FIELD_NAME, _T( "wz" ));

    SpanNearQuery * rgSubNearQuery[ 12 ];
    rgSubNearQuery[ 0 ] = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, -1, -1, false, false );
    rgSubNearQuery[ 1 ] = _CLNEW SpanNearQuery( rgSubQuery+2, rgSubQuery+4, -1, -1, false, false );
    rgSubNearQuery[ 2 ] = _CLNEW SpanNearQuery( rgSubQuery+4, rgSubQuery+6, -1, -1, false, false );
    rgSubNearQuery[ 3 ] = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, -1, -1, false, false );
    rgSubNearQuery[ 4 ] = _CLNEW SpanNearQuery( rgSubQuery+2, rgSubQuery+4, 0, -1, false, false );
    rgSubNearQuery[ 5 ] = _CLNEW SpanNearQuery( rgSubQuery+4, rgSubQuery+6, -1, -1, false, false );
    rgSubNearQuery[ 6 ] = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, -1, -1, false, false );
    rgSubNearQuery[ 7 ] = _CLNEW SpanNearQuery( rgSubQuery+2, rgSubQuery+4, 1, -1, false, false );
    rgSubNearQuery[ 8 ] = _CLNEW SpanNearQuery( rgSubQuery+4, rgSubQuery+6, -1, -1, false, false );
    rgSubNearQuery[ 9 ] = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, -1, -1, false, false );
    rgSubNearQuery[ 10 ] = _CLNEW SpanNearQuery( rgSubQuery+2, rgSubQuery+4, 1, 1, false, false );
    rgSubNearQuery[ 11 ] = _CLNEW SpanNearQuery( rgSubQuery+4, rgSubQuery+6, -1, -1, false, false );

    for( int maxSlop = 0; maxSlop < 5; maxSlop++ )
    {
        for( int minSlop = 0; minSlop <= maxSlop; minSlop++ )
        {
            SpanNearQuery * queryWxWa = _CLNEW SpanNearQuery( rgSubQuery+1, rgSubQuery+3, maxSlop, minSlop, false, false );
            doSearching( queryWxWa );
            assertHighlightAllHits( 2, 40, maxSlop - minSlop + 2 );

            SpanNearQuery * queryW1xW1a = _CLNEW SpanNearQuery( rgSubNearQuery, rgSubNearQuery+2, maxSlop, minSlop, false, false );
            doSearching( queryW1xW1a );
            assertHighlightAllHits( 2, 40, maxSlop - minSlop + 2 );

            SpanNearQuery * queryW1xW2a = _CLNEW SpanNearQuery( rgSubNearQuery+3, rgSubNearQuery+5, maxSlop, minSlop, false, false );
            doSearching( queryW1xW2a );
            assertHighlightAllHits( 2, 40, 1 + min( 5, maxSlop + 2 ) - minSlop );

            SpanNearQuery * queryW1xW3a = _CLNEW SpanNearQuery( rgSubNearQuery+6, rgSubNearQuery+8, maxSlop, minSlop, false, false );
            doSearching( queryW1xW3a );
            assertHighlightAllHits( 2, 40, 1 + min( 5, maxSlop + 3 ) - minSlop );

            SpanNearQuery * queryW1xWa_a = _CLNEW SpanNearQuery( rgSubNearQuery+9, rgSubNearQuery+11, maxSlop, minSlop, false, false );
            doSearching( queryW1xWa_a );
            assertHighlightAllHits( 2, 40, minSlop == 2 ? 3 : (minSlop > 2 ? 0 : (minSlop == maxSlop ? ( 1 + min( 5, maxSlop + 2 ) - minSlop ) : ( 1 + min( 5, maxSlop + 3 ) - minSlop ))));

            SpanNearQuery * queryWaWy = _CLNEW SpanNearQuery( rgSubQuery+3, rgSubQuery+5, maxSlop, minSlop, false, false );
            doSearching( queryWaWy );
            assertHighlightAllHits( 2, 40, maxSlop - minSlop + 2 );

            SpanNearQuery * queryW1aW1y = _CLNEW SpanNearQuery( rgSubNearQuery+1, rgSubNearQuery+3, maxSlop, minSlop, false, false );
            doSearching( queryW1aW1y );
            assertHighlightAllHits( 2, 40, maxSlop - minSlop + 2 );

            SpanNearQuery * queryW2aW1y = _CLNEW SpanNearQuery( rgSubNearQuery+4, rgSubNearQuery+6, maxSlop, minSlop, false, false );
            doSearching( queryW2aW1y );
            assertHighlightAllHits( 2, 40,  1 + min( 5, maxSlop + 2 ) - minSlop );

            SpanNearQuery * queryW3aW1y = _CLNEW SpanNearQuery( rgSubNearQuery+7, rgSubNearQuery+9, maxSlop, minSlop, false, false );
            doSearching( queryW3aW1y );
            assertHighlightAllHits( 2, 40,  1 + min( 5, maxSlop + 3 ) - minSlop );

            SpanNearQuery * queryWa_aW1y = _CLNEW SpanNearQuery( rgSubNearQuery+10, rgSubNearQuery+12, maxSlop, minSlop, false, false );
            doSearching( queryWa_aW1y );
            assertHighlightAllHits( 2, 40, minSlop == 2 ? 3 : (minSlop > 2 ? 0 : (minSlop == maxSlop ? ( 1 + min( 5, maxSlop + 2 ) - minSlop ) : ( 1 + min( 5, maxSlop + 3 ) - minSlop ))));

            SpanNearQuery * queryWzWz = _CLNEW SpanNearQuery( rgSubQuery+6, rgSubQuery+8, maxSlop, minSlop, false, false );
            doSearching( queryWzWz );
            assertHighlightAllHits( 2, 40, min(7, ( 7 - minSlop - 1 ) * 2 ));
        }
    }

    _CLDELETE( rgSubNearQuery[ 0 ] );
    _CLDELETE( rgSubNearQuery[ 1 ] );
    _CLDELETE( rgSubNearQuery[ 2 ] );
    _CLDELETE( rgSubNearQuery[ 3 ] );
    _CLDELETE( rgSubNearQuery[ 4 ] );
    _CLDELETE( rgSubNearQuery[ 5 ] );
    _CLDELETE( rgSubNearQuery[ 6 ] );
    _CLDELETE( rgSubNearQuery[ 7 ] );
    _CLDELETE( rgSubNearQuery[ 8 ] );
    _CLDELETE( rgSubNearQuery[ 9 ] );
    _CLDELETE( rgSubNearQuery[ 10 ] );
    _CLDELETE( rgSubNearQuery[ 11 ] );

    _CLDELETE( rgSubQuery[ 0 ] );
    _CLDELETE( rgSubQuery[ 1 ] );
    _CLDELETE( rgSubQuery[ 2 ] );
    _CLDELETE( rgSubQuery[ 3 ] );
    _CLDELETE( rgSubQuery[ 4 ] );
    _CLDELETE( rgSubQuery[ 5 ] );
    _CLDELETE( rgSubQuery[ 6 ] );
    _CLDELETE( rgSubQuery[ 7 ] );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testNearSpanMinSlopQuery3()
{
    SpanTermQuery * rgSubQuery[ 8 ];
    rgSubQuery[ 0 ] = createSpanTermQuery( FIELD_NAME, _T( "vx" ));
    rgSubQuery[ 1 ] = createSpanTermQuery( FIELD_NAME, _T( "vx" ));
    rgSubQuery[ 2 ] = createSpanTermQuery( FIELD_NAME, _T( "va" ));
    rgSubQuery[ 3 ] = createSpanTermQuery( FIELD_NAME, _T( "va" ));
    rgSubQuery[ 4 ] = createSpanTermQuery( FIELD_NAME, _T( "vy" ));
    rgSubQuery[ 5 ] = createSpanTermQuery( FIELD_NAME, _T( "vy" ));
    rgSubQuery[ 6 ] = createSpanTermQuery( FIELD_NAME, _T( "vz" ));
    rgSubQuery[ 7 ] = createSpanTermQuery( FIELD_NAME, _T( "vz" ));

    SpanNearQuery * rgSubNearQuery[ 12 ];
    rgSubNearQuery[ 0 ] = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, -1, -1, false, false );
    rgSubNearQuery[ 1 ] = _CLNEW SpanNearQuery( rgSubQuery+2, rgSubQuery+4, -1, -1, false, false );
    rgSubNearQuery[ 2 ] = _CLNEW SpanNearQuery( rgSubQuery+4, rgSubQuery+6, -1, -1, false, false );
    rgSubNearQuery[ 3 ] = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, -1, -1, false, false );
    rgSubNearQuery[ 4 ] = _CLNEW SpanNearQuery( rgSubQuery+2, rgSubQuery+4, 0, -1, false, false );
    rgSubNearQuery[ 5 ] = _CLNEW SpanNearQuery( rgSubQuery+4, rgSubQuery+6, -1, -1, false, false );
    rgSubNearQuery[ 6 ] = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, -1, -1, false, false );
    rgSubNearQuery[ 7 ] = _CLNEW SpanNearQuery( rgSubQuery+2, rgSubQuery+4, 1, -1, false, false );
    rgSubNearQuery[ 8 ] = _CLNEW SpanNearQuery( rgSubQuery+4, rgSubQuery+6, -1, -1, false, false );
    rgSubNearQuery[ 9 ] = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, -1, -1, false, false );
    rgSubNearQuery[ 10 ] = _CLNEW SpanNearQuery( rgSubQuery+2, rgSubQuery+4, 1, 1, false, false );
    rgSubNearQuery[ 11 ] = _CLNEW SpanNearQuery( rgSubQuery+4, rgSubQuery+6, -1, -1, false, false );

    for( int maxSlop = 0; maxSlop < 5; maxSlop++ )
    {
        for( int minSlop = 0; minSlop <= maxSlop; minSlop++ )
        {
            SpanNearQuery * queryWxWa = _CLNEW SpanNearQuery( rgSubQuery+1, rgSubQuery+3, maxSlop, minSlop, false, false );
            doSearching( queryWxWa );
            assertHighlightAllHits( 2, 40, 2 * ( maxSlop - minSlop + 2 ));

            SpanNearQuery * queryW1xW1a = _CLNEW SpanNearQuery( rgSubNearQuery, rgSubNearQuery+2, maxSlop, minSlop, false, false );
            doSearching( queryW1xW1a );
            assertHighlightAllHits( 2, 40, 2 * ( maxSlop - minSlop + 2 ));

            SpanNearQuery * queryW1xW2a = _CLNEW SpanNearQuery( rgSubNearQuery+3, rgSubNearQuery+5, maxSlop, minSlop, false, false );
            doSearching( queryW1xW2a );
            assertHighlightAllHits( 2, 40, 2 * ( 1 + min( 5, maxSlop + 2 ) - minSlop ));

            SpanNearQuery * queryW1xW3a = _CLNEW SpanNearQuery( rgSubNearQuery+6, rgSubNearQuery+8, maxSlop, minSlop, false, false );
            doSearching( queryW1xW3a );
            assertHighlightAllHits( 2, 40, 2 * ( 1 + min( 5, maxSlop + 3 ) - minSlop ));

            SpanNearQuery * queryW1xWa_a = _CLNEW SpanNearQuery( rgSubNearQuery+9, rgSubNearQuery+11, maxSlop, minSlop, false, false );
            doSearching( queryW1xWa_a );
            assertHighlightAllHits( 2, 40, 2 * ( minSlop == 2 ? 3 : (minSlop > 2 ? 0 : (minSlop == maxSlop ? ( 1 + min( 5, maxSlop + 2 ) - minSlop ) : ( 1 + min( 5, maxSlop + 3 ) - minSlop )))));

            SpanNearQuery * queryWaWy = _CLNEW SpanNearQuery( rgSubQuery+3, rgSubQuery+5, maxSlop, minSlop, false, false );
            doSearching( queryWaWy );
            assertHighlightAllHits( 2, 40, 2 * ( maxSlop - minSlop + 2 ));

            SpanNearQuery * queryW1aW1y = _CLNEW SpanNearQuery( rgSubNearQuery+1, rgSubNearQuery+3, maxSlop, minSlop, false, false );
            doSearching( queryW1aW1y );
            assertHighlightAllHits( 2, 40, 2 * ( maxSlop - minSlop + 2 ));

            SpanNearQuery * queryW2aW1y = _CLNEW SpanNearQuery( rgSubNearQuery+4, rgSubNearQuery+6, maxSlop, minSlop, false, false );
            doSearching( queryW2aW1y );
            assertHighlightAllHits( 2, 40,  2 * ( 1 + min( 5, maxSlop + 2 ) - minSlop ));

            SpanNearQuery * queryW3aW1y = _CLNEW SpanNearQuery( rgSubNearQuery+7, rgSubNearQuery+9, maxSlop, minSlop, false, false );
            doSearching( queryW3aW1y );
            assertHighlightAllHits( 2, 40,  2 * ( 1 + min( 5, maxSlop + 3 ) - minSlop ));

            SpanNearQuery * queryWa_aW1y = _CLNEW SpanNearQuery( rgSubNearQuery+10, rgSubNearQuery+12, maxSlop, minSlop, false, false );
            doSearching( queryWa_aW1y );
            assertHighlightAllHits( 2, 40, 2 * ( minSlop == 2 ? 3 : (minSlop > 2 ? 0 : (minSlop == maxSlop ? ( 1 + min( 5, maxSlop + 2 ) - minSlop ) : ( 1 + min( 5, maxSlop + 3 ) - minSlop )))));

            SpanNearQuery * queryWzWz = _CLNEW SpanNearQuery( rgSubQuery+6, rgSubQuery+8, maxSlop, minSlop, false, false );
            doSearching( queryWzWz );
            assertHighlightAllHits( 2, 40, 2 * ( min(7, ( 7 - minSlop - 1 ) * 2 )));
        }
    }

    _CLDELETE( rgSubNearQuery[ 0 ] );
    _CLDELETE( rgSubNearQuery[ 1 ] );
    _CLDELETE( rgSubNearQuery[ 2 ] );
    _CLDELETE( rgSubNearQuery[ 3 ] );
    _CLDELETE( rgSubNearQuery[ 4 ] );
    _CLDELETE( rgSubNearQuery[ 5 ] );
    _CLDELETE( rgSubNearQuery[ 6 ] );
    _CLDELETE( rgSubNearQuery[ 7 ] );
    _CLDELETE( rgSubNearQuery[ 8 ] );
    _CLDELETE( rgSubNearQuery[ 9 ] );
    _CLDELETE( rgSubNearQuery[ 10 ] );
    _CLDELETE( rgSubNearQuery[ 11 ] );

    _CLDELETE( rgSubQuery[ 0 ] );
    _CLDELETE( rgSubQuery[ 1 ] );
    _CLDELETE( rgSubQuery[ 2 ] );
    _CLDELETE( rgSubQuery[ 3 ] );
    _CLDELETE( rgSubQuery[ 4 ] );
    _CLDELETE( rgSubQuery[ 5 ] );
    _CLDELETE( rgSubQuery[ 6 ] );
    _CLDELETE( rgSubQuery[ 7 ] );
}


/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testNearSpanMinSlopQuery2()
{
    SpanTermQuery * rgSubQuery[ 4 ];
    rgSubQuery[ 0 ] = createSpanTermQuery( FIELD_NAME, _T( "wl" ));
    rgSubQuery[ 1 ] = createSpanTermQuery( FIELD_NAME, _T( "wr" ));
    rgSubQuery[ 2 ] = createSpanTermQuery( FIELD_NAME, _T( "we" ));
    rgSubQuery[ 3 ] = createSpanTermQuery( FIELD_NAME, _T( "wo" ));

    for( int maxSlop = 0; maxSlop < 5; maxSlop++ )
    {
        for( int minSlop = 0; minSlop <= maxSlop; minSlop++ )
        {

            SpanNearQuery * queryWlWr = _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, maxSlop, minSlop, false, false );
            doSearching( queryWlWr );
            assertHighlightAllHits( 2, 40, min(2 * (maxSlop + 1), 6) - max(0, (minSlop-2) * 2));

            SpanNearQuery * queryWoWe = _CLNEW SpanNearQuery( rgSubQuery+2, rgSubQuery+4, maxSlop, minSlop, false, false );
            doSearching( queryWoWe );
            if( maxSlop == minSlop && (maxSlop % 2) == 1 )
                assertHighlightAllHits( 2, 40, 0);
            else if( minSlop == 3 || minSlop == 4 )
                assertHighlightAllHits( 2, 40, 2 );
            else
                assertHighlightAllHits( 2, 40, 6 );
        }
    }

    _CLDELETE( rgSubQuery[ 0 ] );
    _CLDELETE( rgSubQuery[ 1 ] );
    _CLDELETE( rgSubQuery[ 2 ] );
    _CLDELETE( rgSubQuery[ 3 ] );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testNotSpanSimpleQuery()
{
    SpanTermQuery * rgSubQuery[ 2 ];

    rgSubQuery[ 0 ] = createSpanTermQuery( FIELD_NAME, _T( "shot" ));
    rgSubQuery[ 1 ] = createSpanTermQuery( FIELD_NAME, _T( "kennedy" ));
    SpanNotQuery * query = _CLNEW SpanNotQuery( _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, 3, INT_MIN, false, true ),
                                                createSpanTermQuery( FIELD_NAME, _T( "john" )), true );
    doSearching( query );
    assertHighlightAllHits( 2, 40, 4 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetBestFragmentsSimpleQuery()
{
    doSearching( _T( "Kennedy" ));
    assertHighlightAllHits( 2, 40, 4 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetFuzzyFragments()
{
    doSearching( _T( "Kinnedy~" ));
//    assertHighlightAllHits( 2, 40, 5 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetWildCardFragments()
{
    doSearching( _T( "K?nnedy" ));
    assertHighlightAllHits( 2, 40, 4 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetMidWildCardFragments()
{
    doSearching( _T( "K*dy" ));
    assertHighlightAllHits( 2, 40, 5 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetRangeFragments()
{
    QueryParser parser( FIELD_NAME, &analyzer );
    parser.setUseOldRangeQuery( true );

    doSearching( parser.parse( FIELD_NAME _T( ":[kannedy TO kznnedy]" ) ));
    assertHighlightAllHits( 2, 40, 5 );

    doSearching( parser.parse( FIELD_NAME _T( ":[kannedy TO kznnedy]" )), false );
    assertHighlightAllHits( 2, 40, 5, true );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetConstantScoreRangeFragments()
{
    QueryParser parser( FIELD_NAME, &analyzer );
    Query * pQuery = parser.parse( FIELD_NAME _T( ":[kannedy TO kznnedy]" ));

    // Can't rewrite ConstantScoreRangeQuery if you want to highlight it (doSearching rewrites it and remembers the rewritten one)
    doSearching( pQuery, false );
    assertHighlightAllHits( 2, 40, 5, true );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetBestFragmentsPhrase()
{
    doSearching( _T( "\"John Kennedy\"" ));
    assertHighlightAllHits( 2, 40, 2 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetBestFragmentsSpan()
{
    SpanTermQuery * rgSubQuery[ 2 ];

    rgSubQuery[ 0 ] = createSpanTermQuery( _T( "contents" ), _T( "john" ));
    rgSubQuery[ 1 ] = createSpanTermQuery( _T( "contents" ), _T( "kennedy" ));
    doSearching( _CLNEW SpanNearQuery( rgSubQuery, rgSubQuery+2, 1, INT_MIN, false, true ));
    assertHighlightAllHits( 2, 40, 2 );
}

/////////////////////////////////////////////////////////////////////////////
// Filtered query not implemented?
// void HighlighterTest::testGetBestFragmentsFilteredQuery()
// {
//     RangeFilter * rf = _CLNEW RangeFilter( _T( "contents" ), _T( "john" ), _T( "john" ), true, true );
//     SpanQuery * clauses[ 2 ];
//     clauses[ 0 ] = createSpanTermQuery( _T( "contents" ), _T( "john" ));
//     clauses[ 1 ] = createSpanTermQuery( _T( "contents" ), _T( "kennedy" ));
// 
//     SpanNearQuery * snq = _CLNEW SpanNearQuery( clauses, clauses+2, 1, true, true );
//     FilteredQuery * fq = _CLNEW FilteredQuery( snq, rf );
// 
//     doSearching( fq );
//     assertHighlightAllHits( 2, 40, 2 );
// }
// 
// void HighlighterTest::testGetBestFragmentsFilteredPhraseQuery()
// {
//     RangeFilter * rf = _CLNEW RangeFilter( _T( "contents" ), _T( "john" ), _T( "john" ), true, true );
//     PhraseQuery * pq = _CLNEW PhraseQuery();
// 
//     Term * t1 = _CLNEW Term( _T( "contents" ), _T( "john" ));
//     Term * t2 = _CLNEW Term( _T( "contents" ), _T( "kennedy" ));
//     pq->add( t1 );
//     pq->add( t2 );
//     FilteredQuery * fq = new FilteredQuery( pq, rf );
// 
//     doSearching( fq );
//     assertHighlightAllHits( 2, 40, 2 );
// }

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetBestFragmentsMultiTerm()
{
    doSearching( _T( "John Kenn*" ));
    assertHighlightAllHits( 2, 40, 5 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetBestFragmentsWithOr()
{
    doSearching( _T( "JFK OR Kennedy" ));
    assertHighlightAllHits( 2, 40, 5 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetBestSingleFragment()
{
    doSearching( _T( "Kennedy" ));

    formatter.reset();
    for( size_t i = 0; i < hits->length(); i++ )
    {
        const TCHAR * text = hits->doc( i ).get( FIELD_NAME );
        StringReader reader( text );
        CachingTokenFilter tokenStream( analyzer.tokenStream( FIELD_NAME, &reader ), true );
        SpanHighlightScorer spanScorer;
        spanScorer.init( query, FIELD_NAME, &tokenStream );
        Highlighter highlighter( &formatter, &spanScorer );
        SimpleFragmenter fragmenter( 40 );
        highlighter.setTextFragmenter( &fragmenter );
        tokenStream.reset();
        TCHAR * hilited = highlighter.getBestFragment( &tokenStream, text );
        _CLDELETE_ARRAY( hilited );
    }
    assertTrueMsg( _T( "Failed to find correct number of highlights." ), formatter.numHighlights == 4 );

    formatter.reset();
    for( size_t i = 0; i < hits->length(); i++ )
    {
        const TCHAR * text = hits->doc( i ).get( FIELD_NAME );
        StringReader reader( text );
        CachingTokenFilter tokenStream( analyzer.tokenStream( FIELD_NAME, &reader ), true );
        SpanHighlightScorer spanScorer;
        spanScorer.init( query, FIELD_NAME, &tokenStream );
        Highlighter highlighter( &formatter, &spanScorer );
        SimpleFragmenter fragmenter( 40 );
        highlighter.setTextFragmenter( &fragmenter );
        tokenStream.reset();
        TCHAR * hilited = highlighter.getBestFragment( &analyzer, FIELD_NAME, text );
        _CLDELETE_ARRAY( hilited );
    }
    assertTrueMsg( _T( "Failed to find correct number of highlights." ), formatter.numHighlights == 4 );
 
//     formatter.reset();
//     for( size_t i = 0; i < hits->length(); i++ )
//     {
//         const TCHAR * text = hits->doc( i ).get( FIELD_NAME );
//         StringReader reader( text );
//         CachingTokenFilter tokenStream( analyzer.tokenStream( FIELD_NAME, &reader ), true );
//         SpanHighlightScorer spanScorer( query, FIELD_NAME, &tokenStream );
//         Highlighter highlighter( &formatter, &spanScorer );
//         SimpleFragmenter fragmenter( 40 );
//         highlighter.setTextFragmenter( &fragmenter );
//         tokenStream.reset();
//         TCHAR ** hilited = highlighter.getBestFragments( &analyzer, text, 10 );
//         _CLDELETE_CARRAY_ALL( hilited );
//     }
//     assertTrueMsg( _T( "Failed to find correct number of highlights." ), formatter.numHighlights == 4 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testGetBestSingleFragmentWithWeights()
{
    vector<WeightedSpanTerm::PositionSpan *> vSpans;
    WeightedSpanTerm * weightedTerms[ 2 ];

    vSpans.push_back( _CLNEW WeightedSpanTerm::PositionSpan( 0, 0, INT_MIN ));
    weightedTerms[ 0 ] = new WeightedSpanTerm( 10.0f, _T( "hello" ));
    weightedTerms[ 0 ]->addPositionSpans( vSpans );
    _CLLDECDELETE( vSpans[ 0 ] );

    vSpans[ 0 ] = _CLNEW WeightedSpanTerm::PositionSpan( 14, 14, INT_MIN );
    weightedTerms[ 1 ] = new WeightedSpanTerm( 1.0f, _T( "kennedy" ));
    weightedTerms[ 1 ]->addPositionSpans( vSpans );
    _CLLDECDELETE( vSpans[ 0 ] );

    {
        StringReader reader( texts[ 0 ] );
        CachingTokenFilter tokenStream( analyzer.tokenStream( FIELD_NAME, &reader ), true );
        SpanHighlightScorer spanScorer( weightedTerms, 2, false );
        Highlighter highlighter( &formatter, &spanScorer );
        SimpleFragmenter fragmenter( 2 );
        highlighter.setTextFragmenter( &fragmenter );
        tokenStream.reset();
        TCHAR * hilited = highlighter.getBestFragment( &tokenStream, texts[ 0 ] );
        assertTrueMsg( _T( "Failed to find best section using weighted terms." ), 0 == _tcscmp(  _T( "<B>Hello</B>" ), hilited ));
        _CLDELETE_ARRAY( hilited );
    }

    // readjust weights
    weightedTerms[ 1 ]->setWeight( 50.0f );
    {
        StringReader reader( texts[ 0 ] );
        CachingTokenFilter tokenStream( analyzer.tokenStream( FIELD_NAME, &reader ), true );
        SpanHighlightScorer spanScorer( weightedTerms, 2, true );
        Highlighter highlighter( &formatter, &spanScorer );
        SimpleFragmenter fragmenter( 2 );
        highlighter.setTextFragmenter( &fragmenter );
        tokenStream.reset();
        TCHAR * hilited = highlighter.getBestFragment( &tokenStream, texts[ 0 ] );
        assertTrueMsg( _T( "Failed to find best section using weighted terms." ), 0 == _tcscmp(  _T( " <B>kennedy</B>" ), hilited ));
        _CLDELETE_ARRAY( hilited );
    }
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testUnRewrittenQuery()
{
    QueryParser parser( FIELD_NAME, &analyzer );
    Query * pQuery = parser.parse( _T( "JF? or Kenned*" ));

    // Multiterm queries must be rewritten, otherwise they do not produce highlights.
    doSearching( pQuery, false );
    assertHighlightAllHits( 2, 40, 0 );
}

/////////////////////////////////////////////////////////////////////////////
void HighlighterTest::testNoFragments()
{
    doSearching( _T( "AnInvalidQueryWhichShouldYieldNoResults" ));
    formatter.reset();
    for( int i = 0; texts[ i ]; i++ ) 
    {
        StringReader reader( texts[ i ] );
        CachingTokenFilter tokenStream( analyzer.tokenStream( FIELD_NAME, &reader ), true );
        SpanHighlightScorer spanScorer;
        spanScorer.init( query, FIELD_NAME, &tokenStream );
        Highlighter highlighter( &formatter, &spanScorer );
        SimpleFragmenter fragmenter( 40 );
        highlighter.setTextFragmenter( &fragmenter );
        tokenStream.reset();
        TCHAR * hilited = highlighter.getBestFragments( &tokenStream, texts[ i ], 2, _T( "..." ));
        assertTrueMsg( _T( "Fragments should be empty." ), !*hilited ); // Java requires null.
        _CLDELETE_ARRAY( hilited );
    }
    assertTrueMsg( _T( "Failed to find correct number of highlights." ), formatter.numHighlights == 0 );
}



/////////////////////////////////////////////////////////////////////////////
void testHighlighter( CuTest* tc )
{
    HighlighterTest * pTest = _CLNEW HighlighterTest( tc );

    pTest->testSimpleSpanHighlighter();
    pTest->testSimpleSpanPhraseHighlighting();
    pTest->testSimpleSpanPhraseHighlighting2();
    pTest->testSimpleSpanPhraseHighlighting3();
    pTest->testPosTermStdTerm();
    pTest->testSpanMultiPhraseQueryHighlighting();
    pTest->testSpanMultiPhraseQueryHighlightingWithGap();
    pTest->testNearSpanSimpleQuery();
    pTest->testSpanHighlighting();
    pTest->testNotSpanSimpleQuery();
    pTest->testNearSpanMinSlopQuery();
    pTest->testNearSpanMinSlopQuery2();
    pTest->testNearSpanMinSlopQuery3();
    pTest->testGetBestFragmentsSimpleQuery();
    pTest->testGetFuzzyFragments();
    pTest->testGetWildCardFragments();
    pTest->testGetMidWildCardFragments();
    pTest->testGetRangeFragments();
    pTest->testGetConstantScoreRangeFragments();
    pTest->testGetBestFragmentsPhrase();
    pTest->testGetBestFragmentsSpan();
    pTest->testGetBestFragmentsMultiTerm();
    pTest->testGetBestFragmentsWithOr();
    pTest->testGetBestSingleFragment();
    pTest->testGetBestSingleFragmentWithWeights();
    pTest->testUnRewrittenQuery();
    pTest->testNoFragments();
    _CLDELETE( pTest );
}


/////////////////////////////////////////////////////////////////////////////
// Test suite for all tests of span queries
CuSuite *testHighlighter(void)
{
	CuSuite *suite = CuSuiteNew( _T( "CLucene Highlighter Tests" ));

    SUITE_ADD_TEST( suite, testHighlighter );

	return suite; 
}
