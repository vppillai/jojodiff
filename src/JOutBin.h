/*
 * JOutBin.h
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

#ifndef JOUTBIN_H_
#define JOUTBIN_H_

#include <stdio.h>
#include "JOut.h"

#define MINEQL 2    // start EQL-sequence on 3'rd byte

namespace JojoDiff {

class JOutBin: public JOut {
    JOutBin(JOutBin const&) = delete;
    JOutBin& operator=(JOutBin const&) = delete;

public:
    JOutBin(FILE *apFilOut );
    virtual ~JOutBin();

    virtual bool put (
      int   aiOpr,
      off_t azLen,
      int   aiOrg,
      int   aiNew,
      off_t azPosOrg,
      off_t azPosNew
    );

private:
    FILE *mpFilOut ;        /**< output file */

    int   miOprCur ;        /**< current operand: INS, MOD, EQL or DEL. */
    off_t mzEqlCnt ;        /**< number of pending equal bytes */
    int   miEqlBuf[MINEQL]; /**< first four equal bytes */
    int   mbOutEsc;         /**< Pending escape character in data stream  ?*/

    /**@brief Output one byte of data */
    void ufPutByt ( int aiByt ) ;

    /**@brief Output an operator sequence */
    void ufPutOpr ( int aiOpr ) ;

    /**@brief Output an operator offset */
    void ufPutLen ( off_t azLen ) ;

};

}

#endif /* JOUTBIN_H_ */
