/**
 * Copyright 2002-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <limits.h> //MU: Linux - Include because of INT_MIN usage, should be the same as in tlcore prolog.h

#include "CLucene/_ApiHeader.h"

#include "CLucene/index/IndexReader.h"
#include "CLucene/index/IndexWriter.h"
#include "CLucene/index/Terms.h"
#include "CLucene/index/Term.h"

#include "CLucene/document/Document.h"

#include "CLucene/analysis/Analyzers.h"
// #include "CLucene/analysis/CachingTokenFilter.h"

#include "CLucene/search/Query.h"
#include "CLucene/search/BooleanQuery.h"
#include "CLucene/search/BooleanClause.h"
#include "CLucene/search/PhraseQuery.h"
#include "CLucene/search/TermQuery.h"
#include "CLucene/search/MultiPhraseQuery.h"
#include "CLucene/search/ConstantScoreQuery.h"
#include "CLucene/search/FuzzyQuery.h"
#include "CLucene/search/WildcardQuery.h"
#include "CLucene/search/PrefixQuery.h"
#include "CLucene/search/RangeQuery.h"
#include "CLucene/search/MatchAllDocsQuery.h"

#include "CLucene/search/spans/Spans.h"
#include "CLucene/search/spans/SpanTermQuery.h"
#include "CLucene/search/spans/SpanNearQuery.h"
#include "CLucene/search/spans/SpanNotQuery.h"
#include "CLucene/search/spans/SpanFirstQuery.h"
#include "CLucene/search/spans/SpanOrQuery.h"

#include "CLucene/store/RAMDirectory.h"

#include "CLucene/util/_Arrays.h"

#include "WeightedSpanTermExtractor.h"
#include "WeightedSpanTerm.h"

CL_NS_USE(index);
CL_NS_USE(analysis);
CL_NS_USE(document);
CL_NS_USE(search);
CL_NS_USE2(search,spans);
CL_NS_USE(store);
CL_NS_USE(util);


CL_NS_DEF2(search,highlight)


/////////////////////////////////////////////////////////////////////////////
WeightedSpanTermExtractor::PositionCheckingMap::PositionCheckingMap( WeightedSpanTermMap& weightedSpanTerms )
: mapSpanTerms( weightedSpanTerms )
{
}

WeightedSpanTermExtractor::PositionCheckingMap::~PositionCheckingMap()
{
}

void WeightedSpanTermExtractor::PositionCheckingMap::putAll( WeightedSpanTermExtractor::PositionCheckingMap * m )
{
    for( WeightedSpanTermMap::iterator iST = m->mapSpanTerms.begin(); iST != m->mapSpanTerms.end(); iST++ )
        put( iST->second );
}

void WeightedSpanTermExtractor::PositionCheckingMap::put( WeightedSpanTerm * spanTerm )
{
    const TCHAR * term = spanTerm->getTerm();
    WeightedSpanTermMap::iterator iST = mapSpanTerms.find( term );
    if( iST != mapSpanTerms.end() )
    {
        if( ! spanTerm->isPositionSensitive() )
            iST->second->setPositionSensitive( false );  

        _CLDELETE( spanTerm );
    }
    else
    {
        mapSpanTerms[ term ] = spanTerm;
    }
}

WeightedSpanTerm * WeightedSpanTermExtractor::PositionCheckingMap::get( const TCHAR * term )
{
    WeightedSpanTermMap::iterator iST = mapSpanTerms.find( term );
    if( iST != mapSpanTerms.end() )
        return iST->second;
    else
        return NULL;
}

/////////////////////////////////////////////////////////////////////////////
WeightedSpanTermExtractor::Context::Context()
{
}

WeightedSpanTermExtractor::Context::~Context()
{
    for( auto cachedQuery : cachedQueries )
    {
        if( cachedQuery.first != cachedQuery.second )
            freeQueryImpl( cachedQuery.second );
    }
    cachedQueries.clear();

    for( auto cachedWeight : cachedWeights )
    {
        freeWeightImpl( cachedWeight.second );
    }
    cachedWeights.clear();

    for( auto cachedScorer : cachedScorers )
    {
        freeScorerImpl( cachedScorer.second );
    }
    cachedScorers.clear();

    for( auto cachedTermDoc : cachedTermDocs )
    {
        freeTermDocsImpl( cachedTermDoc.second );
    }
    cachedTermDocs.clear();

    for( auto cachedTermSet : cachedTermSets )
    {
        freeTermSetImpl( cachedTermSet.second );
    }
    cachedTermSets.clear();

    for( auto cachedSpan : cachedSpans )
    {
        freeSpansImpl( cachedSpan.second );
    }
    cachedSpans.clear();
}

void WeightedSpanTermExtractor::Context::freeQueryImpl( CL_NS(search)::Query * pQuery )
{
    _CLLDELETE( pQuery );
}

void WeightedSpanTermExtractor::Context::freeWeightImpl( CL_NS(search)::Weight * pWeight )
{
    _CLLDELETE( pWeight );
}

void WeightedSpanTermExtractor::Context::freeScorerImpl( CL_NS(search)::Scorer * pScorer )
{
    _CLLDELETE( pScorer );
}

void WeightedSpanTermExtractor::Context::freeSpansImpl( CL_NS2(search,spans)::Spans * pSpans )
{
    _CLLDELETE( pSpans );
}

void WeightedSpanTermExtractor::Context::freeTermDocsImpl( CL_NS(index)::TermDocs * pTermDocs )
{
    pTermDocs->close();
    _CLLDELETE( pTermDocs );
}

void WeightedSpanTermExtractor::Context::freeTermSetImpl(  CL_NS(search)::TermSet * pTermSet )
{
    for( TermSet::iterator itTerms = pTermSet->begin(); itTerms != pTermSet->end(); itTerms++ )
    {
        Term * pTerm = *itTerms;
        _CLLDECDELETE( pTerm );
    }
    pTermSet->clear();
    _CLLDELETE( pTermSet );
}

/////////////////////////////////////////////////////////////////////////////
WeightedSpanTermExtractor::WeightedSpanTermExtractor( bool bAutoRewriteQueries )
{
    m_bAutoRewriteQueries    = bAutoRewriteQueries;
    
    m_tszFieldName           = NULL;
    m_pTokenStream           = NULL;
    m_pReader                = NULL;
    m_pFieldReader           = NULL;
    m_nDocId                 = -1;

    m_bScoreTerms            = false;
    m_bExactTermSpans        = false;

    m_pContext               = NULL;
}

WeightedSpanTermExtractor::~WeightedSpanTermExtractor()
{
    closeContext();
    closeFieldReader();
}

void WeightedSpanTermExtractor::setIndexReader( CL_NS(index)::IndexReader * pReader )
{
    m_pReader = pReader;
}

void WeightedSpanTermExtractor::createContext()
{
    closeContext();
    m_pContext = _CLNEW Context();
}

void WeightedSpanTermExtractor::closeContext()
{
    if( m_pContext )
    {
        _CLDELETE( m_pContext );
    }
}

void WeightedSpanTermExtractor::setScoreTerms( bool bScoreTerms )
{
    m_bScoreTerms = bScoreTerms;
}

bool WeightedSpanTermExtractor::scoreTerms()
{
    return m_bScoreTerms;
}

void WeightedSpanTermExtractor::setAutoRewriteQueries( bool bAutoRewriteQueries )
{
    m_bAutoRewriteQueries = bAutoRewriteQueries;
}

bool WeightedSpanTermExtractor::autoRewriteQueries()
{
    return m_bAutoRewriteQueries;
}

void WeightedSpanTermExtractor::setExtractExactTermSpans( bool bExactTermSpans )
{
    m_bExactTermSpans = bExactTermSpans;
}

bool WeightedSpanTermExtractor::exactTermSpans()
{
    return m_bExactTermSpans;
}

void WeightedSpanTermExtractor::extractWeightedSpanTerms( WeightedSpanTermMap& weightedSpanTerms, CL_NS(search)::Query * pQuery, const TCHAR * tszFieldName, CL_NS(analysis)::TokenStream * pTokenStream, int32_t nDocId )
{
    if( !m_pContext )
        m_pContext = _CLNEW DummyContext();

    m_tszFieldName = tszFieldName;
    m_pTokenStream = pTokenStream;
    m_nDocId       = nDocId;

    WeightedSpanTermExtractor::PositionCheckingMap terms( weightedSpanTerms );
    WeightedSpanTerm::PositionSpans wholeDocSpan;
    extract( pQuery, wholeDocSpan, terms );
    
    closeFieldReader();
    
    m_tszFieldName = NULL;
    m_pTokenStream = NULL;
    m_nDocId       = -1;

    if( m_bScoreTerms && m_pReader )
    {
        int32_t nTotalNumDocs = m_pReader->numDocs();
        Term *  pTerm = _CLNEW Term();
        for( WeightedSpanTermMap::iterator iSpanTerm = weightedSpanTerms.begin(); iSpanTerm != weightedSpanTerms.end(); iSpanTerm++ )
        {
            pTerm->set( tszFieldName, iSpanTerm->first );
            int32_t nDocFreq = m_pReader->docFreq( pTerm );

            if( nTotalNumDocs < nDocFreq )
                nDocFreq = nTotalNumDocs;

            // IDF algorithm taken from DefaultSimilarity class
            float_t idf = (float_t)( log( (float_t)nTotalNumDocs / (float_t)(nDocFreq + 1) ) + 1.0f );
            iSpanTerm->second->setWeight( iSpanTerm->second->getWeight() * idf );
        }
    }
}

bool WeightedSpanTermExtractor::matchesField( const TCHAR * tszFieldNameToCheck )
{
    return ( tszFieldNameToCheck == m_tszFieldName || 0 == _tcscmp( tszFieldNameToCheck, m_tszFieldName ));
}

void WeightedSpanTermExtractor::extract( Query * pQuery, WeightedSpanTerm::PositionSpans& spans, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    if( pQuery->instanceOf( TermQuery::getClassName() ))
    {
        extractWeightedTerms( pQuery, spans, terms );
    }
    else if( pQuery->instanceOf( SpanTermQuery::getClassName() ))
    {
        extractWeightedSpanTerms( (SpanQuery *) pQuery, spans, terms );
    }
    else if( pQuery->instanceOf( BooleanQuery::getClassName() ))
    {
        extractFromBooleanQuery( (BooleanQuery *) pQuery, spans, terms );
    }
    else if( pQuery->instanceOf( PhraseQuery::getClassName() )) 
    {
        extractFromPhraseQuery( (PhraseQuery *) pQuery, spans, terms );
    }
    else if( pQuery->instanceOf( SpanNearQuery::getClassName() ))
    {
        extractFromSpanQuery( (SpanQuery *) pQuery, ((SpanNearQuery *) pQuery)->getMinSlop(), spans, terms );
    } 
    else if( pQuery->instanceOf( SpanFirstQuery::getClassName() )
          || pQuery->instanceOf( SpanNotQuery::getClassName() )
          || pQuery->instanceOf( SpanOrQuery::getClassName() ))
    {
        extractFromSpanQuery( (SpanQuery *) pQuery, 0, spans, terms );
    } 
    else if( pQuery->instanceOf( MultiPhraseQuery::getClassName() )) 
    {
        extractFromMultiPhraseQuery( (MultiPhraseQuery *) pQuery, spans, terms );
    }
    else if( pQuery->instanceOf( ConstantScoreRangeQuery::getClassName() ))
    {
        extractFromConstantScoreRangeQuery( (ConstantScoreRangeQuery *) pQuery, spans, terms );
    }
    else if( pQuery->instanceOf( FuzzyQuery::getClassName() )
          || pQuery->instanceOf( PrefixQuery::getClassName() )
          || pQuery->instanceOf( RangeQuery::getClassName() )
          || pQuery->instanceOf( WildcardQuery::getClassName() ))
    { 
        rewriteAndExtract( pQuery, spans, terms );
    }
    else if( ! pQuery->instanceOf( MatchAllDocsQuery::getClassName() )
          && ! pQuery->instanceOf( ConstantScoreQuery::getClassName() ))
    
    {
        extractFromCustomQuery( pQuery, spans, terms );
    }
}

void WeightedSpanTermExtractor::extractFromBooleanQuery( BooleanQuery * pQuery, WeightedSpanTerm::PositionSpans& spans, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    int32_t nClauses = (int32_t) pQuery->getClauseCount();
    if( nClauses > 0 )
    {
        BooleanClause ** rgQueryClauses = _CL_NEWARRAY( BooleanClause*, nClauses );
        pQuery->getClauses( rgQueryClauses );

        for( int32_t i = 0; i < nClauses; i++ )
        {
            if( ! rgQueryClauses[ i ]->isProhibited() )
                extract( rgQueryClauses[ i ]->getQuery(), spans, terms );
        }
        _CLDELETE_ARRAY( rgQueryClauses );
    }
} 

void WeightedSpanTermExtractor::extractFromPhraseQuery( PhraseQuery * pQuery, WeightedSpanTerm::PositionSpans& spans, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    SpanNearQuery * pNearQuery = (SpanNearQuery *) m_pContext->getQuery( pQuery );
    if( ! pNearQuery )
    {
        Term** rgPhraseQueryTerms = pQuery->getTerms();
        vector<SpanQuery *> vSpans;
        for( int32_t i = 0; rgPhraseQueryTerms[ i ]; i++ )
            vSpans.push_back( _CLNEW SpanTermQuery( rgPhraseQueryTerms[ i ] ));

        int32_t nSlop = pQuery->getSlop();
        bool bInOrder = ( nSlop == 0 );

        pNearQuery = _CLNEW SpanNearQuery( vSpans.begin(), vSpans.end(), nSlop, INT_MIN, bInOrder, true );
        pNearQuery->setBoost( pQuery->getBoost() );

        _CLDELETE_ARRAY( rgPhraseQueryTerms );
        m_pContext->putQuery( pQuery, pNearQuery );
    }

    extractFromSpanQuery( pNearQuery, 0, spans, terms );
    m_pContext->freeQuery( pNearQuery );
} 

void WeightedSpanTermExtractor::extractFromMultiPhraseQuery( MultiPhraseQuery * pQuery, WeightedSpanTerm::PositionSpans& spans, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    SpanNearQuery * pNearQuery = (SpanNearQuery *) m_pContext->getQuery( pQuery );
    if( ! pNearQuery )
    {
        const CLArrayList<ArrayBase<Term*>*>* termArrays   = NULL;
        ValueArray<int32_t>                   positions;

        termArrays = pQuery->getTermArrays();
        pQuery->getPositions( positions );

        if( positions.length > 0 )
        {
            int32_t nMaxPosition = positions[ positions.length - 1 ];
            for( size_t i = 0; i < positions.length - 1; i++ )
            {
                if( positions[ i ] > nMaxPosition )
                    nMaxPosition = positions[ i ];
            }

            vector<SpanTermQuery *>** disjunctLists = _CL_NEWARRAY( vector<SpanTermQuery *>*, nMaxPosition + 1 );
            memset( disjunctLists, 0, sizeof( vector<SpanTermQuery *>* ) * ( nMaxPosition + 1 ) );
            int32_t nDistinctPositions = 0;
            for( size_t i = 0; i < termArrays->size(); i++ )
            {
                ArrayBase<Term*> * termArray = termArrays->at( i );
                if( ! disjunctLists[ positions[ i ]] )
                    disjunctLists[ positions[ i ]] = _CLNEW vector<SpanTermQuery *>;
                vector<SpanTermQuery *>& disjuncts = *( disjunctLists[ positions[ i ]] );
                if( disjuncts.empty() )
                    nDistinctPositions++;
    
                for( size_t j = 0; j < termArray->length; j++ )
                    disjuncts.push_back( _CLNEW SpanTermQuery( (*termArray)[ j ] ));
            }

            int32_t nPositionGaps = 0;
            int32_t nPosition = 0;
            SpanQuery** rgClauses = _CL_NEWARRAY( SpanQuery*, nDistinctPositions );
            for( int32_t i = 0; i <= nMaxPosition; i++ )
            {
                vector<SpanTermQuery *> * pDisjuncts = disjunctLists[ i ];
                if( pDisjuncts && ! pDisjuncts->empty() )
                    rgClauses[ nPosition++ ] = _CLNEW SpanOrQuery( pDisjuncts->begin(), pDisjuncts->end(), true );
                else
                    nPositionGaps++;
            }

            int32_t nSlop = pQuery->getSlop();
            bool bInOrder = ( nSlop == 0 );

            pNearQuery = _CLNEW SpanNearQuery( rgClauses, rgClauses + nDistinctPositions, nSlop + nPositionGaps, INT_MIN, bInOrder, true );
            pNearQuery->setBoost( pQuery->getBoost() );

            _CLDELETE_ARRAY( rgClauses );
            for( int32_t i = 0; i <= nMaxPosition; i++ )
                _CLDELETE( disjunctLists[ i ] );
            _CLDELETE_ARRAY( disjunctLists ); 

            m_pContext->putQuery( pQuery, pNearQuery );
        }
    }

    if( pNearQuery )
    {
        extractFromSpanQuery( pNearQuery, 0, spans, terms );
        m_pContext->freeQuery( pNearQuery );
    }
} 

void WeightedSpanTermExtractor::extractFromConstantScoreRangeQuery( ConstantScoreRangeQuery * pQuery, WeightedSpanTerm::PositionSpans& spans, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    if( m_bAutoRewriteQueries && matchesField( pQuery->getField() ))
    {
        TermSet * pTermSet = m_pContext->getTermSet( pQuery );
        if( ! pTermSet )
        {
            Term * pLower = _CLNEW Term( m_tszFieldName, pQuery->getLowerVal() );
            Term * pUpper = _CLNEW Term( m_tszFieldName, pQuery->getUpperVal() );

            IndexReader * pReader = getFieldReader();
            try 
            {
                TermEnum * pTermEnum = pReader->terms( pLower );
                pTermSet = _CLNEW TermSet();
                do 
                {
                    Term * pTerm = pTermEnum->term( true );
                    if( pTerm && pUpper->compareTo( pTerm ) >= 0 ) {
                        pTermSet->insert( pTerm );
                    }
                    else {
                        _CLLDECDELETE( pTerm );
                        break;
                    }
                }
                while( pTermEnum->next() );

                 m_pContext->putTermSet( pQuery, pTermSet );
                _CLLDELETE( pTermEnum );
            }
            catch( ... )
            {
                _CLLDECDELETE( pUpper );
                _CLLDECDELETE( pLower );
                throw;
            }
            _CLLDECDELETE( pUpper );
            _CLLDECDELETE( pLower );
        }

        processNonWeightedTerms( terms, *pTermSet, pQuery->getBoost(), spans );
        m_pContext->freeTermSet( pTermSet );
    }
}

void WeightedSpanTermExtractor::rewriteAndExtract( Query * pQuery, WeightedSpanTerm::PositionSpans& spans, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    if( m_bAutoRewriteQueries )
    {
        Query * pRewrittenQuery = m_pContext->getQuery( pQuery );
        if( ! pRewrittenQuery )
        {
            IndexReader * pReader = getFieldReader();
            pRewrittenQuery = pQuery->rewrite( pReader );
            m_pContext->putQuery( pQuery, pRewrittenQuery );
        }

        if( pRewrittenQuery && pRewrittenQuery != pQuery )
        {
            try
            {
                extract( pRewrittenQuery, spans, terms );
                m_pContext->freeQuery( pRewrittenQuery );
            }
            catch( ... )
            {
                m_pContext->freeQuery( pRewrittenQuery );
                throw;
            }
        }
    }
}

void WeightedSpanTermExtractor::extractFromCustomQuery( Query * pQuery, WeightedSpanTerm::PositionSpans& spans, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
}

bool WeightedSpanTermExtractor::spanMatchesPositionSpans(Spans * pSpans, WeightedSpanTerm::PositionSpans& spans )
{
    if( spans.empty() )
        return true;

    for( WeightedSpanTerm::PositionSpans::iterator iSP = spans.begin(); iSP != spans.end(); iSP++ )
    {
        WeightedSpanTerm::PositionSpan * span = *iSP;
        if( span->start <= pSpans->start() )
        {
            if( pSpans->end() <= ( span->end + 1 ))
                return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

void WeightedSpanTermExtractor::extractFromSpanQuery( CL_NS2(search,spans)::SpanQuery * pSpanQuery, int32_t minSlop, WeightedSpanTerm::PositionSpans& spans, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    Spans * pSpans = m_pContext->getSpans( pSpanQuery );
    if( ! pSpans )
    {
        pSpans = _CLNEW GuardedSpans( pSpanQuery->getSpans( getFieldReader(), true ) );
        m_pContext->putSpans( pSpanQuery, pSpans );
    }

    int32_t spanLenMin = minSlop > 0 ? minSlop + (int32_t) pSpanQuery->getClausesCount() - 1 : 0;
    int32_t spanLenMark = spanLenMin < 0 ? 0 : spanLenMin;
    if( !spans.empty() && (*(spans.begin()))->minlen > 0 )
        spanLenMark = INT_MIN;

    vector<WeightedSpanTerm::PositionSpan *> spanPositions;
    if( m_nDocId == -1 )
    {
        while( pSpans->next() )
        {
            if( spanMatchesPositionSpans( pSpans, spans ))
                spanPositions.push_back( _CLNEW WeightedSpanTerm::PositionSpan( pSpans->start(), pSpans->end() - 1, spanLenMark ));
        }
    }
    else
    {
        if( pSpans->doc() < m_nDocId )
            pSpans->skipTo( m_nDocId );
        if( pSpans->doc() == m_nDocId  )
        {
            if( spanMatchesPositionSpans( pSpans, spans ))
                spanPositions.push_back( _CLNEW WeightedSpanTerm::PositionSpan( pSpans->start(), pSpans->end() - 1, spanLenMark ));

            while( pSpans->next() && pSpans->doc() == m_nDocId )
            {
                if( spanMatchesPositionSpans( pSpans, spans ))
                    spanPositions.push_back( _CLNEW WeightedSpanTerm::PositionSpan( pSpans->start(), pSpans->end() - 1, spanLenMark ));
            }
        }
    }
    m_pContext->freeSpans( pSpans );
    pSpans = NULL;

    if( spanPositions.size() > 0 ) 
    {
        if( spanLenMark == INT_MIN )
        {
            vector<WeightedSpanTerm::PositionSpan *> localSpans;
            for( WeightedSpanTerm::PositionSpans::iterator iSpan = spans.begin(); iSpan != spans.end(); iSpan++ )
            {
                WeightedSpanTerm::PositionSpan * span = *iSpan;

                localSpans.clear();
                for( vector<WeightedSpanTerm::PositionSpan *>::iterator iSpanPos = spanPositions.begin(); iSpanPos != spanPositions.end(); iSpanPos++ )
                {
                    if( span->start <= (*iSpanPos)->start )
                    {
                        if( (*iSpanPos)->end <= span->end )
                            localSpans.push_back(*iSpanPos);
                    }
                    if( span->end < (*iSpanPos)->start )
                        break;
                }

                if( !localSpans.empty() )
                {
                    bool leading = (*(localSpans.begin()))->start == span->start;
                    bool trailing = (*(localSpans.rbegin()))->end == span->end;
                    bool all = !leading && !trailing;

                    int32_t leadingMaxEnd = span->end - span->minlen;
                    int32_t trailingMinStart = span->start + span->minlen;
                    for( vector<WeightedSpanTerm::PositionSpan *>::iterator iLocalSpan = localSpans.begin(); iLocalSpan != localSpans.end(); iLocalSpan++ )
                    {
                        WeightedSpanTerm::PositionSpan * localSpan = *iLocalSpan;
                        if( all )
                            localSpan->minlen = spanLenMin;
                        else
                        {
                            if( leading && localSpan->end <= leadingMaxEnd )
                                localSpan->minlen = spanLenMin;
                            if( trailing && localSpan->start >= trailingMinStart )
                                localSpan->minlen = spanLenMin;
                        }
                    }
                }
            }

            for( vector<WeightedSpanTerm::PositionSpan *>::iterator iSpanPos = spanPositions.begin(); iSpanPos != spanPositions.end(); )
            {
                if( (*iSpanPos)->minlen == INT_MIN )
                {
                    _CLLDECDELETE( *iSpanPos );
                    iSpanPos = spanPositions.erase( iSpanPos );
                }
                else
                {
                    iSpanPos++;
                }
            }
        }

        SpanQuery ** rgClauses = pSpanQuery->getClauses();
        if( rgClauses )
        {
        for( size_t nClause = 0; nClause < pSpanQuery->getClausesCount(); nClause++ )
            extract( rgClauses[ nClause ], spanPositions, terms );
        }
        else
            extractWeightedTerms( pSpanQuery, spanPositions, terms );

        for( vector<WeightedSpanTerm::PositionSpan *>::iterator iPS = spanPositions.begin(); iPS != spanPositions.end(); iPS++ )
            _CLLDECDELETE( *iPS );
    }
}

void WeightedSpanTermExtractor::extractWeightedSpanTerms( CL_NS2(search,spans)::SpanQuery * pQuery, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms )
{
    if( m_bExactTermSpans || (!spans.empty() && (*(spans.begin()))->minlen > 0 ))
        extractFromSpanQuery( pQuery, 0, spans, terms );
    else
        extractWeightedTerms( pQuery, spans, terms );
}


void WeightedSpanTermExtractor::extractWeightedTerms( CL_NS(search)::Query * pQuery, WeightedSpanTerm::PositionSpans& spans, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    TermSet * pNonWeightedTerms = m_pContext->getTermSet( pQuery );
    if( ! pNonWeightedTerms )
    {
        pNonWeightedTerms = _CLNEW TermSet();
        pQuery->extractTerms( pNonWeightedTerms );
        m_pContext->putTermSet( pQuery, pNonWeightedTerms );
    }
    processNonWeightedTerms( terms, *pNonWeightedTerms, pQuery->getBoost(), spans );
    m_pContext->freeTermSet( pNonWeightedTerms );
}

void WeightedSpanTermExtractor::processNonWeightedTerms( PositionCheckingMap& terms, TermSet& nonWeightedTerms, float_t fBoost, WeightedSpanTerm::PositionSpans& spans )
{
    for( TermSet::iterator iter = nonWeightedTerms.begin(); iter != nonWeightedTerms.end(); iter++ )
    {
        Term * pTerm = *iter;
        if( matchesField( pTerm->field() )) 
        {
            WeightedSpanTerm * pWeightedSpanTerm = terms.get( pTerm->text() );
            if( ! pWeightedSpanTerm )
            {
                WeightedSpanTerm * pWeightedSpanTerm = _CLNEW WeightedSpanTerm( fBoost, pTerm->text() );
                if( ! spans.empty() )
                {
                    pWeightedSpanTerm->addPositionSpans( spans );
                    pWeightedSpanTerm->setPositionSensitive( true );
                }
                terms.put( pWeightedSpanTerm );
            }
            else
            {
                if( spans.empty() )
                    pWeightedSpanTerm->setPositionSensitive( false );
                else
                    pWeightedSpanTerm->addPositionSpans( spans );
            }
        }
    }
}

IndexReader * WeightedSpanTermExtractor::getFieldReader()
{
    if( ! m_pFieldReader )
    {
        if( m_pReader && m_nDocId != -1 )
        {
            m_pFieldReader = m_pReader;
        }
        else
        {
            RAMDirectory *  pDirectory = _CLNEW RAMDirectory();
     	    IndexWriter     writer( pDirectory, NULL, true );
            Document        doc;

            writer.setMaxFieldLength( LUCENE_INT32_MAX_SHOULDBE );
            
            Field * pField = _CLNEW Field( m_tszFieldName, Field::STORE_NO | Field::INDEX_TOKENIZED );
            pField->setValue( m_pTokenStream );
            doc.add( *pField );

            writer.addDocument( &doc );
            writer.close();

            m_pFieldReader = IndexReader::open( pDirectory, true );
            _CLDECDELETE( pDirectory );
        }
    }

    return m_pFieldReader;
}

void WeightedSpanTermExtractor::closeFieldReader()
{
    if( m_pFieldReader && m_pFieldReader != m_pReader )
    {
        m_pFieldReader->close();
        _CLDELETE( m_pFieldReader );
    }

    m_pFieldReader = NULL;
}

CL_NS_END2
  
