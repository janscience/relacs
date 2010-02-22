/*
  patchclampprojects/findthreshold.cc
  Finds the current threshold.

  RELACS - Relaxed ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2009 Jan Benda <j.benda@biologie.hu-berlin.de>

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

#include <fstream>
#include <relacs/tablekey.h>
#include <relacs/patchclampprojects/findthreshold.h>
using namespace relacs;

namespace patchclampprojects {


FindThreshold::FindThreshold( void )
  : RePro( "FindThreshold", "FindThreshold", "patchclampprojects",
	   "Jan Benda", "1.0", "Feb 08, 2010" ),
    P( this ),
    VUnit( "mV" ),
    IUnit( "nA" ),
    IInFac( 1.0 )
{
  // add some options:
  addLabel( "Traces" );
  addSelection( "involtage", "Input voltage trace", "V-1" );
  addSelection( "incurrent", "Input current trace", "Current-1" );
  addSelection( "outcurrent", "Output trace", "Current-1" );
  addLabel( "Timing" );
  addSelection( "durationsel", "Set duration of stimulus", "in milliseconds|as multiples of membrane time constant" );
  addNumber( "duration", "Duration of stimulus", 0.1, 0.0, 1000.0, 0.001, "sec", "ms" ).setActivation( "durationsel", "in milliseconds" );
  addNumber( "durationfac", "Duration of stimulus", 1.0, 0.0, 1000.0, 0.1, "tau_m" ).setActivation( "durationsel", "as multiples of membrane time constant" );
  addNumber( "searchpause", "Duration of pause between outputs during search", 0.5, 0.0, 1000.0, 0.01, "sec", "ms" );
  addNumber( "pause", "Duration of pause between outputs", 1.0, 0.0, 1000.0, 0.01, "sec", "ms" );
  addNumber( "savetime", "Length of trace to be saved and analyzed", 0.5, 0.0, 1000.0, 0.01, "sec", "ms" );
  addNumber( "skiptime", "Initial time skipped from spike-count analysis", 0.0, 0.0, 1000.0, 0.01, "sec", "ms" );
  addInteger( "repeats", "Repetitions of stimulus", 10, 0, 10000, 1 );
  addLabel( "Stimulus amplitudes" );
  addSelection( "amplitudesrc", "Use initial amplitude from", "custom|DC|threshold" );
  addNumber( "startamplitude", "Initial amplitude of current stimulus", 0.0, 0.0, 1000.0, 0.01 ).setActivation( "amplitudesrc", "custom" );
  addNumber( "startamplitudestep", "Initial size of amplitude steps used for searching threshold", 0.1, 0.0, 1000.0, 0.001 );
  addNumber( "amplitudestep", "Final size of amplitude steps used for oscillating around threshold", 0.01, 0.0, 1000.0, 0.001 );
  addNumber( "minspikecount", "Minimum required spike count for each trial", 1.0, 0.0, 10000.0, 1.0 );
  addBoolean( "resetcurrent", "Reset current to zero after each stimulus", false );
  addTypeStyle( OptWidget::Bold, Parameter::Label );

  // plot:
  P.lock();
  P.setXLabel( "Time [ms]" );
  P.unlock();
}


void FindThreshold::config( void )
{
  setText( "involtage", spikeTraceNames() );
  setToDefault( "involtage" );
  setText( "incurrent", currentTraceNames() );
  setToDefault( "incurrent" );
  setText( "outcurrent", currentOutputNames() );
  setToDefault( "outcurrent" );
}


void FindThreshold::notify( void )
{
  int involtage = index( "involtage" );
  if ( involtage >= 0 && SpikeTrace[involtage] >= 0 ) {
    VUnit = trace( SpikeTrace[involtage] ).unit();
  }

  int outcurrent = index( "outcurrent" );
  if ( outcurrent >= 0 && CurrentOutput[outcurrent] >= 0 ) {
    IUnit = outTrace( CurrentOutput[outcurrent] ).unit();
    setUnit( "startamplitude", IUnit );
    setUnit( "startamplitudestep", IUnit );
    setUnit( "amplitudestep", IUnit );
  }

  int incurrent = index( "incurrent" );
  if ( incurrent >= 0 && CurrentTrace[incurrent] >= 0 ) {
    string iinunit = trace( CurrentTrace[incurrent] ).unit();
    IInFac = Parameter::changeUnit( 1.0, iinunit, IUnit );
  }
}


int FindThreshold::main( void )
{
  int involtage = index( "involtage" );
  int incurrent = traceIndex( text( "incurrent", 0 ) );
  int outcurrent = outTraceIndex( text( "outcurrent", 0 ) );
  int durationsel = index( "durationsel" );
  double duration = number( "duration" );
  double durationfac = number( "durationfac" );
  double searchpause = number( "searchpause" );
  double measurepause = number( "pause" );
  double savetime = number( "savetime" );
  double skiptime = number( "skiptime" );
  int repeats = integer( "repeats" );
  int amplitudesrc = index( "amplitudesrc" );
  double amplitude = number( "startamplitude" );
  double amplitudestep = number( "startamplitudestep" );
  double finalamplitudestep = number( "amplitudestep" );
  double minspikecount = number( "minspikecount" );
  double orgdcamplitude = stimulusData().number( outTraceName( outcurrent ) );
  bool resetcurrent = boolean( "resetcurrent" );
  if ( amplitudesrc == 1 )
    amplitude = orgdcamplitude;
  else if ( amplitudesrc == 2 )
    amplitude = metaData( "Cell" ).number( "ithreshss" );

  if ( amplitudestep < finalamplitudestep ) {
    warning( "startamplitudestep must be larger than amplitudestep!" );
    return Failed;
  }

  if ( involtage < 0 || SpikeTrace[ involtage ] < 0 || SpikeEvents[ involtage ] < 0 ) {
    warning( "Invalid input voltage trace or missing input spikes!" );
    return Failed;
  }
  if ( outcurrent < 0 ) {
    warning( "Invalid output current trace!" );
    return Failed;
  }
  double membranetau = metaData( "Cell" ).number( "taum" );
  if ( durationsel == 1 ) {
    if ( membranetau <= 0.0 ) {
      warning( "Membrane time constant was not measured yet!" );
      return Failed;
    }
    duration = durationfac*membranetau;
  }
  if ( skiptime > duration || skiptime > savetime ) {
    warning( "skiptime too long!" );
    return Failed;
  }
  double pause = searchpause;

  double samplerate = trace( SpikeTrace[involtage] ).sampleRate();

  // don't print repro message:
  noMessage();

  // init:
  bool record = false;
  bool search = true;
  DoneState state = Completed;
  Results.clear();
  Amplitudes.clear();
  Amplitudes.reserve( 100 );
  Spikes.clear();
  Spikes.reserve( repeats > 0 ? repeats : 100 );
  SpikeCount = 0;
  TrialCount = 0;
  TableKey tracekey;
  ofstream tf;

  // header:
  Header.clear();
  Header.addInteger( "index", completeRuns() );
  Header.addInteger( "ReProIndex", reproCount() );
  Header.addNumber( "ReProTime", "s", "%0.3f", reproStartTime() );
  Header.addNumber( "duration", "ms", "%0.1f", 1000.0*duration );

  // plot trace:
  plotToggle( true, true, 0.5*duration+savetime, 0.5*duration );

  // plot:
  P.lock();
  P.setXRange( 0.0, 1000.0*savetime );
  P.setYLabel( trace( SpikeTrace[involtage] ).ident() + " [" + VUnit + "]" );
  P.unlock();

  // signal:
  OutData signal( duration, 1.0/samplerate );
  signal.setTrace( outcurrent );

  // write stimulus:
  sleep( pause );
  for ( int count=1; softStop() == 0; count++ ) {

    timeStamp();

    Str s;
    if ( ! record )
      s = "<b>Search threshold: </b>";
    else
      s = "<b>Measure threshold: </b>";
    s += "Amplitude <b>" + Str( amplitude ) + " " + IUnit +"</b>";
    if ( record )
      s += ",  Loop <b>" + Str( count ) + "</b>";
    message( s );

    // signal:
    signal = amplitude;
    signal.setIdent( "const ampl=" + Str( amplitude ) + IUnit );
    if ( resetcurrent )
      signal.back() = 0.0;
    write( signal );
    if ( signal.failed() ) {
      warning( signal.errorText() );
      if ( ! record || count <= 1 )
	state = Failed;
      break;
    }

    // sleep:
    sleep( duration + 0.5*pause );
    if ( interrupt() ) {
      if ( ! record || count <= 1 )
	state = Aborted;
      break;
    }

    // analyze, plot, and save:
    analyze( involtage, incurrent, amplitude, duration, savetime, skiptime );
    if ( record )
      saveTrace( tf, tracekey, count-1 );
    plot( record, duration );

    // change stimulus amplitude:
    if ( Results.back().SpikeCount >= minspikecount )
      amplitude -= amplitudestep;
    else
      amplitude += amplitudestep;

    // switch modes:
    if ( ! record ) {
      // find threshold:
      if ( Results.size() > 1 &&
	   ( ( Results[Results.size()-2].SpikeCount < minspikecount &&
	       Results[Results.size()-1].SpikeCount >= minspikecount ) ||
	     ( Results[Results.size()-2].SpikeCount >= minspikecount &&
	       Results[Results.size()-1].SpikeCount < minspikecount ) ) ) {
	if ( amplitudestep <= finalamplitudestep ) {
	  amplitudestep = finalamplitudestep;
	  pause = measurepause;
	  count = 0;
	  Results.clear();
	  SpikeCount = 0;
	  TrialCount = 0;
	  Amplitudes.clear();
	  Amplitudes.reserve( repeats > 0 ? repeats : 1000 );
	  Latencies.clear();
	  Latencies.reserve( repeats > 0 ? repeats : 1000 );
	  Spikes.clear();
	  Spikes.reserve( repeats > 0 ? repeats : 100 );
	  if ( search )
	    search = false;
	  else {
	    record = true;
	    openFiles( tf, tracekey, incurrent );
	  }
	}
	else
	  amplitudestep *= 0.5;
      }
    }
    TrialCount = count;

    sleepOn( duration + pause );

    if ( record && repeats > 0 && count >= repeats )
      break;

  }

  tf << '\n';
  if ( record && TrialCount > 0 )
    save();
  Results.clear();
  Latencies.clear();
  Amplitudes.clear();
  Spikes.clear();
  return state;
}


void FindThreshold::analyze( int involtage, int incurrent,
			     double amplitude, double duration,
			     double savetime, double skiptime )
{
  if ( Results.size() >= 20 )
    Results.pop_front();

  if ( incurrent >= 0 ) {
    Results.push_back( Data( savetime, trace( SpikeTrace[involtage] ),
			     trace( incurrent ) ) );
    Results.back().Current *= IInFac;
    Results.back().Amplitude = Results.back().Current.mean( 0.5*duration, duration );
  }
  else {
    Results.push_back( Data( savetime, trace( SpikeTrace[involtage] ) ) );
    Results.back().Amplitude = amplitude;
  }
  double sigtime = events( SpikeEvents[involtage] ).signalTime();
  events( SpikeEvents[involtage] ).copy( sigtime, sigtime + savetime,
					 sigtime, Results.back().Spikes );
  Results.back().SpikeCount = Results.back().Spikes.count( skiptime, savetime );

  if ( Results.back().SpikeCount > 0 ) {
    SpikeCount++;
    Latencies.push( Results.back().Spikes.latency( 0.0 ) );
  }
  Amplitudes.push( Results.back().Amplitude );
  Spikes.push( Results.back().Spikes );
}


void FindThreshold::openFiles( ofstream &tf, TableKey &tracekey,
			       int incurrent )
{
  tracekey.addNumber( "t", "ms", "%7.2f" );
  tracekey.addNumber( "V", VUnit, "%6.1f" );
  if ( incurrent >= 0 )
    tracekey.addNumber( "I", IUnit, "%6.1f" );
  if ( completeRuns() <= 0 )
    tf.open( addPath( "findthreshold-traces.dat" ).c_str() );
  else
    tf.open( addPath( "findthreshold-traces.dat" ).c_str(),
             ofstream::out | ofstream::app );
  Header.save( tf, "# " );
  tf << "# status:\n";
  stimulusData().save( tf, "#   " );
  tf << "# settings:\n";
  settings().save( tf, "#   " );
  tf << '\n';
  tracekey.saveKey( tf, true, false );
  tf << '\n';
}


void FindThreshold::saveTrace( ofstream &tf, TableKey &tracekey, int index )
{
  tf << "#       index: " << Str( index ) << '\n';
  tf << "#   amplitude: " << Str( Results.back().Amplitude ) << IUnit << '\n';
  tf << "# spike count: " << Str( Results.back().SpikeCount ) << '\n';
  if ( ! Results.back().Current.empty() ) {
    for ( int k=0; k<Results.back().Voltage.size(); k++ ) {
      tracekey.save( tf, 1000.0*Results.back().Voltage.pos( k ), 0 );
      tracekey.save( tf, Results.back().Voltage[k] );
      tracekey.save( tf, Results.back().Current[k] );
      tf << '\n';
    }
  }
  else {
    for ( int k=0; k<Results.back().Voltage.size(); k++ ) {
      tracekey.save( tf, 1000.0*Results.back().Voltage.pos( k ), 0 );
      tracekey.save( tf, Results.back().Voltage[k] );
      tf << '\n';
    }
  }
  tf << '\n';
}


void FindThreshold::save( void )
{
  double asd = 0.0;
  double am = Amplitudes.mean( asd );
  Header.addNumber( "amplitude", IUnit, "%7.3f", am );
  Header.addNumber( "amplitude s.d.", IUnit, "%7.3f", asd );
  Header.addNumber( "trials", "1", "%6.0f", (double)TrialCount );
  Header.addNumber( "spikes", "1", "%6.0f", (double)SpikeCount );
  Header.addNumber( "prob", "%", "%5.1f", 100.0*(double)SpikeCount/(double)TrialCount );
  double lsd = 0.0;
  double lm = Latencies.mean( lsd );
  Header.addNumber( "latency", "ms", "%6.2f", 1000.0*lm );
  Header.addNumber( "latency s.d.", "ms", "%6.2f", 1000.0*lsd );

  saveSpikes();
  saveData();
}


void FindThreshold::saveSpikes( void )
{
  ofstream sf;
  if ( completeRuns() <= 0 )
    sf.open( addPath( "findthreshold-spikes.dat" ).c_str() );
  else
    sf.open( addPath( "findthreshold-spikes.dat" ).c_str(),
             ofstream::out | ofstream::app );

  Header.save( sf, "# " );
  sf << "# status:\n";
  stimulusData().save( sf, "#   " );
  sf << "# settings:\n";
  settings().save( sf, "#   " );
  sf << '\n';

  TableKey spikekey;
  spikekey.addNumber( "t", "ms", "%7.2f" );
  spikekey.saveKey( sf, true, false );
  sf << '\n';

  for ( int k=0; k<Spikes.size(); k++ ) {
    sf << "#       index: " << Str( k ) << '\n';
    sf << "#   amplitude: " << Str( Results.back().Amplitude ) << IUnit << '\n';
    sf << "# spike count: " << Str( Results.back().SpikeCount ) << '\n';
    Spikes[k].saveText( sf, 1000.0, 7, 2, 'f', "-0" );
    sf << '\n';
  }

  sf << '\n';
}


void FindThreshold::saveData( void )
{
  TableKey datakey;
  datakey.addLabel( "Data" );
  datakey.addNumber( "duration", "ms", "%6.1f", Header.number( "duration" ) );
  double asd = 0.0;
  double am = Amplitudes.mean( asd );
  datakey.addNumber( "amplitude", IUnit, "%7.3f", am );
  datakey.addNumber( "s.d.", IUnit, "%7.3f", asd );
  datakey.addNumber( "trials", "1", "%6.0f", (double)TrialCount );
  datakey.addNumber( "spikes", "1", "%6.0f", (double)SpikeCount );
  datakey.addNumber( "prob", "%", "%5.1f", 100.0*(double)SpikeCount/(double)TrialCount );
  double lsd = 0.0;
  double lm = Latencies.mean( lsd );
  datakey.addNumber( "latency", "ms", "%6.2f", 1000.0*lm );
  datakey.addNumber( "s.d.", "ms", "%6.2f", 1000.0*lsd );
  datakey.addLabel( "Traces" );
  datakey.add( stimulusData() );

  ofstream df;
  if ( completeRuns() <= 0 ) {
    df.open( addPath( "findthreshold-data.dat" ).c_str() );
    datakey.saveKey( df );
  }
  else
    df.open( addPath( "findthreshold-data.dat" ).c_str(),
             ofstream::out | ofstream::app );

  datakey.saveData( df );

  metaData( "Cell" ).setNumber( "ithreshss", am );
}


void FindThreshold::plot( bool record, double duration )
{
  P.lock();
  P.clear();
  double am = Amplitudes.mean();
  double lsd = 0.0;
  double lm = Latencies.mean( lsd );
  if ( record )
    P.setTitle( "p=" + Str( 100.0*(double)SpikeCount/(double)TrialCount, 0, 0, 'f' ) +
		"%,  latency=(" + Str( 1000.0*lm, 0, 0, 'f' ) +
		"+/-" + Str( 1000.0*lsd, 0, 0, 'f' ) +
		") ms, amplitude=" + Str( am, 0, 2, 'f' ) + " " + IUnit );
  else
    P.setTitle( "" );
  P.plotVLine( 0, Plot::White, 2 );
  P.plotVLine( 1000.0*duration, Plot::White, 2 );
  for ( unsigned int k=0; k<Results.size()-1; k++ ) {
    SampleDataD vtrace = Results[k].Voltage;
    vtrace += 10.0*(Results.size() - k - 1);
    P.plot( vtrace, 1000.0, Plot::Orange, 2, Plot::Solid );
  }
  if ( ! Results.empty() )
    P.plot( Results.back().Voltage, 1000.0, Plot::Yellow, 4, Plot::Solid );
  P.unlock();
  P.draw();
}


FindThreshold::Data::Data( double savetime, const InData &voltage,
			   const InData &current )
  : Spikes( 10 )
{
  Amplitude = 0.0;
  Voltage.resize( 0.0, savetime, voltage.stepsize(), 0.0 );
  voltage.copy( voltage.signalTime(), Voltage );
  Current.resize( 0.0, savetime, current.stepsize(), 0.0 );
  current.copy( current.signalTime(), Current );
  SpikeCount = 0;
}


FindThreshold::Data::Data( double savetime, const InData &voltage )
  : Spikes( 10 )
{
  Amplitude = 0.0;
  Voltage.resize( 0.0, savetime, voltage.stepsize(), 0.0 );
  voltage.copy( voltage.signalTime(), Voltage );
  Current.clear();
  SpikeCount = 0;
}


addRePro( FindThreshold );

}; /* namespace patchclampprojects */

#include "moc_findthreshold.cc"
