/*
 * JFile.cpp
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

#include <string.h>
#include "JDefs.h"
#include "JDebug.h"
#include "JFile.h"

namespace JojoDiff {

/**
 * Construct a buffered JFile on an istream.
 */
JFile::JFile(char const * const asJid, const bool abSeq )
: msJid(asJid), mbSeq(abSeq), mzPosEof(MAX_OFF_T)
{
}

void JFile::chkSeq(){
    // Check if file can be seeked by seeking EOF position
    if (! mbSeq) {
        mzPosEof = jeofpos();
        if (mzPosEof < 0) {
            mbSeq = true ;
            mzPosEof = MAX_OFF_T ;
        }
    }
}

} /* namespace */
