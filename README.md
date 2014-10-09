# Quick Start
 
# Disclaimer and Licensing
 
ConcurrentPsi is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
ConcurrentPsi is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with ConcurrentPsi. If not, see <http://www.gnu.org/licenses/>.
The full software license for ConcurrentPsi version 1.0.0 
can be found in
file COPYING. 

# Please cite this work

ConcurrentPsi is a free and open source implementation of the 
DCA algorithm for models of strongly correlated electrons. 
The full software license for ConcurrentPsi version 1.0.0 
can be found in
file COPYING. 
You are welcomed to use it and publish data 
obtained with ConcurrentPsi. If you do, please cite this
work. Explain How To Cite This Work. FIXME. TBW.


# Hash of the latest commit 

Hash of the latest commit is also posted at
https://web.ornl.gov/~gz1/hashes.html

# Building and Running ConcurrentPsi

## Required Software

* GNU C++
* The LAPACK and BLAS libraries
* PsimagLite (see below)

## Optional Software

* make or gmake (only needed to use the Makefile)
* perl (may be needed to run some auxiliary script)
* doxygen (to build the manual) 

## Quick Start

1. Use your distribution repository tool to install gcc with support for C++,
the LAPACK and BLAS libraries, the gsl library, make, perl, doxygen and git 
if you don't have them.

2. Issue

    cd someDirectory/

    git clone https://github.com/g1257/PsimagLite.git

    git clone https://github.com/g1257/ConcurrentPsi.git

3. Compile PsimagLite

    cd PsimagLite/lib/

    make -f Makefile.sample

    cd ../../

4. Now issue

    cd ConcurrentPsi/examples/
   
   perl make.pl 

   make

5. You can run it with

   ./test1 totalOuter totalInner nPthreads 

