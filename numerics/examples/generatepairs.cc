/*
  generatepairs.cc
  

  RELACS - Relaxed ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2015 Jan Benda <jan.benda@uni-tuebingen.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  
  RELACS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
generates random paired samples for testing paired t-test and Wilcoxon test.
*/

#include <iostream>
#include <relacs/random.h>
using namespace std;
using namespace relacs;


int n = 400;
double diffsigma = 60.0;
double diffmean = 5.0;
double xsigma = 100.0;
double xmean = 200.0;

int main( void )
{
  for ( int k=0; k<n; k++ ) {
    double diff = diffsigma*rnd.gaussian() + diffmean;
    double x = xsigma*rnd.gaussian() + xmean;
    double y = x + diff;
    cout << x << " " << y << '\n';
  }
  return 0;
}
