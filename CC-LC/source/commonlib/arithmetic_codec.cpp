// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Fast arithmetic coding implementation                                     -
// -> 32-bit variables, 32-bit product, periodic updates, table decoding     -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Version 1.01  -  November 28, 2005                                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                                  WARNING                                  -
//                                 =========                                 -
//                                                                           -
// The only purpose of this program is to demonstrate the basic principles   -
// of arithmetic coding. It is provided as is, without any express or        -
// implied warranty, without even the warranty of fitness for any particular -
// purpose, or that the implementations are correct.                         -
//                                                                           -
// Permission to copy and redistribute this code is hereby granted, provided -
// that this warning and copyright notices are not removed or altered.       -
//                                                                           -
// Copyright (c) 2004 by Amir Said (said@ieee.org) &                         -
//                       William A. Pearlman (pearlw@ecse.rpi.edu)           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// A description of the arithmetic coding method used here is available in   -
//                                                                           -
// Lossless Compression Handbook, ed. K. Sayood                              -
// Chapter 5: Arithmetic Coding (A. Said), pp. 101-152, Academic Press, 2003 -
//                                                                           -
// A. Said, Introduction to Arithetic Coding Theory and Practice             -
// HP Labs report HPL-2004-76  -  http://www.hpl.hp.com/techreports/         -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - Inclusion - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <stdlib.h>
#include <iostream>  
#include "arithmetic_codec.h"
#include "global_arithmetic.h"
#include <cmath>
using namespace std;
// - - Constants - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const unsigned AC__MinLength = 0x01000000U;   // threshold for renormalization
const unsigned AC__MaxLength = 0xFFFFFFFFU;      // maximum AC interval length

                                           // Maximum values for binary models
const unsigned BM__LengthShift = 13;     // length bits discarded before mult.
const unsigned BM__MaxCount    = 1 << BM__LengthShift;  // for adaptive models

                                          // Maximum values for general models
const unsigned DM__LengthShift = 15;     // length bits discarded before mult.
const unsigned DM__MaxCount    = 1 << DM__LengthShift;  // for adaptive models


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Static functions  - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void AC_Error(const char * msg)
{
  fprintf(stderr, "\n\n -> Arithmetic coding error: ");
  fputs(msg, stderr);
  fputs("\n Execution terminated!\n", stderr);
  exit(1);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Coding implementations  - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::propagate_carry(void)
{
  unsigned char * p;            // carry propagation on compressed data buffer
  for (p = ac_pointer - 1; *p == 0xFFU; p--) *p = 0;
  ++*p;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::renorm_enc_interval(void)
{
  do {                                          // output and discard top byte
    *ac_pointer++ = (unsigned char)(base >> 24);
    base <<= 8;
	writeBytes++;
  } while ((length <<= 8) < AC__MinLength);        // length multiplied by 256
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::renorm_dec_interval(void)
{
  do {                                          // read least-significant byte
    value = (value << 8) | unsigned(*++ac_pointer);
  } while ((length <<= 8) < AC__MinLength);        // length multiplied by 256
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::put_bit(unsigned bit)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
#endif

  length >>= 1;                                              // halve interval
  if (bit) {
    unsigned init_base = base;
    base += length;                                               // move base
    if (init_base > base) propagate_carry();               // overflow = carry
  }

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::get_bit(void)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  length >>= 1;                                              // halve interval
  unsigned bit = (value >= length);                              // decode bit
  if (bit) value -= length;                                       // move base

  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  return bit;                                         // return data bit value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::put_bits(unsigned data, unsigned bits)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
  if ((bits < 1) || (bits > 20)) AC_Error("invalid number of bits");
  if (data >= (1U << bits)) AC_Error("invalid data");
#endif

  unsigned init_base = base;
  base += data * (length >>= bits);            // new interval base and length

  if (init_base > base) propagate_carry();                 // overflow = carry
  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::get_bits(unsigned bits)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
  if ((bits < 1) || (bits > 20)) AC_Error("invalid number of bits");
#endif

  unsigned s = value / (length >>= bits);      // decode symbol, change length

  value -= length * s;                                      // update interval
  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned bit,
                              Static_Bit_Model & M)
{
	g_count += 1;
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
#endif

  unsigned x = M.bit_0_prob * (length >> BM__LengthShift);   // product l x p0
                                                            // update interval
  if (bit == 0)
    length  = x;
  else {
    unsigned init_base = base;
    base   += x;
    length -= x;
    if (init_base > base) propagate_carry();               // overflow = carry
  }

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Static_Bit_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  unsigned x = M.bit_0_prob * (length >> BM__LengthShift);   // product l x p0
  unsigned bit = (value >= x);                                     // decision
                                                    // update & shift interval
  if (bit == 0)
    length  = x;
  else {
    value  -= x;                                 // shifted interval base = 0
    length -= x;
  }

  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  return bit;                                         // return data bit value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned bit,
                              Adaptive_Bit_Model & M)
{
	g_count += 1;
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
#endif

  unsigned x = M.bit_0_prob * (length >> BM__LengthShift);   // product l x p0
                                                            // update interval
  if (bit == 0) {
    length = x;
    ++M.bit_0_count;
  }
  else {
    unsigned init_base = base;
    base   += x;
    length -= x;
    if (init_base > base) propagate_carry();               // overflow = carry
  }

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization

  if (--M.bits_until_update == 0) M.update();         // periodic model update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Adaptive_Bit_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  unsigned x = M.bit_0_prob * (length >> BM__LengthShift);   // product l x p0
  unsigned bit = (value >= x);                                     // decision
                                                            // update interval
  if (bit == 0) {
    length = x;
    ++M.bit_0_count;
  }
  else {
    value  -= x;
    length -= x;
  }

  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  if (--M.bits_until_update == 0) M.update();         // periodic model update

  return bit;                                         // return data bit value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned data,
                              Static_Data_Model & M)
{
	g_count += 1;
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
  if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

  unsigned x, init_base = base;
                                                           // compute products
  if (data == M.last_symbol) {
    x = M.distribution[data] * (length >> DM__LengthShift);
    base   += x;                                            // update interval
    length -= x;                                          // no product needed
  }
  else {
    x = M.distribution[data] * (length >>= DM__LengthShift);
    base   += x;                                            // update interval
    length  = M.distribution[data+1] * length - x;
  }
             
  if (init_base > base) propagate_carry();                 // overflow = carry

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Static_Data_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  unsigned n, s, x, y = length;

  //if (M.decoder_table) {              // use table look-up for faster decoding

  //  unsigned dv = value / (length >>= DM__LengthShift);
  //  unsigned t = dv >> M.table_shift;

  //  s = M.decoder_table[t];         // initial decision based on table look-up
  //  n = M.decoder_table[t+1] + 1;

  //  while (n > s + 1) {                        // finish with bisection search
  //    unsigned m = (s + n) >> 1;
  //    if (M.distribution[m] > dv) n = m; else s = m;
  //  }
  //                                                         // compute products
  //  x = M.distribution[s] * length;
  //  if (s != M.last_symbol) y = M.distribution[s+1] * length;
  //}

  //else {                                  // decode using only multiplications

    x = s = 0;
    length >>= DM__LengthShift;
    unsigned m = (n = M.data_symbols) >> 1;
                                                // decode via bisection search
    do {
      unsigned z = length * M.distribution[m];
      if (z > value) {
        n = m;
        y = z;                                             // value is smaller
      }
      else {
        s = m;
        x = z;                                     // value is larger or equal
      }
    } while ((m = (s + n) >> 1) != s);
  //}

  value -= x;                                               // update interval
  length = y - x;

  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned data,
                              Adaptive_Data_Model & M)
{
	g_count += 1;
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
  if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

  unsigned x, init_base = base;
                                                           // compute products
  if (data == M.last_symbol) {
    x = M.distribution[data] * (length >> DM__LengthShift);
    base   += x;                                            // update interval
    length -= x;                                          // no product needed
  }
  else {
    x = M.distribution[data] * (length >>= DM__LengthShift);
    base   += x;                                            // update interval
    length  = M.distribution[data+1] * length - x;
  }

  if (init_base > base) propagate_carry();                 // overflow = carry

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization

  ++M.symbol_count[data];
  if (--M.symbols_until_update == 0) M.update(true);  // periodic model update
}

void Arithmetic_Codec::noencode(unsigned data,
	Adaptive_Data_Model & M)
{
	++M.symbol_count[data];
	if (--M.symbols_until_update == 0) M.update(true);  // periodic model update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Adaptive_Data_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  unsigned n, s, x, y = length;

  //if (M.decoder_table) {              // use table look-up for faster decoding

  //  unsigned dv = value / (length >>= DM__LengthShift);
  //  unsigned t = dv >> M.table_shift;

  //  s = M.decoder_table[t];         // initial decision based on table look-up
  //  n = M.decoder_table[t+1] + 1;

  //  while (n > s + 1) {                        // finish with bisection search
  //    unsigned m = (s + n) >> 1;
  //    if (M.distribution[m] > dv) n = m; else s = m;
  //  }
  //                                                         // compute products
  //  x = M.distribution[s] * length;
  //  if (s != M.last_symbol) y = M.distribution[s+1] * length;
  //}

  //else {                                  // decode using only multiplications

    x = s = 0;
    length >>= DM__LengthShift;
    unsigned m = (n = M.data_symbols) >> 1;
                                                // decode via bisection search
    do {
      unsigned z = length * M.distribution[m];
      if (z > value) {
        n = m;
        y = z;                                             // value is smaller
      }
      else {
        s = m;
        x = z;                                     // value is larger or equal
      }
    } while ((m = (s + n) >> 1) != s);
  //}

  value -= x;                                               // update interval
  length = y - x;

  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  ++M.symbol_count[s];
  if (--M.symbols_until_update == 0) M.update(false);  // periodic model update

  return s;
}

void Arithmetic_Codec::nodecode(Adaptive_Data_Model & M, int s)
{
	++M.symbol_count[s];
	if (--M.symbols_until_update == 0) M.update(false);  // periodic model update
}

void Arithmetic_Codec::encode(unsigned data, Adaptive_Data_Model & M, int forbidden)
{
	g_count += 1;
#ifdef _DEBUG
	if (mode != 1) AC_Error("encoder not initialized");
	if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif
	//update distribution
	if (forbidden >= 0 && forbidden < M.data_symbols)
	{
		unsigned j, k, sum = 0;

		//int total = 0;
		//for (int i = 0; i < M.data_symbols; i++) total += M.symbol_count[i];

		unsigned scale = 0x80000000U / (M.total_count - M.symbol_count[forbidden]);
		for (k = 0; k < M.data_symbols; k++)
		{
			if (k == forbidden) continue;
			j = k;
			if (j > forbidden) j -= 1;
			M.distribution_forbidden[j] = (scale * sum) >> (31 - DM__LengthShift);
			sum += M.symbol_count[k];
		}
		unsigned data_code = data;
		if (data_code > forbidden) data_code -= 1;

		unsigned x, init_base = base;
		// compute products


		if (data_code == M.last_symbol - 1) {
			x = M.distribution_forbidden[data_code] * (length >> DM__LengthShift);
			base += x;                                            // update interval
			length -= x;                                          // no product needed
		}
		else {
			x = M.distribution_forbidden[data_code] * (length >>= DM__LengthShift);
			base += x;                                            // update interval
			length = M.distribution_forbidden[data_code + 1] * length - x;
		}

		if (init_base > base) propagate_carry();                 // overflow = carry

		if (length < AC__MinLength) renorm_enc_interval();        // renormalization

		++M.symbol_count[data];
		if (--M.symbols_until_update == 0) M.update(true);  // periodic model update
	}
	else
	{
		unsigned x, init_base = base;
		// compute products
		if (data == M.last_symbol) {
			x = M.distribution[data] * (length >> DM__LengthShift);
			base += x;                                            // update interval
			length -= x;                                          // no product needed
		}
		else {
			x = M.distribution[data] * (length >>= DM__LengthShift);
			base += x;                                            // update interval
			length = M.distribution[data + 1] * length - x;
		}

		if (init_base > base) propagate_carry();                 // overflow = carry

		if (length < AC__MinLength) renorm_enc_interval();        // renormalization

		++M.symbol_count[data];
		if (--M.symbols_until_update == 0) M.update(true);  // periodic model update
	}
}

unsigned Arithmetic_Codec::decode(Adaptive_Data_Model & M, int forbidden)
{
#ifdef _DEBUG
	if (mode != 2) AC_Error("decoder not initialized");
#endif

	if (forbidden >= 0 && forbidden < M.data_symbols)
	{
		unsigned j, k, sum = 0;
		unsigned scale = 0x80000000U / (M.total_count - M.symbol_count[forbidden]);
		for (k = 0; k < M.data_symbols; k++)
		{
			if (k == forbidden) continue;
			j = k;
			if (j > forbidden) j -= 1;
			M.distribution_forbidden[j] = (scale * sum) >> (31 - DM__LengthShift);
			sum += M.symbol_count[k];
		}

		unsigned n, s, x, y = length;

		//if (M.decoder_table) {              // use table look-up for faster decoding

		//	unsigned dv = value / (length >>= DM__LengthShift);
		//	unsigned t = dv >> M.table_shift;

		//	s = M.decoder_table[t];         // initial decision based on table look-up
		//	n = M.decoder_table[t + 1] + 1;

		//	while (n > s + 1) {                        // finish with bisection search
		//		unsigned m = (s + n) >> 1;
		//		if (M.distribution[m] > dv) n = m; else s = m;
		//	}
		//	// compute products
		//	x = M.distribution[s] * length;
		//	if (s != M.last_symbol) y = M.distribution[s + 1] * length;
		//}

		//else {                                  // decode using only multiplications

			x = s = 0;
			length >>= DM__LengthShift;
			unsigned m = (n = (M.data_symbols - 1)) >> 1;
			// decode via bisection search
			do {
				unsigned z = length * M.distribution_forbidden[m];
				if (z > value) {
					n = m;
					y = z;                                             // value is smaller
				}
				else {
					s = m;
					x = z;                                     // value is larger or equal
				}
			} while ((m = (s + n) >> 1) != s);
		//}

		value -= x;                                               // update interval
		length = y - x;

		if (length < AC__MinLength) renorm_dec_interval();        // renormalization
		if (s >= forbidden) s++;

		++M.symbol_count[s];
		if (--M.symbols_until_update == 0) M.update(false);  // periodic model update

		return s;
	}
	else
	{
		unsigned n, s, x, y = length;

		//if (M.decoder_table) {              // use table look-up for faster decoding

		//	unsigned dv = value / (length >>= DM__LengthShift);
		//	unsigned t = dv >> M.table_shift;

		//	s = M.decoder_table[t];         // initial decision based on table look-up
		//	n = M.decoder_table[t + 1] + 1;

		//	while (n > s + 1) {                        // finish with bisection search
		//		unsigned m = (s + n) >> 1;
		//		if (M.distribution[m] > dv) n = m; else s = m;
		//	}
		//	// compute products
		//	x = M.distribution[s] * length;
		//	if (s != M.last_symbol) y = M.distribution[s + 1] * length;
		//}

		//else {                                  // decode using only multiplications

			x = s = 0;
			length >>= DM__LengthShift;
			unsigned m = (n = M.data_symbols) >> 1;
			// decode via bisection search
			do {
				unsigned z = length * M.distribution[m];
				if (z > value) {
					n = m;
					y = z;                                             // value is smaller
				}
				else {
					s = m;
					x = z;                                     // value is larger or equal
				}
			} while ((m = (s + n) >> 1) != s);
		//}

		value -= x;                                               // update interval
		length = y - x;

		if (length < AC__MinLength) renorm_dec_interval();        // renormalization

		++M.symbol_count[s];
		if (--M.symbols_until_update == 0) M.update(false);  // periodic model update

		return s;
	}
}

void Arithmetic_Codec::encode(unsigned data, Adaptive_Data_Model &M, int forbidden1, int forbidden2)
{
	g_count += 1;
#ifdef _DEBUG
	if (mode != 1) AC_Error("encoder not initialized");
	if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif
	if (forbidden2 < forbidden1) swap(forbidden1, forbidden2);

	//int total = 0;
	//for (int i = 0; i < M.data_symbols; i++) total += M.symbol_count[i];

	unsigned j, k, sum = 0;
	unsigned scale = 0x80000000U / (M.total_count - M.symbol_count[forbidden1] - M.symbol_count[forbidden2]);
	for (k = 0; k < M.data_symbols; k++)
	{
		if (k == forbidden1 || k == forbidden2) continue;
		j = k;
		if (j > forbidden2) j -= 2;
		else if (j > forbidden1) j -= 1;
		M.distribution_forbidden[j] = (scale * sum) >> (31 - DM__LengthShift);
		sum += M.symbol_count[k];
	}
	unsigned data_code = data;
	if (data_code > forbidden2) data_code -= 2;
	else if(data_code > forbidden1) data_code -= 1;

	unsigned x, init_base = base;

	if (data_code == M.last_symbol - 2) {
		x = M.distribution_forbidden[data_code] * (length >> DM__LengthShift);
		base += x;                                            // update interval
		length -= x;                                          // no product needed
	}
	else {
		x = M.distribution_forbidden[data_code] * (length >>= DM__LengthShift);
		base += x;                                            // update interval
		length = M.distribution_forbidden[data_code + 1] * length - x;
	}

	if (init_base > base) propagate_carry();                 // overflow = carry

	if (length < AC__MinLength) renorm_enc_interval();        // renormalization

	++M.symbol_count[data];
	if (--M.symbols_until_update == 0) M.update(true);  // periodic model update
}

unsigned Arithmetic_Codec::decode(Adaptive_Data_Model &M, int forbidden1, int forbidden2)
{
#ifdef _DEBUG
	if (mode != 2) AC_Error("decoder not initialized");
#endif
	if (forbidden2 < forbidden1) swap(forbidden1, forbidden2);

	unsigned j, k, sum = 0;
	unsigned scale = 0x80000000U / (M.total_count - M.symbol_count[forbidden1] - M.symbol_count[forbidden2]);
	for (k = 0; k < M.data_symbols; k++)
	{
		if (k == forbidden1 || k == forbidden2) continue;
		j = k;
		if (j > forbidden2) j -= 2;
		else if (j > forbidden1) j -= 1;
		M.distribution_forbidden[j] = (scale * sum) >> (31 - DM__LengthShift);
		sum += M.symbol_count[k];
	}

	unsigned n, s, x, y = length;

	x = s = 0;
	length >>= DM__LengthShift;
	unsigned m = (n = (M.data_symbols - 2)) >> 1;
	do {
		unsigned z = length * M.distribution_forbidden[m];
		if (z > value) {
			n = m;
			y = z;                                             // value is smaller
		}
		else {
			s = m;
			x = z;                                     // value is larger or equal
		}
	} while ((m = (s + n) >> 1) != s);

	value -= x;                                               // update interval
	length = y - x;

	if (length < AC__MinLength) renorm_dec_interval();        // renormalization
	if (s >= forbidden1) s++;
	if (s >= forbidden2) s++;

	++M.symbol_count[s];
	if (--M.symbols_until_update == 0) M.update(false);  // periodic model update

	return s;
}
int Arithmetic_Codec::getByteNum()
{
	return writeBytes;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Other Arithmetic_Codec implementations  - - - - - - - - - - - - - - - -

Arithmetic_Codec::Arithmetic_Codec(void)
{
  mode = buffer_size = 0;
  new_buffer = code_buffer = 0;
}

Arithmetic_Codec::Arithmetic_Codec(unsigned max_code_bytes,
                                   unsigned char * user_buffer)
{
  mode = buffer_size = 0;
  new_buffer = code_buffer = 0;
  set_buffer(max_code_bytes, user_buffer);
}

Arithmetic_Codec::~Arithmetic_Codec(void)
{
  delete [] new_buffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::set_buffer(unsigned max_code_bytes,
                                  unsigned char * user_buffer)
{
                                                  // test for reasonable sizes
  if ((max_code_bytes < 16) || (max_code_bytes > 0x1000000U))
    AC_Error("invalid codec buffer size");
  if (mode != 0) AC_Error("cannot set buffer while encoding or decoding");

  if (user_buffer != 0) {                       // user provides memory buffer
    buffer_size = max_code_bytes;
    code_buffer = user_buffer;               // set buffer for compressed data
    delete [] new_buffer;                 // free anything previously assigned
    new_buffer = 0;
    return;
  }

  if (max_code_bytes <= buffer_size) return;               // enough available

  buffer_size = max_code_bytes;                           // assign new memory
  delete [] new_buffer;                   // free anything previously assigned
  if ((new_buffer = new unsigned char[buffer_size+16]) == 0) // 16 extra bytes
    AC_Error("cannot assign memory for compressed data buffer");
  code_buffer = new_buffer;                  // set buffer for compressed data
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::start_encoder(void)
{
  if (mode != 0) AC_Error("cannot start encoder");
  if (buffer_size == 0) AC_Error("no code buffer set");

  mode   = 1;
  base   = 0;            // initialize encoder variables: interval and pointer
  length = AC__MaxLength;
  ac_pointer = code_buffer;                       // pointer to next data byte
  writeBytes = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::start_decoder(void)
{
  if (mode != 0) AC_Error("cannot start decoder");
  if (buffer_size == 0) AC_Error("no code buffer set");

                  // initialize decoder: interval, pointer, initial code value
  mode   = 2;
  length = AC__MaxLength;
  ac_pointer = code_buffer + 3;
  value = (unsigned(code_buffer[0]) << 24)|(unsigned(code_buffer[1]) << 16) |
          (unsigned(code_buffer[2]) <<  8)| unsigned(code_buffer[3]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::read_from_file(FILE * code_file)
{
  unsigned shift = 0, code_bytes = 0;
  int file_byte;
                      // read variable-length header with number of code bytes
  do {
    if ((file_byte = getc(code_file)) == EOF)
      AC_Error("cannot read code from file");
    code_bytes |= unsigned(file_byte & 0x7F) << shift;
    shift += 7;
  } while (file_byte & 0x80);
                                                       // read compressed data
  if (code_bytes > buffer_size) AC_Error("code buffer overflow");
  if (fread(code_buffer, 1, code_bytes, code_file) != code_bytes)
    AC_Error("cannot read code from file");

  start_decoder();                                       // initialize decoder
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::stop_encoder(void)
{
  if (mode != 1) AC_Error("invalid to stop encoder");
  mode = 0;

  unsigned init_base = base;            // done encoding: set final data bytes

  if (length > 2 * AC__MinLength) {
    base  += AC__MinLength;                                     // base offset
    length = AC__MinLength >> 1;             // set new length for 1 more byte
  }
  else {
    base  += AC__MinLength >> 1;                                // base offset
    length = AC__MinLength >> 9;            // set new length for 2 more bytes
  }

  if (init_base > base) propagate_carry();                 // overflow = carry

  renorm_enc_interval();                // renormalization = output last bytes

  unsigned code_bytes = unsigned(ac_pointer - code_buffer);
  if (code_bytes > buffer_size) AC_Error("code buffer overflow");

  return code_bytes;                                   // number of bytes used
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::write_to_file(FILE * code_file)
{
  unsigned header_bytes = 0, code_bytes = stop_encoder(), nb = code_bytes;

                     // write variable-length header with number of code bytes
  do {
    int file_byte = int(nb & 0x7FU);
    if ((nb >>= 7) > 0) file_byte |= 0x80;
    if (putc(file_byte, code_file) == EOF)
      AC_Error("cannot write compressed data to file");
    header_bytes++;
  } while (nb);
                                                      // write compressed data
  if (fwrite(code_buffer, 1, code_bytes, code_file) != code_bytes)
    AC_Error("cannot write compressed data to file");

  return code_bytes + header_bytes;                              // bytes used
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::stop_decoder(void)
{
  if (mode != 2) AC_Error("invalid to stop decoder");
  mode = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - Static bit model implementation - - - - - - - - - - - - - - - - - - - - -

Static_Bit_Model::Static_Bit_Model(void)
{
  bit_0_prob = 1U << (BM__LengthShift - 1);                        // p0 = 0.5
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Static_Bit_Model::set_probability_0(double p0)
{
  if ((p0 < 0.0001)||(p0 > 0.9999)) AC_Error("invalid bit probability");
  bit_0_prob = unsigned(p0 * (1 << BM__LengthShift));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - Adaptive bit model implementation - - - - - - - - - - - - - - - - - - - -

Adaptive_Bit_Model::Adaptive_Bit_Model(void)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Bit_Model::reset(void)
{
                                       // initialization to equiprobable model
  bit_0_count = 1;
  bit_count   = 2;
  bit_0_prob  = 1U << (BM__LengthShift - 1);
  update_cycle = bits_until_update = 1;// 4;         // start with frequent updates
}

void Adaptive_Bit_Model::printPro(void)
{
  printf("%f ", float(bit_0_prob)/8191);
  printf("%f \n", 1 - float(bit_0_prob)/8191);
}

void Adaptive_Bit_Model::setPro(float prob0)
{
  bit_count = 10;
  bit_0_count = int(bit_count*prob0);
  unsigned scale = 0x80000000U / bit_count;
  bit_0_prob = (bit_0_count * scale) >> (31 - BM__LengthShift);
}

void Adaptive_Bit_Model::copy(int * count_list)
{
	bit_count = count_list[0] + count_list[1];
	bit_0_count = count_list[0];
	unsigned scale = 0x80000000U / bit_count;
	bit_0_prob = (bit_0_count * scale) >> (31 - BM__LengthShift);
}

float Adaptive_Bit_Model::get_bits(unsigned data)
{
	if (data == 0)	return -log2(bit_0_count / float(bit_count));
	else return -log2(1 - bit_0_count / float(bit_count));
}

int Adaptive_Bit_Model::get_count()
{
	return bit_count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Bit_Model::update(void)
{
                                   // halve counts when a threshold is reached

  if ((bit_count += update_cycle) > BM__MaxCount) {
    bit_count = (bit_count + 1) >> 1;
    bit_0_count = (bit_0_count + 1) >> 1;
    if (bit_0_count == bit_count) ++bit_count;
  }
                                           // compute scaled bit 0 probability
  unsigned scale = 0x80000000U / bit_count;
  bit_0_prob = (bit_0_count * scale) >> (31 - BM__LengthShift);

                                             // set frequency of model updates
  update_cycle = (5 * update_cycle) >> 2;
  if (update_cycle > 64) update_cycle = 64;
  bits_until_update = update_cycle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Static data model implementation  - - - - - - - - - - - - - - - - - - -

Static_Data_Model::Static_Data_Model(void)
{
  data_symbols = 0;
  distribution = 0;
}

Static_Data_Model::~Static_Data_Model(void)
{
  delete [] distribution;
}

Static_Data_Model::Static_Data_Model(unsigned number_of_symbols)
{
	data_symbols = 0;
	distribution = 0;
	set_alphabet(number_of_symbols);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Static_Data_Model::set_alphabet(unsigned number_of_symbols,
                                         const double probability[])
{
  if ((number_of_symbols < 2) || (number_of_symbols > (1 << 11)))
    AC_Error("invalid number of data symbols");

  if (data_symbols != number_of_symbols) {     // assign memory for data model
    data_symbols = number_of_symbols;
    last_symbol = data_symbols - 1;
    delete [] distribution;
                                     // define size of table for fast decoding
/*    if (data_symbols > 16) {
      unsigned table_bits = 3;
      while (data_symbols > (1U << (table_bits + 2))) ++table_bits;
      table_size  = (1 << table_bits) + 4;
      table_shift = DM__LengthShift - table_bits;
      distribution = new unsigned[data_symbols+table_size+6];
      decoder_table = distribution + data_symbols;
    }
    else {                  */                // small alphabet: no table needed
      decoder_table = 0;
      table_size = table_shift = 0;
      distribution = new unsigned[data_symbols];
    //}
    if (distribution == 0) AC_Error("cannot assign model memory");
  }
                             // compute cumulative distribution, decoder table
  unsigned s = 0;
  double sum = 0.0, p = 1.0 / double(data_symbols);

  for (unsigned k = 0; k < data_symbols; k++) {
    if (probability) p = probability[k];
    if ((p < 0.0001) || (p > 0.9999)) AC_Error("invalid symbol probability");
    distribution[k] = unsigned(sum * (1 << DM__LengthShift));
    sum += p;
    if (table_size == 0) continue;
    unsigned w = distribution[k] >> table_shift;
    while (s < w) decoder_table[++s] = k - 1;
  }

  //if (table_size != 0) {
  //  decoder_table[0] = 0;
  //  while (s <= table_size) decoder_table[++s] = data_symbols - 1;
  //}

  if ((sum < 0.9999) || (sum > 1.0001)) AC_Error("invalid probabilities");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Adaptive data model implementation  - - - - - - - - - - - - - - - - - -

Adaptive_Data_Model::Adaptive_Data_Model(void)
{
  data_symbols = 0;
  distribution = 0;
  distribution_forbidden = 0;
}

Adaptive_Data_Model::Adaptive_Data_Model(unsigned number_of_symbols)
{
  data_symbols = 0;
  distribution = 0;
  distribution_forbidden = 0;
  set_alphabet(number_of_symbols);
}

Adaptive_Data_Model::~Adaptive_Data_Model(void)
{
  delete [] distribution;
  delete [] distribution_forbidden;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::set_alphabet(unsigned number_of_symbols)
{
  if ((number_of_symbols < 2) || (number_of_symbols > (1 << 11)))
    AC_Error("invalid number of data symbols");

  if (data_symbols != number_of_symbols) {     // assign memory for data model
    data_symbols = number_of_symbols;
    last_symbol = data_symbols - 1;
	delete [] distribution;
	delete [] distribution_forbidden;
                                     // define size of table for fast decoding
/*    if (data_symbols > 16) {
      unsigned table_bits = 3;
      while (data_symbols > (1U << (table_bits + 2))) ++table_bits;
      table_size  = (1 << table_bits) + 4;
      table_shift = DM__LengthShift - table_bits;
      distribution = new unsigned[2*data_symbols+table_size+6];
	  distribution_forbidden = new unsigned[data_symbols - 1];
      decoder_table = distribution + 2 * data_symbols;
    }
    else {       */                           // small alphabet: no table needed
      decoder_table = 0;
      table_size = table_shift = 0;
      distribution = new unsigned[2*data_symbols];
	  distribution_forbidden = new unsigned[data_symbols - 1];
    //}
    symbol_count = distribution + data_symbols;
    if (distribution == 0) AC_Error("cannot assign model memory");
  }

  reset();                                                 // initialize model
}

void Adaptive_Data_Model::printPro(void)
{
  for (int i = 0; i < data_symbols - 1; i++)
  {
    printf("%f ", float(int(distribution[i+1])-int(distribution[i]))/32767);
  }
  printf("%f ", float(32767-int(distribution[data_symbols-1]))/32767);
  printf("\n");
}

void Adaptive_Data_Model::setPro(float prob[])
{
  total_count = 9;
  for (unsigned k = 0; k < data_symbols; k++) 
    symbol_count[k] = int(10 * prob[k]);
  update(false);
}

void Adaptive_Data_Model::copy(Adaptive_Data_Model & M, int start)
{
	total_count = 0;
	unsigned j = 0;
	for (unsigned k = 0; k < data_symbols; k++)
	{
		symbol_count[k] = M.symbol_count[k+ start];
		total_count += symbol_count[k];
	}
	total_count--;
	update_cycle = 10;
	update(false);
}

int init[434] = { 
	              522, 239,
                  452, 253, 21,   
	              396, 258, 38, 6,
	              358, 254, 53, 11, 3,
	              330, 246, 64, 18, 5, 2,
	              310, 238, 72, 23, 8, 3, 2, 
	              289, 230, 79, 29, 11, 4, 2, 1,
                  277, 221, 82, 33, 14, 6, 3, 1, 1,
	              264, 213, 85, 37, 17, 8, 4, 2, 1, 1,
	              252, 206, 88, 40, 19, 10, 5, 3, 1, 1, 1,              
	              240, 199, 89, 43, 22, 12, 6, 3, 2, 1, 1, 1,
	              230, 193, 90, 45, 24, 14, 8, 5, 3, 2, 1, 1, 1,
	              217, 184, 91, 49, 27, 16, 9, 5, 3, 2, 1, 1, 1, 1,
	              209, 177, 92, 51, 30, 17, 10, 6, 4, 3, 2, 1, 1, 1, 1,
	              214, 181, 89, 47, 28, 17, 10, 7, 4, 3, 2, 1, 1, 1, 1, 1,
	              196, 173, 89, 51, 32, 20, 12, 8, 6, 4, 3, 2, 1, 1, 1, 1, 1,
	              192, 165, 89, 53, 32, 21, 14, 9, 7, 4, 3, 2, 1, 1, 1, 1, 1, 1,
	              197, 169, 89, 51, 31, 19, 13, 9, 7, 4, 3, 2, 1, 1, 1, 1, 1, 1, 1,
	              190, 162, 88, 54, 32, 21, 14, 10, 7, 5, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1,
	              196, 169, 84, 49, 31, 22, 14, 9, 7, 5, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	              180, 150, 85, 53, 36, 25, 16, 12, 9, 7, 5, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	              173, 150, 89, 53, 34, 22, 17, 12, 8, 6, 5, 4, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	              177, 146, 82, 51, 35, 25, 17, 13, 9, 8, 6, 5, 4, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	              168, 144, 82, 53, 37, 25, 19, 14, 11, 7, 5, 4, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	              168, 139, 76, 51, 36, 28, 20, 16, 12, 9, 7, 6, 4, 4, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	              158, 136, 82, 56, 41, 27, 19, 15, 11, 8, 6, 4, 3, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	              141, 124, 79, 56, 41, 31, 23, 17, 13, 11, 8, 6, 4, 4, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	              137, 120, 80, 59, 43, 31, 22, 17, 15, 9, 8, 7, 5, 4, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1   //29
};


void Adaptive_Data_Model::copy(int* count_list, int low)
{
	total_count = 0;

	//bool initial = false;
	//for (unsigned k = 0; k < data_symbols; k++)
	//{
	//	if (count_list[256 + k + low] > 1) initial = true;
	//}
	//if (initial)
	//{
	//	for (unsigned k = 0; k < data_symbols; k++)
	//	{
	//		symbol_count[k] = count_list[256 + k + low];
	//		total_count += count_list[256 + k + low];
	//	}
	//}
	//else
	//{
		if (low >= 0 || low + int(data_symbols) - 1 <= 0)
			for (unsigned k = 0; k < data_symbols; k++)
			{
				symbol_count[k] = count_list[256 + k + low];// count_list[256 + k + low];
				total_count += count_list[256 + k + low];// count_list[256 + k + low];
			}
		else
		{
			int max = low + int(data_symbols) - 1;
			int side = -low;
			int scale = 3;
			if (max > side) side = max;
			if (side > 28)
			{
				int offset = (28 + 2)*(28 - 1) / 2;
				symbol_count[-low] = (init[offset] + scale - 1) / scale + count_list[256] - 1;
				total_count += (init[offset] + scale - 1) / scale + count_list[256] - 1;
				for (int k = 1; k < side + 1; k++)
				{
					if ((-low - k) >= 0)
					{
						if (k > 28)
						{
							symbol_count[-low - k] = 1 + count_list[256 - k] - 1;
							total_count += 1 + count_list[256 - k] - 1;
						}
						else
						{
							symbol_count[-low - k] = (init[offset + k] + scale - 1) / scale + count_list[256 - k] - 1;
							total_count += (init[offset + k] + scale - 1) / scale + count_list[256 - k] - 1;
						}
					}
					if ((-low + k) < data_symbols)
					{
						if (k > 28)
						{
							symbol_count[-low + k] = 1 + count_list[256 + k] - 1;
							total_count += 1 + count_list[256 + k] - 1;
						}
						else
						{
							symbol_count[-low + k] = (init[offset + k] + scale - 1) / scale + count_list[256 + k] - 1;
							total_count += (init[offset + k] + scale - 1) / scale + count_list[256 + k] - 1;
						}
					}
				}
			}
			else
			{
				int offset = (side + 2)*(side - 1) / 2;
				symbol_count[-low] = (init[offset] + scale - 1) / scale + count_list[256] - 1;
				total_count += (init[offset] + scale - 1) / scale + count_list[256] - 1;
				for (int k = 1; k < side + 1; k++)
				{
					if ((-low - k) >= 0)
					{
						symbol_count[-low - k] = (init[offset + k] + scale - 1) / scale + count_list[256 - k] - 1;
						total_count += (init[offset + k] + scale - 1) / scale + count_list[256 - k] - 1;
					}
					if ((-low + k) < data_symbols)
					{
						symbol_count[-low + k] = (init[offset + k] + scale - 1) / scale + count_list[256 + k] - 1;
						total_count += (init[offset + k] + scale - 1) / scale + count_list[256 + k] - 1;
					}
				}
			}
		}
	//}
	total_count--;
	update(false);
}

void Adaptive_Data_Model::copy(int * count_list)
{
	total_count = 0;

	for (unsigned k = 0; k < data_symbols; k++)
	{
		symbol_count[k] = count_list[k];
		total_count += count_list[k];
	}

	total_count--;
	update(false);
}

float Adaptive_Data_Model::get_bits(unsigned data)
{
	return -log2(symbol_count[data]/ float(total_count));
}

float Adaptive_Data_Model::get_bits(unsigned data, int forbidden)
{
	if (forbidden >= 0 && forbidden < data_symbols)
	{
		return -log2(float(symbol_count[data]) / (total_count - symbol_count[forbidden]));
	}
	else
	{
		return -log2(float(symbol_count[data]) / total_count);
	}
}

int Adaptive_Data_Model::get_count()
{
	return total_count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::update(bool from_encoder)
{
                                   // halve counts when a threshold is reached

  if ((total_count += update_cycle) > DM__MaxCount) {
    total_count = 0;
    for (unsigned n = 0; n < data_symbols; n++)
      total_count += (symbol_count[n] = (symbol_count[n] + 1) >> 1);
  }
                             // compute cumulative distribution, decoder table
  unsigned k, sum = 0, s = 0;
  unsigned scale = 0x80000000U / total_count;

  //if (from_encoder || (table_size == 0))
    for (k = 0; k < data_symbols; k++) {
      distribution[k] = (scale * sum) >> (31 - DM__LengthShift);
      sum += symbol_count[k];
    }
  //else {
  //  for (k = 0; k < data_symbols; k++) {
  //    distribution[k] = (scale * sum) >> (31 - DM__LengthShift);
  //    sum += symbol_count[k];
  //    unsigned w = distribution[k] >> table_shift;
  //    while (s < w) decoder_table[++s] = k - 1;
  //  }
  //  decoder_table[0] = 0;
  //  while (s <= table_size) decoder_table[++s] = data_symbols - 1;
  //}
                                             // set frequency of model updates
  update_cycle = (5 * update_cycle) >> 2;
  unsigned max_cycle = (data_symbols + 6) << 3;
  if (update_cycle > max_cycle) update_cycle = max_cycle;
  symbols_until_update = update_cycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::reset(void)
{
  if (data_symbols == 0) return;

                      // restore probability estimates to uniform distribution
  total_count = 0;
  update_cycle = data_symbols;
  for (unsigned k = 0; k < data_symbols; k++) symbol_count[k] = 1;
  update(false);
  symbols_until_update = update_cycle = 1;// (data_symbols + 6) >> 1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
