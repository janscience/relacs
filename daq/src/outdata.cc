/*
  outdata.cc
  An output signal for a data acquisition board.

  RELACS - Relaxed ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2011 Jan Benda <benda@bio.lmu.de>

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

#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <relacs/random.h>
#include <relacs/outdata.h>
using namespace std;

namespace relacs {


double OutData::DefaultMinSampleInterval = 0.001;


OutData::OutData( void ) 
  : SampleDataF()
{
  construct();
}


OutData::OutData( int n, double stepsize ) 
  : SampleDataF( n, 0.0, stepsize, 0.0 )
{
  construct();
}


OutData::OutData( double duration, double stepsize ) 
  : SampleDataF( 0.0, duration, stepsize, 0.0 )
{
  construct();
}


OutData::OutData( double value ) 
  : SampleDataF( 1, 0.0, 0.01, value )
{
  construct();
}


OutData::OutData( const OutData  &od ) 
  : SampleDataF( od )
{
  Delay = od.Delay;
  StartSource = od.StartSource;
  Priority = od.Priority;
  Continuous = od.Continuous;
  Restart = od.Restart;
  MaxRate = od.MaxRate;
  FixedRate = od.FixedRate;
  Device = od.Device;
  Channel = od.Channel;
  Trace = od.Trace;
  TraceName = od.TraceName;
  Ident = od.Ident;
  Descriptions = od.Descriptions;
  Reglitch = od.Reglitch;
  RequestMinValue = od.RequestMinValue;
  RequestMaxValue = od.RequestMaxValue;
  GainIndex = od.GainIndex;
  GainData = 0;
  Scale = od.Scale;
  Unit = od.Unit;
  WriteTime = od.WriteTime;
  MinVoltage = od.MinVoltage;
  MaxVoltage = od.MaxVoltage;
  SignalDelay = od.SignalDelay;
  Intensity = od.Intensity;
  CarrierFreq = od.CarrierFreq;
  Level = od.Level;
  DeviceIndex = od.DeviceIndex;
  DeviceDelay = od.DeviceDelay;
  DeviceCount = od.DeviceCount;
  setError( od.error() );
  Dummy.clear();
}


OutData::~OutData( void )
{
  if ( GainData != 0 )
    delete [] GainData;
}


void OutData::construct( void )
{
  Delay = 0.0;
  StartSource = 0;
  Priority = false;
  Continuous = false;
  Restart = false;
  MaxRate = -1.0;
  FixedRate = false;
  Device = 0;
  Channel = 0;
  Trace = -1;
  TraceName = "";
  Ident = "";
  Descriptions.clear();
  Reglitch = false;
  RequestMinValue = AutoRange;
  RequestMaxValue = AutoRange;
  GainIndex = 0;
  GainData = 0;
  Scale = 1.0;
  Unit = "V";
  WriteTime = 0.0;
  MinVoltage = -1.0;
  MaxVoltage = +1.0;
  SignalDelay = 0.0;
  Intensity = NoIntensity;
  CarrierFreq = 0.0;
  Level = NoLevel;
  DeviceIndex = 0;
  DeviceDelay = 0;
  DeviceCount = 0;
  clearError();
  Dummy.clear();
}


const OutData &OutData::operator=( const OutData &od )
{
  return assign( od );
}


/* Used by macro OUTDATAOPS1SCALARDEF to generate
   definitions for unary operators of class OutData 
   that take a scalar as argument. 
   \a COPNAME is the operator name (like operator+= ),
   \a COP is the operator (like += ), and
   \a SCALAR is the type of the scalar argument. */
#define OUTDATAOPS1SINGLESCALARDEF( COPNAME, COP, SCALAR ) \
  const OutData &OutData::COPNAME( SCALAR x )		\
  {									\
    iterator iter1 = begin();						\
    iterator end1 = end();						\
    while ( iter1 != end1 ) {						\
      (*iter1) COP static_cast< value_type >( x );			\
      ++iter1;								\
    };									\
    return *this;							\
  }									\


/* Generates definitions for unary operators of class OutData 
   that take scalars as argument.
   \a COPNAME is the operator name (like operator+= ), and
   \a COP is the operator name (like += ). */
#define OUTDATAOPS1SCALARDEF( COPNAME, COP ) \
  OUTDATAOPS1SINGLESCALARDEF( COPNAME, COP, float ) \
    OUTDATAOPS1SINGLESCALARDEF( COPNAME, COP, double ) \
    OUTDATAOPS1SINGLESCALARDEF( COPNAME, COP, long double ) \
    OUTDATAOPS1SINGLESCALARDEF( COPNAME, COP, signed char ) \
    OUTDATAOPS1SINGLESCALARDEF( COPNAME, COP, unsigned char ) \
    OUTDATAOPS1SINGLESCALARDEF( COPNAME, COP, signed int ) \
    OUTDATAOPS1SINGLESCALARDEF( COPNAME, COP, unsigned int ) \
    OUTDATAOPS1SINGLESCALARDEF( COPNAME, COP, signed long ) \
    OUTDATAOPS1SINGLESCALARDEF( COPNAME, COP, unsigned long ) \


OUTDATAOPS1SCALARDEF( operator=, = )

#undef OUTDATAOPS1SINGLESCALARDEF
#undef OUTDATAOPS1SCALARDEF


const OutData &OutData::assign( const OutData &od )
{
  SampleDataF::assign( (SampleDataF&)od );
  Delay = od.Delay;
  StartSource = od.StartSource;
  Priority = od.Priority;
  Continuous = od.Continuous;
  Restart = od.Restart;
  MaxRate = od.MaxRate;
  FixedRate = od.FixedRate;
  Device = od.Device;
  Channel = od.Channel;
  Trace = od.Trace;
  TraceName = od.TraceName;
  Ident = od.Ident;
  Descriptions = od.Descriptions;
  Reglitch = od.Reglitch;
  RequestMinValue = od.RequestMinValue;
  RequestMaxValue = od.RequestMaxValue;
  GainIndex = od.GainIndex;
  GainData = 0;
  Scale = od.Scale;
  Unit = od.Unit;
  WriteTime = od.WriteTime;
  MinVoltage = od.MinVoltage;
  MaxVoltage = od.MaxVoltage;
  SignalDelay = od.SignalDelay;
  Intensity = od.Intensity;
  CarrierFreq = od.CarrierFreq;
  Level = od.Level;
  DeviceIndex = od.DeviceIndex;
  DeviceDelay = od.DeviceDelay;
  DeviceCount = od.DeviceCount;
  setError( od.error() );
  Dummy.clear();
  return *this;
}


const OutData &OutData::copy( OutData &od ) const
{
  SampleDataF::copy( (SampleDataF&)od );
  od.Delay = Delay;
  od.StartSource = StartSource;
  od.Priority = Priority;
  od.Continuous = Continuous;
  od.Restart = Restart;
  od.MaxRate = MaxRate;
  od.FixedRate = FixedRate;
  od.Device = Device;
  od.Channel = Channel;
  od.Trace = Trace;
  od.TraceName = TraceName;
  od.Ident = Ident;
  od.Descriptions = Descriptions;
  od.Reglitch = Reglitch;
  od.RequestMinValue = RequestMinValue;
  od.RequestMaxValue = RequestMaxValue;
  od.GainIndex = GainIndex;
  od.GainData = 0;
  od.Scale = Scale;
  od.Unit = Unit;
  od.WriteTime = WriteTime;
  od.MinVoltage = MinVoltage;
  od.MaxVoltage = MaxVoltage;
  od.SignalDelay = SignalDelay;
  od.Intensity = Intensity;
  od.CarrierFreq = CarrierFreq;
  od.Level = Level;
  od.DeviceIndex = DeviceIndex;
  od.DeviceDelay = DeviceDelay;
  od.DeviceCount = DeviceCount;
  od.setError( error() );
  Dummy.clear();
  return *this;
}


const OutData &OutData::append( float a, int n )
{
  SampleDataF::append( a, n );
  return *this;
}


const OutData &OutData::append( const OutData &od )
{
  SampleDataF::append( (SampleDataF&)od );
  Descriptions.insert(Descriptions.end(), od.Descriptions.begin(), od.Descriptions.end());
  return *this;
}


string OutData::errorMessage( void ) const
{
  if ( success() )
    return "";

  ostringstream ss;
  ss << "\"" << ident()
     << "\", channel " << channel() 
     << " on device " << device()
     << ": " << errorStr();
  return ss.str();
}


double OutData::sampleRate( void ) const
{
  return 1.0/stepsize();
}


void OutData::setSampleRate( double rate )
{
  if ( rate > 0.0 )
    setStepsize( 1.0/rate );
}


double OutData::sampleInterval( void ) const
{
  return stepsize();
}


void OutData::setSampleInterval( double step )
{
  setStepsize( step );
}


double OutData::delay( void ) const 
{
  return Delay;
}


void OutData::setDelay( double delay )
{
  Delay = delay >= 0.0 ? delay : 0.0;
}


int OutData::startSource( void ) const
{
  return StartSource;
}


void OutData::setStartSource( int startsource ) 
{
  StartSource = startsource;
}


bool OutData::priority( void ) const 
{
  return Priority;
}


void OutData::setPriority( const bool priority )
{
  Priority = priority;
}


bool OutData::continuous( void ) const 
{
  return Continuous;
}


void OutData::setContinuous( bool continuous )
{
  Continuous = continuous; 
}


bool OutData::restart( void ) const 
{
  return Restart;
}


void OutData::setRestart( bool restart )
{
  Restart = restart; 
}


int OutData::device( void ) const
{
  return Device;
}


void OutData::setDevice( int device )
{
  Device = device;
}


int OutData::channel( void ) const
{
  return Channel;
}


void OutData::setChannel( int channel )
{
  Channel = channel;
}


void OutData::setChannel( int channel, int device )
{
  Channel = channel;
  Device = device;
}


int OutData::trace( void ) const
{
  return Trace;
}


void OutData::setTrace( int index )
{
  Trace = index;
  TraceName = "";
  Device = -1;
  Channel = -1;
}


string OutData::traceName( void ) const
{
  return TraceName;
}


void OutData::setTraceName( const string &name )
{
  Trace = -1;
  TraceName = name;
  Device = -1;
  Channel = -1;
}


double OutData::signalDelay( void ) const
{
  return SignalDelay;
}


void OutData::setSignalDelay( double sigdelay )
{
  SignalDelay = sigdelay;
}


double OutData::intensity( void ) const
{
  return Intensity;
}


void OutData::setIntensity( double intensity )
{
  Intensity = intensity;
}


void OutData::setNoIntensity( void )
{
  Intensity = NoIntensity;
}


bool OutData::noIntensity( void ) const
{
  return ( Intensity == NoIntensity );
}


void OutData::mute( void )
{
  Intensity = MuteIntensity;
}


double OutData::carrierFreq( void ) const
{
  return CarrierFreq;
}


void OutData::setCarrierFreq( double carrierfreq )
{
  CarrierFreq = carrierfreq;
}


double OutData::level( void ) const
{
  return Level;
}


void OutData::setLevel( double level )
{
  Level = level;
}


void OutData::setNoLevel( void )
{
  Level = NoLevel;
}


bool OutData::noLevel( void ) const
{
  return ( Level == NoLevel );
}


string OutData::ident( void ) const
{
  return Ident;
}


void OutData::setIdent( const string &ident )
{
  Ident = ident;
}


int OutData::descriptions( void ) const
{
  return Descriptions.size();
}


const Options &OutData::description( int i ) const
{
  if ( i < 0 || i >= (int)Descriptions.size() ) {
    Dummy.clear();
    return Dummy;
  }
  else
    return Descriptions[ i ];
}


Options &OutData::description( int i )
{
  if ( i < 0 || i >= (int)Descriptions.size() ) {
    Dummy.clear();
    return Dummy;
  }
  else
    return Descriptions[ i ];
}


const Options &OutData::description( void ) const
{
  if ( Descriptions.empty() ) {
    Dummy.clear();
    return Dummy;
  }
  else
    return Descriptions.back();
}


Options &OutData::description( void )
{
  if ( Descriptions.empty() ) {
    Dummy.clear();
    return Dummy;
  }
  else
    return Descriptions.back();
}


Options &OutData::addDescription( const string &type )
{
  Descriptions.push_back( Options() );
  Descriptions.back().addText( "type", type );
  return Descriptions.back();
}


void OutData::clearDescriptions( void )
{
  Descriptions.clear();
}


bool OutData::reglitch( void ) const
{
  return Reglitch;
}


void OutData::setReglitch( bool reglitch )
{
  Reglitch = reglitch;
}


double OutData::requestedMin( void ) const
{
  return RequestMinValue;
}


double OutData::requestedMax( void ) const
{
  return RequestMaxValue;
}


void OutData::request( double min, double max )
{
  RequestMinValue = min;
  RequestMaxValue = max;
}


void OutData::setExtRef( void )
{
  request( -1.0, ExtRef );
}


int OutData::gainIndex( void ) const
{
  return GainIndex;
}


void OutData::setGainIndex( int index )
{
  GainIndex = index;
}


char *OutData::gainData( void ) const
{
  return GainData;
}


void OutData::setGainData( char *data )
{
  GainData = data;
}


double OutData::voltage( int index ) const
{
  return operator[]( index ) * scale();
}


double OutData::getVoltage( double val ) const
{
  return val * scale();
}


double OutData::minVoltage( void ) const
{
  return MinVoltage;
}


double OutData::maxVoltage( void ) const
{
  return MaxVoltage;
}


void OutData::setMinVoltage( double minv )
{
  MinVoltage = minv;
}


void OutData::setMaxVoltage( double maxv )
{
  MaxVoltage = maxv;
}


double OutData::scale( void ) const
{
  return Scale;
}


void OutData::setScale( double scale )
{
  Scale = scale;
}


void OutData::multiplyScale( double fac )
{
  Scale *= fac;
}


string OutData::unit( void ) const
{
  return Unit;
}


void OutData::setUnit( const string &unit )
{
  Unit = unit;
}


void OutData::setUnit( double scale, const string &unit )
{
  Scale = scale;
  Unit = unit;
}


double OutData::minValue( void ) const
{
  if ( noIntensity() && noLevel() )
    return MinVoltage/Scale;
  else
    return -1.0;
}


double OutData::maxValue( void ) const
{
  if ( noIntensity() && noLevel() )
    return MaxVoltage/Scale;
  else
    return 1.0;
}


double OutData::writeTime( void ) const
{
  return WriteTime;
}


void OutData::setWriteTime( double time )
{
  WriteTime = time;
}


double OutData::duration( void ) const
{
  return length();
}


double OutData::totalDuration( void ) const
{
  return Delay + duration();
}


double OutData::maxSampleRate( void )
{
  if ( MaxRate > 0.0 )
    return MaxRate;
  else
    return 1.0/DefaultMinSampleInterval;
}


void OutData::setMaxSampleRate( double maxrate )
{
  if ( maxrate > 0.0 )
    MaxRate = maxrate;
  FixedRate = false;
}


double OutData::minSampleInterval( void )
{
  if ( MaxRate > 0.0 )
    return 1.0/MaxRate;
  else
    return DefaultMinSampleInterval;
}


void OutData::setMinSampleInterval( double minsample )
{
  if ( minsample > 0.0 )
    MaxRate = 1.0/minsample;
  FixedRate = false;
}


void OutData::setFixedSampleRate( double rate )
{
  if ( rate > 0.0 )
    MaxRate = rate;
  FixedRate = true;
}


void OutData::setFixedSampleInterval( double interval )
{
  if ( interval > 0.0 )
    MaxRate = 1.0/interval;
  FixedRate = true;
}


bool OutData::fixedSampleRate( void ) const
{
  return FixedRate;
}


double OutData::defaultMaxSampleRate( void )
{
  return 1.0/DefaultMinSampleInterval;
}


void OutData::setDefaultMaxSampleRate( double maxrate )
{
  if ( maxrate > 0.0 )
    DefaultMinSampleInterval = 1.0/maxrate;
}


double OutData::defaultMinSampleInterval( void )
{
  return DefaultMinSampleInterval;
}


void OutData::setDefaultMinSampleInterval( double minsample )
{
  if ( minsample > 0.0 )
    DefaultMinSampleInterval = minsample;
}


double OutData::bestSampleRate( double carrierfreq )
{
  double msr = maxSampleRate();
  if ( fixedSampleRate() )
    return msr;

  if ( carrierfreq <= 1.0e-8 )
    return msr;
  else {
    double rate = ::floor( msr/4.0/carrierfreq )*4.0*carrierfreq;
    if ( rate <= 1.0e-8 )
      return msr;
    else
      return rate;
  }
}


double OutData::bestSampleInterval( double carrierfreq )
{
  double msi = minSampleInterval();
  if ( fixedSampleRate() )
    return msi;

  if ( carrierfreq <= 1.0e-8 )
    return msi;
  else {
    double rate = ::floor( 1.0/msi/4.0/carrierfreq )*4.0*carrierfreq;
    if ( rate <= 1.0e-8 )
      return msi;
    else
      return 1.0/rate;
  }
}


void OutData::setBestSample( double carrierfreq )
{
  setCarrierFreq( carrierfreq );
  setStepsize( bestSampleInterval( carrierfreq ) );
}


istream &OutData::load( istream &str,
			const string &ident,
			double carrierfreq )
{
  free();

  // skip header and read key:
  double tfac = 1.0;
  string s;
  while ( getline( str, s ) && 
	  ( s.empty() || s.find( '#' ) == 0 ) ) {
    if ( s.find( "#Key" ) == 0 ) {
      for ( int k=0; 
	    getline( str, s ) && 
	      ( s.empty() || s.find( '#' ) == 0 ); 
	    k++ ) {
	if ( k < 4 && s.find( "ms" ) != string::npos )
	  tfac = 0.001;
      }
      break;
    }
  }

  // load data:
  SampleDataF::load( str, "EMPTY", &s );

  if ( tfac != 0.0 )
    range() *= tfac;

  setIdent( ident );
  setCarrierFreq( carrierfreq );
  clearError();

  return str;
}


OutData &OutData::load( const string &filename, 
			const string &ident,
			double carrierfreq )
{
  free();

  // open file:
  ifstream str( filename.c_str() );
  if ( str.bad() )
    return *this;

  // load:
  load( str, ident.empty() ? filename : ident, carrierfreq );

  return *this;
}


double OutData::maximize( double max )
{
  double maxval = ::relacs::max( *this );
  double c = max/maxval;
  *this *= c;
  return c;
}

void OutData::fill( const SampleDataD &am, double carrierfreq,
		    const string &ident )
{
  free();

  if ( carrierfreq <= 0.0 )
    return;

  setStepsize( bestSampleInterval( carrierfreq ) );
  setRangeBack( am.length() );

  if ( am.size() < 2 )
    return;

  // create lookup-table for one sine wave:
  SampleDataF sinbuf;
  sinbuf.sin( LinearRange( int( rint( 1.0 / ( carrierfreq * stepsize() ) ) ), 
			   0.0, stepsize() ), carrierfreq );
  
  // fill am with carrier sine wave:
  double slope = (am[1]-am[0])/am.stepsize();
  for ( int i=0, j=0, k=1; i<size(); i++, j++ ) {
    double t = pos( i );
    while ( am.pos( k ) < t && k+1 < am.size() ) {
      k++;
      slope = (am[k]-am[k-1])/am.stepsize();
    }
    if ( j >= sinbuf.size() ) 
      j=0;
    operator[]( i ) = sinbuf[j] * ( am[k-1] + slope * ( t - am.pos(k-1) ) );
  }
  back() = 0.0;

  setCarrierFreq( carrierfreq );
  setIdent( ident );
  clearError();
}



void OutData::fill( const OutData &am, double carrierfreq,
		    const string &ident )
{
  free();

  if ( carrierfreq <= 0.0 )
    return;

  setStepsize( bestSampleInterval( carrierfreq ) );
  setRangeBack( am.length() );

  if ( am.size() < 2 )
    return;

  // create lookup-table for one sine wave:
  SampleDataF sinbuf;
  sinbuf.sin( LinearRange( int( rint( 1.0 / ( carrierfreq * stepsize() ) ) ), 
			   0.0, stepsize() ), carrierfreq );
  
  // fill am with carrier sine wave:
  double slope = (am[1]-am[0])/am.stepsize();
  for ( int i=0, j=0, k=1; i<size(); i++, j++ ) {
    double t = pos( i );
    while ( am.pos( k ) < t && k+1 < am.size() ) {
      k++;
      slope = (am[k]-am[k-1])/am.stepsize();
    }
    if ( j >= sinbuf.size() ) 
      j=0;
    operator[]( i ) = sinbuf[j] * ( am[k-1] + slope * ( t - am.pos(k-1) ) );
  }
  back() = 0.0;

  Descriptions.clear();
  Descriptions = am.Descriptions;
  Options &opt = addDescription("stimulus/sine_wave");
  opt.addNumber("Frequency", carrierfreq );
  opt.addNumber("Duration", am.length() );
  opt.addNumber("TemporalOffset", 0.0 );

  setCarrierFreq( carrierfreq );
  setIdent( ident );
  clearError();
}


void OutData::loadAM( const string &filename, double carrierfreq, 
		      const string &ident, SampleDataD *stimulus )
{
  SampleDataD *am;
  if ( stimulus != 0 )
    am = stimulus;
  else
    am = new SampleDataD;

  am->load( filename );

  string s = ( ident == "" ? filename :  ident );

  fill( *am, carrierfreq, s );
}


void OutData::sineWave( double freq, double duration, double ampl, double r,
			const string &ident )
{
  sin( 0.0, duration, bestSampleInterval( freq ), freq );
  if ( ampl != 1.0 )
    array() *= ampl;
  if ( r > 0.0 )
    ramp( r );
  back() = 0.0;

  Options &opt = addDescription( "stimulus/sine_wave" );
  opt.addNumber( "Frequency", freq );
  opt.addNumber( "Amplitude", ampl );
  opt.addNumber( "TemporalOffset", 0.0 );
  opt.addNumber( "Duration", duration );
  opt.addNumber( "Phase", 0.0 );

  setCarrierFreq( freq );
  setIdent( ident );
  clearError();
}


void OutData::noiseWave( double cutofffreq, double duration, double stdev,
			 double r, const string &ident )
{
  whiteNoise( duration, bestSampleInterval( cutofffreq ), 0.0, 
	      cutofffreq, ::relacs::rnd );
  if ( stdev != 1.0 )
    array() *= stdev;
  if ( r > 0.0 )
    ramp( r );
  back() = 0.0;

  Options &opt = addDescription( "stimulus/white_noise" );
  opt.addNumber( "Duration", duration );
  opt.addNumber( "UpperCutoffFrequency", cutofffreq );
  opt.addNumber( "LowerCutoffFrequency", 0.0);
  opt.addNumber( "Mean", 0.0);
  opt.addNumber( "Variance", std::pow(stdev,2.0));
  opt.addNumber( "OffsetTime", 0.0);


  setCarrierFreq( cutofffreq );
  setIdent( ident );
  clearError();
}


void OutData::bandNoiseWave( double cutofffreqlow, double cutofffreqhigh, 
			     double duration, double stdev,
			     double r, const string &ident )
{
  whiteNoise( duration, bestSampleInterval( cutofffreqhigh ), 
	      cutofffreqlow, cutofffreqhigh, ::relacs::rnd );
  if ( stdev != 1.0 )
    array() *= stdev;
  if ( r > 0.0 )
    ramp( r );
  back() = 0.0;

  Options &opt = addDescription( "stimulus/white_noise" );
  opt.addNumber( "Duration", duration );
  opt.addNumber( "UpperCutoffFrequency", cutofffreqhigh );
  opt.addNumber( "LowerCutoffFrequency", cutofffreqlow);
  opt.addNumber( "Mean", 0.0);
  opt.addNumber( "Variance", std::pow(stdev,2.0));
  opt.addNumber( "OffsetTime", 0.0);

  setCarrierFreq( cutofffreqhigh );
  setIdent( ident );
  clearError();
}


void OutData::ouNoiseWave( double tau, double duration, double stdev,
			   double r, const string &ident )
{
  ouNoise( duration, minSampleInterval(), tau, ::relacs::rnd );
  if ( stdev != 1.0 )
    array() *= stdev;
  if ( r > 0.0 )
    ramp( r );
  back() = 0.0;

  Options &opt = addDescription( "stimulus/colored_noise" );
  opt.addNumber( "Duration", duration );
  opt.addNumber( "CorrelationTime", tau );
  opt.addNumber( "Mean", 0.0);
  opt.addNumber( "Variance", std::pow(stdev,2.0));
  opt.addNumber( "OffsetTime", 0.0);

  setCarrierFreq( 1/tau );
  setIdent( ident );
  clearError();
}

void OutData::sweepWave( double duration, double ampl, double stepsize, 
			 double startfreq, double endfreq, double r, 
			 const string &ident )
{
  sweep(0.0, duration, stepsize, startfreq, endfreq);
  if ( ampl != 1.0 )
    array() *= ampl;
  if ( r > 0.0 )
    ramp( r );
  back() = 0.0;

  Options &opt = addDescription( "stimulus/sweep_wave" );
  opt.addNumber( "StartFrequency", startfreq );
  opt.addNumber( "EndFrequency", endfreq );
  opt.addNumber( "Amplitude", ampl );
  opt.addNumber( "TemporalOffset", 0.0 );
  opt.addNumber( "Duration", duration );
  opt.addNumber( "Phase", 0.0 );

  setIdent( ident );
  clearError();
}


void OutData::rectangleWave( double duration, double ampl, double stepsize, 
			     double period, double width, double ramp,
			     const string &ident)
{
  rectangle(0.0, duration, stepsize, period, width, ramp);
  if ( ampl != 1.0 )
    array() *= ampl;
  back() = 0.0;

  Options &opt = addDescription( "stimulus/square_wave" );
  opt.addNumber( "Duration", duration );
  opt.addNumber( "TemporalOffset", 0.0 );
  opt.addNumber( "DutyCycle", width/period );
  opt.addNumber( "Amplitude", ampl );
  opt.addNumber( "Frequency", 1.0/period );
  opt.addNumber( "StartAmplitude", 0.0 );

  setCarrierFreq( 1.0/period );
  setIdent( ident );
  clearError();
}

void OutData::sawUpWave( double duration, double ampl, double stepsize, 
			 double period, double ramp, const string &ident)
{
  sawUp(0.0, duration, stepsize, period, ramp);
  if ( ampl != 1.0 )
    array() *= ampl;
  back() = 0.0;

  Options &opt = addDescription( "stimulus/sawtooth" );
  opt.addNumber( "Duration", duration );
  opt.addNumber( "TemporalOffset", 0.0 );
  opt.addNumber( "UpstrokeWidth", period-ramp );
  opt.addNumber( "DownstrokeWidth", ramp );
  opt.addNumber( "Ramp", ramp );
  opt.addNumber( "Amplitude", ampl );
  opt.addNumber( "Period", period );

  setIdent( ident );
  clearError();
}

void OutData::sawDownWave( double duration, double ampl, double stepsize, 
			   double period, double ramp, const string &ident)
{
  sawDown(0.0, duration, stepsize, period, ramp);
  if ( ampl != 1.0 )
    array() *= ampl;
  back() = 0.0;

  Options &opt = addDescription( "stimulus/sawtooth" );
  opt.addNumber( "Duration", duration );
  opt.addNumber( "TemporalOffset", 0.0 );
  opt.addNumber( "UpstrokeWidth", ramp );
  opt.addNumber( "DownstrokeWidth", period-ramp );
  opt.addNumber( "Ramp", ramp );
  opt.addNumber( "Amplitude", ampl );
  opt.addNumber( "Period", period );

  setIdent( ident );
  clearError();
}

void OutData::triangleWave( double duration, double ampl, double stepsize, 
			    double period, const string &ident )
{
  triangle(0.0, duration, stepsize, period);
  if ( ampl != 1.0 )
    array() *= ampl;
  back() = 0.0;

  Options &opt = addDescription( "stimulus/sawtooth" );
  opt.addNumber( "Duration", duration );
  opt.addNumber( "TemporalOffset", 0.0 );
  opt.addNumber( "UpstrokeWidth", 0.5*period );
  opt.addNumber( "DownstrokeWidth", 0.5*period );
  opt.addNumber( "Amplitude", ampl );
  opt.addNumber( "Period", period );

  setIdent( ident );
  clearError();
}


ostream &operator<<( ostream &str, const OutData &od )
{
  //  str << SampleDataF( od );
  str << "Offset: " << od.offset() << '\n';
  str << "Stepsize: " << od.stepsize() << '\n';
  str << "Size: " << od.size() << '\n';
  str << DaqError( od );
  str << "Delay: " << od.Delay << '\n';
  str << "StartSource: " << od.StartSource << '\n';
  str << "Priority: " << od.Priority << '\n';
  str << "Continuous: " << od.Continuous << '\n';
  str << "Restart: " << od.Restart << '\n';
  str << "MaxRate: " << od.MaxRate << '\n';
  str << "FixedRate: " << od.FixedRate << '\n';
  str << "Device: " << od.Device << '\n';
  str << "Channel: " << od.Channel << '\n';
  str << "Trace: " << od.Trace << '\n';
  str << "TraceName: " << od.TraceName << '\n';
  str << "Ident: " << od.Ident << '\n';
  str << "Reglitch: " << od.Reglitch << '\n';
  str << "RequestMinValue: " << od.RequestMinValue << '\n';
  str << "RequestMaxValue: " << od.RequestMaxValue << '\n';
  str << "GainIndex: " << od.GainIndex << '\n';
  //  str << "GainData: " << od.GainData << '\n';
  str << "Scale: " << od.Scale << '\n';
  str << "Unit: " << od.Unit << '\n';
  str << "WriteTime: " << od.WriteTime << '\n';
  str << "MinVoltage: " << od.MinVoltage << '\n';
  str << "MaxVoltage: " << od.MaxVoltage << '\n';
  str << "SignalDelay: " << od.SignalDelay << '\n';
  str << "Intensity: " << od.Intensity << '\n';
  str << "CarrierFreq: " << od.CarrierFreq << '\n';
  str << "Level: " << od.Level << '\n';
  str << "DeviceIndex: " << od.DeviceIndex << '\n';
  str << "DeviceDelay: " << od.DeviceDelay << '\n';
  str << "DeviceCount: " << od.DeviceCount << '\n';
  if ( od.Descriptions.empty() )
    str << "Descriptions: <empty>\n";
  else {
    str << "Descriptions: \n";
    for ( int k=0; k<(int)od.Descriptions.size(); k++ ) {
      if ( od.Descriptions[k].empty() )
	str << "  Description empty\n";
      else {
	str << "  Description:\n";
	od.Descriptions[k].save( str, "  " );
      }
    }
  }
  return str;
}


}; /* namespace relacs */

