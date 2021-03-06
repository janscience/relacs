/*
  efish/chirps.h
  Measures responses to chirps.

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

#ifndef _RELACS_EFISH_CHIRPS_H_
#define _RELACS_EFISH_CHIRPS_H_ 1

#include <vector>
#include <relacs/array.h>
#include <relacs/map.h>
#include <relacs/sampledata.h>
#include <relacs/options.h>
#include <relacs/tablekey.h>
#include <relacs/multiplot.h>
#include <relacs/ephys/traces.h>
#include <relacs/efield/traces.h>
#include <relacs/efield/eodtools.h>
#include <relacs/repro.h>
using namespace relacs;

namespace efish {


/*!
\class Chirps
\brief [RePro] Measures responses to chirps.
\author Jan Benda
\version 2.2 (May 19, 2020)

\par Screenshot
\image html chirps.png

\par Options
- \c Chirp parameter
    - \c nchirps=10: Number of chirps (\c integer)
    - \c beatpos=10: Number of beat positions (\c integer)
    - \c beatstart=0.25: Beat position of first chirp (between 0 and 1) (\c number)
    - \c minspace=200ms: Minimum time between chirps (\c number)
    - \c minperiods=1: Minimum number of beat periods between chirps (\c number)
    - \c firstspace=200ms: Minimum time preceeding first chirp (\c number)
    - \c minspace=200ms: Minimum time between chirps (\c number)
    - \c minperiods=1: Minimum number of beat periods preceeding first chirp and between chirps (\c number)
    - \c chirpampl=2%: Amplitude of chirp (\c number)
    - \c chirpsel=generated: Chirp waveform (\c string)
    - \c chirpkurtosis=1: Kurtosis of Gaussian chirp (\c number)
    - \c file=: Chirp-waveform file (\c string)
- \c Beat parameter
    - \c beatsel=Delta f: Stimulus frequency (\c string)
    - \c deltaf=10Hz: Delta f (\c number)
    - \c releodf=1.1: Relative EODf (\c number)
    - \c contrast=20%: Contrast (\c number)
    - \c am=true: Amplitude modulation (\c boolean)
    - \c sinewave=true: Use sine wave (\c boolean)
    - \c playback=false: Playback AM (\c boolean)
    - \c pause=1000ms: Pause between signals (\c number)
    - \c repeats=6: Repeats (\c integer)
- \c Analysis
    - \c phaseestimation=beat: Base estimation of beat phase on (\c string)
    - \c sigma=2ms: Standard deviation of rate smoothing kernel (\c number)
    - \c adjust=true: Adjust input gain? (\c boolean)

\par Files
- \b chirps.dat : General information for each chirp in each stimulus.
- \b chirptraces.dat : Frequency and amplitude of the transdermal EOD for each chirp.
- \b chirpspikes#.dat : The spikes elicited by each chirp of trace #.
- \b chirpallspikes#.dat : The spikes elicited by each stimulus of trace #.
- \b chirpnerveampl.dat : The nerve potential elicited by each chirp.
- \b chirpnervesmoothampl.dat : The smoothed nerve potential elicited by each chirp.
- \b chirpallnerveampl.dat : The nerve potential elicited by each stimulus.
- \b chirpallnervesmoothampl.dat : The smoothed nerve potential elicited by each stimulus.
- \b chirpeodampl.dat : Amplitude of the transdermal EOD for each stimulus.
- \b chirprate.dat : The firing rate for each chirp position.

\par Plots
- \b Rate: Spikes (direct stimulus: red, AM: magenta) and 
  firing rate (direct stimulus: yellow, AM: orange) for each chirp position.
- \b Beat: Amplitude of transdermal EOD for each chirp position (darkgreen). Last stimulus green.

\par Requirements
- Optional: EOD events (\c EODEvents1). Recommended if direct stimulation is used.
- Transdermal EOD recording (\c EODTrace2) and events (\c EODEvents2).
- Recording of the stimulus events (\c SignalEvents1) if direct stimuli are used.
- One or more spike events (\c SpikeEvents[*]) or nerve recordings (\c NerveTrace1).
*/


class Chirps
  : public RePro,
    public ephys::Traces,
    public efield::Traces,
    public efield::EODTools
{
  Q_OBJECT

public:

  Chirps( void );
  virtual int main( void );
  virtual void sessionStarted( void );
  virtual void sessionStopped( bool saved );

  void stop( void );
  void saveChirps( void );
  void saveChirpTraces( void );
  void saveChirpSpikes( int trace );
  void saveChirpNerve( void );
  void saveAmplitude( void );
  void saveSpikes( int trace );
  void saveChirpRate( int trace );
  void saveNerve( void );
  void save( void );

  void plot( void );
  void analyze( void );


private:

  void initMultiPlot( double ampl, double contrast );
  void createEOD( OutData &signal );
  void createPlayback( OutData &signal );
  int createAM( OutData &signal );

  // parameter:
  int ReadCycles;
  int NChirps;
  double MinSpace;
  double MinPeriods;
  double FirstSpace;
  double Pause;
  double ChirpSize;
  double ChirpWidth;
  int ChirpSel;
  double ChirpKurtosis;
  double ChirpDip;
  string ChirpFile;
  int BeatPos;
  double BeatStart;
  double Sigma;
  double DeltaF;
  double BeatF;
  double Contrast;
  int Repeats;
  double SaveWindow;
  bool AM;
  bool SineWave;
  bool Playback;
  bool PhaseOnBeat;

  // variables:
  int Mode;
  MapD ChirpFreqs;
  MapD ChirpAmpls;
  double TrueContrast;
  double Duration;
  double StimulusRate;
  double IntensityGain;
  double FishRate;
  double FishAmplitude;
  double ChirpPhase;
  ArrayD ChirpTimes;
  ArrayD BeatPhases;
  double Intensity;
  int Count;
  int StimulusIndex;
  bool OutWarning;
  string EOD2Unit;

  struct ChirpData
  {
    ChirpData( int i, int m, int tr, double t, 
	       double s, double w, double k, double a, double p,
	       double er, double bf, double bph, int bbin,
	       double bb, double ba, double bp, double bt ) 
      : Index( i ), Mode( m ), Trace( tr ), Time( t ), Size( s ), Width( w ),
	Kurtosis( k ), Amplitude( a ), Phase( p ), EODRate( er ),
	 BeatFreq( bf ), BeatPhase( bph ), BeatBin( bbin ), 
	 BeatBefore( bb ), BeatAfter( ba ), BeatPeak( bp ), BeatTrough( bt ),
	 EODTime(), EODFreq(), EODAmpl(), Spikes(),
      NerveAmplP(), NerveAmplT(), NerveAmplM(), NerveAmplS() {};
    int Index;
    int Mode;
    int Trace;
    double Time;
    double Size;
    double Width;
    double Kurtosis;
    double Amplitude;
    double Phase;
    double EODRate;
    double BeatFreq;
    double BeatPhase;
    int BeatBin;
    double BeatBefore;
    double BeatAfter;
    double BeatPeak;
    double BeatTrough;
    ArrayD EODTime;
    ArrayD EODFreq;
    ArrayD EODAmpl;
    EventData Spikes[MaxTraces];
    MapD NerveAmplP;
    MapD NerveAmplT;
    MapD NerveAmplM;
    SampleDataD NerveAmplS;
  };
  vector < ChirpData > Response;
  long FirstResponse;

  struct RateData
  {
    RateData( void ) : Trials( 0 ), Rate( 0 ), Frequency( 0 ) {};
    RateData( double width, double dt ) : Trials( 0 ),
      Rate( -width, width, dt ), Frequency( -width, width, dt ) {};
    int Trials;
    SampleDataD Rate;
    SampleDataD Frequency;
  };

  EventData Spikes[MaxTraces];
  vector < vector < RateData > > SpikeRate[MaxTraces];
  double MaxRate[MaxTraces];

  vector< vector < SampleDataD > > NerveMeanAmplP;
  vector< vector < SampleDataD > > NerveMeanAmplT;
  vector< vector < SampleDataD > > NerveMeanAmplM;
  vector< vector < SampleDataD > > NerveMeanAmplS;
  MapD NerveAmplP;
  MapD NerveAmplT;
  MapD NerveAmplM;
  SampleDataD NerveAmplS;
  AcceptEOD<InData::const_iterator,InDataTimeIterator> NerveAcceptEOD;

  MapD EODAmplitude;

  Options Header;
  TableKey ChirpKey;
  TableKey ChirpTraceKey;
  TableKey SpikesKey;
  TableKey NerveKey;
  TableKey SmoothKey;
  TableKey AmplKey;

  MultiPlot P;
  int Rows;
  int Cols;

};


}; /* namespace efish */

#endif /* ! _RELACS_EFISH_CHIRPS_H_ */
