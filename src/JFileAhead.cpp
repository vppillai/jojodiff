/*
 * JFileAhead.cpp
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

#include "JDefs.h"

#include <stdlib.h>
#include <stdio.h>
#include <exception>
#if debug
#include <string.h>
#endif

#include "JFileAhead.h"
#include "JDebug.h"

namespace JojoDiff {

/**
 * Construct a buffered JFile on an istream.
 */
JFileAhead::JFileAhead(char const * const asJid, const long alBufSze, const int aiBlkSze, const bool abSeq )
: JFile(asJid, abSeq)
, mlBufSze(alBufSze == 0 ? 1024 : alBufSze)
, miBlkSze(aiBlkSze)
{
    // Block size  cannot be zero and cannot be larger than buffer size
    // Buffer size cannot be zero and must be aligned on block size
    if (miBlkSze == 0){
        fprintf(JDebug::stddbg, "Warning: Block size cannot be zero: set to %d.\n", 1);
        miBlkSze = 1;
    }
    if (mlBufSze % miBlkSze != 0){
        mlBufSze -= mlBufSze % miBlkSze ;
        fprintf(JDebug::stddbg, "Warning: Buffer size misaligned with block size: set to %ld.\n", mlBufSze);
    }
    if (mlBufSze == 0){
        mlBufSze = miBlkSze;
        fprintf(JDebug::stddbg, "Warning: Buffer size cannot be zero: set to %ld.\n", mlBufSze);
    }

    // Allocate buffer
    mpBuf = (jchar *) malloc(mlBufSze) ;
#ifdef JDIFF_THROW_BAD_ALLOC
    if (mpBuf == null){
        throw bad_alloc() ;
    }
#endif

    // Initialize buffer logic
    mpMax = mpBuf + mlBufSze ;
    mpInp = mpBuf;
    mpRed = mpInp;

    miBufUsd = 0;
    mzPosInp = 0;
    mzPosEof = MAX_OFF_T ;
    mzPosRed = 0 ;
    mzPosBse = 0 ;
    miRedSze = 0 ;

#if debug
    if (JDebug::gbDbg[DBGBUF])
        fprintf(JDebug::stddbg, "ufFabOpn(%s):(buf=%p,max=%p,sze=%ld)\n",
                asJid, mpBuf, mpMax, mlBufSze);
#endif
}

JFileAhead::~JFileAhead() {
	if (mpBuf != null) free(mpBuf) ;
}

/**
 * @brief Return number of seeks performed.
 */
long JFileAhead::seekcount() const {
    return mlFabSek;
}

/**
 * @brief Return the position of the buffer
 *
 * @return  -1=no buffering, > 0 : first position in buffer
 */
off_t JFileAhead::getBufPos(){
    return mzPosInp - miBufUsd ;
}

/**
 * @brief Return the size of the buffer
 *
 * @return  -1=no buffering, > 0 : size of the buffer
 */
long JFileAhead::getBufSze() {
    return mlBufSze ;
}

/**
 * @brief Set lookahead base: soft lookahead will fail when reading after base + buffer size
 *
 * Attention: the base will be implicitly changed by get on non-buffered reads too !
 *
 * @param   azBse	base position
 */
void JFileAhead::set_lookahead_base (
    const off_t azBse	/* new base position */
) {
    mzPosBse = azBse ;
}

/**
 * Tries to get data from the buffer. Calls get_outofbuffer if that is not possible.
 * @param azPos     position to read from
 * @param aiSft     0=read, 1=hard ahead, 2=soft ahead
 * @return data at requested position, EOF or EOB.
 */
int JFileAhead::get_frombuffer (
    const off_t azPos,     /* position to read from                */
    const eAhead aiSft     /* 0=read, 1=hard ahead, 2=soft ahead   */
){
	jchar *lpDta ;
	off_t lzLen ;

	lpDta = getbuf(azPos, lzLen, aiSft) ;
	if (lpDta == null) {
	    // EOF, EOB or any other problem
        mzPosRed = -1;
        mpRed = null;
        miRedSze = 0 ;
        return lzLen ;
	} else {
	    #if debug
	    // double-verify contents of the buffer
        if (JDebug::gbDbg[DBGRED]) {
            // detect buffer logic failure
            int lzDbg ;
            jchar *lpDbg ;
            lzDbg = (mzPosInp - azPos) ;
            lpDbg = (mpInp - lzDbg) ;
            if (lpDbg < mpBuf || lpDbg >= mpMax)
                lpDbg += mlBufSze ;
            if (lpDbg != lpDta){
                fprintf(JDebug::stddbg, "JFileAhead(%s," P8zd ",%d)->%c=%2x (mem %p): pos-error !\n",
                   msJid, azPos, aiSft, *lpDta, *lpDta, lpDta );
            }

            // detect buffer contents failure
            static jchar lcTst[1024*1024] ;
            int liDne ;
            int liCmp ;
            int liLen ;
            jseek(azPos) ;
            if (lzLen > (off_t) sizeof(lcTst))
                liLen = sizeof(lcTst);
            else
                liLen = lzLen ;
            liDne = jread(lcTst, liLen) ;
            if (liDne != liLen){
                fprintf(JDebug::stddbg, "JFileAhead(%s," P8zd ",%d)->%c=%2x (mem %p): len-error !\n",
                   msJid, azPos, aiSft, *lpDta, *lpDta, lpDta );
            }
            liCmp = memcmp(lpDta, lcTst, liLen) ;
            if (liCmp != 0) {
                fprintf(JDebug::stddbg, "JFileAhead(%s," P8zd ",%d)->%c=%2x (mem %p): buf-error !\n",
                   msJid, azPos, aiSft, *lpDta, *lpDta, lpDta );
            }
            jseek(mzPosInp);
	    }
	    #endif

        // prepare next reading position (but do not increase lpDta!!!)
        mzPosRed = azPos + 1;
        miRedSze = lzLen - 1;
        mpRed = lpDta + 1;
        if (mpRed == mpMax) {
            mpRed = mpBuf ;
        }

        // return data at current position
        return *lpDta ;
	}
}

/**
 * @brief Get access to buffered read.
 *
 * @param   azPos   in:  position to get access to
 * @param   azLen   out: number of bytes in buffer
 * @param   aiSft   in:  0=read, 1=hard read ahead, 2=soft read ahead
 *
 * @return  buffer, null = azPos not in buffer
 */
jchar * JFileAhead::getbuf(const off_t azPos, off_t &azLen, const eAhead aiSft) {
	jchar *lpDta=null ;

	if (azPos >= mzPosEof) {
        /* eof */
        azLen = EOF ;
        return null ;
	} else if (azPos < mzPosInp && azPos >= mzPosInp - miBufUsd){
	    // Data is already in the buffer
	} else {
	    // Get data from underlying file
        switch (get_fromfile(azPos, aiSft)) {
        case EndOfBuffer: azLen = EOB ;    return null ;
        case EndOfFile:   azLen = EOF ;    return null ;
        case SeekError:   azLen = EXI_SEK; return null ;
        case ReadError:   azLen = EXI_RED; return null ;
        case Added: // data added
            break ;
        } // switch get_fromfile
    } // if elseif else azPos

    // Calculate position of azPos
    azLen = mzPosInp - azPos ;
    if (azLen <= mpInp - mpBuf){
        lpDta = mpInp - azLen ;
    } else {
        lpDta = mpInp + mlBufSze - azLen ;
        azLen = mpMax - lpDta ;
    }

    #if debug
    if (lpDta < mpBuf || lpDta >= mpMax){
        fprintf(JDebug::stddbg, "JFileAhead::getbuf(%s,%" PRIzd ",%" PRIzd ",%d)->   (sto %p) out of bounds !\n",
                msJid, azPos, azLen, aiSft, lpDta);
        exit(- EXI_SEK);
    }
    if (azPos >= mzPosInp || azPos < mzPosInp - miBufUsd) {
        fprintf(JDebug::stddbg, "JFileAhead::getbuf(%s,%" PRIzd ",%" PRIzd ",%d)->%2x (sto %p) failed !\n",
                msJid, azPos, azLen, aiSft, *lpDta, lpDta);
        exit(- EXI_SEK);
    }
    #endif

    return lpDta ;
}

/**
 * Retrieve requested position into the buffer, trying to keep the buffer as
 * large as possible (i.e. invalidating/overwriting as less as possible).
 *
 * @param azPos     position to read from
 * @param azLen     out: > 0 : available bytes in buffer, < 0 : EOF or EOB.
 * @param aiSft     0=read, 1=hard ahead, 2=soft ahead
 *
 * @return EOF, EOB
 * @return 0 = data read
 * @return 1 = data read but buffer cycled
 * @return 3 = partial read, unaligned/broken read
 */
JFileAhead::eBufDne JFileAhead::get_fromfile (
    const off_t azPos,      /**< position to read from                */
    const eAhead aiSft      /**< 0=read, 1=hard ahead, 2=soft ahead   */
){
    eBufOpr liSek ;         /**< buffer logic operation type    */
    int liDne ;             /**< number of bytes read           */

    /* Preparation: Check what should be done and set liSek accordingly */
    if (azPos < mzPosInp - miBufUsd ) {
        // Reading before the start of the buffer:
        // - not allowed in sequential nor soft-reading mode
        // - either cancel the whole buffer: easiest, but we loose all data in the buffer
        // - either scroll back the buffer: harder, but we may not loose all data in the buffer
        if (aiSft == SoftAhead)
            return EndOfBuffer ;
        else if (mbSeq){
            if (aiSft == HardAhead)
                return EndOfBuffer ;
            else
                return SeekError ;
        } else if (azPos + mlBufSze - miBlkSze > mzPosInp - miBufUsd)
            liSek = Scrollback;
        else
            liSek = Reset ;
    } else if (azPos >= mzPosInp + mlBufSze) {
        // Advancing more than the size of the buffer:
        // - not allowed when soft-reading
        // - just reset
        if (aiSft == SoftAhead)
            return EndOfBuffer ;
        else
            liSek = Reset ;
    } else {
        // Append to the buffer if possible
        if (aiSft == SoftAhead && azPos > mzPosBse + mlBufSze - miBlkSze)
            return EndOfBuffer;
        else
            liSek = Append ;
    }

    switch (liSek){
    case Reset:
        if (! mbSeq){
            // Calculate position and length
            mzPosInp = (azPos / miBlkSze) * miBlkSze ;
        } else {
            // In sequential mode: jump forward and then append, keep the buffer as large as possible
            mzPosInp = ((azPos - mlBufSze + miBlkSze) / miBlkSze) * miBlkSze ;
        }

        // Reset buffer
        mpInp    = mpBuf;
        mzPosBse = mzPosInp ;
        miBufUsd = 0 ;

        // Seek
        if (jseek(mzPosInp) != EXI_OK)
            return SeekError ;
        mlFabSek++ ;

        // Read
        liDne = readblocks(mpInp, mzPosInp, azPos);
        if (liDne == EOF)
            return EndOfFile ;
    break ;

    case Append:
        liDne = readblocks(mpInp, mzPosInp, azPos);
        if (liDne == EOF)
            return EndOfFile ;
    break ;

    case Scrollback: {
        // Calculate scrollback position
        off_t  lzPos = (azPos / miBlkSze) * miBlkSze ;   /**< position to seek               */
        off_t  lzLen = mzPosInp - lzPos ;                /**< new potential buffer length    */
        jchar* lpInp = mpInp - lzLen ;
        if (lzLen  > mpInp - mpBuf) {
            lpInp += mlBufSze ;
        }

        // Make room in the buffer for the scrollback
        if (lzLen > mlBufSze){
            lzLen    = lzLen - mlBufSze ;
            miBufUsd = miBufUsd - lzLen ;
            mzPosInp = lzPos + mlBufSze ;
            if (lzLen  > mpInp - mpBuf)
                lpInp += mlBufSze ;
            mpInp    = lpInp ;
        }

        // Seek
        if (jseek(lzPos) != EXI_OK)
            return SeekError ;
        mlFabSek++ ;

        // Read loop
        liDne = readblocks(lpInp, lzPos, mzPosInp - miBufUsd - 1);
        if (liDne == EOF){
            // A scrollback cannot issue an EOF unless there's a hardware error
            // or the file is being truncated while we're reading it.
            // In both cases, the outcome will probably be unusable.
            // The buffer variables are set here just for the sake of "correctness".
            mpInp = lpInp ;
            mzPosInp = lzPos ;
            miBufUsd = liDne ;
            return ReadError ;
        }

        // @Seek
        if (jseek(mzPosInp) != EXI_OK)
            return SeekError ;
        mlFabSek++ ;
        } // scrollback
    break ;
    } /* switch liSek */

    return Added ;
} /* get_fromfile */

/**
* @brief Read blocks till the specified end
*/
int JFileAhead::readblocks(jchar* &apInp, off_t &azInp, off_t const azEnd)
{
    int liTdo ;   /**< Number of bytes to read */
    int liDne=0 ; /**< Number of bytes read */

    // Read loop
    while (azInp <= azEnd){
        // Prepare
        liTdo = miBlkSze ;
        if (apInp == mpMax)
            apInp = mpBuf ;
        else if (mpMax - apInp < liTdo)
            liTdo = mpMax - apInp ;

        // Read
        liDne = jread(apInp, liTdo) ;

        // Update buffer vars
        apInp    += liDne ;
        azInp    += liDne ;
        miBufUsd += liDne ;

        // Handle EOF
        if (liDne < liTdo){
            mzPosEof = azInp ;
            if ( miBufUsd > mlBufSze )
                miBufUsd = mlBufSze ;
            if ( azEnd >= mzPosEof )
                return EOF ;
            else
                return liDne ;
        }
    }

    // Update buffer vars
    if ( miBufUsd > mlBufSze )
        miBufUsd = mlBufSze ;

    return liDne ;
} /* readblocks */

} /* namespace JojoDiff */
