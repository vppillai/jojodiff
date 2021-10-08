/*
 * JFileAheadIStream.cpp
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
#include "JFileAheadIStream.h"

namespace JojoDiff {

JFileAheadIStream::JFileAheadIStream(istream &apFil, char const * const asFid,
                          const long alBufSze, const int aiBlkSze, const bool abSeq)
: JFileAhead(asFid, alBufSze, aiBlkSze, abSeq)
, mpFil(apFil)
{
    chkSeq() ;  // Check if file is sequential or not
}

JFileAheadIStream::~JFileAheadIStream()
{
    //dtor
}

/**
* @brief Seek abstraction, for override by subclasses
*
* @param EXI_OK (0) or EXI_SEK in cae of error
*/
int JFileAheadIStream::jseek(const off_t azPos) {
    mpFil.seekg(azPos) ;
    if (mpFil.fail()){
        mpFil.clear();
        return EXI_SEK;
    }
    else {
        return EXI_OK ;
    }
} ;

/**
* @brief Seek EOF abstraction
*
* @param EXI_OK (0) or EXI_SEK in cae of error
*/
off_t JFileAheadIStream::jeofpos() {
    mpFil.seekg(0, mpFil.end) ;
    if (mpFil.fail()){
        mpFil.clear();
        return EXI_SEK;
    }
    else {
        off_t lzEof = mpFil.tellg();
        mpFil.seekg(0, mpFil.beg) ;
        return lzEof ;
    }
} ;

/**
* @brief Read abstraction, for override by subclasses
*
* @param >= 0: number of bytes read
*/
size_t JFileAheadIStream::jread(jchar * const apInp, const size_t aiLen) {
    size_t liDne ;
    mpFil.read((char *)apInp, aiLen) ;
    liDne = mpFil.gcount();
    // Reset EOF state
    if (liDne < aiLen)
      if (mpFil.eof())
          mpFil.clear();

    return liDne ;
} ;
} /* namespace */
