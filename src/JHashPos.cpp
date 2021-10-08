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
 */

#include <stdlib.h>
#include <new>
#include <string.h>
#include <limits.h>
using namespace std;

#include "JHashPos.h"


namespace JojoDiff {

const int COLLISION_THRESHOLD = 4 ; /* override when collision counter exceeds threshold  */
const int COLLISION_HIGH = 4 ;      /* rate at which high quality samples should override */
const int COLLISION_LOW = 1 ;       /* rate at which low quality samples should override  */

/**
  * @brief Create a new hash-table with size (number of elements) not larger that the given size.
  *
  * Actual size will be based on the highest prime below the highest power of 2
  * lower or equal to the specified size, e.g. aiSze=8192 will create a hashtable
  * of 8191 elements.
  *
  * One element may be 4, 6 or 8 bytes.
  *
  * @param aiSze   size, in number of elements.
  */
JHashPos::JHashPos(int aiSze)
:  miHshColMax(COLLISION_THRESHOLD), miHshColCnt(COLLISION_THRESHOLD),
   miHshRlb(SMPSZE + SMPSZE / 2), miHshHit(0)
{
    /* get largest prime < aiSze */
    int liSzeIdx ;
    if (aiSze < 1)
        liSzeIdx = 1 ;
    else
        liSzeIdx = aiSze ;
    liSzeIdx = (liSzeIdx * 1024 * 1024) /                   // convert Mb to number of elements
                    (sizeof(hkey)+sizeof(off_t));
    liSzeIdx = getLowerPrime(liSzeIdx);                     // find nearest lower prime

    /* allocate hashtable */
    miHshPme = liSzeIdx ;                                   // keep for reference
    miHshSze = miHshPme * (sizeof(off_t) + sizeof(hkey));   // convert to bytes
    mzHshTblPos = (off_t *) malloc(miHshPme *               // allocate and initialize
                        (sizeof(off_t) + sizeof(hkey))) ;
    mkHshTblHsh = (hkey *) &mzHshTblPos[miHshPme] ;         // set address of hashes
    miLodCnt = miHshPme ;

    #if debug
      if (JDebug::gbDbg[DBGHSH])
        fprintf(JDebug::stddbg, "Hash Ini sizeof=%2ld+%2ld=%2ld, %d samples, %d bytes, address=%p-%p,%p-%p.\n",
            sizeof(hkey), sizeof(off_t), sizeof(hkey) + sizeof(off_t),
            miHshPme, miHshSze,
            mzHshTblPos, &mzHshTblPos[miHshPme], mkHshTblHsh, &mkHshTblHsh[miHshPme]) ;
    #endif
    #ifdef JDIFF_THROW_BAD_ALLOC
      if ( mzHshTblPos == null ) {
          throw bad_alloc() ;
      }
    #endif // JDIFF_THROW_BAD_ALLOC
}

/*
 * Destructor
 */
JHashPos::~JHashPos() {
	free(mzHshTblPos);
	mzHshTblPos = null ;
	mkHshTblHsh = null ;
}

/**
 * Hashtable add
 * @param alCurHsh      Hash key to add
 * @param azPos         Position to add
 * @param aiEqlCnt      Quality of the sample
 */
void JHashPos::add (hkey akCurHsh, off_t azPos, int aiEqlCnt ){
    /* Every time the load factor increases by 1
     * - increase miHshColMax: the ratio at which we store values to achieve a uniform distribution of samples
     * - increase miHshRlb: the number of bytes to verify (reliability range) to be sure there is no match
     */
    if ( miLodCnt > 0 ) {
        miLodCnt -- ;
    } else {
        miLodCnt = miHshPme ;
        miHshColMax += COLLISION_THRESHOLD ;
        miHshRlb += 4 ;
    }

    /* Increase the collision strategy counter
     * - HIGH for "good" samples
     * - LOW  for low-quality samples
     */
    if (aiEqlCnt <= SMPSZE * 2)
        miHshColCnt-= COLLISION_HIGH ;
    else
        miHshColCnt-= COLLISION_LOW ;    // reduce overrides by low-quality samples

    /* store key and value when the collision counter reaches the collision threshold */
    if (miHshColCnt <= 0 ) {
        /* calculate the index in the hashtable for the given key */
        int liIdx = (akCurHsh % miHshPme) ;

        /* debug */
        #if debug
        if (JDebug::gbDbg[DBGHSH])
            fprintf(JDebug::stddbg, "Hash Add %8d " P8zd " %8" PRIhkey " %c\n",
                    liIdx, azPos, akCurHsh,
                    (mkHshTblHsh[liIdx] == 0)?'.':'!');
        #endif

        /* store */
        mkHshTblHsh[liIdx] = akCurHsh ;
        mzHshTblPos[liIdx] = azPos ;
        miHshColCnt = miHshColMax ; // reset subsequent lost collisions counter
    }
} /* ufHshAdd */

/**
* @brief  Hashtable reset: consider table to be empty
*/
void JHashPos::reset () {
    miLodCnt = miHshPme ;
    miHshColMax = COLLISION_THRESHOLD;
    miHshColCnt = COLLISION_THRESHOLD;
    miHshRlb = SMPSZE + SMPSZE / 2;
};


/**
 * @brief Hasttable lookup
 * @param alCurHsh  in:  hash key to lookup
 * @param lzPos     out: position found
 * @return true=found, false=notfound
 */
bool JHashPos::get (const hkey akCurHsh, off_t &azPos)
{ int   liIdx ;

  /* calculate key and the corresponding entries' address */
  liIdx    = (akCurHsh % miHshPme) ;

  /* lookup value into hashtable for new file */
  if (mkHshTblHsh[liIdx] == akCurHsh)  {
    miHshHit++;
    azPos = mzHshTblPos[liIdx];
    return true ;
  }
  return false ;
}

/**
 * @brief Print hashtable content (for debugging or auditing)
 */
void JHashPos::print(){
    int liHshIdx;

    for (liHshIdx = 0; liHshIdx < miHshPme; liHshIdx ++)  {
        if (mzHshTblPos[liHshIdx] != 0) {
            fprintf(JDebug::stddbg, "Hash Pnt %12d " P8zd "-%08" PRIhkey "x\n", liHshIdx,
                    mzHshTblPos[liHshIdx], mkHshTblHsh[liHshIdx]) ;
        }
    }
}

/**
 * @brief Print hashtable distribution  (for debugging or auditing)
 * @param azMax     Largest possible position to find
 * @param aiBck     Number of buckets
 */
void JHashPos::dist(off_t azMax, int aiBck){
    int liHshIdx;
    int liHshDiv;   // Number of positions by bucket
    int *liBckCnt;  // Number of elements by bucket
    int liIdx;

    int liCnt = 0 ;
    int liMin = INT_MAX ;
    int liMax = 0;

    fprintf(JDebug::stddbg, "Hash Dist Overload    = %d\n", miHshColMax / COLLISION_THRESHOLD - 1);
    fprintf(JDebug::stddbg, "Hash Dist Reliability = %d\n", miHshRlb);

    liBckCnt = (int *) malloc(aiBck * sizeof(int));
    if (liBckCnt != null){
    	/* Init mem */
    	memset(liBckCnt, 0, aiBck * sizeof(int));

    	/* Fill the buckets */
    	liHshDiv = (azMax / aiBck) ;
        for (liHshIdx = 0; liHshIdx < miHshPme; liHshIdx ++)  {
            if (mzHshTblPos[liHshIdx] > 0 && mzHshTblPos[liHshIdx] <= azMax) {
            	liIdx =mzHshTblPos[liHshIdx] / liHshDiv ;
            	if (liIdx >= aiBck) {
            		liIdx = 0 ;
            	} else {
            		liBckCnt[liIdx] ++ ;
            	}
            }
        }

        /* Printout */
        for (liIdx = 0; liIdx < aiBck; liIdx ++)  {
        	liCnt += liBckCnt[liIdx] ;
        	if (liBckCnt[liIdx] < liMin) liMin = liBckCnt[liIdx] ;
        	if (liBckCnt[liIdx] > liMax) liMax = liBckCnt[liIdx] ;

        	fprintf(JDebug::stddbg, "Hash Dist %8d Pos=" P8zd ":" P8zd " Cnt=%8d Rlb=%d\n",
        			liIdx, (off_t) liIdx * liHshDiv, (off_t) (liIdx + 1) * liHshDiv, liBckCnt[liIdx],
        			(liBckCnt[liIdx]==0)?-1:liHshDiv / liBckCnt[liIdx]) ;
        }
        fprintf(JDebug::stddbg, "Hash Dist Avg/Min/Max/%% = %d/%d/%d/%d%%\n",
                liCnt / aiBck, liMin, liMax, liMax > 0 ? (100 - (liMin / (liMax / 100))) : -1);
        fprintf(JDebug::stddbg, "Hash Dist Load          = %d/%d=%d%%\n",
                liCnt, miHshPme, miHshPme > 0 ? (liCnt / (miHshPme / 100)) : -1);
    }
} /* JHasPos::dist */

} /* namespace jojodiff */
