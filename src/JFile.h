/*
 * JFile.h
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

#ifndef JFILE_H_
#define JFILE_H_
#include <cstdio>

#include "JDefs.h"

namespace JojoDiff {

/**
 * @brief JojoDiff's input file abstraction with absolute adressing.
 *
 * JDiff performs "addressed" file accesses when reading, to simplify certain parts of the algorithm.
 * Hence this abstract wrapper class to translate between streamed and addressed access.
 * For performance, certain parts of the algorithm switch back to streamed buffered access,
 * so there's a little bit of both in JFile.
 *
 * This abstraction also allows anyone to easily apply JDiff to his own data structures,
 * by providing a JFile descendant to JDiff.
 */
class JFile {
public:
    JFile(JFile const&) = delete;
    JFile& operator=(JFile const&) = delete;
	virtual ~JFile(){};

	/**
	* Constructor
	* @param    asJid   JFile-id: Org for source file, New for destination file
	* @param    abSeq   Sequential file (default = no) ?
	*/
	JFile(char const * const asJid, const bool abSeq = false );

	/**
	* Reading type:
	* - Read = normal read
	* - HardAhead = ahead reading, extend the buffer if needed
	* - SoftAhead = buffered ahead reading, return EOB (EndOfBuffer) if data is not available
	* - Test      = read for testing purposes, use data from buffer when possible but never
	*               change the buffer (to avoid interference)
	*/
    enum eAhead { Read, HardAhead, SoftAhead } ;

	/**
	 * @brief Get one byte at specified address and increment the address to the next byte.
	 *
	 * Soft read ahead will return an EOB when date is not available in the buffer.
	 *
	 * @param   azPos	position to read, incremented on read (not for EOF or EOB)
	 * @param   aiSft	soft reading type: 0=read, 1=hard read ahead, 2=soft read ahead
	 * @return 			the read character or EOF or EOB.
	 */
    inline int get (const off_t &azPos, const eAhead aiSft = Read){
        if ((azPos == mzPosRed) && (miRedSze > 0)) {
            mzPosRed++ ;
            miRedSze--;
            return *mpRed++;
        } else {
            return get_frombuffer(azPos, aiSft);
        }
    }

    /**
     * @brief Get next byte
     *
     * Soft read ahead will return an EOB when date is not available in the buffer.
     *
     * @param   aiSft       soft reading type: 0=read, 1=hard read ahead, 2=soft read ahead
     * @return                      the read character or EOF or EOB.
     */
    inline int get (const eAhead aiSft = Read){
        return get(mzPosRed, aiSft);
    } ;

	/**
	 * @brief Set lookahead base: soft lookahead will fail when reading after base + buffer size
	 *
	 * Attention: the base will be implicitly changed by get on non-buffered reads too !
	 *
	 * @param   azBse	base position
	 */
	virtual void set_lookahead_base (
	    const off_t azBse	/* new base position for soft lookahead */
	) = 0 ;

	/**
	* @brief Return if this file is a sequential file.
	*/
	bool isSequential() { return mbSeq ; }

	/**
	 * @brief Return number of seek operations performed.
	 */
	virtual long seekcount() { return mlFabSek ; }

	/**
	* @brief Get underlying file descriptor.
	*/
	virtual int get_fd() const { return -1 ; }

	/**
	 * @brief Return the position of the buffer
	 *
	 * @return  -1=no buffering, > 0 : first position in buffer
	 */
	virtual off_t getBufPos() { return -1 ; }

	/**
	 * @brief Return the size of the buffer
	 *
	 * @return  -1=no buffering, > 0 : size of the buffer
	 */
	virtual long getBufSze() { return -1 ; }

	 /**
	 * @brief Get access to (fast) buffered read.
	 *
	 * @param   azPos   in:  position to get access to
	 * @param   azLen   out: number of bytes in buffer
	 * @param   aiSft   in:  0=read, 1=hard read ahead, 2=soft read ahead
	 *
	 * @return  buffer, null = azPos not in buffer or no buffer
	 */
	virtual jchar *getbuf(const off_t azPos, off_t &azLen, const eAhead aiSft = Read ) {
	     return null ;
    }

protected:

    /**
    * @brief Check if file is sequential or not by looking for EOF
    *
    */
    void chkSeq() ;

    /**
    * @brief Seek EOF abstraction, for override by subclasses
    *
    * @return >= 0: EOF position, EXI_SEK in case of error
    */
    virtual off_t jeofpos() = 0;

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
    ) = 0 ;

    char const * const msJid ;      /**< JFile-id                                           */
    bool mbSeq ;                    /**< Sequential file                                    */
    long miRedSze=0;                /**< distance between izPosRed and izPosInp             */
    jchar *mpRed=null;              /**< last position read from buffer				        */
    off_t mzPosInp=0;               /**< current position in file                           */
    off_t mzPosRed=0;               /**< last position read from buffer				        */
    off_t mzPosEof ;                /**< EOF-position                                       */

    long mlFabSek = 0 ;             /**< Number of times an fseek operation was performed   */

};
} /* namespace */
#endif /* JFILE_H_ */
