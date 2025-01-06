/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include "CLucene/search/QueryFilter.h"
#include "CLucene/search/ChainedFilter.h"


	void testBefore(CuTest *tc) {
	// create an index
		char fsdir[CL_MAX_PATH];
		_snprintf(fsdir,CL_MAX_PATH,"%s/%s",cl_tempDir, "dfindex");
		
		FSDirectory* indexStore = FSDirectory::getDirectory( fsdir);
		Analyzer* a = _CLNEW SimpleAnalyzer();
		IndexWriter* writer = _CLNEW IndexWriter(indexStore, a, true);
     	int64_t now = Misc::currentTimeMillis()/1000;

     	Document doc;
     	// add time that is in the past
		TCHAR* tn = DateField::timeToString(now - 1000);
     	doc.add(*_CLNEW Field(_T("datefield"), tn,Field::STORE_YES | Field::INDEX_UNTOKENIZED));
		_CLDELETE_CARRAY(tn);
     	doc.add(*_CLNEW Field(_T("body"), _T("Today is a very sunny day in New York City"),Field::STORE_YES | Field::INDEX_TOKENIZED));
      	writer->addDocument(&doc);
    	writer->close();
		_CLDELETE( writer );

		IndexReader* reader = IndexReader::open(indexStore);
    	IndexSearcher* searcher = _CLNEW IndexSearcher(reader);
	
    	// filter that should preserve matches
		DateFilter* df1 = DateFilter::Before(_T("datefield"), now);
	
    	// filter that should discard matches
    	DateFilter* df2 = DateFilter::Before(_T("datefield"), now - 999999);
	
    	// search something that doesn't exist with DateFilter
		Term* term = _CLNEW Term(_T("body"), _T("NoMatchForThis"));
    	Query* query1 = _CLNEW TermQuery(term);
		_CLDECDELETE(term);

    	// search for something that does exists
		term=_CLNEW Term(_T("body"), _T("sunny"));
    	Query* query2 = _CLNEW TermQuery(term);
		_CLDECDELETE(term);
	
    	Hits* result = NULL;
	
    	// ensure that queries return expected results without DateFilter first
    	result = searcher->search(query1, NULL);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);
	
    	result = searcher->search(query2, NULL);
   		CLUCENE_ASSERT(1 == result->length());
		_CLDELETE(result);
	
    	// run queries with DateFilter
    	result = searcher->search(query1, NULL, df1);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);

    	result = searcher->search(query1, NULL, df2);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);

     	result = searcher->search(query2, NULL, df1);
     	CLUCENE_ASSERT(1 == result->length());
		_CLDELETE(result);

    	result = searcher->search(query2, NULL, df2);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);

		reader->close();
		searcher->close();
		_CLDELETE(a)
		_CLDELETE(reader);
		_CLDELETE(searcher);
		_CLDELETE(query1);
		_CLDELETE(query2);
		_CLDELETE(df1);
		_CLDELETE(df2);
		
		indexStore->close();
		_CLDECDELETE(indexStore);
	}

	void testAfter(CuTest *tc) {
		// create an index
		RAMDirectory* indexStore = _CLNEW RAMDirectory;
		Analyzer* a = _CLNEW SimpleAnalyzer();
		IndexWriter* writer = _CLNEW IndexWriter(indexStore, a, true);
		int64_t now = Misc::currentTimeMillis()/1000;

		// add time that is in the future
		TCHAR* tf = DateField::timeToString(now + 888888);

     	Document* doc = _CLNEW Document;
		doc->add(*_CLNEW Field(_T("datefield"), tf,Field::STORE_YES | Field::INDEX_UNTOKENIZED));
		_CLDELETE_CARRAY(tf);

     	doc->add(*_CLNEW Field(_T("body"), _T("Today is a very sunny day in New York City"),Field::STORE_YES | Field::INDEX_TOKENIZED));
		writer->addDocument(doc);
		_CLDELETE(doc);

   		writer->close();
		_CLDELETE( writer );

		//read the index
		IndexReader* reader = IndexReader::open(indexStore);
    	IndexSearcher* searcher = _CLNEW IndexSearcher(reader);

		// filter that should preserve matches
    	DateFilter* df1 = DateFilter::After(_T("datefield"), now);
	
    	// filter that should discard matches
    	DateFilter* df2 = DateFilter::After(_T("datefield"), now + 999999);
	
    	// search something that doesn't exist with DateFilter
		Term* term = _CLNEW Term(_T("body"), _T("NoMatchForThis"));
    	Query* query1 = _CLNEW TermQuery(term);
		_CLDECDELETE(term);
	
    	// search for something that does exists
		term=_CLNEW Term(_T("body"), _T("sunny"));
    	Query* query2 = _CLNEW TermQuery(term);
		_CLDECDELETE(term);
	
    	Hits* result = NULL;

		// ensure that queries return expected results without DateFilter first
    	result = searcher->search(query1, NULL);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);

    	result = searcher->search(query2, NULL);
    	CLUCENE_ASSERT(1 == result->length());
		_CLDELETE(result);

		// run queries with DateFilter
    	result = searcher->search(query1, NULL, df1);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);

		result = searcher->search(query1, NULL, df2);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);

     	result = searcher->search(query2, NULL, df1);
     	CLUCENE_ASSERT(1 == result->length());
		_CLDELETE(result);
		
    	result = searcher->search(query2, NULL, df2);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);

		reader->close();
		searcher->close();

		_CLDELETE(query1);
		_CLDELETE(query2);
		_CLDELETE(df1);
		_CLDELETE(df2);
		_CLDELETE(reader);
		_CLDELETE(searcher);
		_CLDELETE(a);
		indexStore->close();
		_CLDECDELETE(indexStore);
	}

	void testDateFilterDestructor(CuTest *tc){
		char loc[1024];
		strcpy(loc, clucene_data_location);
		strcat(loc, "/reuters-21578-index");

		CuAssert(tc,_T("Index does not exist"),Misc::dir_Exists(loc));
		IndexReader* reader = IndexReader::open(loc);
		int64_t now = Misc::currentTimeMillis()/1000;
    	DateFilter* df1 = DateFilter::After(_T("datefield"), now);
		BitSet* bs = df1->bits(reader, NULL);
		_CLDELETE(bs);
		_CLDELETE(df1);

		reader->close();
		_CLDELETE(reader);
	}


	void testChain(CuTest *tc) {
	// create an index
		char fsdir[CL_MAX_PATH];
		_snprintf(fsdir,CL_MAX_PATH,"%s/%s",cl_tempDir, "dfindex");
		
		FSDirectory* indexStore = FSDirectory::getDirectory( fsdir);
		Analyzer* a = _CLNEW SimpleAnalyzer();
		IndexWriter* writer = _CLNEW IndexWriter(indexStore, a, true);
     	int64_t now = Misc::currentTimeMillis()/1000;

     	Document doc;
     	Document doc2;
     	// add time that is in the past
		TCHAR* tn = DateField::timeToString(now - 1000);
     	doc.add(*_CLNEW Field(_T("datefield"), tn,Field::STORE_YES | Field::INDEX_UNTOKENIZED));
     	doc.add(*_CLNEW Field(_T("condition"), L"1",Field::STORE_YES | Field::INDEX_UNTOKENIZED));
     	doc.add(*_CLNEW Field(_T("body"), _T("Today is a very sunny day in New York City"),Field::STORE_YES | Field::INDEX_TOKENIZED));
      	writer->addDocument(&doc);

     	doc2.add(*_CLNEW Field(_T("datefield"), tn,Field::STORE_YES | Field::INDEX_UNTOKENIZED));
     	doc2.add(*_CLNEW Field(_T("condition"), L"0",Field::STORE_YES | Field::INDEX_UNTOKENIZED));
     	doc2.add(*_CLNEW Field(_T("body"), _T("Today is a very sunny day in Prague"),Field::STORE_YES | Field::INDEX_TOKENIZED));
      	writer->addDocument(&doc2);
        
        writer->close();

		_CLDELETE_CARRAY(tn);
		_CLDELETE( writer );

		IndexReader* reader = IndexReader::open(indexStore);
    	IndexSearcher* searcher = _CLNEW IndexSearcher(reader);
	
    	// positive filter for date
		DateFilter* df1 = DateFilter::Before(_T("datefield"), now);
	
    	// negative filter for date
    	DateFilter* df2 = DateFilter::Before(_T("datefield"), now - 999999);

        // positive condition filter
		Term* term = _CLNEW Term(_T("condition"), _T("1"));
    	Query* queryF1 = _CLNEW TermQuery(term);
        QueryFilter* qf1 = _CLNEW QueryFilter( queryF1 );
		_CLDECDELETE(term);
        
        // negative condition filter
		term = _CLNEW Term(_T("condition"), _T("2"));
    	Query* queryF2 = _CLNEW TermQuery(term);
        QueryFilter* qf2 = _CLNEW QueryFilter( queryF2 );
		_CLDECDELETE(term);

    	// search something that doesn't exist with DateFilter
		term = _CLNEW Term(_T("body"), _T("NoMatchForThis"));
    	Query* query1 = _CLNEW TermQuery(term);
		_CLDECDELETE(term);

    	// search for something that does exists
		term=_CLNEW Term(_T("body"), _T("sunny"));
    	Query* query2 = _CLNEW TermQuery(term);
		_CLDECDELETE(term);
	
    	Hits* result = NULL;
	
    	// ensure that queries return expected results without DateFilter first
    	result = searcher->search(query1, NULL);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);
	
    	result = searcher->search(query2, NULL);
   		CLUCENE_ASSERT(2 == result->length());
		_CLDELETE(result);
	
    	// run queries with DateFilter
    	result = searcher->search(query1, NULL, df1);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);

     	result = searcher->search(query2, NULL, df1);
     	CLUCENE_ASSERT(2 == result->length());
		_CLDELETE(result);

    	// run queries with QueryFilter
    	result = searcher->search(query1, NULL, qf1);
    	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);

     	result = searcher->search(query2, NULL, qf1);
     	CLUCENE_ASSERT(1 == result->length());
		_CLDELETE(result);

     	result = searcher->search(query2, NULL, qf2);
     	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);

        // create filter chains
        Filter * filters[3];

        filters[0] = df1;
        filters[1] = qf1;
        filters[2] = NULL;

        ChainedFilter *cf1 = _CLNEW ChainedFilter( filters, ChainedFilter::AND );
     	result = searcher->search(query2, NULL, cf1);
     	CLUCENE_ASSERT(1 == result->length());
		_CLDELETE(result);
		_CLDELETE(cf1);

        filters[0] = df1;
        filters[1] = qf2;
        cf1 = _CLNEW ChainedFilter( filters, ChainedFilter::AND );
     	result = searcher->search(query2, NULL, cf1);
     	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);
		_CLDELETE(cf1);

        filters[0] = df1;
        filters[1] = qf2;
        cf1 = _CLNEW ChainedFilter( filters, ChainedFilter::ANDNOT );
     	result = searcher->search(query2, NULL, cf1);
     	CLUCENE_ASSERT(2 == result->length());
		_CLDELETE(result);
		_CLDELETE(cf1);

        filters[1] = qf1;
        filters[0] = df2;
        cf1 = _CLNEW ChainedFilter( filters, ChainedFilter::AND );
     	result = searcher->search(query2, NULL, cf1);
     	CLUCENE_ASSERT(0 == result->length());
		_CLDELETE(result);
		_CLDELETE(cf1);

        filters[0] = qf1;
        filters[1] = df2;
        cf1 = _CLNEW ChainedFilter( filters, ChainedFilter::ANDNOT );
     	result = searcher->search(query2, NULL, cf1);
     	CLUCENE_ASSERT(1 == result->length());
		_CLDELETE(result);
		_CLDELETE(cf1);

        filters[0] = qf1;
        filters[1] = df1;
        cf1 = _CLNEW ChainedFilter( filters, ChainedFilter::OR );
     	result = searcher->search(query2, NULL, cf1);
     	CLUCENE_ASSERT(2 == result->length());
		_CLDELETE(result);
		_CLDELETE(cf1);




		reader->close();
		searcher->close();
		_CLDELETE(a)
		_CLDELETE(reader);
		_CLDELETE(searcher);
		_CLDELETE(query1);
		_CLDELETE(query2);
		_CLDELETE(df1);
		_CLDELETE(df2);
		_CLDELETE(qf1);
		_CLDELETE(qf2);
		_CLDELETE(queryF1);
		_CLDELETE(queryF2);
		
		indexStore->close();
		_CLDECDELETE(indexStore);
	}

CuSuite *testdatefilter(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene DateFilter Test"));

    SUITE_ADD_TEST(suite, testDateFilterDestructor);
    SUITE_ADD_TEST(suite, testBefore);
    SUITE_ADD_TEST(suite, testAfter);
    SUITE_ADD_TEST(suite, testChain);

    return suite; 
}
// EOF
