/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "BitSet.h"
#include "CLucene/store/Directory.h"
#include "CLucene/store/IndexInput.h"
#include "CLucene/store/IndexOutput.h"

CL_NS_USE(store)
CL_NS_DEF(util)


const uint8_t BitSet::BYTE_COUNTS[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

const uint8_t BitSet::BYTE_OFFSETS[256] = {
    8, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};


BitSet::BitSet( const BitSet& copy ) :
	_size( copy._size ),
	_count(-1)
{
	int32_t len = (_size >> 5) + 1;
	bits = _CL_NEWARRAY(uint32_t, len);
	memcpy( bits, copy.bits, len * sizeof( uint32_t ) );
}

BitSet::BitSet ( int32_t size ):
  _size(size),
  _count(-1)
{
	int32_t len = (_size >> 5) + 1;
	bits = _CL_NEWARRAY(uint32_t, len);
	// memset is not needed - _CL_NEWARRAY uses calloc 
    // memset(bits,0,len);
}

BitSet::BitSet(CL_NS(store)::Directory* d, const char* name)
{
	_count=-1;
	CL_NS(store)::IndexInput* input = d->openInput( name );
	try 
    {
        _size = input->readInt();			  // read size
        if (_size == -3) 
        {
            readDgaps(input);
        }
        else if (_size == -2) 
        {
            readBitsCompat(input);
        } 
        else if (_size == -1) 
        {
            readDgaps(input);
        } 
        else 
        {
            readBits(input);
        }

	} _CLFINALLY (
	    input->close();
	    _CLDELETE(input );
	);
}
	
void BitSet::write(CL_NS(store)::Directory* d, const char* name) {
	CL_NS(store)::IndexOutput* output = d->createOutput(name);
	try {
    if (isSparse()) {
      writeDgaps(output); // sparse bit-set more efficiently saved as d-gaps.
    } else {
      writeBits(output);
    }
	} _CLFINALLY (
	    output->close();
	    _CLDELETE(output);
	);
}
BitSet::~BitSet(){
	_CLDELETE_ARRAY(bits);
}


void BitSet::set(const int32_t bit, bool val){
    if (bit >= _size) {
	      _CLTHROWA(CL_ERR_IndexOutOfBounds, "bit out of range");
    }

    _count = -1;

	if (val)
		bits[bit >> 5] |= 1 << (bit & 0x1F);
	else
		bits[bit >> 5] &= ~(1 << (bit & 0x1F));
}

int32_t BitSet::size() const {
  return _size;
}
int32_t BitSet::count(){
	// if the BitSet has been modified
    if (_count == -1) {

      int32_t c = 0;
      int32_t end = (_size >> 5) + 1;
      for (int32_t i = 0; i < end; i++)
          c += itemCount( bits[i] );
      _count = c;
    }
    return _count;
}
BitSet* BitSet::clone() const {
	return _CLNEW BitSet( *this );
}

int32_t BitSet::itemCount( uint32_t val )
{
    int32_t c = 0;
    c += BYTE_COUNTS[ val & 0xFF ];	  // sum bits per uint8_t
    val >>= 8;
    c += BYTE_COUNTS[ val & 0xFF ];	  // sum bits per uint8_t
    val >>= 8;
    c += BYTE_COUNTS[ val & 0xFF ];	  // sum bits per uint8_t
    val >>= 8;
    c += BYTE_COUNTS[ val & 0xFF ];	  // sum bits per uint8_t
    return c;
}

int32_t BitSet::itemOffset( uint32_t val ) const
{
    if ( val & 0xFF )
        return BYTE_OFFSETS[ val & 0xFF ];
    val >>= 8;
    if ( val & 0xFF )
        return 8 + BYTE_OFFSETS[ val & 0xFF ];
    val >>= 8;
    if ( val & 0xFF )
        return 16 + BYTE_OFFSETS[ val & 0xFF ];
    val >>= 8;
    if ( val & 0xFF )
        return 24 + BYTE_OFFSETS[ val & 0xFF ];
    return 32;
}

// BK: We don't need to shuffle bytes on little endian, I'm stupid
// void BitSet::shuffleBytes()
// {
//     int32_t m = (_size >> 5) + 1;
//     for ( int32_t i = 0; i < m; i++ ) 
//     {
//         if ( bits[i] != 0 ) 
//         {
//             bits[i] = (bits[i] >> 24) | (bits[i] >> 8 & 0x0000FF00) | (bits[i] << 8 & 0x00FF0000) | (bits[i] << 24);
//         }
//     }
// }


/** Read as a bit set stored with previous (wrong-sized) version */
void BitSet::readBitsCompat(IndexInput* input) 
{
    _size = input->readInt();       // (re)read size
    _count = input->readInt();        // read count
    bits = _CL_NEWARRAY(uint32_t,(_size >> 5) + 1);      // allocate bits
    input->readBytes((uint8_t*)bits, 4 * ((_size >> 5) + 1));   // read bits
}

/** Read as a bit set  */
void BitSet::readBits(IndexInput* input) 
{
    _count = input->readInt();        // read count
    bits = _CL_NEWARRAY(uint32_t,(_size >> 5) + 1);      // allocate bits
    input->readBytes((uint8_t*)bits, ((_size >> 3) + 1));   // read bits
}

/** read as a d-gaps list */
void BitSet::readDgaps(IndexInput* input) 
{
    _size = input->readInt();         // (re)read size
    _count = input->readInt();        // read count
    bits = _CL_NEWARRAY(uint32_t,(_size >> 5) + 1);     // allocate bits
    int32_t last=0;
    int32_t n = count();
    uint8_t * pbits = (uint8_t *)bits;
    while (n>0) 
    {
        last += input->readVInt();
        pbits[last] = input->readByte();
        n -= BYTE_COUNTS[pbits[last] & 0xFF];
    }
}

  /** Write as a bit set */
void BitSet::writeBits(IndexOutput* output) 
{
    output->writeInt(size());       // write size
    output->writeInt(count());        // write count
    output->writeBytes((uint8_t*)bits, ((_size >> 3) + 1));   // write bits
}

  /** Write as a d-gaps list */
void BitSet::writeDgaps(IndexOutput* output) 
{
    output->writeInt(-1);            // mark using d-gaps
    output->writeInt(size());        // write size
    output->writeInt(count());       // write count
    int32_t last=0;
    int32_t n = count();
    int32_t m = (_size >> 3) + 1;
    uint8_t * pbits = (uint8_t *)bits;
    for (int32_t i=0; i<m && n>0; i++) 
    {
        if (pbits[i]!=0) 
        {
            output->writeVInt(i-last);
            output->writeByte(pbits[i]);
            last = i;
            n -= BYTE_COUNTS[pbits[i] & 0xFF];
        }
    }
}

  /** Indicates if the bit vector is sparse and should be saved as a d-gaps list, or dense, and should be saved as a bit set. */
  bool BitSet::isSparse() {
    // note: order of comparisons below set to favor smaller values (no binary range search.)
    // note: adding 4 because we start with ((int) -1) to indicate d-gaps format.
    // note: we write the d-gap for the byte number, and the byte (bits[i]) itself, therefore
    //       multiplying count by (8+8) or (8+16) or (8+24) etc.:
    //       - first 8 for writing bits[i] (1 byte vs. 1 bit), and
    //       - second part for writing the byte-number d-gap as vint.
    // note: factor is for read/write of byte-arrays being faster than vints.
    int32_t factor = 10;
    if ((_size >> 3) < (1<< 7)) return factor * (4 + (8+ 8)*count()) < size();
    if ((_size >> 3) < (1<<14)) return factor * (4 + (8+16)*count()) < size();
    if ((_size >> 3) < (1<<21)) return factor * (4 + (8+24)*count()) < size();
    if ((_size >> 3) < (1<<28)) return factor * (4 + (8+32)*count()) < size();
    return                             factor * (4 + (8+40)*count()) < size();
  }

  int32_t BitSet::nextSetBit(int32_t fromIndex) const 
  {
      if (fromIndex < 0)
          _CLTHROWT(CL_ERR_IndexOutOfBounds, _T("fromIndex < 0"));

      if (fromIndex >= _size)
          return -1;

      unsigned int _max =  (_size >> 5) + 1;

      unsigned int i = (int)( fromIndex>>5 );
      unsigned int subIndex = fromIndex & 0x1F; // index within the byte
      uint32_t val = bits[i] >> subIndex;  // skip all the bits to the right of index

      if ( val != 0 ) 
      {
          return ( ( i<<5 ) + subIndex + itemOffset( val ) );
      }

      while( ++i < _max ) 
      {
          val = bits[i];
          if ( val != 0 ) 
              return ( ( i<<5 ) + itemOffset( val ) );
      }
      return -1;
  }

BitSet& BitSet::operator &=( const BitSet& input )
{
    if ( _size  != input.size() )
        _CLTHROWA(CL_ERR_IndexOutOfBounds, "bitsets have different size");

    int32_t nSize = ( _size >> 5 ) + 1;
    for ( int32_t i = 0; i < nSize; i++ )
        bits[i] &= input.bits[i];

    _count = -1;
    return( *this );
}

BitSet& BitSet::operator |=( const BitSet& input )
{
    if ( _size  != input.size() )
        _CLTHROWA(CL_ERR_IndexOutOfBounds, "bitsets have different size");

    int32_t nSize = ( _size >> 5 ) + 1;
    for ( int32_t i = 0; i < nSize; i++ )
        bits[i] |= input.bits[i];

  	_count = -1;
    return( *this );
}

BitSet& BitSet::operator ^=( const BitSet& input )
{
    if ( _size  != input.size() )
        _CLTHROWA(CL_ERR_IndexOutOfBounds, "bitsets have different size");

    int32_t nSize = ( _size >> 5 ) + 1;
    for ( int32_t i = 0; i < nSize; i++ )
        bits[i] ^= input.bits[i];

    _count = -1;
    return( *this );
}

BitSet& BitSet::andnot( const BitSet& input )
{
    if ( _size  != input.size() )
        _CLTHROWA(CL_ERR_IndexOutOfBounds, "bitsets have different size");

    int32_t nSize = ( _size >> 5 ) + 1;
    for ( int32_t i = 0; i < nSize; i++ )
        bits[i] = bits[i] & ~input.bits[i];

  	_count = -1;
    return( *this );
}

BitSet& BitSet::complement()
{
    int32_t nSize = ( _size >> 5 ) + 1;
    for ( int32_t i = 0; i < nSize; i++ )
        bits[i] = ~bits[i];

    // we must clear unused bits, otherwise count() returns incorrect value (because 0 changet to 1)
    int nRest = 32 - ( ( nSize * 32 ) - _size );
    uint32_t mask = ( 1 << nRest ) - 1;
    bits[ nSize-1 ] &= mask;

  	_count = -1;
    return( *this );
}



CL_NS_END
