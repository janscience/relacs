/*
  patchclampprojects/phaseresettingcurve.h
  Measures phase-resetting curves of spiking neurons.

  RELACS - Relaxed ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2010 Jan Benda <benda@bio.lmu.de>

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

#ifndef _RELACS_PATCHCLAMP_PHASERESETTINGCURVE_H_
#define _RELACS_PATCHCLAMP_PHASERESETTINGCURVE_H_ 1

#include <relacs/plot.h>
#include <relacs/repro.h>
#include <relacs/ephys/traces.h>
using namespace relacs;

namespace patchclamp {


/*!
\class PhaseResettingCurve
\brief [RePro] Measures phase-resetting curves of spiking neurons.
\author Jan Benda
\version 1.0 (Nov 25, 2010)
\par Screenshot
\image html phaseresettingcurve.png

\par Options
- \c dcamplitudesrc=DC: Set dc-current to (\c string)
- \c dcamplitude=0nA: Amplitude of dc-current (\c number)
- \c amplitude=0.1nA: Test-pulse amplitude (\c number)
- \c duration=1ms: Duration of test-pulse (\c number)
- \c nperiods=5: Number of ISIs between test-pulses (\c integer)
- \c repeats=100: Number of test-pulses (\c integer)
- \c rateduration=1000ms: Time for initial estimate of firing rate (\c number)
- \c averageisis=10test-pulses: Average ISI over (\c integer)
*/


class PhaseResettingCurve : public RePro, public ephys::Traces
{
  Q_OBJECT

public:

  PhaseResettingCurve( void );
  virtual void config( void );
  virtual int main( void );


protected:

  void openTraceFile( ofstream &tf, TableKey &tracekey, const Options &header );
  void saveTrace( ofstream &tf, TableKey &tracekey, int index,
		  const SampleDataF &voltage, const SampleDataF &current,
		  double T, double t, double dt, double p, double dp );
  void saveData( const Options &header, const ArrayD &periods,
		 const ArrayD &meanperiods, const ArrayD &perturbedperiods,
		 const MapD &prctimes, const MapD &prcphases );
  void savePRC( const Options &header, const SampleDataD &medianprc );

  Plot P;
  string VUnit;
  string IUnit;
  double IInFac;
  double PrevDCAmplitude;

};


}; /* namespace patchclamp */

#endif /* ! _RELACS_PATCHCLAMP_PHASERESETTINGCURVE_H_ */
