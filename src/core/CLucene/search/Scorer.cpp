/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Scorer.h"
#include "SearchHeader.h"

CL_NS_DEF(search)

Scorer::Scorer(Similarity* _similarity) : similarity(_similarity){
}

Scorer::~Scorer(){
}

void Scorer::score(HitCollector* hc) {
	while (next()) {
		if (!hc->collect(doc(), score()))
			break;
	}
}

bool Scorer::score( HitCollector* results, const int32_t maxDoc ) {
	while( doc() < maxDoc ) {
		bool cr = results->collect( doc(), score() );
		if ( !cr || !next() )
			return false;
	}
	return true;
}
bool Scorer::sort(const Scorer* elem1, const Scorer* elem2){
	return elem1->doc() < elem2->doc();
}

CL_NS_END
