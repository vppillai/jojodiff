/*
 * JFileAheadStdio.cpp
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
#ifndef JFileAheadStdio_H
#define JFileAheadStdio_H

#include <stdio.h>
#include "JFileAhead.h"

namespace JojoDiff {

class JFileAheadStdio : public JFileAhead
{
    JFileAheadStdio(JFileAheadStdio const&) = delete;
    JFileAheadStdio& operator=(JFileAheadStdio const&) = delete;

public:
    /** Default constructor */
    JFileAheadStdio(FILE * const apFil, char const * const asFid,
                      const long alBufSze = 256*1024, const int aiBlkSze = 4096,
                      const bool abSeq = false );

    /** Default destructor */
    virtual ~JFileAheadStdio();

 	/**
	* @brief Get underlying file descriptor.
	*/
	virtual int get_fd() const ;

protected:

    /**
    * @brief Seek abstraction, for override by subclasses
    *
    * @param EXI_OK (0) or EXI_SEK in cae of error
    */
    virtual int jseek(const off_t azPos) ;

    /**
    * @brief Seek EOF abstraction, for override by subclasses
    *
    * @return >= 0: EOF position, EXI_SEK in case of error
    */
    virtual off_t jeofpos() ;

    /**
    * @brief Read abstraction, for override by subclasses
    *
    * @param >= 0: number of bytes read
    */
    virtual size_t jread(jchar * const ptr, const size_t count) ;

private:
    FILE * const mpFil;        /**< file handle    */
};
} /* namespace */
#endif // JFileAheadStdio_H
