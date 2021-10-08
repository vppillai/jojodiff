/*******************************************************************************
 * JDiff.cpp
 *
 * Jojo's diff on binary files: main class.
 *
 * Copyright (C) 2002-2020 Joris Heirbaut
 *
 * This file is part of JojoDiff.
 *
 * This program is free software: you can redistribute it and/or modify
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
 * ----------------------------------------------------------------------------
 *
 * Class JDiff takes two JFiles as input and outputs to a JOut instance.
 *
 * If you want to reuse JojoDiff, you need following files:
 * - JDiff.h/cpp        The main JojoDiff class
 * - JHashPos.h/cpp     The hash table collection of (sample-key, position)
 * - JMatchTable.h/cpp  The matching table logic
 * - JDefs.h            Global definitions
 * - JDebug.h/cpp       Debugging definitions
 * - JOut.h             Abstract output class
 * - JFile.h            Abstract input class
 * + at least one descendant of JOut and JFile (may be of your own).
 *
 * Method jdiff performs the actual diffing:
 * 1) first, we create a hashtable collection of (hkey, position) pairs, where
 *      hkey = hash key of a 32 byte sample in the left input file
 *      position = position of this sample in the file
 * 2) compares both files byte by byte
 * 3) when a difference is found, looks ahead using search() to find the nearest
 *    equal region between both files
 * 4) output skip/delete/backtrace instructions to reach the found regions
 * 5) repeat steps 2-4 each time the end of an equal region is reached.
 *
 * Method search looks ahead on the input files to find the nearest equals regions:
 * - for every 32-byte sample in the right file, look in the hastable whether a
 *   similar sample exists in the left file.
 * - creates a table of matching regions using this catalog
 * - returns the nearest match from the table of matches
 *
 * There are two reasons for creating a matching table on top of the hashtable:
 * - The hashtable may only contain a (small) percentage of all samples, hence
 *   the nearest equal region is not always detected first.
 * - Samples are not always equal when their hash-keys are equal:
 *   In theory, there's only 1/2^(7*SMPSZE) chance that samples are really equal
 *   when their hash-keys match. In practice, it will be a lot higher unless source
 *   and destination files both contain random data.
 *
 * For explication of the hash function, see JHashPos.h
 *
 * Method buildFullIndex scans the source file and creates the hash table.
 *
 *******************************************************************************/
#include "JDefs.h"
#include "JDiff.h"
#include <limits.h>

#ifdef _FILE_OFFSET_BITS
#pragma message "INFO: FILE OFFSET BITS = " XSTR(_FILE_OFFSET_BITS)
#else
#pragma message "INFO: FILE OFFSET BITS NOT SET !"
#endif

#ifdef JDIFF_LARGEFILE
#pragma message "INFO: JDIFF_LARGEFILE set"
#else
#pragma message "INFO: JDIFF_LARGEFILE not set !"
#endif // JDIFF_LARGEFILE

#ifdef __MINGW32__
#pragma message "INFO: __MINGW32__ set"
#endif
#ifdef __MINGW64__
#pragma message "INFO: __MINGW64__ set"
#endif
#if debug
#pragma message "INFO: debug activated"
#else
#pragma message "INFO: debug not activated"
#endif
#pragma message "INFO: PRIzd = " XSTR(PRIzd)

#define PGSMRK 0x100000    /**< Progress mark: show progress in Mb (1024 * 1024 or 0x400 x 0x400)  */
#define PGSMSK 0x1ffffff   /**< Progress mask: show progress every 32Mb when (lzPos & PGSMSK == 0) */

namespace JojoDiff {

/*
 * Constructor
 */
JDiff::JDiff(
    JFile * const apFilOrg,     /* Original file */
    JFile * const apFilNew,     /* New file */
    JOut  * const apOut,        /* Output patch file */
    const int aiHshSze,         /* Hashtable size in MB */
    const int aiVerbse,         /* Verbosity level: between 0 and 3 */
    const int abSrcBkt,         /* Allow backtracking on original file (default: yes) */
    const int aiSrcScn,         /* Scan source-file: 0=no, 1=yes, 2=done */
    const int aiMchMax,         /* Maximum matches to search for */
    const int aiMchMin,         /* Minimum matches to search for */
    const int aiAhdMax,         /* Lookahead maximum (in bytes) */
    const bool abCmpAll         /* Compare all matches ? */
) : mpFilOrg(apFilOrg), mpFilNew(apFilNew), mpOut(apOut),
    gpHsh(null), gpMch(null),
    miVerbse(aiVerbse), mbSrcBkt(abSrcBkt),
    miMchMax(aiMchMax),
    miMchMin(aiMchMin > miMchMax ? miMchMax - 1 : aiMchMin),
    miAhdMax(aiAhdMax<1024?1024:aiAhdMax),
    mbCmpAll(abCmpAll), miSrcScn(aiSrcScn)
{
	gpHsh = new JHashPos(aiHshSze) ;
	gpMch = new JMatchTable(gpHsh, mpFilOrg, mpFilNew, aiMchMax, abCmpAll, aiAhdMax);
}

/*
 * Destructor
 */
JDiff::~JDiff() {
	delete gpHsh ;
	delete gpMch ;
}

/**
* @brief Difference function
*
* Takes two files as arguments and writes out the differences
*
* Principle:
*   Take one byte from each file and compare. If they are equal, then continue.
*   If they are different, start lookahead to find next equal blocks within file.
*   If equal blocks are found,
*   - first insert or delete the specified number of bytes,
*   - then continue reading on both files until equal blocks are reached,
*
* @return 0    all ok
* @return < 0  error: see EXIT-codes
*/
int JDiff::jdiff()
{
    int lcOrg;              /**< byte from original file */
    int lcNew;              /**< byte from new file */
    off_t lzPosOrg = 0 ;    /**< position in original file */
    off_t lzPosNew = 0 ;    /**< position in new file */

    bool  lbEql = false;    /**< accumulate equal bytes? */
    off_t lzEql = 0;        /**< accumulated equal bytes */
    off_t lzCnt ;           /**< counter */

    int liFnd = 0;          /**< offsets are pointing to a valid solution (= equal regions) ?   */
    off_t lzAhd=0;          /**< number of bytes to advance on both files to reach the solution */
    off_t lzSkpOrg=0;       /**< number of bytes to skip on original file to reach the solution */
    off_t lzSkpNew=0;       /**< number of bytes to skip on new      file to reach the solution */
    off_t lzLapSml=MAX_OFF_T; /**< lap for reducing number of progress messages for -vv           */

    if (miVerbse > 0) {
      fprintf(JDebug::stddbg, "Comparing : ...           ");
      if (miVerbse > 1)
        lzLapSml = PGSMRK ;
    }

    /* Take one byte from each file ... */
    lcOrg = mpFilOrg->get(lzPosOrg, JFile::Read);
    lcNew = mpFilNew->get(lzPosNew, JFile::Read);

    while (lcNew >= 0) {
        #if debug
        if (JDebug::gbDbg[DBGPRG])
            fprintf(JDebug::stddbg, "Input " P8zd "->%2x " P8zd "->%2x.\n",
                    lzPosOrg - 1, lcOrg, lzPosNew - 1, lcNew)  ;
        #endif

        /* Incremental source scan */
        if (miSrcScn == 0 && lzPosOrg == mzAhdOrg) {
            mlHshOrg = hash(mlHshOrg, miPrvOrg, lcOrg, miEqlOrg) ;
            gpHsh->add(mlHshOrg, mzAhdOrg, miEqlOrg) ;
            mzAhdOrg ++ ;
        }

        /* Compare and process... */
        if (lcOrg == lcNew){
            /* Output or count equals */
            if (! lbEql){
                // the first bytes may be kept in reserve, then switch to counting asap
                lbEql = mpOut->put(EQL, 1, lcOrg, lcNew, lzPosOrg, lzPosNew) ;
                lzAhd -- ;      // decrease ahead counter

                lcOrg = mpFilOrg->get(++ lzPosOrg, JFile::Read) ;
                lcNew = mpFilNew->get(++ lzPosNew, JFile::Read) ;
            } else if (miSrcScn == 0){
                lzCnt = 0;
                while (lcOrg == lcNew && lcNew >= 0 && lzPosNew < lzLapSml){
                    lzCnt ++ ;
                    if (lzPosOrg == mzAhdOrg) {
                        mlHshOrg = hash(mlHshOrg, miPrvOrg, lcOrg, miEqlOrg) ;
                        gpHsh->add(mlHshOrg, mzAhdOrg, miEqlOrg) ;
                        mzAhdOrg ++ ;
                    }
                    lcOrg = mpFilOrg->get(++ lzPosOrg, JFile::Read) ;
                    lcNew = mpFilNew->get(++ lzPosNew, JFile::Read) ;
                }
                lzEql += lzCnt ;       // increase equal counter
                lzAhd -= lzCnt ;       // decrease ahead counter
            } else {
                lzCnt = 0;
                while (lcOrg == lcNew && lcNew >= 0 && lzPosNew < lzLapSml){
                    lzCnt ++ ;
                    lcOrg = mpFilOrg->get(++ lzPosOrg, JFile::Read) ;
                    lcNew = mpFilNew->get(++ lzPosNew, JFile::Read) ;
                }
                lzEql += lzCnt ;       // increase equal counter
                lzAhd -= lzCnt ;       // decrease ahead counter
            }
        } else if (lzAhd > 0) {
            /* Output accumulated equals */
            flushEql(lzPosOrg, lzPosNew, lzEql, lbEql);

            /* Output difference */
            if (lcOrg < 0) {
                mpOut->put(INS, 1, lcOrg, lcNew, lzPosOrg, lzPosNew);
                lzAhd -- ;      // decrease ahead counter

                /* Take next byte from destination file ... */
                lcNew = mpFilNew->get(++ lzPosNew, JFile::Read) ;
            } else {
                while (lcOrg != lcNew && lcOrg >= 0 && lcNew >= 0 && lzAhd > 0){
                    mpOut->put(MOD, 1, lcOrg, lcNew, lzPosOrg, lzPosNew);
                    lzAhd -- ;      // decrease ahead counter

                    /* Take next byte from each file ... */
                    lcOrg = mpFilOrg->get(++ lzPosOrg, JFile::Read) ;
                    lcNew = mpFilNew->get(++ lzPosNew, JFile::Read) ;
                }
            }

        } else if ((liFnd == 1) && (lzAhd == 0)) {
            // Oops: the "found" solution did not point to an equal region
            // This may happen, especially for non-compared solutions,
            // but we should hope this does not happen too much
            liFnd = 0 ;

            // Report the miss to the user
            miHshErr++ ;
            if (miVerbse>2 && mbCmpAll){
              fprintf(JDebug::stddbg, "\nInaccurate solution at positions %" PRIzd "/%" PRIzd "!\n", lzPosOrg, lzPosNew);
              fprintf(JDebug::stddbg, "Comparing : ...           ");
            }

            //v083x: advance depending on hashtable overloading
            lzAhd = gpHsh->get_reliability() / 2 ;

        } else {
            // Look for a new solution

            /* Output accumulated equals */
            flushEql(lzPosOrg, lzPosNew, lzEql, lbEql);

            /* Flush output buffer in debug */
            #if debug
            if (JDebug::gbDbg[DBGAHD] || JDebug::gbDbg[DBGMCH]){
                mpOut->put(ESC, 0, 0, 0, lzPosOrg, lzPosNew);
            }
            #endif

            /* Find a new equals-reqion */
            liFnd = search(lzPosOrg, lzPosNew, lzSkpOrg, lzSkpNew, lzAhd) ;
            if (liFnd < 0)
                return liFnd;
            #if debug
              if (JDebug::gbDbg[DBGAHD])
                fprintf(JDebug::stddbg, "Findahead on %" PRIzd " %" PRIzd " skip %" PRIzd " %" PRIzd " ahead %" PRIzd "\n",
                        lzPosOrg, lzPosNew, lzSkpOrg, lzSkpNew, lzAhd)  ;
              if (JDebug::gbDbg[DBGPRG])
                fprintf(JDebug::stddbg, "Current position in new file= %" PRIzd "\n", lzPosNew) ;
            #endif

            /* Execute offsets */
            if (lzSkpOrg > 0) {
                mpOut->put(DEL, lzSkpOrg, 0, 0, lzPosOrg, lzPosNew) ;
                lzPosOrg += lzSkpOrg ;

                lcOrg = mpFilOrg->get(lzPosOrg, JFile::Read);
            } else if (lzSkpOrg < 0) {
                mpOut->put(BKT, - lzSkpOrg, 0, 0, lzPosOrg, lzPosNew) ;
                lzPosOrg += lzSkpOrg ;
                lcOrg = mpFilOrg->get(lzPosOrg, JFile::Read);
            }
            if (lzSkpNew > 0) {
                while (lzSkpNew > 0 && lcNew > EOF) {
                    mpOut->put(INS, 1, 0, lcNew, lzPosOrg, lzPosNew);
                    lzSkpNew-- ;
                    lcNew = mpFilNew->get(++ lzPosNew, JFile::Read);
                }
            }
        } /* if lcOrg == lcNew */

        /* show progress */
        if ((miVerbse > 1) && (lzLapSml <= lzPosNew)) {
          fprintf(JDebug::stddbg, "\rComparing : %12" PRIzd "Mb", lzPosNew / PGSMRK);
          lzLapSml=lzPosNew+PGSMRK;
        }
    } /* while lcNew >= 0 */

    /* Flush output buffer */
    flushEql(lzPosOrg, lzPosNew, lzEql, lbEql);
    mpOut->put(ESC, 0, 0, 0, lzPosOrg, lzPosNew);

    /* Show progress */
    if (miVerbse > 0) {
      fprintf(JDebug::stddbg, "\rComparing : %12" PRIzd "Mb", (lzPosNew + PGSMRK / 2) / PGSMRK);
    }

    /* Show final hashtable distribution in case of incremental source scanning (miSrcScn==0) */
    if (miVerbse>2 && miSrcScn == 0){
          gpHsh->dist(lzPosOrg, 10);
    }

    /* Return code */
    if (lcNew < EOB || lcOrg < EOB){
        return (lcNew < lcOrg)?lcNew:lcOrg;
    }

    return EXI_OK;
} /* jdiff */

/**
 * @brief Flush pending EQL's
 */
void JDiff::flushEql(const off_t &lzPosOrg, const off_t &lzPosNew, off_t &lzEql, bool &lbEql) const {
    /* Output accumulated equals */
    if (lzEql > 0){
        mpOut->put(EQL, lzEql, 0, 0, lzPosOrg - lzEql, lzPosNew - lzEql);
        lzEql = 0;
    }
    lbEql=false;
} /* ufPutEql */

/**
 * @brief The hash function
 *
 * Generate a new hash value by adding a new byte.
 * Old bytes are shifted out from the hash value in such a way that
 * the new value corresponds to a sample of 32 bytes (the lowest bit of the 32'th
 * byte still influences the highest bit of the hash value).
 *
 * @param   acNew       character to hash
 * @param   akCurHsh    hash key (in & out)
 * @param   aiEql       equal-chars count
 */
inline hkey JDiff::hash ( hkey const akCurHsh, int &acOld, int const acNew, int &aiEql) const {
    if (acOld == acNew) {
        if (aiEql < SMPSZE)
            aiEql ++;
    } else {
        acOld = acNew ;
        if (aiEql != 0)     // improves performance
            aiEql = 0;
    }
    return (akCurHsh * 2) + acNew + aiEql ; // multiplication by 2 is faster than << 2
}


/**
 * @brief Find Ahead function
 *        Read ahead on both files until an equal series of 32 bytes is found.
 *        Then calculate the deplacement vector between two files:
 *          - positive if characters need to be inserted in the original file,
 *          - negative if characters need to be removed from the original file.
 * @param azRedOrg  read position in left file
 * @param azRedNew  read position in right file
 * @param azSkpOrg  out: number of bytes to skip (delete) in left file
 * @param azSkpNew  out: number of bytes to skip (insert) in right file
 * @param azAhd     out: number of bytes to skip on bith files before similarity is reached
 * @return 0    no solution found
 * @return 1    solution found
 * @return < 0  error: see EXIT-codes
 */
int JDiff::search (
  off_t const &azRedOrg,
  off_t const &azRedNew,
  off_t &azSkpOrg,
  off_t &azSkpNew,
  off_t &azAhd
) {
    off_t lzFndOrg=0;   /**< Found position within original file                 */
    off_t lzFndNew=0;   /**< Found position within new file                      */
    off_t lzBseOrg;     /**< Base position on original file                      */
    off_t lzLap=0;      /**< Stop-lap for progress counter                       */

    int liMax;          /**< Max number of bytes to look ahead              */
    int liBck;          /**< Number of bytes to look back                   */

    /* Set Lap for progress counter */
    if (miVerbse > 1) lzLap = azRedNew + PGSMRK ;

    /* Prescan the source file to build the hashtable */
    switch (miSrcScn) {
    case 1: {
            // do a full prescan
            int liRet = buildFullIndex() ;
            if (liRet < 0)
                return liRet ;
            miSrcScn = 2 ;
            miRlb = gpHsh->get_reliability() ;
        }
        break ;

    case 0: {
            // Set lookahead base position and determine lookahead range
            mpFilOrg->set_lookahead_base(azRedOrg);
            if (mbSrcBkt){
                // Backtrace allowed: go ahead as far as possible
                liMax = miAhdMax ;
            } else {
                // Backtrace not allowed:
                // - keep (mzAhdMax - azRedOrg) == miAhdMax / 2
                // - except at the start of the file (azRedOrg < miAhdMax)
                if (mzAhdOrg < (miAhdMax / 2))
                    liMax = miAhdMax - mzAhdOrg ;
                else
                    liMax = miAhdMax / 2 - (mzAhdOrg - azRedOrg) ;
            }

            // scan ahead till EOB or EOF
            int lcOrg ;
            for ( ; liMax > 0 ; liMax --) {
                lcOrg = mpFilOrg->get(mzAhdOrg, JFile::SoftAhead) ;
                if (lcOrg <= EOF)
                    break ;
                mlHshOrg = hash(mlHshOrg, miPrvOrg, lcOrg, miEqlOrg) ;
                gpHsh->add(mlHshOrg, mzAhdOrg, miEqlOrg) ;
                mzAhdOrg ++ ;
            }
            miRlb = gpHsh->get_reliability() ;
        } /* case 0 */
        break ;
    } /* switch scan source file - build hashtable */

    /*
    * How many bytes to look ahead (search) ?
    * As far as possible, but going too far makes no sense.
    * In theory, if we can't find a solution within the (un)reliability range,
    * then there is no solution here !
    * In practice: it does no harm (no speed penalty) to use the buffer
    * and already jump to a far-away solution.
    * Moreover, the unreliability range is only an estimate of the average number
    * of bytes needed to find a solution (due to the index hashtable only covering
    * a small percentage of samples). So using the whole buffer may solve a situation
    * where a solution needs more bytes to be found than indicated by the reliability
    * range. Once a minimum number of potential solutions is found, the lookahead
    * may again be reduced to the reliability range (see below).
    */
    if (mzAhdNew > azRedNew)
        liMax = miAhdMax - (mzAhdNew - azRedNew) ;
    else
        liMax = miAhdMax ;

    if (liMax < miRlb)
        liMax = miRlb  ;    // search at least the reliability distance

    /*
    * How many bytes to look back ?
    * In theory: none, it makes no sense to look back
    * In practice:
    * - looking back avoids the need to reinitialize the hash function
    * - re-initialization of the hash function can take up to SMPSZE * 2 bytes
    * - looking back allows to keep the existing match-table up-to-date
    * Therefore, we allow for some look back.
    */
    liBck = (azRedNew - mzAhdNew);
    if (liBck < 0)
        // mzAhdNew is stil ahead of azRedNew from a previous lookahead
        // continue where the previous left off
        liBck= 0 ;
    else if (liBck > miRlb + 2 * SMPSZE - 1)
        liBck = miRlb + 2 * SMPSZE - 1;   // 2 * SMPSZE to anticipate a reinitialization


    /* Do not backtrace before lzBseOrg */
    lzBseOrg = (mbSrcBkt?0:mpFilOrg->getBufPos()) ;

    /* Cleanup the old matches */
    int liFnd=0;          /**< Number of matches found                        */
    switch (gpMch->cleanup(lzBseOrg, azRedNew)){
    case JMatchTable::Error:
    case JMatchTable::Full: // table is full
        liFnd = miMchMax ;
        break ;

    case JMatchTable::Best: // a good match is already available : reduce search
    case JMatchTable::Good: // a good match is already available : reduce search
        if (liMax > miRlb * 2)
            liMax = miRlb * 2 ;   // reduce search (but not to zero)
        break ;

    default: ;
    }

    /* If there's room to work */
    if (liFnd < miMchMax) {
        // Set lookahead base position
        mpFilNew->set_lookahead_base(azRedNew);

        // Switch to soft reading if the minimum number of matches is obtained
        JFile::eAhead liSftNew = (liFnd >= miMchMin) ? JFile::SoftAhead : JFile::HardAhead ;

        /*
        * Re-Initialize hash function (read 31 or 63 bytes) if
        * - ahead position has been reset, or
        * - read position has jumped over the ahead position
        */
        if (mzAhdNew == 0 || mzAhdNew + liBck < azRedNew)  {
            // Don't go back more than the buffer allows (to avoid EOB)
            mzAhdNew = mpFilNew->getBufPos() ;

            // Set looking back position, but never before the buffer
            if (azRedNew > mzAhdNew + liBck){
                mzAhdNew = azRedNew - liBck;
                if (mzAhdNew < 0)
                    mzAhdNew = 0;
            }

            // Initialize hash: at the start of the file (mzAhdNew == 0),
            // SMPSZE suffices to initialize, but within the file (mzAhdNew > 0),
            // in a worst case, we first need SMPSZE to initialize miEqlNew and
            // then another SMPSZE to initialize the hash
            if (mzAhdNew == 0)
                liBck = SMPSZE - 1 ;        // to initialize mkHsh (miEql=0 is correct)
            else
                liBck = SMPSZE * 2 - 1 ;    // to initialize mkHsh and miEql
            mzAhdNew -- ; // switch to pre-increments
            mlHshNew = 0;
            miEqlNew = 0;
            miPrvNew = EOF;
            for (int liIdx = 0 ; liIdx < liBck; liIdx++) {
                miValNew = mpFilNew->get(++ mzAhdNew, liSftNew) ;
                if (miValNew <= EOF){
                    mzAhdNew --;
                    break ;
                }
                mlHshNew = hash(mlHshNew, miPrvNew, miValNew, miEqlNew) ;

                // The following line needs some explication.
                // The goal of this line is to terminate the initialization ASAP.
                // To explain, consider SMPSZE == 8, then we need 7 valid miEql's to initialize.
                // For example, consider an initialization starting at position 4 (hex data)
                //    mzAhd :   4 5 6 7 8 9 A B C D E F ...
                //    miVal :   0 0 0 0 7 6 5 4 3 2 1 0 4 9 7 4  ...
                //    miPrv : EOF 0 0 0 0 7 6 5 4 3 2 1 0 4 9 7 4 ...
                //    miEql :   0 1 2 3 0 0 0 0 0 0 0 ...
                //    liIdx :   0 1 2 3 4 5 6 7 8 9 A B C ...
                //    init            +-----------+
                // We don't know if position 3 is 0 or not, so we don't know what value miEql
                // at position 4 should have. So the first four bytes cannot be used,
                // because miEql may not be correct.
                // As soon as miEql is reset to 0 by miPrv != miVal, miEql becomes correct
                // and initialization will be ok after SMPSZE-1 bytes (position D in the example)
                // Reset can be detected by miEql != liIdx. Hence, when miEql != liIdx,
                // we can reduce liMax to liIdx + SMPSZE - 1.
                if (liIdx != miEqlNew && liBck > liIdx + (SMPSZE - 1))
                    liBck = liIdx + (SMPSZE - 1) ;
            }
        }

        /* Add the resulting look-back to liMax */
        if (mzAhdNew < azRedNew)
            liMax += (azRedNew - mzAhdNew) ;

        /*
        * Build the table of matches
        */
        while ((liMax > 0)) {
            /* hash the new value */
            miValNew = mpFilNew->get(++ mzAhdNew, liSftNew) ;
            if (miValNew <= EOF){
                mzAhdNew --;
                break ;
            }
            mlHshNew = hash(mlHshNew, miPrvNew, miValNew, miEqlNew) ;
            liMax --;

            /* lookup the new value in the hashtable and add it to the table of matches...*/
            if (gpHsh->get(mlHshNew, lzFndOrg)) {
                /* ...unless it's not usable because we've been instructed not to backtrack on source file */
                if (lzFndOrg > lzBseOrg) {
                    /* it's usable: add to the table of matches */
                    switch (gpMch->add(lzFndOrg, mzAhdNew, azRedNew)){
                    case JMatchTable::Error:        // Table in unexpectedly full
                        #if debug
                            fprintf(JDebug::stddbg, "Matchtable overflow at " P8zd "\n", mzAhdNew) ;
                        #endif // debug
                    // no break: continue with next case

                    case JMatchTable::Full:         // Table is full
                        liMax = 0;
                        continue ;

                    case JMatchTable::Enlarged:     // Existing solution has been enlarged
                    case JMatchTable::Invalid:      // Match does not point to a valid solution
                        break ;                     // Do nothing

                    case JMatchTable::Best:
                    case JMatchTable::Good:
                        // This seems to be a very good solution.
                        // However, due to the unreliable nature of the checksums and the hash-table,
                        // the first good solution is not always the best one,
                        // but a better one should be found within the reliability range.
                        //
                        // Why ? Because the reliability range estimates the number of bytes
                        // to search before finding all solutions hidden behind the unreliability.
                        // So after the (estimated) reliability range, no better solution should be found anymore.
                        // reduce the lookahead to be sure and to improve performance
                        if (liMax > miRlb)
                            liMax = miRlb ;
                    // no break: continue with next case

                    case JMatchTable::Valid:        // solution added
                        liFnd ++ ;
                        if (mzAhdNew > azRedNew) {
                            if (liFnd >= miMchMin)
                                liSftNew=JFile::SoftAhead ;   // switch to soft reading
                            if (liFnd >= miMchMax){
                                liMax = 0; continue ;         // stop lookahead
                            }
                        }
                    } /* switch */
                } /* if usable */
            } /* lookup */

            /* show progress */
            if ((miVerbse > 1) && (lzLap <= mzAhdNew)) {
              fprintf(JDebug::stddbg, "+%-12" PRIzd "\b\b\b\b\b\b\b\b\b\b\b\b\b", (mzAhdNew - azRedNew) / PGSMRK);
              lzLap=lzLap+PGSMRK;
            }
        } /* while ! EOF */
    } /* if liFnd <= miMchMax */

    /* Check for errors  */
    if (miValNew < EOB ) {
        return miValNew ;
    }

    /* show progress */
    if ((miVerbse>1) && (lzLap > azRedNew+PGSMRK))  {
        fprintf(JDebug::stddbg, "+%-12" PRIzd "...\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", (mzAhdNew - azRedNew) / PGSMRK);
    }

    /*
     * Get the best match and calculate the offsets
     */
    bool lbFnd = gpMch->getbest(azRedOrg, azRedNew, /* out */ lzFndOrg, lzFndNew);

    /* clear search progress */
    if (miVerbse>1){
        if (lzLap > azRedNew+PGSMRK)
            fprintf(JDebug::stddbg, "                \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b") ;
    }

    /* Calculate the resulting offsets */
    if (! lbFnd)  {
        // No solution has been found. Maybe the search window size is too small, or
        // the hashtable, or the buffers, or maybe the files are simply different.
        // Anyway, iterating over the same search windows makes no sense, so
        // jump forward for at least SMPSZE bytes
        azSkpOrg = 0 ;
        azSkpNew = 0 ;
        azAhd    = (mzAhdNew - azRedNew) ;
        if (azAhd < SMPSZE){
            #if debug
            if (JDebug::gbDbg[DBGAHD]){
                fprintf(JDebug::stddbg, "\nForcing skip of SMPSZE bytes\n");
            }
            #endif // debug
            azAhd = SMPSZE ;
        }
        return 0 ;
    }  else  {
        if (lzFndOrg >= azRedOrg) {
            if (lzFndOrg - azRedOrg >= lzFndNew - azRedNew) {
                /* go forward on original file */
                azSkpOrg = lzFndOrg - azRedOrg + azRedNew - lzFndNew ;
                azSkpNew = 0 ;
                azAhd    = lzFndNew - azRedNew ;
            } else {
                /* go forward on new file */
                azSkpOrg = 0;
                azSkpNew = lzFndNew - azRedNew + azRedOrg - lzFndOrg ;
                azAhd    = lzFndOrg - azRedOrg ;
            }
        } else {
            /* backtrack on original file */
            azSkpOrg = azRedOrg - lzFndOrg + lzFndNew - azRedNew ;
            if (azSkpOrg <= (azRedOrg - lzBseOrg)) {
                azSkpNew = 0 ;
                azSkpOrg = - azSkpOrg ;
                azAhd = lzFndNew - azRedNew ;
            } else {
                /* do not backtrace before beginning of file */
                azSkpNew = azSkpOrg - (azRedOrg - lzBseOrg) ;
                azSkpOrg = lzBseOrg - azRedOrg ;
                azAhd = (lzFndNew - azRedNew) - azSkpNew ;
            }
        }

        return 1 ;
    }
} /* search */

/**
 * @brief   Prescan the original file.
 *
 *          Calculates a hash-key for every 32-bytes sample
 *          in the source file and stores them with their position in a hash-table.
 */
int JDiff::buildFullIndex ()
{
    hkey  lkHshOrg=0;     // Current hash value for original file
    int   liEqlOrg=0;     // Number of times current value occurs in hash value
    int   lcValOrg=0;     // Current  file value
    int   lcValPrv=EOF;   // Previous file value
    off_t lzPosOrg=-1;    // Position within original file

    int liIdx ;

    if (miVerbse > 0) {
        fprintf(JDebug::stddbg, "\nIndexing  : ...           ");
    }

    /* Read SMPSZE-1 bytes (31 or 63) to initialize the hash function */
    for (liIdx=0; (liIdx < SMPSZE - 1); liIdx++) {
        lcValOrg = mpFilOrg->get(++ lzPosOrg, JFile::HardAhead);
        if (lcValOrg <= EOF)
            break ;
        lkHshOrg = hash(lkHshOrg, lcValPrv, lcValOrg, liEqlOrg) ;
    }

    /* Build hashtable */
    if (miVerbse > 1) {
        /* slow version with user feedback */
        while (lcValOrg > EOF) {
            lcValOrg = mpFilOrg->get(++ lzPosOrg, JFile::HardAhead);
            if (lcValOrg <= EOF)
                break ;
            lkHshOrg = hash(lkHshOrg, lcValPrv, lcValOrg, liEqlOrg) ;
            gpHsh->add(lkHshOrg, lzPosOrg, liEqlOrg) ;

            #if debug
            if (JDebug::gbDbg[DBGAHH])
                fprintf(JDebug::stddbg, "ufHshAdd(%2x -> %8" PRIhkey ", " P8zd ", %8d)\n",
                        lcValOrg, lkHshOrg, lzPosOrg, 0);
            #endif

            /* output position every 16MB */
            if ((lzPosOrg & PGSMSK) == 0) {
              fprintf(JDebug::stddbg, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b%12" PRIzd "Mb", lzPosOrg / PGSMRK);
            }
        }
    } else {
        /* fast version, no user feedback nor debug */
        while (lcValOrg > EOF) {
            lcValOrg = mpFilOrg->get(++ lzPosOrg, JFile::HardAhead);
            if (lcValOrg <= EOF)
                break ;
            lkHshOrg = hash(lkHshOrg, lcValPrv, lcValOrg, liEqlOrg) ;
            gpHsh->add(lkHshOrg, lzPosOrg, liEqlOrg) ;
        }
    }

    if (miVerbse > 0) {
        /* output final position */
        fprintf(JDebug::stddbg, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b%12" PRIzd "Mb\n", lzPosOrg / PGSMRK);
        fprintf(JDebug::stddbg, "Comparing : ...           ");
    }
    if (miVerbse>2){
        gpHsh->dist(lzPosOrg, 10);
    }

    if (lcValOrg < EOB)
        return lcValOrg ;
    else
        return 0 ;
} /* buildFullIndex */

} /* namespace */
