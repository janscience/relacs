/*
  patchclamp/bridgetest.h
  Short current pulses for testing the bridge

  RELACS - Relaxed ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2012 Jan Benda <benda@bio.lmu.de>

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

#ifndef _RELACS_PATCHCLAMP_BRIDGETEST_H_
#define _RELACS_PATCHCLAMP_BRIDGETEST_H_ 1

#include <relacs/plot.h>
#include <relacs/repro.h>
#include <relacs/ephys/traces.h>
using namespace relacs;

namespace patchclamp {


/*!
\class BridgeTest
\brief [RePro] Short current pulses for testing the bridge
\author Jan Benda
\version 1.0 (Oct 05, 2012)
\par Options
- \c amplitude=1nA: Amplitude of output signal (\c number)
- \c duration=10ms: Duration of output (\c number)
- \c pause=10ms: Duration of pause bewteen outputs (\c number)
*/


class BridgeTest : public RePro, public ephys::Traces
{
  Q_OBJECT

public:

  BridgeTest( void );
  virtual int main( void );


protected:

  Plot P;

};


}; /* namespace patchclamp */

#endif /* ! _RELACS_PATCHCLAMP_BRIDGETEST_H_ */