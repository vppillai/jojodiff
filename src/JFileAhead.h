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

#ifndef JFileAhead_H_
#define JFileAhead_H_

#include <istream>
using namespace std;

#include "JDefs.h"
#include "JFile.h"

namespace JojoDiff {
/**
 * Buffered JFile access: optimized buffering logic for the specific way JDiff
 * accesses files, that is reading ahead to find equal regions and then coming
 * back to the base position for actual comparisons.
 */
class JFileAhead: public JFile {
    JFileAhead(JFileAhead const&) = delete;
    JFileAhead& operator=(JFileAhead const&) = delete;

public:
    JFileAhead(char const * const asFid, const long alBufSze = 256*1024,
               const int aiBlkSze = 8192, const bool abSeq = false);
    virtual ~JFileAhead();

	 /**
	 * @brief Get access to (fast) buffered read.
	 *
	 * @param   azPos   in:  position to get access to
	 * @param   azLen   out: number of bytes in buffer
	 * @param   aiSft   in:  0=read, 1=hard read ahead, 2=soft read ahead
	 *
	 * @return  buffer, null = azPos not in buffer or no buffer
	 */
	jchar *getbuf(const off_t azPos, off_t &azLen, const eAhead aiSft = Read) ;

	/**
	 * @brief Set lookahead base: soft lookahead will fail when reading after base + buffer size
	 *
	 * Attention: the base will be implicitly changed by get on non-buffered reads too !
	 *
	 * @param   azBse	base position
	 */
	virtual void set_lookahead_base (
	    const off_t azBse	/* new base position for soft lookahead */
	) ;

    /**
	 * @brief Return the position of the buffer
	 *
	 * @return  -1=no buffering, > 0 : first position in buffer
	 */
	off_t getBufPos();

    /**
	 * @brief Return the size of the buffer
	 *
	 * @return  -1=no buffering, > 0 : size of the buffer
	 */
	long getBufSze() ;

    /**
     * Return number of seek operations performed.
     */
    long seekcount() const ;


protected:

    /**
    * @brief Seek abstraction, for override by subclasses
    *
    * @param   azPos    Position to seek to
    * @return  EXI_OK for succes or EXI_SEK in case of error
    */
    virtual int jseek(const off_t azPos) = 0 ;

    /**
    * @brief Seek EOF abstraction, for override by subclasses
    *
    * @return >= 0: EOF position, EXI_SEK in case of error
    */
    virtual off_t jeofpos() = 0 ;

    /**
    * @brief Read abstraction, for override by subclasses
    *
    * @param >= 0: number of bytes read
    */
    virtual size_t jread(jchar * const ptr, const size_t count) = 0 ;


private:
    enum eBufOpr { Append, Reset, Scrollback } ;
    enum eBufDne { Added, EndOfFile = EOF, EndOfBuffer = EOB, SeekError = EXI_SEK, ReadError = EXI_RED } ;

    /**
     * @brief Get data from the buffer. Call get_fromfile such is not possible.
     *
     * @param azPos		position to read from
     * @param aiSft		0=read, 1=hard ahead, 2=soft ahead
     * @return data at requested position, EOF or EOB.
     */
    virtual int get_frombuffer(
        const off_t azPos,    /* position to read from                */
        const eAhead aiSft    /* 0=read, 1=hard ahead, 2=soft ahead   */
    );

    /**
     * @brief Get data from the underlying system file.
     *
     * Retrieves requested position into the buffer, trying to keep the buffer as
     * large as possible (i.e. invalidating/overwriting as less as possible).
     * Calls get_frombuffer afterwards to get the data from the buffer.
     *
     * @param azPos		position to read from
     * @param aiSft		0=read, 1=hard ahead, 2=soft ahead
     * @return 1 = data read
     * @return 2 = buffer cycled
     * @return 3 = unaligned/broken read
     */
    eBufDne get_fromfile(
        const off_t azPos,  /* position to read from                */
        const eAhead aiSft  /* 0=read, 1=hard ahead, 2=soft ahead   */
    );

    /**
    * @brief Read blocks of data
    * @param apInp      input buffer
    * @param azInp      input position
    * @param azEnd      stop reading when end position in reached
    */
    int readblocks(
        jchar* &apInp,      /* input buffer     */
        off_t &azInp,       /* input position   */
        const off_t azEnd   /* end position     */
    );

private:
    /* Settings */
    long mlBufSze;      /**< File lookahead buffer size                   */
    int miBlkSze;       /**< Block size: read from file in blocks         */

    /* Buffer state */
    long miBufUsd=0;    /**< number of bytes used in buffer               */
    jchar *mpBuf=null;  /**< read-ahead buffer                            */
    jchar *mpMax=null;  /**< read-ahead buffer end                        */
    jchar *mpInp=null;  /**< current position in buffer                   */
    off_t mzPosBse=0;   /**< base position for soft reading               */
};
}/* namespace */
#endif /* JFileAhead_H_ */
