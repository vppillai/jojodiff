/*
 * JDefs.h
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

namespace JojoDiff {

/* List of primes we select from when size is specified on commandline
const int giPme[24] = { 2147483647, 1073741789, 536870909,  268435399,
                         134217689,   67108859,  33554393,   16777213,
                           8388593,    4194301,   2097143,    1048573,
                            524287,     262139,    131071,      65521,
                             32749,      16381,      8191,       4093,
                              2039,       1021,       509,        251} ;*/

/**
* @brief   Check if number is a prime number.
* @param   number     Number to check
* @return  true = a prime, false = not a prime
*/
bool isPrime(int number){
    if(number < 2) return 0;
    if(number == 2) return 1;
    if(number % 2 == 0) return 0;
    for(int  i=3; number/i >= i; i+=2){
        if(number % i == 0 ) return false;
    }
    return true;
}

/**
* @brief Get highest lower prime.
*
* @param    aiNum   number to get a prime for
* @return   > 0     largest prime lower than aiNum
*/
int getLowerPrime(int aiNum){
    switch (aiNum){
        case 1024: return 1021 ;
        case  32 * 1024 * 1024  : return 33554393 ;
        case  16 * 1024 * 1024  : return 16777213 ;
        case   8 * 1024 * 1024   : return 8388593 ;
        case 128 * 1024 * 1024 : return 134217689 ;
        case 512 * 1024 * 1024 : return 536870909 ;
        default:
            for (; aiNum > 0; aiNum --)
                if (isPrime(aiNum))
                    return aiNum ;
    }
    return aiNum;
}

} /* namespace JojoDiff */
