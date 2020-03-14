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

#ifndef _lucene_search_highlight_weightedspantermextractor_
#define _lucene_search_highlight_weightedspantermextractor_

#include <limits.h>

#include "CLucene/search/Query.h"
#include "CLucene/index/Terms.h"
#include "CLucene/highlighter/SpanHighlightScorer.h"
#include "CLucene/highlighter/WeightedSpanTerm.h"

#include "CLucene/search/spans/Spans.h"

CL_CLASS_DEF(analysis,TokenStream);
CL_CLASS_DEF(index,IndexReader);

CL_CLASS_DEF(search,BooleanQuery);
CL_CLASS_DEF(search,PhraseQuery);
CL_CLASS_DEF(search,MultiPhraseQuery);
CL_CLASS_DEF(search,ConstantScoreRangeQuery);
CL_CLASS_DEF2(search,spans,SpanQuery);

CL_NS_DEF2(search,highlight)

/**
 * Class used to extract {@link WeightedSpanTerm}s from a {@link Query} based on whether Terms from the query are contained in a supplied TokenStream.
 */
class CLUCENE_CONTRIBS_EXPORT WeightedSpanTermExtractor : LUCENE_BASE
{
public:
    /**
     * Map holding Weighted span terms
     */
    class CLUCENE_CONTRIBS_EXPORT PositionCheckingMap
    {
    public:
        WeightedSpanTermMap&    mapSpanTerms;

    public:
        PositionCheckingMap( WeightedSpanTermMap& weightedSpanTerms );
        virtual ~PositionCheckingMap();

        void putAll( PositionCheckingMap * m );
        void put( WeightedSpanTerm * spanTerm );
        WeightedSpanTerm * get( const TCHAR * term );
    };

protected:
    class CLUCENE_CONTRIBS_EXPORT GuardedSpans : public CL_NS2(search,spans)::Spans
    {
    private:
        CL_NS2(search,spans)::Spans *   m_pSpans;
        bool                            m_bMore;

    public:
        GuardedSpans( CL_NS2(search,spans)::Spans * pSpans )    { m_pSpans = pSpans; m_bMore = true; };
        virtual ~GuardedSpans()                                 { _CLDELETE( m_pSpans ); };

        virtual bool next()                                     { return m_bMore ? m_bMore = m_pSpans->next() : false; };
        virtual bool skipTo( int32_t target )                   { return m_bMore ? m_bMore = m_pSpans->skipTo( target ) : false; };
        virtual int32_t doc() const                             { return m_bMore ? m_pSpans->doc() : INT_MAX; };
        virtual int32_t start() const                           { return m_bMore ? m_pSpans->start() : -1; };
        virtual int32_t end() const                             { return m_bMore ? m_pSpans->end() : -1; };
        virtual TCHAR* toString() const                         { return m_pSpans->toString(); }
    };

    /**
     * Context for highlighting multiple documents in docid order
     */
    class CLUCENE_CONTRIBS_EXPORT Context
    {
        std::map<CL_NS(search)::Query *, CL_NS(search)::Query *>                    cachedQueries;
        std::map<CL_NS(search)::Query *, CL_NS(search)::Weight *>                   cachedWeights;
        std::map<CL_NS(search)::Query *, CL_NS(search)::Scorer *>                   cachedScorers;
        std::map<CL_NS(search)::Query *, CL_NS(index)::TermDocs *>                  cachedTermDocs;
        std::map<CL_NS(search)::Query *, CL_NS(search)::TermSet *>                  cachedTermSets;
        std::map<CL_NS2(search,spans)::SpanQuery *, CL_NS2(search,spans)::Spans *>  cachedSpans;

    public:
        Context();
        virtual ~Context();

        virtual bool                            isCaching() { return true; };

        virtual CL_NS(search)::Query *          getQuery( CL_NS(search)::Query * pQuery ) { auto it = cachedQueries.find( pQuery ); return it == cachedQueries.end() ? NULL : it->second; };
        virtual void                            putQuery( CL_NS(search)::Query * pQuery, CL_NS(search)::Query * pValue ) { cachedQueries.emplace( pQuery, pValue ); };
        virtual void                            freeQuery( CL_NS(search)::Query * pValue ) {};

        virtual CL_NS(search)::Weight *         getWeight( CL_NS(search)::Query * pQuery ) { auto it = cachedWeights.find( pQuery ); return it == cachedWeights.end() ? NULL : it->second; };
        virtual void                            putWeight( CL_NS(search)::Query * pQuery, CL_NS(search)::Weight * pWeight ) { cachedWeights.emplace( pQuery, pWeight ); };
        virtual void                            freeWeight( CL_NS(search)::Weight * pWeight ) {};

        virtual CL_NS(search)::Scorer *         getScorer( CL_NS(search)::Query * pQuery ) { auto it = cachedScorers.find( pQuery ); return it == cachedScorers.end() ? NULL : it->second; };
        virtual void                            putScorer( CL_NS(search)::Query * pQuery, CL_NS(search)::Scorer * pScorer ) { cachedScorers.emplace( pQuery, pScorer ); };
        virtual void                            freeScorer( CL_NS(search)::Scorer * pScorer ) {};

        virtual CL_NS2(search,spans)::Spans *   getSpans( CL_NS2(search,spans)::SpanQuery * pQuery ) { auto it = cachedSpans.find( pQuery ); return it == cachedSpans.end() ? NULL : it->second; };
        virtual void                            putSpans( CL_NS2(search,spans)::SpanQuery * pQuery, CL_NS2(search,spans)::Spans * pSpans ) { cachedSpans.emplace( pQuery, pSpans ); };
        virtual void                            freeSpans( CL_NS2(search,spans)::Spans * pSpans ) {};

        virtual CL_NS(index)::TermDocs *        getTermDocs( CL_NS(search)::Query * pQuery ) { auto it = cachedTermDocs.find( pQuery ); return it == cachedTermDocs.end() ? NULL : it->second; };
        virtual void                            putTermDocs( CL_NS(search)::Query * pQuery, CL_NS(index)::TermDocs * pTermDocs ) { cachedTermDocs.emplace( pQuery, pTermDocs ); };
        virtual void                            freeTermDocs( CL_NS(index)::TermDocs * pTermDocs ) {};

        virtual CL_NS(search)::TermSet *        getTermSet( CL_NS(search)::Query * pQuery ) { auto it = cachedTermSets.find( pQuery ); return it == cachedTermSets.end() ? NULL : it->second; };
        virtual void                            putTermSet( CL_NS(search)::Query * pQuery, CL_NS(search)::TermSet * pTermSet ) { cachedTermSets.emplace( pQuery, pTermSet ); };
        virtual void                            freeTermSet( CL_NS(search)::TermSet * pTermSet ) {};

    protected:
        void freeQueryImpl( CL_NS(search)::Query * pQuery );
        void freeWeightImpl( CL_NS(search)::Weight * pWeight );
        void freeScorerImpl( CL_NS(search)::Scorer * pScorer );
        void freeSpansImpl( CL_NS2(search,spans)::Spans * pSpans );
        void freeTermDocsImpl( CL_NS(index)::TermDocs * pTermDocs );
        void freeTermSetImpl( CL_NS(search)::TermSet * pTermSet );
    };

    /**
     * Dummy context that stores nothing
     */
    class CLUCENE_CONTRIBS_EXPORT DummyContext : public Context
    {
    public:
        DummyContext() {};
        virtual ~DummyContext() {};

        virtual bool                            isCaching() { return false; };

        virtual CL_NS(search)::Query *          getQuery( CL_NS(search)::Query * pQuery ) { return NULL; };
        virtual void                            putQuery( CL_NS(search)::Query * pQuery, CL_NS(search)::Query * pValue ) {};
        virtual void                            freeQuery( CL_NS(search)::Query * pValue ) { freeQueryImpl( pValue ); };

        virtual CL_NS(search)::Weight *         getWeight( CL_NS(search)::Query * pQuery ) { return NULL;};
        virtual void                            putWeight( CL_NS(search)::Query * pQuery, CL_NS(search)::Weight * pWeight ) {};
        virtual void                            freeWeight( CL_NS(search)::Weight * pWeight ) { freeWeightImpl( pWeight ); };

        virtual CL_NS(search)::Scorer *         getScorer( CL_NS(search)::Query * pQuery ) { return NULL;};
        virtual void                            putScorer( CL_NS(search)::Query * pQuery, CL_NS(search)::Scorer * pScorer ) {};
        virtual void                            freeScorer( CL_NS(search)::Scorer * pScorer ) { freeScorerImpl( pScorer ); };

        virtual CL_NS2(search,spans)::Spans *   getSpans( CL_NS2(search,spans)::SpanQuery * pQuery ) { return NULL;};
        virtual void                            putSpans( CL_NS2(search,spans)::SpanQuery * pQuery, CL_NS2(search,spans)::Spans * pSpans ) {};
        virtual void                            freeSpans( CL_NS2(search,spans)::Spans * pSpans ) { freeSpansImpl( pSpans ); };

        virtual CL_NS(index)::TermDocs *        getTermDocs( CL_NS(search)::Query * pQuery ) { return NULL;};
        virtual void                            putTermDocs( CL_NS(search)::Query * pQuery, CL_NS(index)::TermDocs * pTermDocs ) {};
        virtual void                            freeTermDocs( CL_NS(index)::TermDocs * pTermDocs ) { freeTermDocsImpl( pTermDocs ); };

        virtual CL_NS(search)::TermSet *        getTermSet( CL_NS(search)::Query * pQuery ) { return NULL;};
        virtual void                            putTermSet( CL_NS(search)::Query * pQuery, CL_NS(search)::TermSet * pTermSet ) {};
        virtual void                            freeTermSet( CL_NS(search)::TermSet * pTermSet ) { freeTermSetImpl( pTermSet ); };
    };


protected:
    const TCHAR *                               m_tszFieldName;
    CL_NS(analysis)::TokenStream *              m_pTokenStream;
    
    CL_NS(index)::IndexReader *                 m_pReader;
    CL_NS(index)::IndexReader *                 m_pFieldReader;
    Context *                                   m_pContext;
    int32_t                                     m_nDocId;

    bool                                        m_bAutoRewriteQueries;
    bool                                        m_bExactTermSpans;
    bool                                        m_bScoreTerms;

public:
    /**
     * Creates the extractor
     * @param autoRewriteQueries        - should we try to rewrite not rewritten queries and evaluate ConstantScoreRangeQueries
     */
    WeightedSpanTermExtractor( bool autoRewriteQueries = false );
    
    /// Destructor
    virtual ~WeightedSpanTermExtractor();

    /**
     * Sets index reader used to score terms, and if docId is specified in init() then also to rewrite queries and evaluate span queries
     * @param reader                    used to score each term and evaluate the queries
     */
    void setIndexReader( CL_NS(index)::IndexReader * pReader );

    /**
     * Creates and closes the caching context for the extractor
     */
    void createContext();
    void closeContext();

    /**
     * Should the index reader be used to score each terms based on its idf?
     */
    void setScoreTerms( bool bScoreTerms );
    bool scoreTerms();

    /**
     * @return whether ConstantScoreRangeQueries are set to be highlighted and other queries rewritten
     */
    void setAutoRewriteQueries( bool bAutoRewriteQueries );
    bool autoRewriteQueries();

    /**
     * Compute exact term spans
     */
    void setExtractExactTermSpans( bool bExactTermSpans );
    bool exactTermSpans();

    /**
     * Creates a Map of <code>WeightedSpanTerms</code> from the given <code>Query</code> and <code>TokenStream</code>. 
     * 
     * @param weightedSpanTerms         result, this map will be filled with terms
     * @param pQuery                    query that caused hit
     * @param tszFieldName              restricts Term's used based on field name
     * @param pTokenFilter              text to be scored
     * @param nDocId                    id of document to be scored
     * @throws IOException
     */
    void extractWeightedSpanTerms( WeightedSpanTermMap& weightedSpanTerms, CL_NS(search)::Query * pQuery, const TCHAR * tszFieldName, CL_NS(analysis)::TokenStream * pTokenStream, int32_t nDocId = -1 );

protected:
    /**
     * Fills a <code>Map</code> with <@link WeightedSpanTerm>s using the terms from the supplied <code>Query</code>.
     * 
     * @param query         Query to extract Terms from
     * @param span          Current span - operate only within this span
     * @param terms         Map to place created WeightedSpanTerms in
     * @throws IOException
     */
    void extract( CL_NS(search)::Query * pQuery, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms );

    /**
     * Methods for extracting info from different types of queries
     */
    void rewriteAndExtract( Query * pQuery, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms );
    void extractFromBooleanQuery( BooleanQuery * pQuery, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms );
    void extractFromPhraseQuery( PhraseQuery * pQuery, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms );
    void extractFromMultiPhraseQuery( MultiPhraseQuery * pQuery, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms );
    void extractFromConstantScoreRangeQuery( ConstantScoreRangeQuery * pQuery, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms );
    
    /**
     * Overwrite this method to handle custom queries
     */
    virtual void extractFromCustomQuery( CL_NS(search)::Query * pQuery, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms );

    /**
     * Fills a <code>Map</code> with <@link WeightedSpanTerm>s using the terms from the supplied <code>SpanQuery</code>.
     * 
     * @param spanQuery     SpanQuery to extract Terms from
     * @param minSlop       min slop defined by the span query 
     * @param spans         external span within which the span should lay
     * @param terms         Map to place created WeightedSpanTerms in
     * @throws IOException
     */
    void extractFromSpanQuery( CL_NS2(search,spans)::SpanQuery * pSpanQuery, int32_t minSlop, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms );

    /**
     * Fills a <code>Map</code> with <@link WeightedSpanTerm>s using the terms from the supplied <code>Query</code>.
     * 
     * @param terms         Map to place created WeightedSpanTerms in
     * @param query         Query to extract Terms from
     * @throws IOException
     */
    void extractWeightedTerms( CL_NS(search)::Query * pQuery, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms );
    void extractWeightedSpanTerms( CL_NS2(search,spans)::SpanQuery * pQuery, WeightedSpanTerm::PositionSpans& spans, PositionCheckingMap& terms );
    void processNonWeightedTerms( PositionCheckingMap& terms, TermSet& nonWeightedTerms, float_t fBoost, WeightedSpanTerm::PositionSpans& spans );

    /**
     * Checks the field to match the current field
     */
    bool matchesField( const TCHAR * tszFieldNameToCheck );

    /**
     * Returns reader for the current field - it returns the supplied reader if the docid is specified, otherwise it creates a new on
     */
    CL_NS(index)::IndexReader * getFieldReader();

    /**
     * Closes allocated reader
     */
    void closeFieldReader();

    /**
     * Checks span match
     */
    bool spanMatchesPositionSpans( CL_NS2(search,spans)::Spans * pSpans, WeightedSpanTerm::PositionSpans& spans );
};

CL_NS_END2
#endif // _lucene_search_highlight_weightedspantermextractor_
