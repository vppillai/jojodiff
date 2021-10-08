/*
 * JHashPos.cpp
 *
 * Copyright (C) 2002-2020 Joris Heirbaut
 *
 * This file is part of JojoDiff.
 *
 * JojoDiff is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************
 * Hash table functions:
 *  add      Insert value into hashtable
 *  get      Lookup value into hashtable
 *  hash     Incremental hash function on array of bytes
 *
 * The hash table stores positions within files. The key reflects the contents
 * of the file at that position. This way, we can efficiently find regions
 * that are equal between both files.
 *
 * Hash function (see JDiff.cpp::hash) on array of bytes:
 *
 * Principles:
 * -----------
 * Input:  a[n]   8-bit values (characters, n=32 or 64 or ...)
 *         e[n]   equality values, each e[x] is between 0 and SMPSZE
 *                  e[x] = 0             if a[x-1] <> a[x]
 *                  e[x] = e[x-1] + 1    if a[x-1] == a[x] and e[x-1] < SMPSZE
 *                  e[x] = e[x-1]        if a[x-1] == a[x] and e[x-1] == SMPSZE
 *                  e[0] = 0
 *         p      prime number
 * Output: h[n]   hash for every a[n]
 *         k[n]   key (index) for every a[n]
 *
 * Calculations are as follows:
 *         h = ((a[0] + e[0]) x 2^n + (a[1] + e[1]) x 2^(n-1) + .. + (a[n] + e[n])) % 2^n
 *         k = h % p
 *
 * Example (simplified to 4-bit keys and values and n=8)
 *     n :  0 1 2 3 4 5 6 7 8 9 a b c d e f ...
 *   a[x]:  5 2 7 0 0 0 0 0 0 7 2 3 3 3 3 3 3 3 3 3 3 3 3 2 ...
 *   e[x]:  0 0 0 0 1 2 3 4 5 0 0 0 1 2 3 4 5 6 7 8 8 8 8 0 ...
 *   h[x]:  5 c f e d c b a 9 9 4 b a 9 8 7 6 5 4 3 1 d 5 c ...
 *
 * By adding  e[x] to every  a[x], long runs of equal bytes (0's or 3's in the example)
 * become distinguishable: every 0 or 3 yields a different h[x]. Only after n x 2 times
 * the same value, hashkeys become indistinguishable, leaving the algorithm n x 2 bytes
 * to detect equal regions.
 *
 * By comparison, without e the hashkey would become as follows:
 *     n :  0 1 2 3 4 5 6 7 8 9 a b c d e f ...
 *   a[x]:  5 2 7 0 0 0 0 0 0 7 2 3 3 3 3 3 3 3 3 3 3 3 3 2 ...
 *  h'[n]:  5 c f e c 8 0 0 0 7 0 3 9 5 d d d d d d d d d c ...
 *
 * Largest n-bit primes: 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521,
 *                       131071 (17 bit), ..., 4294967291 (32 bit)
 *
 * Table entries contain a 32-bit hash value and a file position.
 *
 * The collision strategy tries to create a uniform distributed set of samples
 * over the  investigated  region,  which is  either  the  whole  file or  the
 * look-ahead region.
 * This is achieved by overwriting the original entry if either
 * - the original entry lies before the investigated region (the base position), or
 * - a number of subsequent non-overwriting collisions occur where
 *   x = (region size / hashtable size) + 2
 *   --> after x collisions, the collision wins where x is decreasing
 *       as the region mapped by the hashtable grows.
 *
 * as of 23-06-2009 (v0.7)
 * - samples having a high number of equal characters are less meaningfull
 *   than samples without equal characters, therefore higher quality samples
 *   win in the collision strategy.
 *
 * Only samples from the original file are stored.
 * Samples from the new file are looked up.
 *
 * The investigated region is either
 * - the whole file when the prescan option is used (default)
 * - the look-ahead region otherwise (option -ff)
 *
 * With prescan enabled, the algorithm behaves like a kind of
 * copy/insert algorithm (simulated with insert/delete/modify and backtrace
 * instructions). Without prescan nor backtrace, the algorithm behaves like an
 * insert/delete algorithm.
 *******************************************************************************/

#ifndef JHASHPOS_H_
#define JHASHPOS_H_

#include "JDefs.h"
#include "JDebug.h"

namespace JojoDiff {

/*
 * Hashtable of file positions for JDiff.
 */
class JHashPos {
public:
    /**
     * @brief Create a new hash-table with size not larger that the given size.
     *
     * Actual size will be based on the highest prime below the highest power of 2
     * lower or equal to the specified size, e.g. aiSze=8192 will create a hashtable
     * of 8191 elements.
     *
     * @param aiSze   size, in number of elements.
     */
	JHashPos(int aiSze);

	virtual ~JHashPos();
	JHashPos(JHashPos const&) = delete ;
	JHashPos& operator=(JHashPos const&) = delete ;

	/**
	* @brief Add key and position to the index hashtable.
	*
	* @param hkey   akCurHsh   Key
	* @param azPos  azPos      Associated file position
	* @param int    aiEqlCnt   Indication of equal byte within associated sample sequence
	*/
	void add (hkey akCurHsh, off_t azPos, int aiEqlCnt ) ;

	/**
	* @brief  Hashtable lookup
	*
	* @param  akCurHsh  Input:  Hashkey
	* @param  &azPos    Output: Associated file position
	* @return false = key not found, true = key found
	*/
	bool get (const hkey akCurHsh, off_t &azPos) ;

	/**
	* @brief  Hashtable reset: consider table to be empty
	*/
	void reset () ;

	/**
	* @brief Return the (un)reliability range
	*
	* The unreliability range an estimate of the number of bytes to verify
	* to find a match. Reliability decreases as the hashtable (over)load
	* increases.
	*
	*/
	inline int get_reliability() const {
		return miHshRlb ;
	}

	/**
	* @brief Hashtable printout
	*/
	void print() ;

	/**
	* @brief Printout hashtable distribution
	*/
	void dist(off_t azMax, int aiBck);

	/**
    * @brief return hashtable prime number
    */
	int get_hashprime(){return miHshPme;}

	/**
	* @brief return hashtable size in bytes
	*/
	int get_hashsize(){return miHshSze;}

	/**
	* @brief return hastable collision override threshold
	*/
	int get_hashcolmax(){return miHshColMax;}

	/**
	* @brief return number of hits found by this hashtable
	*/
	int get_hashhits(){return miHshHit;}

private:
	/* The hash table. Using a struct causes certain compilers (gcc) to align        */
	/* fields on 64-bit boundaries, causing 25% memory loss. Therefore, I use        */
	/* two arrays instead of an array of structs.                                    */
	off_t *mzHshTblPos=null ;    /**< Hash values: positions within the original file       */
	hkey  *mkHshTblHsh=null ;    /**< Hash keys                                             */

	/* Size */
	int miHshPme=0  ;       /**< prime number for size and hashing              				*/
	int miHshSze=0 ;        /**< Actual size in bytes of the hashtable          				*/

    /* State */
	int miHshColMax;        /**< max number of collisions before override       			  */
	int miHshColCnt;        /**< current number of subsequent collisions.               	  */
	int miHshRlb ;          /**< hashtable reliability: decreases as the overloading grows 	  */
    int miLodCnt=0 ;        /**< hashtable load-counter                                       */

    /* Statistics */
    int miHshHit;           /**< number of hits found by this hashtable                       */
};
}
#endif /* JHASHPOS_H_ */
