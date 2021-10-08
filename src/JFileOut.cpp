/*
 * JFileOut.h
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
#include "JFileOut.h"

namespace JojoDiff {

/**
* @brief JojoDiff's Output File abstraction
*/
JFileOut::JFileOut( FILE * const apFil):mpFil(apFil)
{
    //ctor
}

int JFileOut::copyfrom( JFile &apFilInp, off_t azPos, off_t azLen){
    jchar *lpBuf ;
    off_t lzLen ;

    // First try buffered copying
    lpBuf = apFilInp.getbuf(azPos, lzLen);
    if (lpBuf != null ){
        while (azLen > 0) {
            if (lpBuf == null ){
                fprintf(stderr, "Error reading source file.\n");
                return (EXI_RED);
            }
            if (lzLen > azLen)
                lzLen = azLen ;
            if (fwrite(lpBuf, sizeof(jchar), lzLen, mpFil) != (size_t) lzLen) {
                fprintf(stderr, "Error writing output file.\n");
                return (EXI_WRI);
            }
            azLen -= lzLen;
            azPos += lzLen;
            if (azLen > 0)
                lpBuf = apFilInp.getbuf(azPos, lzLen);
        }
    } else {
        // Copy character by character
        int lcVal ;
        for (; azLen > 0; azLen --, azPos ++){
            lcVal = apFilInp.get(azPos);
            if (lcVal <= EOF)
                break ;
            if (putc(lcVal) < 0) {
                fprintf(stderr, "Error writing output file.\n");
                return (EXI_WRI);
            }
            lcVal = apFilInp.get() ;
        }
        if (azLen > 0){
            fprintf(stderr, "Error reading source file.\n");
            return (EXI_RED);
        }
    }
    return (EXI_OK);
} /* copyfrom */

/**
* @brief    Write a byte to the output.
* @param    aiDta   data to write
* @return   EOF on error
*/
int JFileOut::putc(const int aiDta){
    return fputc(aiDta, mpFil) ;
} /* putc */



} /* namespace */
