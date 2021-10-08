
JojoDiff - diff utility for binary files

Copyright © 2002-2020 Joris Heirbaut

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see https://www.gnu.org/licenses/gpl-3.0.html.

This software is hosted by:<http://sourceforge.net>

1. Purpose

JDIFF differentiates two files so that the second file can be recreated from the first 
by "undiffing". JDIFF aims for the smallest possible diff-file.

For example:
  jdiff -j old-file.tar new-file.tar dif-file.jdf
  jdiff -u old-file.tar dif-file.jdf new-file2.tar
will recreate new-file2.tar identical to new-file.tar.

Possible applications include:
 - incremental backups, or
 - synchronising files between two computers over a slow network.

Download JojoDiff from http://sourceforge.net/projects/jojodiff/.

2. Version and history

The current version of this utility is bèta 0.8.4c from September 2020.
Change history:
  v0.8.5  Oct  2020 Bugfix in Windows binary, improved internal logic.
  v0.8.4  Sept 2020 Improved progress feedback, support for sequential files.
  v0.8    Dec  2011 Conversion to C++.
  v0.7    Nov  2009 Index table collision strategy for uniform distribution.
  v0.6    Apr  2005 Large files support.
  v0.5    Sept 2003 Revision of file-buffering logic.
  v0.4c   Jan  2003 Selection algorithm between multiple matches.
  v0.4b   Oct  2002 Optimize matches.
  v0.4a   Sept 2002 Select "best" of multiple matches.
  v0.3a   July 2002 Copy/insert algorithm.
  v0.2c   July 2002 Bugfix on divide-by-zero in verbose mode.
  v0.2b   July 2002 Bugfix on code-length of 252.
  v0.2a   June 2002 Optimized patch files.
  v0.1    June 2002 Insert/delete algorithm.

3. Installation

On Windows systems:
  Compiled executables are within the "win" directory. 
  You can run them from a command prompt.

On Linux systems:
  Compile the source by running "make" within the "src" directory or
  use the compiled executable within the linux directory.
  Copy the resulting executable to your /usr/local/bin.

4. Usage

jdiff -j [options] source_file destination_file [diff_file]
jdiff -u [options] source_file diff_file [destination_file]

Options:
  -j                      JDiff: create a difference file.
  -u                      Undiff: undiff a difference file.
  -v  --verbose           Verbose: greeting, results and tips.
  -vv                     Extra Verbose: progress info and statistics.
  -vvv                    Ultra Verbose: all info, including help and details.
  -h -hh  --help          Help, additional help (-hh) and exit.
  -l  --listing           Detailed human readable output.
  -r  --regions           Grouped human readable output.
  -c  --console           Write verbose and debug info to stdout.
  -b  --better            Better: use more memory, search more.
  -bb                     Best: even more memory, search more.
  -f  --lazy              Lazy: no unbuffered searching (often slower).
  -ff                     Lazier: no full index table.
  -p  --sequential-source Sequential source (to avoid !) (with - for stdin).
  -q  --sequential-dest   Sequential destination (with - for stdin).
  -s  --stdio             Use stdio files (for testing).
  -a  --search-size       Size (in KB) to search (default=buffer-size).
  -i  --index-size        Size (in MB) for index table (default 64MB).
  -k  --block-size        Block size in bytes for reading (default 8192).
  -m  --buffer-size       Size (in MB) for search buffers (default 2MB).
  -n  --search-min        Minimum number of matches to search (default 2).
  -x  --search-max        Maximum number of matches to search (default 512).

Hint: Do not use jdiff on compressed files. Rather use jdiff first and compress afterwards,
e.g.: jdiff -j old new | gzip >dif.jdf.gz (or 7z with -si)

Principles:
  JDIFF tries to find equal regions between two binary files using a heuristic hash algorithm 
  and outputs the differences between both files. Heuristics are generally used for improving 
  performance and memory usage, at the cost of accuracy. Therefore, this program may not find 
  a minimal set of differences between files.

Notes:
  Options -b, -bb, -f and -ff should be used before other options.
  Accuracy may be improved by increasing the index table size (-i) or the buffer size (-m).
  The index table size is always lowered to the nearest lower prime number.
  Output is sent to standard output if no output file is specified.