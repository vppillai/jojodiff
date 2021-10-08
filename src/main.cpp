/*******************************************************************************
 * Jojo's Diff : diff on binary files
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
 * Usage:
 * ------
 * jdiff [options] <original file> <new file> [<output file>]
 *   -v          Verbose (greeting, results and tips).
 *   -vv         Verbose (debug info).
 *   -h          Help (this text).
 *   -l          Listing (ascii output).
 *   -r          Regions (ascii output).
 *   -b          Try to be better (using more memory).
 *   -f          Try to be faster: no out of buffer compares.
 *   -ff         Try to be faster: no out of buffer compares, nor pre-scanning.
 *   -m size     Size (in kB) for look-ahead buffers (default 128).
 *   -k size     Block size (in bytes) for reading from files (default 4096).
 *   -i size     Index table size in Mb (default 32Mb).
 *   -n count    Minimum number of solutions to find before choosing one.
 *   -x count    Maximum number of solutions to find before choosing one.
 *
 * Exit codes
 * ----------
 *  0  ok, differences found
 *  1  ok, no differences found
 *  2  error: not enough arguments
 *  3  error: could not open first input file
 *  4  error: could not open second input file
 *  5  error: could not open output file
 *  6  error: seek or other i/o error when reading or writing
 *  7  error: 64 bit numbers not supported
 *  8  error on reading
 *  9  error on writing
 *  10  error: malloc failed
 *  20  error: any other error
 *
 * Author    Version Date       Modification
 * --------  ------- -------    -----------------------
 * joheirba  v0.0    10-06-2002 hashed compare
 * joheirba          14-06-2002 full compare
 * joheirba          17-06-2002 global positions
 * joheirba  v0.1    18-06-2002 first well-working runs!!!
 * joheirba          19-06-2002 compare in buffer before read position
 * joheirba  v0.1    20-06-2002 optimized esc-sequences & lengths
 * joheirba  v0.2    24-06-2002 running okay again
 * joheirba  v0.2b   01-07-2002 bugfix on length=252
 * joheirba  v0.2c   09-07-2002 bugfix on divide by zero in statistics
 * joheirba  v0.3a   09-07-2002 hashtable hint only on samplerate
 * joheirba    |     09-07-2002 exit code 1 if files are equal
 * joheirba    |     12-07-2002 bugfix using ufFabPos in function call
 * joheirba  v0.3a   16-07-2002 backtrack on original file
 * joheirba  v0.4a   19-07-2002 prescan sourcefile
 * joheirba    |     30-08-2002 bugfix in ufFabRst and ufFabPos
 * joheirba    |     03-08-2002 bugfix for backtrack before start-of-file
 * joheirba    |     09-09-2002 reimplemented filebuffer
 * joheirba  v0.4a   10-09-2002 take best of multiple possibilities
 * joheirba  v0.4b   11-09-2002 soft-reading from files
 * joheirba    |     18-09-2002 moved ufFabCmp from ufFndAhdChk to ufFndAhdAdd/Bst
 * joheirba    |     18-09-2002 ufFabOpt - optimize a found solution
 * joheirba    |     10-10-2002 added Fab->izPosEof to correctly handle EOF condition
 * joheirba  v0.4b   16-10-2002 replaces ufFabCmpBck and ufFabCmpFwd with ufFabFnd
 * joheirba  v0.4c   04-11-2002 use ufHshFnd after prescanning
 * joheirba    |     04-11-2002 no reset of matching table
 * joheirba    |     21-12-2002 rewrite of matching table logic
 * joheirba    |     24-12-2002 no compare in ufFndAhdAdd
 * joheirba    |     02-01-2003 restart finding matches at regular intervals when no matches are found
 * joheirba    |     09-01-2003 renamed ufFabBkt to ufFabSek, use it for DEL and BKT instructions
 * joheirba  v0.4c   23-01-2003 distinguish between EOF en EOB
 * joheirba  v0.5    27-02-2003 dropped "fast" hash method (it was slow!)
 * joheirba    |     22-05-2003 started    rewrite of FAB-abstraction
 * joheirba    |     30-06-2003 terminated rewrite ...
 * joheirba    |     08-07-2003 correction in ufMchBst (llTstNxt = *alBstNew + 1 iso -1)
 * joheirba  v0.5    02-09-2003 production
 * joheirba  v0.6    29-04-2005 large-file support
 * joheirba  v0.7    23-06-2009 differentiate between position 0 and notfound in ufMchBst
 * joheirba    |     23-06-2009 optimize collission strategy using sample quality
 * joheirba    |     24-06-2009 introduce quality of samples
 * joheirba    |     26-06-2009 protect first samples
 * joheirba  v0.7g   24-09-2009 use fseeko for cygwin largefiles
 * joheirba    |     24-09-2009 removed casts to int from ufFndAhd
 * joheirba  v0.7h   24-09-2009 faster ufFabGetNxt
 * joheirba    |     24-09-2009 faster ufHshAdd: remove quality of hashes
 * joheirba  v0.7i   25-09-2009 drop use of ufHshBitCnt
 * joheirba  v0.7l   04-10-2009 increment glMchMaxDst as hashtable overloading grows
 * joheirba    |     16-10-2009 finalization
 * joheirba  v0.7m   17-10-2009 gprof optimization ufHshAdd
 * joheirba  v0.7n   17-10-2009 gprof optimization ufFabGet
 * joheirba  v0.7o   18-10-2009 gprof optimization asFab->iiRedSze
 * joheirba  v0.7p   19-10-2009 ufHshAdd: check uniform distribution
 * joheirba  v0.7q   23-10-2009 ufFabGet: scroll back on buffer
 * joheirba    |     19-10-2009 ufFndAhd: liMax = giBufSze
 * joheirba    |     25-10-2009 ufMchAdd: gliding matches
 * joheirba    |     25-10-2009 ufOut: return true for faster EQL sequences in jdiff function
 * joheirba  v0.7r   27-10-2009 ufMchBst: test position for gliding matches
 * joheirba    |     27-10-2009 ufMchBst: remove double loop
 * joheirba    |     27-10-2009 ufMchBst: double linked list ordered on azPosOrg
 * joheirba    |     27-10-2009 ufFndAhd: look back on reset (liBck)
 * joheirba    |     27-10-2009 ufFndAhd: reduce lookahead after giMchMin (speed optimization)
 * joheirba  v0.7x   05-11-2009 ufMchAdd: hashed method
 * joheirba  v0.7y   13-11-2009 ufMchBst: store unmatched samples too (reduce use of ufFabFnd)
 * joheirba  v0.8     Sep  2011 Conversion to C++
 * joheirba  v0.8.1   Dec  2011 Correction in Windows exe for files > 2GB
 * joheirba  v0.8.2   Dec  2015 use jfopen/jfclose/jfread/jfseek to avoid interferences with LARGEFILE redefinitions
 * joheirba    |      Feb  2015 bugfix: virtual destructors for JFile and JOut to avoid memory leaks
 * joheirba  v0.8.3a  July 2020 improved progress feedback
 *                              use getopts_long for option processing
 *                              index tablke in mb, search for nearest lower prime with isPrime
 * joheirba  v083b    July 2020 tweak JDiff.ufFndAhd.liMax for longer searches
 * joheirba  v083g    July 2020 JMatchTable::get refactoring
 * joheirba  v083h     Aug 2020 hash: use liEql to improve quality of hashes
 * joheirba  v083i     Aug 2020 JDiff review lookahead reset logic
 * joheirba  v083k-o   Aug 2020 Review matching table logic
 * joheirba  v083p     Aug 2020 Reduce control bytes to patch file (default ops)
 * joheirba  v083q     Aug 2020 Refactored hash function: liEql logic included
 * joheirba  v083r     Aug 2020 Dynamic matching table.
 * joheirba  v083s     Aug 2020 Allow standard input with "-"
 * joheirba  v083s     Aug 2020 Option -s for stdio instead of istream
 * joheirba  v083t     Aug 2020 Integration of jpatch with JFile.getbuf
 *                              Refactoring JFileIStreamAhead
 * joheirba  v083z     Aug 2020 Review matchtable logic for non-compared matched
 * joheirba  v084b     Sep 2020 Review hash-reinitialization for incremental scanning
 * joheirba  v084c     Sep 2020 Finalize release of intermediate version
 * joheirba  v085a     Sep 2020 Revised buffer logic for sequential files
 * joheirba  v085b     Sep 2020 Removed unbuffered JFileIStream implementation
 * joheirba  v085d     Oct 2020 Deduplication feature (experimental)
 * joheirba  v085e-k   Oct 2020 Reduce compares (check): reuse negative results, less cleanup
 * joheirba  v085l-r   Oct 2020 Improve gliding match detection
 * joheirba  v085s-z   Oct 2020 Replace freelist by new&old-lists
 * joheirba  v085aa-al Oct 2020 Tuning min and max distances and skip-checks
 * joheirba  v085am-aw Oct 2020 Incremental search
 * joheirba  v085ax-bf Oct 2020 Improve accuracy for non-compared matches (-f or -p)
 * joheirba  v085bg-bl Oct 2020 Improve accuracy of incremental scanning (-ff)
 * joheirba  v085bm-ca Oct 2020 Tune isOld logic
 *
 *******************************************************************************/

 /******************************************************************************
  * TODO     DONE
  * 07-2020  083g   JMatchTable::get : reduce liRlb and improve performance
  * 07-2020  083h   hash: use liEql to improve quality of hashes
  * 07-2020  083?   Differentiate between src/dst out-of-buffer compares for -p/-q
  * 07-2020  083r   Allow stdin as src/dst file
  * 07-2020  083?   Check hashtable dist with -p/-q/-ff
  * 07-2020  drop   JDiff::ufFndAhd count existing matches and new colliding matches
  * 07-2020  083x   Align buffered reads on block boundaries ?
  * 07-2020  083l   Assure that lookahead search advances at least half a buffer
  * 07-2020  083?   ufFndAhd: reduce liMax to azAhdMax ?
  * 08-2020  done   JMatchTable::get : improve performance by prechecking
  * 08-2020  drop   JMatchTable::get : consider length of jump (2+x offsett bytes)
  * 08-2020  083?   Improve gliding match logic to handle skips
  * 08-2020  drop   Drop azPos arguments from output routines
  * 08-2020         Refactor hashtable to structure
  * 08-2020  083q   Refactor get_outofbuffer
  * 08-2020  083l   Remove panic from ufFndAhd (introduced liFre)
  * 08-2020  drop   Drop liMchMax
  * 08-2020  083p   Reduce initialization time of match table (watermark)
  * 08-2020  drop   Drop -a and -x
  * 08-2020  083r   convert Mb/kb upon instantiation
  * 08-2020  083o   Integrate JMatchTable add, get and cleanup
  * 08-2020  085a   Remove JFileStreamAhead.miRed
  * 08-2020  083x   Remove potential indefinite loop in uf_get_outofbuf ?
  * 08-2020  drop   Make EQLMAX a parameter
  * 08-2020  fail   Reduce garbage collect in ufFnhAhd
  * 08-2020  fail   Reduce incremental garbage collect in ufMchBst for future matches
  * 08-2020  083q   Adaptive liDst in ufMchBst based on azBstNew-azRedNew+EQLMAX
  * 08-2020  083q   For maximum best matches, use iiCnt
  * 08-2020  083q   Improve hash function: hash2
  * 08-2020  083s   Direct reading in buffer space for better performance ?
  * 08-2020  083z   Check for IO errors in JMatchTable add and get
  * 09-2020  085a   Refactor scrollback, align with blocksize
  * 09-2020  drop   JMatchTable::check - Performance: Soft ahead once EQLSZE bytes are found ?
  * 09-2020  085b   JMatchTable::check - Accuracy: Distinguish between EOB and short equals.
  * 09-2020  drop   Remove liMax logic from ufFndAhd ?
  * 09-2020  085b   Incremental source scanning: separate function, full buffer with getbuf
  * 09-2020  085a   Optimize Cycle logic in buffer logic (JFileAhead)
  * 10-2020         Refactor JMatchTable logic: reduce cleanup() calls
  *
  ******************************************************************************/
/*
 * Includes
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include <getopt.h>

using namespace std ;

#include <iostream>
#include <istream>
#include <fstream>
#include "JFileAheadIStream.h"
#include "JFileAheadStdio.h"

#include "JDefs.h"
#include "JDiff.h"
#include "JPatcht.h"
#include "JOutBin.h"
#include "JOutAsc.h"
#include "JOutRgn.h"
#include "JFile.h"
#include "JFileOut.h"
#ifdef JDIFF_DEDUP
#include "JOutDedup.h"
#endif // JDIFF_DEDUP

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#warning setmode binary for _WIN32
#endif // _WIN32

using namespace JojoDiff ;

/*********************************************************************************
* Options parsing
*********************************************************************************/
const char *gcOptSht = "a:bcd:fhi:jk:lm:n:pqrst::uvx:y"; /* u:: for optional aruments */

struct option gsOptLng [] = {
    {"better",            no_argument,      NULL,'b'},
    {"lazy",              no_argument,      NULL,'f'},
    {"console",           no_argument,      NULL,'c'},
    {"debug",             required_argument,NULL,'d'},
    {"help",              no_argument,      NULL,'h'},
    {"listing",           no_argument,      NULL,'l'},
    {"regions",           no_argument,      NULL,'r'},
    {"sequential-source", no_argument,      NULL,'p'},
    {"sequential-dest",   no_argument,      NULL,'q'},
    {"stdio",             no_argument,      NULL,'s'},
    {"test",              optional_argument,NULL,'t'},
    {"jdiff",             no_argument,      NULL,'j'},
    {"undiff",            no_argument,      NULL,'u'},
    {"index-size",        required_argument,NULL,'i'},
    {"block-size",        required_argument,NULL,'k'},
    {"buffer-size",       required_argument,NULL,'m'},
    {"search-size",       required_argument,NULL,'a'},
    {"search-min",        required_argument,NULL,'n'},
    {"search-max",        required_argument,NULL,'x'},
    {"reflink",           no_argument,      NULL,'y'},
    {"verbose",           no_argument,      NULL,'v'},
    {NULL,0,NULL,0}
};

/************************************************************************************
* Main function
*************************************************************************************/
int main(int aiArgCnt, char *acArg[])
{
    const char csStdInpOutNam[]="-";
    const char *lcFilNamOrg;      /**< Source filename                                  */
    const char *lcFilNamNew;      /**< Destination filename                             */
    const char *lcFilNamOut;      /**< Output filename (-=stdout)                       */

    FILE  *lpFilOut;              /**< Output file                                      */

    /* Default settings */
    int liOutTyp = 0 ;            /**< 0 = JOutBin, 1 = JOutAsc, 2 = JOutRgn            */
    int liVerbse = 0;             /**< Verbose level 0=no, 1=normal, 2=high             */
    int lbSrcBkt = true;          /**< Backtrace on sourcefile allowed?                 */
    bool lbCmpAll = true ;        /**< Compare even if data not in buffer?              */
    int liSrcScn = 1 ;            /**< Prescan source file: 0=no, 1=do, 2=done          */
    int liMchMax = 128 ;          /**< Maximum entries in matching table.               */
    int liMchMin = 2 ;            /**< Minimum entries in matching table.               */
    int liHshMbt = 32 ;           /**< Hashtable size in MB (* 1024 * 1024)             */
    long llBufOrg = 0 ;           /**< Default source-file buffer in MB                 */
    long llBufNew = 0 ;           /**< Default destin-file buffer in MB                 */
    int liBlkSze = 32*1024 ;      /**< Default block size (in bytes)                    */
    int liAhdMax = 0;             /**< Lookahead range (0=same as llBufSze)             */
    int liHlp=0;                  /**< -h/--help flag: 0=no, 1=-h, 2=-hh, 3=error       */
    bool lbStdio=false;           /**< use stdio                                        */
    int liTst=0;                  /**< test to execute : 0 = normal, 1 etc... see JTest */
    bool lbSeqOrg = false;        /**< Sequential source file ?                         */
    bool lbSeqNew = false;        /**< Sequential destination file ?                    */
    enum {Diff, Patch, Dedup, Test} liFun = Diff;  /**< function to execute             */

    JDebug::stddbg = stderr ;     /**< Debug and informational (verbose) output         */

    /* optional arguments parsing */
    int liOptArgCnt=0 ;           /**< number of options */
    char lcOptSht;                /**< short option code */
    int liOptLng;                 /**< long option index */

    /* Read options */
    char *lcCmd = acArg[0];
    char *lcCmdTst = strrchr(lcCmd, '/');
    if (lcCmdTst != null)
        lcCmd = lcCmdTst + 1;
    lcCmdTst = strrchr(lcCmd, '\\');
    if (lcCmdTst != null)
        lcCmd = lcCmdTst + 1;
    if (strncasecmp(lcCmd, "jpatch", 6) == 0)
        liFun=Patch ;
    else if (strncasecmp(lcCmd, "jdedup", 6) == 0)
        liFun=Dedup ;
    else if (strncasecmp(lcCmd, "jtst", 4) == 0)
        liFun=Test ;

    /* parse option-switches */
    while((lcOptSht = getopt_long(aiArgCnt, acArg, gcOptSht, gsOptLng, &liOptLng))!=EOF) {
        switch(lcOptSht) {
        case 'b': // try-harder: increase hashtable size and more searching
            lbCmpAll = true ;         // verify all hashtable matches
            lbSrcBkt = true;          // allow going back on source file
            liSrcScn = 1 ;            // create full index on source file
            liMchMin *= 2 ;           // increase minimum number of matches to search
            liMchMax *= 4 ;           // increase maximum number of matches to search
            liHshMbt *= 4 ;           // Increase index table size

            // larger buffers (more soft-ahead searching)
            llBufOrg = (llBufOrg <= 0 ? 1 : llBufOrg) * 4 ;
            break;
        case 'f': // faster (or rather: lazier)
            if (lbCmpAll) {
                lbCmpAll = false ;      // No compares out-of-buffer (only verify hashtable matches when data is available in memory buffers)
                lbSrcBkt = true ;
                liSrcScn = 1  ;
                liMchMin *= 2 ;
                liMchMax /= 2 ;

                // increase buffer size to have more lookahead indexing
                llBufOrg = (llBufOrg <= 0 ? 1 : llBufOrg) * 16 ;
            } else {
                // even faster (lazier)
                liSrcScn = 0 ;          // No indexing scan, indexing is limited ookahead search
                liMchMin /= 2 ;         // Reduce lookahead
                liMchMax /= 2 ;
            }
            liHshMbt /= 2 ;             // Reduce index table by 2
            break;
        case 'p': // sequential source file
            lbSeqOrg = true ;
            lbCmpAll = false ;            // only compare data within the buffer
            lbSrcBkt = false ;            // only backtrack on source file in buffer
            liSrcScn = 0;                 // no pre-scan indexing
            break;
        case 'q': // sequential destination file
            lbSeqNew = true ;
            liMchMin = 0;                 // only search within the buffer
            break;

        case 'c': // verbose-stdout
            JDebug::stddbg = stdout;
            break;
        case 'h': // help
            liHlp++;
            break;
        case 'j':   // jdiff
            liFun = Diff ;
            break ;
        case 'l': // "list-details",      no_argument
            liOutTyp = 1 ;
            break;
        case 'r': // "list-groups",       no_argument
            liOutTyp = 2 ;
            break;
        case 's': // use stdio
            lbStdio = true ;
            break;
        case 't':   // test: patch and unpatch in one go
            liFun = Test ;
            if (optarg)
                liTst = atoi(optarg);    // test number
            else
                liTst=0;
            break ;
        case 'u':   // unpatch
            liFun = Patch ;
            break ;
        case 'v': // "verbose",           no_argument
            liVerbse++;
            break;
        case 'y':   // deduplicate
            liFun = Dedup ;
            liOutTyp = 3 ;
            lbStdio = true ;
            if (optarg)
                liTst = atoi(optarg);    //@todo: minimum dedup size
            else
                liTst=0;
            break ;

        case 'a': // search-ahead-size
            if (optarg)
                liAhdMax = atoi(optarg) * 1024 ;
            else
                liAhdMax = 0 ;
            break;

        case 'i': // index-size
            liHshMbt = atoi(optarg) ;
            if (liHshMbt <= 0){
                liHshMbt = 1 ;
                fprintf(JDebug::stddbg, "Warning: invalid --index-size/-i specified, set to 1.\n");
            }
            break;
        case 'k': // "block-size"
            liBlkSze = atoi(optarg) ;
            if (liBlkSze <= 0) {
                liBlkSze = 1 ;
                fprintf(JDebug::stddbg, "Warning: invalid --block-size/-k specified, set to 1.\n");
            }
            break;
        case 'm': // "buffer-size",       required_argument
            if (llBufNew == 0){
                // first -m
                llBufNew = atoi(optarg) / 2 ;
                llBufOrg = llBufNew ;   // first -m specifies source and destination buffer
            } else if (llBufOrg == llBufNew) {
                // second -m
                llBufOrg *= 2 ;
                llBufNew = atoi(optarg) ;
            } else {
                // third and subsequent -m: do nothing
            }
            break;
        case 'n': // "search-min",        required_argument
            liMchMin = atoi(optarg) ;
            if (liMchMin < 0)
                liMchMin=0;
            break;
        case 'x': // "search-max",        required_argument
            liMchMax = atoi(optarg) ;
            if (liMchMax <= 0)
                liMchMax = 1024 ;
            break;

        case 'd': // debug
        #if debug
            if (strcmp(optarg, "hsh") == 0) {
                JDebug::gbDbg[DBGHSH] = true ;
            } else if (strcmp(optarg, "ahd") == 0) {
                JDebug::gbDbg[DBGAHD] = true ;
            } else if (strcmp(optarg, "cmp") == 0) {
                JDebug::gbDbg[DBGCMP] = true ;
            } else if (strcmp(optarg, "prg") == 0) {
                JDebug::gbDbg[DBGPRG] = true ;
            } else if (strcmp(optarg, "buf") == 0) {
                JDebug::gbDbg[DBGBUF] = true ;
            } else if (strcmp(optarg, "hsk") == 0) {
                JDebug::gbDbg[DBGHSK] = true ;
            } else if (strcmp(optarg, "ahh") == 0) {
                JDebug::gbDbg[DBGAHH] = true ;
            } else if (strcmp(optarg, "bkt") == 0) {
                JDebug::gbDbg[DBGBKT] = true ;
            } else if (strcmp(optarg, "red") == 0) {
                JDebug::gbDbg[DBGRED] = true ;
            } else if (strcmp(optarg, "mch") == 0) {
                JDebug::gbDbg[DBGMCH] = true ;
            } else if (strcmp(optarg, "dst") == 0) {
                JDebug::gbDbg[DBGDST] = true ;
            }
        #endif
            break ;
        default:
            liHlp=1;
        }
    }
    liOptArgCnt=optind-1;

    /* Output greetings */
    if ((liVerbse>0) || (liHlp > 0 ) || (aiArgCnt - liOptArgCnt < 3)) {
        fprintf(JDebug::stddbg, "\nJDIFF - binary diff version " JDIFF_VERSION "\n") ;
        fprintf(JDebug::stddbg, JDIFF_COPYRIGHT "\n");
        fprintf(JDebug::stddbg, "\n") ;
        fprintf(JDebug::stddbg, "JojoDiff is free software: you can redistribute it and/or modify it\n");
        fprintf(JDebug::stddbg, "under the terms of the  GNU General Public License  as published by\n");
        fprintf(JDebug::stddbg, "the Free Software Foundation,  either version 3 of the License,  or\n");
        fprintf(JDebug::stddbg, "(at your option) any later version.\n");
        fprintf(JDebug::stddbg, "\n");
        fprintf(JDebug::stddbg, "This program is distributed in the hope that it will be useful,\n");
        fprintf(JDebug::stddbg, "but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
        fprintf(JDebug::stddbg, "MERCHANTABILITY  or  FITNESS FOR A PARTICULAR PURPOSE. See the\n");
        fprintf(JDebug::stddbg, "GNU General Public License for more details.\n");
        fprintf(JDebug::stddbg, "\n");
        fprintf(JDebug::stddbg, "You should have received a copy of the GNU General Public License\n");
        fprintf(JDebug::stddbg, "along with this program. If not, see www.gnu.org/licenses/gpl-3.0\n\n");

        off_t maxoff_t_gb = (MAX_OFF_T >> 30) ;
        #ifndef JDIFF_LARGEFILE
        maxoff_t_gb = 2 ;           // without JDIFF_LARGEFILE, filesize is limited to 2GB
        #endif // JDIFF_LARGEFILE
        const char *maxoff_t_mul = "GB";
        if (maxoff_t_gb > 1024) {
            maxoff_t_gb = maxoff_t_gb >> 10 ;
            maxoff_t_mul = "TB";
        }

        fprintf(JDebug::stddbg, "File adressing is %d bit for files up to %d%s, samples are %d bytes.\n",
                (int) (sizeof(off_t) * 8), (int) maxoff_t_gb, maxoff_t_mul, SMPSZE) ;
    }

    if ((aiArgCnt - liOptArgCnt < 3) || (liHlp > 0) || (liVerbse>2)) {
        // ruler:                0---------1---------2---------3---------4---------5---------6---------7---------8
        fprintf(JDebug::stddbg, "\n");
        fprintf(JDebug::stddbg, "JDiff differentiates two files so that the second file can be recreated from\n");
        fprintf(JDebug::stddbg, "the first by \"undiffing\". JDiff aims for the smallest possible diff file.\n\n"),

        fprintf(JDebug::stddbg, "Usage: jdiff -j [options] <source file> <destination file> [<diff file>]\n") ;
        fprintf(JDebug::stddbg, "   or: jdiff -u [options] <source file> <diff file> [<destination file>]\n\n") ;
        fprintf(JDebug::stddbg, "  -j                       JDiff:  create a difference file.\n");
        #ifdef JDIFF_DEDUP
        #endif // JDIFF_DEDUP
        fprintf(JDebug::stddbg, "  -u                       Undiff: undiff a difference file.\n\n");

        fprintf(JDebug::stddbg, "  -v --verbose             Verbose: greeting, results and tips.\n");
        fprintf(JDebug::stddbg, "  -vv                      Extra Verbose: progress info and statistics.\n");
        fprintf(JDebug::stddbg, "  -vvv                     Ultra Verbose: all info, including help and details.\n");
        fprintf(JDebug::stddbg, "  -h --help -hh            Help, additional help (-hh) and exit.\n");
        fprintf(JDebug::stddbg, "  -l --listing             Detailed human readable output.\n");
        fprintf(JDebug::stddbg, "  -r --regions             Grouped  human readable output.\n");
        fprintf(JDebug::stddbg, "  -c --console             Write verbose and debug info to stdout.\n\n");

        fprintf(JDebug::stddbg, "  -b --better -bb...       Better: use more memory, search more.\n");
        fprintf(JDebug::stddbg, "  -bb                      Best:   even more memory, search more.\n");
        fprintf(JDebug::stddbg, "  -f --lazy                Lazy:   no unbuffered searching (often slower).\n");
        fprintf(JDebug::stddbg, "  -ff                      Lazier: no full index table.\n");
        fprintf(JDebug::stddbg, "  -p --sequential-source   Sequential source (to avoid !) (with - for stdin).\n");
        fprintf(JDebug::stddbg, "  -q --sequential-dest     Sequential destination (with - for stdin).\n");
        #ifndef JDIFF_STDIO_ONLY
        fprintf(JDebug::stddbg, "  -s --stdio               Use stdio files (for testing).\n");
        #endif // JDIFF_STDIO_ONLY
        #ifdef JDIFF_DEDUP
        fprintf(JDebug::stddbg, "  -y --reflink             Reflink to source file when possible.\n") ;
        #endif // JDIFF_DEDUP
        fprintf(JDebug::stddbg, "\n");
        fprintf(JDebug::stddbg, "  -a --search-size <size>  Size (in KB) to search (default=buffer-size).\n");
        fprintf(JDebug::stddbg, "  -i --index-size  <size>  Size (in MB) for index table    (default 64).\n");
        fprintf(JDebug::stddbg, "  -k --block-size  <size>  Block size in bytes for reading (default 8192).\n");
        fprintf(JDebug::stddbg, "  -m --buffer-size <size>  Size (in KB) for search buffers (0=no buffering)\n");
        fprintf(JDebug::stddbg, "  -n --search-min <count>  Minimum number of matches to search (default %d).\n", liMchMin);
        fprintf(JDebug::stddbg, "  -x --search-max <count>  Maximum number of matches to search (default %d).\n\n", liMchMax);

        fprintf(JDebug::stddbg, "Make  diff-file: jdiff -j old-file new-file diff-file.jdf\n");
        fprintf(JDebug::stddbg, "Apply diff-file: jdiff -u old-file diff-file.jdf recreated-new-file\n\n");

        fprintf(JDebug::stddbg, "Hint:\n");
        fprintf(JDebug::stddbg, "  Do not use jdiff on compressed files. Rather use jdiff first and compress\n");
        fprintf(JDebug::stddbg, "  afterwards, e.g.: jdiff -j old new | gzip >dif.jdf.gz (or 7z with -si)\n");

        if ((liHlp>1) || (liVerbse>2)) {
            fprintf(JDebug::stddbg, "\nNotes:\n");
            fprintf(JDebug::stddbg, " - Options -b, -bb, -f, -ff, ... should be used before other options.\n");
            fprintf(JDebug::stddbg, " - Accuracy may be improved by increasing the index table size (-i) or\n");
            fprintf(JDebug::stddbg, "   the buffer size (-m), see below.\n");
            fprintf(JDebug::stddbg, " - The index table size is always lowered to the nearest lower prime number.\n");
            fprintf(JDebug::stddbg, " - Output is sent to standard output if no output file is specified.\n");
            // ruler:                0---------1---------2---------3---------4---------5---------6---------7---------8
            fprintf(JDebug::stddbg, "\nAdditional explications:\n");
            fprintf(JDebug::stddbg, "  JDiff starts by comparing source and destination files.\n");
            fprintf(JDebug::stddbg, "  \n");
            fprintf(JDebug::stddbg, "  When a difference is found, JDiff will first index the source file.\n");
            fprintf(JDebug::stddbg, "  Normally, the full source file is indexed, but this can be disabled by the\n");
            fprintf(JDebug::stddbg, "  -ff or -p options, in which case only the buffered part of the source file\n");
            fprintf(JDebug::stddbg, "  will be indexed. This may be faster, but at a loss of accuracy.\n");
            fprintf(JDebug::stddbg, "  \n");
            fprintf(JDebug::stddbg, "  Using the index, JDiff will search for equal regions between both files.\n");
            fprintf(JDebug::stddbg, "  The index table however has two problems:\n");
            fprintf(JDebug::stddbg, "  - too small, because a full index would require too much memory.\n");
            fprintf(JDebug::stddbg, "  - inaccurate, because the hash-keys are only 32 or 64 bit check-sums.\n");
            fprintf(JDebug::stddbg, "  \n");
            fprintf(JDebug::stddbg, "  The inaccuracy is reduced by either:\n");
            fprintf(JDebug::stddbg, "  - comparing the found matches from the index, which is slower but certain\n");
            fprintf(JDebug::stddbg, "  - confirmation from subsequent matches, which is faster but uncertain\n");
            fprintf(JDebug::stddbg, "  Inaccuracy of course can also be reduced with a bigger index table (-i option)\n");
            fprintf(JDebug::stddbg, "  \n");
            fprintf(JDebug::stddbg, "  Also, the first found solution is not always the best solution.\n");
            fprintf(JDebug::stddbg, "  Therefore, JDiff searches a minimum (-n) number of solutions, and\n");
            fprintf(JDebug::stddbg, "  will continue up to a maximum (-x) number of solutions if data is buffered.\n");
            fprintf(JDebug::stddbg, "  That's why, bigger buffers (-m) can improve accuracy.\n");
            fprintf(JDebug::stddbg, "  \n");
            fprintf(JDebug::stddbg, "  The -b/-bb options increase the index table, buffers and solutions to search.\n");
            fprintf(JDebug::stddbg, "  The -f/-ff options will only compare buffered data to gain some speed, but\n");
            fprintf(JDebug::stddbg, "  will often be slower due to the lower accuracy.\n");
        }
        if (aiArgCnt - liOptArgCnt < 3){
            if  (liHlp == 0)
                fprintf(JDebug::stddbg, "Error: Not enough arguments have been specified !\n");

            exit(- EXI_ARG);
        }
    } else if (liVerbse > 0) {
        fprintf(JDebug::stddbg, "\nUse -h for additional help and usage description.\n");
    }

    /* Read filenames */
    lcFilNamOrg = acArg[1 + liOptArgCnt];
    lcFilNamNew = acArg[2 + liOptArgCnt];
    if (aiArgCnt - liOptArgCnt >= 4)
        lcFilNamOut = acArg[3 + liOptArgCnt];
    else
        lcFilNamOut = "-" ;

    if (strcmp(lcFilNamNew, csStdInpOutNam) == 0 && strcmp(lcFilNamOrg, csStdInpOutNam) == 0 ){
        fprintf(JDebug::stddbg, "%s", "Error: Original and destination files cannot both be from standard input !\n");
        exit(- EXI_ARG);
    }

    // Set default values for llBlk and liBlk
    llBufOrg = (llBufOrg > 0 ? llBufOrg : lbSeqOrg ? 32 : 1) ;
    llBufNew = (llBufNew > 0 ? llBufNew : lbSeqNew ? 16 : llBufOrg) * 1024 * 1024 ;
    llBufOrg = llBufOrg * 1024 * 1024 ;
    liBlkSze = (liBlkSze < 4096 ? 4096 : liBlkSze) ;

    // Buffer size cannot be zero and must be aligned on block size
    // Block size  cannot be larger than buffer size
    if (llBufOrg % liBlkSze != 0){
        llBufOrg -= llBufOrg % liBlkSze ;
        if (llBufOrg <= 0)
            llBufOrg = liBlkSze;
        fprintf(JDebug::stddbg, "Warning: Source buffer size misaligned with block size: set to %ld.\n", llBufOrg);
    }
    if (llBufNew % liBlkSze != 0){
        llBufNew -= llBufNew % liBlkSze ;
        if (llBufNew <= 0)
            llBufNew = liBlkSze;
        fprintf(JDebug::stddbg, "Warning: Destination buffer size misaligned with block size: set to %ld.\n", llBufNew);
    }

    // Default search ahead window
    if (liAhdMax==0){
        liAhdMax = llBufNew - liBlkSze ;
        if (liAhdMax > llBufNew - liBlkSze)
            liAhdMax = llBufNew - liBlkSze ;
        if (liAhdMax < 4096)
            liAhdMax = 4096 ;
    }

    /* Open files and create file handlers */
    JFile *lpJflOrg = NULL ;
    JFile *lpJflNew = NULL ;

    #ifdef JDIFF_STDIO_ONLY
        // MinGW didn't correctly handle files > 2GB using fstream.gse
        // Force use of stdio
        lbStdio=true ;
    #endif // JDIFF_STDIO_ONLY

    FILE *lfFilOrg = NULL ;
    FILE *lfFilNew = NULL ;

    if (lbStdio) {
        /* Open first file */
        if (strcmp(lcFilNamOrg, csStdInpOutNam) == 0 ) {
            // Windows needs some additional tweaking for stdin to work
            #ifdef _WIN32
            if (liVerbse > 1)
                fprintf(JDebug::stddbg, "%s\n", "Setting Windows stdin to binary mode.");
            setmode(fileno(stdin), O_BINARY );
            #endif // __WIN32__

            // create a JFile
            lpJflOrg = new JFileAheadStdio(stdin, "Org", llBufOrg, liBlkSze, lbSeqOrg);
        } else {
            lfFilOrg = jfopen(lcFilNamOrg, "rb") ;
            if (lfFilOrg != NULL) {
                lpJflOrg = new JFileAheadStdio(lfFilOrg, "Org", llBufOrg, liBlkSze, lbSeqOrg);
            }
        }

        /* Open second file */
        if (strcmp(lcFilNamNew, csStdInpOutNam) == 0 ) {
            // Windows needs some additional tweaking for stdin to work
            #ifdef _WIN32
            if (liVerbse > 1)
                fprintf(JDebug::stddbg, "%s\n", "Setting Windows stdin to binary mode.");
            setmode(fileno(stdin), O_BINARY );
            #endif // __WIN32__

            // create a JFile
            lpJflNew = new JFileAheadStdio(stdin, "New", llBufNew, liBlkSze, lbSeqNew);
        } else {
            if (liFun == Dedup)
                lfFilNew = jfopen(lcFilNamNew, "r+b") ;
            else
                lfFilNew = jfopen(lcFilNamNew, "rb") ;
            if (lfFilNew != NULL) {
                lpJflNew = new JFileAheadStdio(lfFilNew, "New", llBufNew, liBlkSze, lbSeqNew);
            }
        }
    }

    #ifndef JDIFF_STDIO_ONLY
    ifstream loSrmOrg;
    ifstream loSrmNew;
    if (! lbStdio) {
        /* Open first file */
        if (strcmp(lcFilNamOrg, csStdInpOutNam) == 0 ){
            // Windows needs some additional tweaking for stdin to work
            #ifdef _WIN32
            if (liVerbse > 1)
                fprintf(JDebug::stddbg, "%s\n", "Setting Windows stdin to binary mode.");
            setmode(fileno(stdin), O_BINARY );
            #endif // __WIN32__

            // create a JFile
            lpJflOrg = new JFileAheadIStream(cin, "Org",  llBufOrg, liBlkSze, lbSeqOrg);
        } else {
            loSrmOrg.open(lcFilNamOrg, ios_base::in | ios_base::binary) ;
            if (loSrmOrg.is_open()) {
                lpJflOrg = new JFileAheadIStream(loSrmOrg, "Org",  llBufOrg, liBlkSze, lbSeqOrg);
            }
        }

        /* Open second file */
        if (strcmp(lcFilNamNew, csStdInpOutNam) == 0 ){
            // Windows needs some additional tweaking for stdin to work
            #ifdef _WIN32
            if (liVerbse > 1)
                fprintf(JDebug::stddbg, "%s\n", "Setting Windows stdin to binary mode.");
            setmode(fileno(stdin), O_BINARY );
            #endif // __WIN32__

            // create a JFile
            lpJflNew = new JFileAheadIStream(cin, "New",  llBufNew, liBlkSze, lbSeqNew);
        } else {
            loSrmNew.open(lcFilNamNew, ios_base::in | ios_base::binary) ;

            if (loSrmNew.is_open()) {
                lpJflNew = new JFileAheadIStream(loSrmNew, "New",  llBufNew, liBlkSze, lbSeqNew);
            }
        }
    }
    #endif // JDIFF_STDIO_ONLY

    if (lpJflOrg == NULL) {
        fprintf(JDebug::stddbg, "Could not open first file %s for reading.\n", lcFilNamOrg);
        exit(- EXI_FRT);
    }

    if (lpJflNew == NULL) {
        fprintf(JDebug::stddbg, "Could not open second file %s for reading.\n", lcFilNamNew);
        exit(- EXI_SCD);
    }

    /* Open output */
    if (liFun == Dedup) {
        lpFilOut = null ;
    } else {
        if (strcmp(lcFilNamOut,csStdInpOutNam) == 0 ){
            lpFilOut = stdout ;

            // Windows needs dome additional tweaking for stdout to work
            #ifdef _WIN32
            if (liVerbse > 1)
                fprintf(JDebug::stddbg, "%s\n", "Setting Windows stdout to binary mode.");
            setmode(fileno(lpFilOut), O_BINARY );
            #endif // __WIN32__
        } else {
            lpFilOut = fopen(lcFilNamOut, "wb") ;
        }
        if ( lpFilOut == null ) {
            fprintf(JDebug::stddbg, "Could not open output file %s for writing.\n", lcFilNamOut) ;
            exit(- EXI_OUT);
        }
    }

    /* Execute required function */
    int liRet = EXI_ARG ; /**< default return code */
    if (liFun == Diff || liFun == Test || liFun == Dedup) {
        /* Perform JDiff */
        // Switch to sequential source file
        if (! lbSeqOrg && lpJflOrg->isSequential()){
            lbSeqOrg = true ;
            lbCmpAll = false ;            // only compare data within the buffer
            lbSrcBkt = false ;            // only backtrack on source file in buffer
            liSrcScn = 0;                 // no pre-scan indexing

            fprintf(JDebug::stddbg, "\n%s\n", "Warning: Source file is a sequential file, assuming -p.");
        }

        // Switch to sequential destination file
        if (! lbSeqNew && lpJflNew->isSequential()){
            lbSeqNew = true ;
            liMchMin = 0;                 // only search within the buffer
            fprintf(JDebug::stddbg, "\n%s\n", "Warning: Destination file is a sequential file, assuming -q.");
        }

        /* Init output */
        JOut *lpOut ;
        switch (liOutTyp) {
        case 0:
            lpOut = new JOutBin(lpFilOut);
            break;
        case 1:
            lpOut = new JOutAsc(lpFilOut);
            break;
        #ifdef JDIFF_DEDUP
        case 3:
            lpOut = new JOutDedup(*lpJflOrg, *lpJflNew, liVerbse) ;
            break ;
        #endif // JDIFF_DEDUP
        case 2:
        default:  // XXX get rid of uninitialized warning
            lpOut = new JOutRgn(lpFilOut);
            break;
        }

        /* Initialize JDiff object */
        JDiff loJDiff(lpJflOrg, lpJflNew, lpOut,
                      liHshMbt, liVerbse,
                      lbSrcBkt, liSrcScn, liMchMax, liMchMin, liAhdMax, lbCmpAll);

        /* Show execution parameters */
        if (liVerbse>1) {
            fprintf(JDebug::stddbg, "\n");
            fprintf(JDebug::stddbg, "Index table size (default: 64Mb) (-s): %dMb (%d samples)\n",
                    ((loJDiff.getHsh()->get_hashsize() + 512) / 1024 + 512) / 1024,
                    loJDiff.getHsh()->get_hashprime()) ;
            fprintf(JDebug::stddbg, "Search size     (0 = buffersize) (-a): %dkb\n",  liAhdMax / 1024 );
            fprintf(JDebug::stddbg, "Buffer size       (default  2Mb) (-m): %ldMb\n", (llBufOrg + llBufNew) / 1024 / 1024);
            fprintf(JDebug::stddbg, "Block  size       (default 32kb) (-b): %dkb\n",  liBlkSze / 1024);
            fprintf(JDebug::stddbg, "Min number of matches to search  (-n): %d\n", liMchMin);
            fprintf(JDebug::stddbg, "Max number of matches to search  (-x): %d\n", liMchMax);
            fprintf(JDebug::stddbg, "Compare out-of-buffer (-f to disable): %s\n",    lbCmpAll?"yes":"no");
            fprintf(JDebug::stddbg, "Full indexing scan   (-ff to disbale): %s\n",   (liSrcScn>0)?"yes":"no");
            fprintf(JDebug::stddbg, "Backtrace allowed     (-p to disable): %s\n",    lbSrcBkt?"yes":"no");
        }

        /* Execute... */
        liRet = loJDiff.jdiff();
        if (liRet == EXI_OK) {
            if (lpOut->gzOutBytDta > 0)
                liRet=EXI_DIF ;
            else
                liRet=EXI_EQL ;
        }

        /* Write statistics */
        if (liVerbse > 1) {
            fprintf(JDebug::stddbg, "\n");
            fprintf(JDebug::stddbg, "Index table hits        = %d\n",   loJDiff.getHsh()->get_hashhits()) ;
            fprintf(JDebug::stddbg, "Index table repairs     = %d\n",   loJDiff.getMch()->getHshRpr()) ;
            fprintf(JDebug::stddbg, "Index table overloading = %d\n",   loJDiff.getHsh()->get_hashcolmax() / 4 - 1);
            fprintf(JDebug::stddbg, "Reliability distance    = %d\n",   loJDiff.getHsh()->get_reliability());
            fprintf(JDebug::stddbg, "Inaccurate  solutions   = %d\n",   loJDiff.getHshErr()) ;
            fprintf(JDebug::stddbg, "Source      seeks       = %ld\n",  lpJflOrg->seekcount());
            fprintf(JDebug::stddbg, "Destination seeks       = %ld\n",  lpJflNew->seekcount());
            fprintf(JDebug::stddbg, "Delete      bytes       = %" PRIzd "\n", lpOut->gzOutBytDel);
            fprintf(JDebug::stddbg, "Backtrack   bytes       = %" PRIzd "\n", lpOut->gzOutBytBkt);
            fprintf(JDebug::stddbg, "Escape      bytes       = %" PRIzd "\n", lpOut->gzOutBytEsc);
            fprintf(JDebug::stddbg, "Control     bytes       = %" PRIzd "\n", lpOut->gzOutBytCtl);
        }
        if (liVerbse > 0) {
            fprintf(JDebug::stddbg, "\n");
            fprintf(JDebug::stddbg, "Equal       bytes       = %" PRIzd "\n", lpOut->gzOutBytEql);
            fprintf(JDebug::stddbg, "Data        bytes       = %" PRIzd "\n", lpOut->gzOutBytDta);
            fprintf(JDebug::stddbg, "Control-Esc bytes       = %" PRIzd "\n", lpOut->gzOutBytCtl + lpOut->gzOutBytEsc);
            fprintf(JDebug::stddbg, "Total       bytes       = %" PRIzd "\n",
                    lpOut->gzOutBytCtl + lpOut->gzOutBytEsc + lpOut->gzOutBytDta);
        }
    } /* liFun == 0 or 2 */
    if (liFun == Patch || liFun == Test) {
        JFileOut loFilOut(lpFilOut) ;

        JPatcht loJPatcht(*lpJflOrg, *lpJflNew, loFilOut, liVerbse) ;
        liRet = loJPatcht.jpatch();
    } /* liFun == 1 or 2 */

    /* Cleanup */
    delete lpJflOrg;
    delete lpJflNew;

    #ifndef JDIFF_STDIO_ONLY
    if (! lbStdio) {
        if (loSrmOrg.is_open()) {
            loSrmOrg.close();
        }
        if (loSrmNew.is_open()) {
            loSrmNew.close();
        }
    }
    #endif // JDIFF_STDIO_ONLY
    if (lfFilOrg != NULL) jfclose(lfFilOrg);
    if (lfFilNew != NULL) jfclose(lfFilNew);


    /* Exit */
    switch (liRet) {
    case EXI_SEK:
        fprintf(JDebug::stddbg, "\nSeek error !\n");
        exit (- EXI_SEK);
    case EXI_LRG:
        fprintf(JDebug::stddbg, "\nError: 64-bit offsets not supported !\n");
        exit (- EXI_LRG);
    case EXI_RED:
        fprintf(JDebug::stddbg, "\nError reading file !\n");
        exit (- EXI_RED);
    case EXI_WRI:
        fprintf(JDebug::stddbg, "\nError writing file !\n");
        exit (- EXI_WRI);
    case EXI_MEM:
        fprintf(JDebug::stddbg, "\nError allocating memory !\n");
        exit (- EXI_MEM);
    case EXI_ARG:
        fprintf(JDebug::stddbg, "\nError in arguments !\n");
        exit (- EXI_ARG);
    case EXI_ERR:
        fprintf(JDebug::stddbg, "\nError occurred !\n");
        exit (- EXI_ERR);
    case EXI_OK:
        exit (EXI_OK) ;
    case EXI_EQL:
        if (liVerbse > 1)
            fprintf(JDebug::stddbg, "\nFound all data within source file.\n");
        exit(EXI_OK) ;
    case EXI_DIF:
        if (liVerbse > 1)
            fprintf(JDebug::stddbg, "\nNot all data has been found in source file.\n");
        exit(EXI_DIF) ;
    default:
        fprintf(JDebug::stddbg, "\nUnknown exit code %d\n", liRet);
        exit (- EXI_ERR);
    }
}
