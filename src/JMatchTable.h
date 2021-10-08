/*
 * JMatchTable.h
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

#ifndef JMATCHTABLE_H_
#define JMATCHTABLE_H_

#include "JDefs.h"
#include "JFile.h"
#include "JHashPos.h"

namespace JojoDiff {

/* JojoDiff Matching Table: this class allows to build and maintain  a table of matching regions
 * between two files and allows to select the "best" match from the table. */
class JMatchTable {
public:
    JMatchTable(JMatchTable const&) = delete;
    JMatchTable& operator=(JMatchTable const&) = delete;

	/**
	* @brief Construct a matching table for specified hashtable, original and new files.
	*
	* @param  cpHsh     JHashPos Hashtable
	* @param  apFilOrg  Source file
	* @param  apFilNew  Destination file
	* @param  aiSze     Size of the matychtable (in number of elements, one element = 9 words)
	* @param  abCmpAll  Compare all matches, or only those available within the filebuffers
	*/
	JMatchTable(JHashPos const * cpHsh,  JFile  * apFilOrg, JFile  * apFilNew,
             const int aiMchSze, const bool abCmpAll, const int aiAhdMax);

	/* Destructor */
	virtual ~JMatchTable();

	enum eMatchReturn {Error, Full, Enlarged, Invalid, Good, Best, Valid};

	/**
	 * @brief Add given match to the array of matches
	 *
	 * - Add to a colliding or gliding match if possible, or
	 * - Add at the end of the list, or
	 * - Override an old match if posible
	 *
	 * @return FullError, Full, Enlarged, Invalid, GoodMatch, BestMatch, Added
	 */
	eMatchReturn add (
	  off_t const &azFndOrgAdd,      /* match to add               */
	  off_t const &azFndNewAdd,
	  off_t const &azRedNew
	);

	/**
	 * @brief   Get the best (=nearest) optimized and valid match from the array of matches.
	 *
	 * @param   azBseOrg Current reading position in original file
	 * @param   azBseNew Current reading position in new file
	 * @param   azBstOrg    out: best found new position for original file
	 * @param   azBstNew    out: best found new position for new file
	 * @return  false= no solution has been found
     * @return  true = a solution has been found
	 */
	bool getbest (
	  off_t const &azBseOrg,       /* base positions       */
	  off_t const &azBseNew,
	  off_t &azBstOrg,             /* best position found  */
	  off_t &azBstNew
	) ;

    /**
     * @brief Cleanup, check free space and fastcheck best match.
     *
     * @param       azBseOrg    Cleanup all matches before this position
     * @param       azRedNew    Current reading position
     *
     * @return  Full, GoodMatch or Added
     */
    eMatchReturn cleanup ( off_t const azBseOrg, off_t const azRedNew);

    /**
    * @brief Get number of hash repairs (matches repaired by comparing).
    */
    int getHshRpr ();

private:
    /**
    * Matchtable structure
    */
 	typedef struct tMch {
	    struct tMch *ipNxt ;    /**< next element on the pseudo-ordered aging stack     */
	    struct tMch *ipCol ;    /**< next element in collision bucket or free list      */
	    struct tMch *ipGld ;    /**< next element in gliding bucket list                */

	    int iiCnt ;             /**< number of colliding matches (= confirming matches) */
 	    int iiGld ;             /**< gliding match recurrence (0=no, <0=mixed, >0=glide)*/
	    off_t izBeg ;           /**< first found match (new file position)              */
	    off_t izNew ;           /**< last  found match (new file position)              */
	    off_t izOrg ;           /**< last  found match (org file position)              */
	    off_t izDlt ;           /**< delta: izOrg = izNew + izDlt                       */
	    off_t izTst ;           /**< result of last compare                             */
	    int iiCmp ;             /**< result of last compare                             */
	} rMch ;

	/**
	 * Matchtable elements
	 */
	JHashPos const * mpHsh ;    /**< Pointer to the hash table */
	int  const miMchSze ;       /**< Size of the matching table                         */
    int  miMchFre ;             /**< Free index                                         */

	int  miMchPme=0 ;           /**< Size of matching hashtables                        */
	rMch *msMch = null;         /**< table of matches (dynamic array)                   */
	rMch **mpCol = null;        /**< hashtable on izDlt for detecting colliding matches */
	rMch **mpGld = null;        /**< hashtable on azOrg for detecting gliding matches   */
	rMch *mpOld = null;         /**< List of old elements */
	rMch *mpNew = null;         /**< List of new elements */
	rMch *mpLst = null;         /**< Last of new elements */
	rMch *mpBst = null;         /**< Current best element */
	off_t mzBstOrg = 0;         /**< Current best source position */
	off_t mzBstNew = 0;         /**< Current best destin position */
	int   miBstCmp = 0;         /**< Current best (estimated) length */
    off_t mzOld = 0 ;           /**< Limit for being old :-( */

    /**
	 * Context: we need access to the two source files
	 */
	JFile * const mpFilOrg ;    /**< Source file */
	JFile * const mpFilNew ;    /**< Destination file */
	bool const mbCmpAll ;       /**< Compare all matches, even if data not in buffer? */
	int  const miAhdMax ;       /**< Lookahead & lookback range                       */
	int  miRlb=0;               /**< Current reliability range from mpHsh             */

	/**
	* Statistics
	*/
	int miHshRpr;               /**< Number of repaired hash hits (by compare)   */

    /**
    * @brief Evaluate a match
    */
    eMatchReturn isGoodOrBest(
        off_t const azRedNew,       /**< Current read position */
        rMch *lpCur                 /**< Element to evaluate */
    ) ;

    /**
    * @brief Check if given solution is the best one.
    */
    bool isBest(rMch * const lpCur, off_t const azRedNew,
                off_t lzTstOrg, off_t lzTstNew, int liCurCmp) ;

    /**
    * @brief Check if a match can be reused (deleted)
    *
    * Matches are never deleted but instead reused (overwritten) by new matches.
    * The matchtable is "full" when no more matches can be reused.
    * If the matchtable is full, searching must stop, which is bad.
    * Reusing (overwriting) a still usable match however is also bad.
    *
    * A match is considered still usable if may contain information beyond the current best match.
    * This is flawed, because a next best match may be shorter than the current.
    * So reusing valid matches is risky but necessary to maximize the search.
    */
    bool isOld2Reuse(rMch const * const lpCur, off_t const azRedNew);

    /**
    * @brief Check if a match can be skipped (iiCmp==-3)
    *
    * Skipping is done purely for performance reasons and reduces accuracy.
    * We're only skipping when the probability of a valid match is really low.
    * To compensate, skipped matches can be reactivated by a new match from the hashtable.
    */
    bool isOld2Skip(rMch const * const lpCur, off_t const azRedNew) ;

    /**
	 * @brief Verify and optimize matches
	 *
     * Searches at given positions for a run of 8 equal bytes.
     * Searching continues for the given length unless soft-reading is specified
     * and the end-of-buffer is reached.
     *
     * @param   &azPosOrg   in/out  position on first file
     * @param   &azPosNew   in/out  position on second file
     * @param   aiLen       in      number of bytes to compare
     * @param   aiGld       in      gliding match recurrence
     * @param   aiSft       in      1=hard read, 2=soft read
     *
     * @return  0    : no run of equal byes found
     * @return  -1   : EOB reached, no equal bytes found
     * @return  > 4  : number of equal bytes found
	 */
	int check (
	    off_t &rzPosOrg, off_t &rzPosNew,
	    int aiLen = 0,
	    int ibGld = 0,
	    const JFile::eAhead aiSft = JFile::eAhead::HardAhead
    ) const ;

    /**
    * @brief Prepare next reusable old element
    * @return true=found, false=notfound
    */
    bool nextold(off_t const azRedNew);

    /**
    * @brief   Calculate position on original file corresponding to given new file position.
    *
    * @param   rMch     *apCur    Match (in)
    * @param   off_t    azTstOrg  Position on org file (out only)
    * @param   off_t    azTstNew  Position on new file (in/out, adapted if azTstOrg would become negative)
    *
    * @return   true = gliding offsets, false = normal offsets
    */
    bool calcPosOrg(rMch *apCur, off_t &azTstOrg, off_t &azTstNew) const ;

    /**
    * @brief Add element to new list
    */
    void addNew(rMch *lpCur) ;

    /**
    * @brief    Delete element from gliding hashtable
    */
    void delGld(rMch * const apGld) ;

    /**
    * @brief    Delete element from collding hashtable
    */
    void delCol(rMch * const apCol) ;
};

}

#endif /* JMATCHTABLE_H_ */
