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

#ifndef JFILEOUT_H
#define JFILEOUT_H

#include <stdio.h>
#include "JDefs.h"
#include "JFile.h"

namespace JojoDiff {

/**
* @brief JojoDiff's Output File abstraction.
*/

class JFileOut
{
    JFileOut(JFileOut const&) = delete;
    JFileOut& operator=(JFileOut const&) = delete;

    public:
        virtual ~JFileOut(){};

        /**
        * @brief    Create JFileOut on a stdio file.
        * @param    apFile  Stdio file, opened and ready for writing
        */
        JFileOut(FILE * const apFil) ;

        /**
        * @brief    Write a byte to the output.
        * @param    aiDta   data to write
        * @return   EOF on error
        */
        virtual int putc(const int aiDta) ;

        /**
        * @brief    Copy a series of bytes from input to output.
        * @param    apFilInp    Input file
        * @param    azPos       Position to copy from
        * @param    azLen       Number of bytes to copy
        * @return   <> 0 = error
        */
        virtual int copyfrom(JFile &apFilInp, off_t azPos, off_t azLen) ;

    protected:

    private:
        FILE * const mpFil ;   /* File to read from */
};
} /* namespace */
#endif // JFILEOUT_H
