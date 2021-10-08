/*
 * JOutRgn.cpp
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

#include "JOutRgn.h"

namespace JojoDiff {

/**
*@brief Regrouped ascii output function for visualisation
*/
JOutRgn::JOutRgn( FILE *apFilOut ) : mpFilOut(apFilOut){
}

JOutRgn::~JOutRgn() {
}

bool JOutRgn::put (
  int   aiOpr,
  off_t azLen,
  int   aiOrg,
  int   aiNew,
  off_t azPosOrg,
  off_t azPosNew
)
{ static int   siOprCur=ESC ;
  static off_t szOprCnt ;

  /* write output when operation code changes */
  if (aiOpr != siOprCur) {
    // output the old operator
    switch (siOprCur) {
      case (MOD) :
        // a MOD sequence is only needed after an INS sequence
        if ( siOprCur == INS ) {
            gzOutBytCtl+=2 ;
        }
        gzOutBytDta+=szOprCnt ;
        fprintf(mpFilOut, P8zd " " P8zd " MOD %" PRIzd "\n", azPosOrg - szOprCnt, azPosNew - szOprCnt, szOprCnt) ;
        break;

      case (INS) :
        gzOutBytCtl+=2 ;
        gzOutBytDta+=szOprCnt ;
        fprintf(mpFilOut, P8zd " " P8zd " INS %" PRIzd "\n", azPosOrg, azPosNew - szOprCnt, szOprCnt) ;
        break;

      case (DEL) :
        gzOutBytCtl+=2+ufPutLen(szOprCnt);
        gzOutBytDel+=szOprCnt;
        fprintf(mpFilOut, P8zd " " P8zd " DEL %" PRIzd "\n", azPosOrg - szOprCnt, azPosNew, szOprCnt);
        break;

      case (BKT) :
        gzOutBytCtl+=2+ufPutLen(szOprCnt);
        gzOutBytBkt+=szOprCnt;
        fprintf(mpFilOut, P8zd " " P8zd " BKT %" PRIzd "\n", azPosOrg + szOprCnt, azPosNew, szOprCnt);
        break;

      case (EQL) :
        gzOutBytCtl+=2+ufPutLen(szOprCnt);
        gzOutBytEql+=szOprCnt ;
        fprintf(mpFilOut, P8zd " " P8zd " EQL %" PRIzd "\n", azPosOrg - szOprCnt, azPosNew - szOprCnt, szOprCnt);
        break;
    }

    // switch to the new operator
    siOprCur = aiOpr;
    szOprCnt = 0;
  }

  /* accumulate operation codes */
  switch (aiOpr) {
	case (INS):
	case (MOD):
		if (aiNew == ESC)
			gzOutBytEsc++;
		// continue !
	case (DEL):
	case (BKT):
	case (EQL):
		szOprCnt += azLen;
		break;
	}
  return true ; // we never need details
} /* ufOutBytRgn */

/* ---------------------------------------------------------------
 * ufPutLen returns a length as follows
 * byte1  following      formula              if number is
 * -----  ---------      -------------------- --------------------
 * 0-251                 1-252                between 1 and 252
 * 252    x              253 + x              between 253 and 508
 * 253    xx             253 + 256 + xx       a 16-bit number
 * 254    xxxx           253 + 256 + xxxx     a 32-bit number
 * 255    xxxxxxxx       253 + 256 + xxxxxxxx a 64-bit number
 * ---------------------------------------------------------------*/
int JOutRgn::ufPutLen ( off_t azLen  )
{ if (azLen <= 252) {
    return 1;
  } else if (azLen <= 508) {
    return 2;
  } else if (azLen <= 0xffff) {
    return 3;
#ifdef JDIFF_LARGEFILE
  } else if (azLen <= 0xffffffff) {
#else
  } else {
#endif
    return 4;
  }
#ifdef JDIFF_LARGEFILE
  else {
    return 8;
  }
#endif
}

} /* namespace */
