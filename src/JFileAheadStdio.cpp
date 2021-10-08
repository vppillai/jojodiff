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
#include "JFileAheadStdio.h"

namespace JojoDiff {

JFileAheadStdio::JFileAheadStdio(FILE * const apFil, char const * const asFid,
                          const long alBufSze, const int aiBlkSze, const bool abSeq)
: JFileAhead(asFid, alBufSze, aiBlkSze, abSeq)
, mpFil(apFil)
{
    chkSeq() ;  // Check if file is sequential or not
}

JFileAheadStdio::~JFileAheadStdio()
{
    //dtor
}

/**
* @brief Seek abstraction, for override by subclasses
*
* @param EXI_OK (0) or EXI_SEK in cae of error
*/
int JFileAheadStdio::jseek(const off_t azPos) {
    if (jfseek(mpFil, azPos, SEEK_SET) != 0)
        return EXI_SEK ;
    else
        return EXI_OK ;
} ;

/**
* @brief Seek EOF abstraction, for override by subclasses
*
* @param EXI_OK (0) or EXI_SEK in cae of error
*/
off_t JFileAheadStdio::jeofpos() {
    if (jfseek(mpFil, 0, SEEK_END) != 0)
        return EXI_SEK;
    else {
        off_t lzEof = jftell(mpFil);
        jfseek(mpFil, 0, SEEK_SET) ;
        return lzEof ;
    }
} ;

/**
* @brief Read abstraction, for override by subclasses
*
* @param >= 0: number of bytes read
*/
size_t JFileAheadStdio::jread(jchar * const apInp, const size_t aiLen) {
    return jfread(apInp, sizeof(jchar), aiLen, mpFil) ;
} ;

/**
* @brief Get underlying file descriptor.
*/
int JFileAheadStdio::get_fd() const {
    return fileno(mpFil) ;
} ;

} /* namespace */
