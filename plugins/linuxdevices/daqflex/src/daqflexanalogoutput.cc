/*
  daqflex/daqflexanalogoutput.h
  Interface for accessing analog output of a DAQFlex board from Measurement Computing.

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

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cmath>
#include <QMutexLocker>
#include <relacs/str.h>
#include <relacs/daqflex/daqflexanalogoutput.h>
using namespace std;
using namespace relacs;

namespace daqflex {


DAQFlexAnalogOutput::DAQFlexAnalogOutput( void )
  : AnalogOutput( "DAQFlexAnalogOutput", DAQFlexAnalogIOType )
{
  UseAIClock = false;
  LevelMaxVolt = 3.0;
  IsPrepared = false;
  NoMoreData = true;
  DAQFlexDevice = NULL;
  BufferSize = 0;
  Buffer = 0;
  NBuffer = 0;
  ChannelValues = 0;

  initOptions();
}


DAQFlexAnalogOutput::DAQFlexAnalogOutput( DAQFlexCore &device, const Options &opts ) 
  : DAQFlexAnalogOutput()
{
  Options::read(opts);
  open( device );
}


DAQFlexAnalogOutput::~DAQFlexAnalogOutput( void )
{
  close();
}

void DAQFlexAnalogOutput::initOptions()
{
  AnalogOutput::initOptions();

  addBoolean( "useaiclock", "Use AI clock", false );
  addNumber( "delays", "Delay between analog input and output", 0.0, 0.0, 1.0, 0.0001, "s", "ms" ).setStyle( Parameter::MultipleSelection );
}

int DAQFlexAnalogOutput::open( DAQFlexCore &daqflexdevice )
{
  if ( isOpen() )
    return -5;

  Info.clear();
  Settings.clear();

  DAQFlexDevice = &daqflexdevice;
  if ( !DAQFlexDevice->isOpen() ) {
    setErrorStr( "Daqflex core device " + DAQFlexDevice->deviceName() + " is not open." );
    return NotOpen;
  }

  // is AO supported?
  if ( DAQFlexDevice->maxAOChannels() == 0 ) {
    setErrorStr( "Device " + DAQFlexDevice->deviceName() + " does not support anaolog output." );
    return InvalidDevice;
  }

  // set basic device infos:
  setDeviceName( DAQFlexDevice->deviceName() );
  setDeviceVendor( DAQFlexDevice->deviceVendor() );
  setDeviceFile( DAQFlexDevice->deviceFile() );

  // initialize ranges:
  BipolarRange.clear();
  UnipolarRange.clear();

  int ern = DAQFlexCore::Success;
  Str response = DAQFlexDevice->getValue( "AO{0}:RANGE", ern );
  if ( ern == DAQFlexCore::Success ) {
    // Analog output ranges:
    // 1608GX_2AO: BIP10V
    // 202, 205: UNI5V
    // 1208FS, 1408FS: UNI5V
    // 2408-2AO:  BIP10V
    // 7204: UNI4.096V
    bool uni = ( response[0] == 'U' );
    double range = response.number( 0.0, 3 );
    if ( range <= 1e-6 ) {
      setErrorStr( "Failed to read out analog output range from device " + DAQFlexDevice->deviceName() );
      return InvalidDevice;
    }    
    if ( uni )
      UnipolarRange.push_back( range );
    else
      BipolarRange.push_back( range );
  }
  else {
    setErrorStr( "Failed to retrieve analog output range from device " + DAQFlexDevice->deviceName() +
		 ". Error: " + DAQFlexDevice->daqflexErrorStr( ern ) );
    return InvalidDevice;
  }

  // set and write default output values for channels:
  ChannelValues = new float[channels()];
  for ( int k=0; k<channels(); k++ )
    ChannelValues[k] = 0.0;
  writeZeros();

  // ai clock:
  UseAIClock = boolean( "useaiclock" );

  // delays:
  vector< double > delays;
  numbers( "delays", delays, "s" );
  setDelays( delays );

  // clear flags:
  IsPrepared = false;
  NoMoreData = true;

  setInfo();

  return 0;
}


int DAQFlexAnalogOutput::open( Device &device )
{
  return open( dynamic_cast<DAQFlexCore&>( device ) );
}


bool DAQFlexAnalogOutput::isOpen( void ) const
{
  lock();
  bool o = ( DAQFlexDevice != NULL && DAQFlexDevice->isOpen() );
  unlock();
  return o;
}


void DAQFlexAnalogOutput::close( void )
{
  if ( ! isOpen() )
    return;

  reset();

  writeZeros();

  // clean up stored channel values:
  if ( ChannelValues != 0 )
    delete[] ChannelValues;
  ChannelValues = 0;

  // clear flags:
  DAQFlexDevice = NULL;
  IsPrepared = false;
  NoMoreData = true;

  Info.clear();
}


void DAQFlexAnalogOutput::writeZeros( void )
{
  QMutexLocker aolocker( mutex() );
  float minvolt = ( BipolarRange.size() > 0) ? -BipolarRange[0] : 0.0;
  float maxvolt = ( BipolarRange.size() > 0) ? BipolarRange[0] : UnipolarRange[0];
  QMutexLocker corelocker( DAQFlexDevice->mutex() );
  float gain = DAQFlexDevice->maxAOData()/(maxvolt-minvolt);
  float v = 0.0;
  for ( int k=0; k<DAQFlexDevice->maxAOChannels(); k++ ) {
    unsigned short data = (unsigned short)( (v-minvolt)*gain );
    DAQFlexDevice->setValueUnlocked( "AO{" + Str( k ) + "}:VALUE", Str( data ) );
  }
}


int DAQFlexAnalogOutput::channels( void ) const
{
  if ( !isOpen() )
    return -1;
  return DAQFlexDevice->maxAOChannels();
}


int DAQFlexAnalogOutput::bits( void ) const
{
  if ( !isOpen() )
    return -1;
  return (int)( log( DAQFlexDevice->maxAOData()+2.0 )/ log( 2.0 ) );
}


double DAQFlexAnalogOutput::maxRate( void ) const
{
  return DAQFlexDevice->maxAORate();
}


int DAQFlexAnalogOutput::maxRanges( void ) const
{
  unsigned int n = BipolarRange.size();
  if ( n < UnipolarRange.size() )
    return UnipolarRange.size();
  else
    return n;
}


double DAQFlexAnalogOutput::unipolarRange( int index ) const
{
  if ( (index < 0) || (index >= (int)UnipolarRange.size()) )
    return -1.0;
  return UnipolarRange[index];
}


double DAQFlexAnalogOutput::bipolarRange( int index ) const
{
  if ( (index < 0) || (index >= (int)BipolarRange.size()) )
    return -1.0;
  return BipolarRange[index];
}


int DAQFlexAnalogOutput::directWrite( OutList &sigs )
{
  // no signals:
  if ( sigs.size() == 0 )
    return -1;

  QMutexLocker aolocker( mutex() );
  QMutexLocker corelocker( DAQFlexDevice->mutex() );

  for ( int k=0; k<sigs.size(); k++ ) {
    // we use only the largest range:
    if ( BipolarRange.size() > 0 ) {
      sigs[k].setMinVoltage( -BipolarRange[0] );
      sigs[k].setMaxVoltage( BipolarRange[0] );
      if ( ! sigs[k].noLevel() ) {
	double maxv = LevelMaxVolt;
	if ( maxv < 1e-8 )
	  maxv = BipolarRange[0];
	sigs[k].multiplyScale( maxv );
      }
    }
    else {
      sigs[k].setMinVoltage( 0.0 );
      sigs[k].setMaxVoltage( UnipolarRange[0] );
      if ( ! sigs[k].noLevel() ) {
	double maxv = LevelMaxVolt;
	if ( maxv < 1e-8 )
	  maxv = UnipolarRange[0];
	sigs[k].multiplyScale( maxv );
      }
    }

    // apply range:
    double minval = sigs[k].minVoltage()/sigs[k].scale();
    double maxval = sigs[k].maxVoltage()/sigs[k].scale();
    double gain = DAQFlexDevice->maxAOData()/(maxval-minval);
    float v = sigs[k].size() > 0 ? sigs[k][0] : 0.0;
    if ( sigs[k].noLevel() ) {
      if ( v < sigs[k].minValue() )
	sigs[k].addError( DaqError::Underflow );
      else if ( v > sigs[k].maxValue() )
	sigs[k].addError( DaqError::Overflow );
    }
    else {
      if ( v > 1.0+1.0e-8 )
	sigs[k].addError( DaqError::Overflow );
      else if ( v < -1.0-1.0e-8 )
	sigs[k].addError( DaqError::Underflow );
      if ( ::fabs( sigs[k].scale() ) < 1.0e-8 ) {
	minval = sigs[k].minVoltage();
	maxval = sigs[k].maxVoltage();
	gain = DAQFlexDevice->maxAOData()/(maxval-minval);
	v = 0.0;
      }
    }
    if ( sigs[k].failed() )
      continue;
    unsigned short data = (unsigned short)( (v-minval)*gain );
    if ( data > DAQFlexDevice->maxAOData() ) {
      sigs[k].addError( DaqError::Overflow );
      continue;
    }

    // write data:
    DAQFlexDevice->setValueUnlocked( "AO{" + Str( sigs[k].channel() ) + "}:VALUE=", Str( data ) );
    if ( DAQFlexDevice->failed() ) {
      sigs[k].addErrorStr( "DAQFlex direct write failed: " + DAQFlexDevice->daqflexErrorStr() );
      return -1;
    }

    ChannelValues[sigs[k].channel()] = v;

  }

  return ( sigs.success() ? 0 : -1 );
}


template < typename T >
int DAQFlexAnalogOutput::convert( char *cbuffer, int nbuffer )
{
  if ( nbuffer < (int)sizeof( T ) )
    return 0;

  // conversion polynomials and scale factors:
  unsigned int maxAOData = DAQFlexDevice->maxAOData();
  double minval[ Sigs.size() ];
  double maxval[ Sigs.size() ];
  double gain[ Sigs.size() ];
  //  const Calibration* calib[Sigs.size()];
  T zeros[ Sigs.size() ];
  for ( int k=0; k<Sigs.size(); k++ ) {
    minval[k] = Sigs[k].minVoltage()/Sigs[k].scale();
    maxval[k] = Sigs[k].maxVoltage()/Sigs[k].scale();
    gain[k] = maxAOData/(maxval[k]-minval[k]);
    // calib[k] = (const Calibration *)Sigs[k].gainData();
    // XXX calibration?
    float v = ChannelValues[Sigs[k].channel()];
    if ( ::fabs( Sigs[k].scale() ) < 1.0e-8 ) {
      minval[k] = Sigs[k].minVoltage();
      maxval[k] = Sigs[k].maxVoltage();
      gain[k] = maxAOData/(maxval-minval);
      v = 0.0;
    }
    else if ( v > maxval[k] )
      v = maxval[k];
    else if ( v < minval[k] ) 
      v = minval[k];
    zeros[k] = (unsigned short)( (v-minval[k])*gain[k] );
    if ( zeros[k] > maxAOData )
      zeros[k] = maxAOData;
  }

  // buffer pointer:
  T *bp = (T*)cbuffer;
  int maxn = nbuffer/sizeof( T )/Sigs.size();
  int n = 0;

  // convert data and multiplex into buffer:
  for ( int i=0; i<maxn && Sigs[0].deviceWriting(); i++ ) {
    for ( int k=0; k<Sigs.size(); k++ ) {
      if ( Sigs[k].deviceCount() < 0 ) {
	*bp = zeros[k];
	Sigs[k].incrDeviceIndex();
	if ( Sigs[k].deviceIndex() >= Sigs[k].deviceDelay() )
	  Sigs[k].incrDeviceCount();
      }
      else {
	float v = Sigs[k].deviceValue();
	if ( ::fabs( Sigs[k].scale() ) < 1.0e-8 )
	  v = 0.0;
	else if ( v > maxval[k] )
	  v = maxval[k];
	else if ( v < minval[k] )
	  v = minval[k];
	// XXX calibration?
	T d = (unsigned short)( (v-minval[k])*gain[k] );
	if ( d > maxAOData )
	  d = maxAOData;
	*bp = d;
	if ( Sigs[k].deviceIndex() >= Sigs[k].size() )
	  Sigs[k].incrDeviceCount();
      }
      ++bp;
      ++n;
    }
  }

  // memorize last values:
  for ( int k=0; k<Sigs.size(); k++ ) {
    if ( Sigs[k].deviceCount() >= 0 && Sigs[k].deviceIndex() > 0 )
      ChannelValues[Sigs[k].channel()] = Sigs[k][Sigs[k].deviceIndex()-1];
    else if ( Sigs[k].deviceCount() > 0 && Sigs[k].deviceIndex() == 0 )
      ChannelValues[Sigs[k].channel()] = Sigs[k].back();
  }

  return n * sizeof( T );
}


int DAQFlexAnalogOutput::testWriteDevice( OutList &sigs )
{
  int retVal = 0;

  double buffertime = sigs[0].interval( 0xffff/sigs.size() );
  if ( buffertime < 0.001 ) {
    sigs.addError( DaqError::InvalidBufferTime );
    retVal = -1;
  }

  if ( sigs.size() > channels() ) {
    sigs.addError( DaqError::InvalidChannel );
    retVal = -1;
  }

  return retVal;
}


int DAQFlexAnalogOutput::prepareWrite( OutList &sigs )
{
  if ( !isOpen() )
    return -1;

  lock();

  if ( Buffer != 0 ) { // should not be necessary!
    delete [] Buffer;
    Buffer = 0;
    cerr << "DAQFlexAnalogOutput::prepareWrite() warning: Buffer was not freed!\n";
  }
  if ( NBuffer != 0 ) { // should not be necessary!
    cerr << "DAQFlexAnalogOutput::prepareWrite() warning: NBuffer=" << NBuffer << " is not zero!\n";
    NBuffer = 0;
  }

  Sigs.clear();
  BufferSize = 0;
  Settings.clear();
  IsPrepared = false;

  unlock();

  // no signals:
  if ( sigs.size() == 0 ) {
    sigs.addError( DaqError::NoData );
    return -1;
  }

  QMutexLocker aolocker( mutex() );

  // copy and sort signal pointers:
  OutList ol;
  ol.add( sigs );
  ol.sortByChannel();

  {
    QMutexLocker corelocker( DAQFlexDevice->mutex() );

    // setup channels:
    DAQFlexDevice->setValueUnlocked( "AOSCAN:LOWCHAN", Str( ol[0].channel() ) );
    if ( DAQFlexDevice->failed() ) {
      sigs.setErrorStr( "AOSCAN:LOWCHAN " + DAQFlexDevice->daqflexErrorStr() );
      return -1;
    } 
    DAQFlexDevice->setValueUnlocked( "AOSCAN:HIGHCHAN", Str( ol.back().channel() ) );
    if ( DAQFlexDevice->failed() ) {
      sigs.setErrorStr( "AOSCAN:HIGHCHAN" + DAQFlexDevice->daqflexErrorStr() );
      return -1;
    } 
    for( int k = 0; k < ol.size(); k++ ) {
      // minimum and maximum values:
      double min = ol[k].requestedMin();
      double max = ol[k].requestedMax();
      if ( min == OutData::AutoRange || max == OutData::AutoRange ) {
	float smin = 0.0;
	float smax = 0.0;
	minMax( smin, smax, ol[k] );
	if ( min == OutData::AutoRange )
	  min = smin;
	if ( max == OutData::AutoRange )
	  max = smax;
      }
      // we use only the largest range and there is only one range:
      if ( BipolarRange.size() > 0 ) {
	ol[k].setMinVoltage( -BipolarRange[0] );
	ol[k].setMaxVoltage( BipolarRange[0] );
	if ( ! ol[k].noLevel() ) {
	  double maxv = LevelMaxVolt;
	  if ( maxv < 1e-8 )
	    maxv = BipolarRange[0];
	  ol[k].multiplyScale( maxv );
	}
      }
      else {
	ol[k].setMinVoltage( 0.0 );
	ol[k].setMaxVoltage( UnipolarRange[0] );
	if ( ! ol[k].noLevel() ) {
	  double maxv = LevelMaxVolt;
	  if ( maxv < 1e-8 )
	    maxv = UnipolarRange[0];
	  ol[k].multiplyScale( maxv );
	}
      }
      // check for signal overflow/underflow:
      if ( ol[k].noLevel() ) {
	if ( min < ol[k].minValue() )
	  ol[k].addError( DaqError::Underflow );
	else if ( max > ol[k].maxValue() )
	  ol[k].addError( DaqError::Overflow );
      }
      else {
	if ( max > 1.0+1.0e-8 )
	  ol[k].addError( DaqError::Overflow );
	else if ( min < -1.0-1.0e-8 )
	  ol[k].addError( DaqError::Underflow );
      }

      // allocate gain factor:
      char *gaindata = ol[k].gainData();
      if ( gaindata != NULL )
	delete [] gaindata;
      gaindata = new char[sizeof(Calibration)];
      ol[k].setGainData( gaindata );
      Calibration *gainp = (Calibration *)gaindata;

      // get calibration:
      Str response = DAQFlexDevice->getValueUnlocked( "AO{" + Str( ol[k].channel() ) + "}:SLOPE" );
      gainp->Slope = response.number();
      response = DAQFlexDevice->getValueUnlocked( "AO{" + Str( ol[k].channel() ) + "}:OFFSET" );
      gainp->Offset = response.number();
      /*
	gainp->Slope *= 2.0*max/DAQFlexDevice->maxAIData();
	gainp->Offset *= 2.0*max/DAQFlexDevice->maxAIData();
	gainp->Offset -= max;
      */

    }

    if ( ! ol.success() )
      return -1;

    int delayinx = ol[0].indices( ol[0].delay() );
    ol.deviceReset( delayinx );

    // setup acquisition:
    DAQFlexDevice->setValueUnlocked( "AOSCAN:STALL", "DISABLE" );
    if ( DAQFlexDevice->failed() ) {
      sigs.setErrorStr( "AOSCAN:STALL " + DAQFlexDevice->daqflexErrorStr() );
      return -1;
    } 
    DAQFlexDevice->setValueUnlocked( "AOSCAN:RATE", Str( sigs[0].sampleRate(), "%g" ) );
    if ( DAQFlexDevice->failed() ) {
      sigs.setErrorStr( "AOSCAN:RATE " + DAQFlexDevice->daqflexErrorStr() );
      return -1;
    } 
    if ( sigs[0].continuous() ) {
      Samples = 0;
      DAQFlexDevice->setValueUnlocked( "AOSCAN:SAMPLES", "0" );
      if ( DAQFlexDevice->failed() ) {
	sigs.setErrorStr( "AOSCAN:SAMPLES " + DAQFlexDevice->daqflexErrorStr() );
	return -1;
      } 
    }
    else {
      Samples = sigs.deviceBufferSize();
      DAQFlexDevice->setValueUnlocked( "AOSCAN:SAMPLES", Str( Samples ) );
      if ( DAQFlexDevice->failed() ) {
	sigs.setErrorStr( "AOSCAN:SAMPLES " + DAQFlexDevice->daqflexErrorStr() );
	return -1;
      } 
    }
    if ( UseAIClock ) {
      if ( ::fabs(sigs[0].sampleRate() - DAQFlexDevice->aiSampleRate()) > 0.1 ) {
	sigs.addError( DaqError::InvalidSampleRate );
	sigs.addErrorStr( "sampling rate " + Str( 0.001*sigs[0].sampleRate(), "%.1f" ) +
			  "kHz does not match AI clock of " + 
			  Str( 0.001*DAQFlexDevice->aiSampleRate(), "%.1f" ) + "kHz" );
	return -1;
      }
      DAQFlexDevice->setValueUnlocked( "AOSCAN:EXTPACER", "ENABLE" );
      if ( DAQFlexDevice->failed() ) {
	sigs.setErrorStr( "AOSCAN:EXTPACER " + DAQFlexDevice->daqflexErrorStr() );
	return -1;
      } 
    }
    
  }  // unlock Core MutexLocker

  // set buffer size:
  if ( DAQFlexDevice->aoFIFOSize() > 0 ) {
    BufferSize = sigs.size()*DAQFlexDevice->aoFIFOSize()*2;
    int nbuffer = sigs.deviceBufferSize()*2;
    int outps = DAQFlexDevice->outPacketSize();
    if ( BufferSize > nbuffer ) {
      BufferSize = nbuffer;
      if ( BufferSize < outps )
	BufferSize = outps;
    }
    else
      BufferSize = ::ceil((BufferSize/outps)*outps); // round up to full package size
    if ( BufferSize > 0xfffff )
      BufferSize = 0xfffff;
    double timeout = 0.1*sigs[0].interval( BufferSize/2/sigs.size()-1 );
    int timeoutms = (int)::ceil( 1000.0*timeout );
    setWriteSleep( timeoutms );
  }
  else
    BufferSize = sigs.deviceBufferSize()*2;
  if ( BufferSize <= 0 )
    sigs.addError( DaqError::InvalidBufferTime );

  setSettings( ol, BufferSize );

  if ( ! ol.success() )
    return -1;

  Sigs = ol;
  Buffer = new char[ BufferSize ];  // Buffer was deleted in reset()!

  if ( DAQFlexDevice->aoFIFOSize() <= 0 ) {
    // no FIFO and bulk transfer:
    convert<unsigned short>( Buffer, BufferSize );
  }

  int r = 0;
  if ( DAQFlexDevice->aoFIFOSize() > 0 ) {
    r = writeData();
    if ( r < -1 )
      return -1;
  }

  IsPrepared = Sigs.success();
  NoMoreData = ( r == -1 );

  return 0;
}


int DAQFlexAnalogOutput::startWrite( QSemaphore *sp )
{
  QMutexLocker aolocker( mutex() );
  if ( !IsPrepared || Sigs.empty() ) {
    Sigs.setErrorStr( "AO not prepared or no signals!" );
    return -1;
  }
  if ( DAQFlexDevice->aoFIFOSize() > 0 ) {
    int ern = DAQFlexDevice->sendMessage( "AOSCAN:START" );
    if ( ern != DAQFlexCore::Success ) {
      Sigs.setErrorStr( "Failed to start AO device: " + DAQFlexDevice->daqflexErrorStr( ern ) );
      return -1;
    }
  }
  int r = NoMoreData ? 0 : 1;
  startThread( sp );
  return r;
}


int DAQFlexAnalogOutput::writeData( void )
{
  if ( Sigs.empty() )
    return -2;

  // no FIFO:
  if ( DAQFlexDevice->aoFIFOSize() <= 0 ) {
    {
      QMutexLocker corelocker( DAQFlexDevice->mutex() );
      unsigned short *bp = (unsigned short *)Buffer;
      for ( int k=0; k<Sigs.size(); k++ ) {
	unsigned short val = *(bp+NBuffer/2);
	DAQFlexDevice->setValueUnlocked( "AO{" + Str( Sigs[k].channel() ) + "}:VALUE", Str( val ) );
	NBuffer += 2;
      }
    }
    // no more data:
    if ( NBuffer >= BufferSize ) {
      if ( Buffer != 0 )
	delete [] Buffer;
      Buffer = 0;
      BufferSize = 0;
      NBuffer = 0;
      NoMoreData = true;
      return -1;
    }
    else
      return Sigs.size();
  }

  // bulk writing with FIFO:

  // device stopped writing?
  if ( IsPrepared ) {  // not stopped!
    if ( statusUnlocked() == Underrun ) {
      cerr << "AO UNDERRUN\n";
      Sigs.addError( DaqError::OverflowUnderrun );
      setErrorStr( Sigs );
      return -2;
    }
  }

  // convert data into buffer:
  if ( Sigs[0].deviceWriting() ) {
    int bytesConverted = convert<unsigned short>( Buffer+NBuffer, BufferSize-NBuffer );
    NBuffer += bytesConverted;
  }

  if ( ! Sigs[0].deviceWriting() && NBuffer <= 0 ) {
    if ( Buffer != 0 )
      delete [] Buffer;
    Buffer = 0;
    BufferSize = 0;
    NBuffer = 0;
    NoMoreData = true;
    return -1;
  }

  // transfer buffer to device:
  int outps = DAQFlexDevice->outPacketSize();
  int bytesToWrite = NBuffer;
  if ( bytesToWrite > DAQFlexDevice->aoFIFOSize() * 2 )
    bytesToWrite = DAQFlexDevice->aoFIFOSize() * 2;
  bytesToWrite = (bytesToWrite/outps)*outps;
  if ( bytesToWrite <= 0 )
    bytesToWrite = NBuffer;
  int bytesWritten = 0;
  int ern = DAQFlexDevice->writeBulkTransfer( (unsigned char*)(Buffer), 
					      bytesToWrite, &bytesWritten, 1 );

  // update buffer:
  int datams = 0;
  if ( bytesWritten > 0 ) {
    memmove( Buffer, Buffer+bytesWritten, NBuffer-bytesWritten );
    NBuffer -= bytesWritten;
    datams = writeSleep();
  }

  if ( ern != DAQFlexCore::Success && ern != DAQFlexCore::ErrorLibUSBTimeout ) {
    // error:
    switch( ern ) {
    case DAQFlexCore::ErrorLibUSBPipe:
      Sigs.addError( DaqError::OverflowUnderrun );
      break;
    case DAQFlexCore::ErrorLibUSBBusy:
      Sigs.addError( DaqError::Busy );
      break;
    case DAQFlexCore::ErrorLibUSBNoDevice:
      Sigs.addError( DaqError::NoDevice );
      break;
    default:
      Sigs.addErrorStr( DAQFlexDevice->daqflexErrorStr( ern ) );
      Sigs.addError( DaqError::Unknown );
    }
    setErrorStr( Sigs );
    return -2;
  }

  return datams;
}


int DAQFlexAnalogOutput::stop( void )
{
  if ( ! IsPrepared )
    return 0;

  {
    QMutexLocker aolocker( mutex() );
    int ern = DAQFlexDevice->sendMessage( "AOSCAN:STOP" );
    //int ern = DAQFlexDevice->sendCommand( "AOSCAN:STOP" );
    if ( ern != DAQFlexCore::Success )
      cerr << "FAILED TO STOP ANALOG OUTPUT " << DAQFlexDevice->daqflexErrorStr( ern ) << "\n";
    IsPrepared = false;
  }

  stopWrite();

  return 0;
}


int DAQFlexAnalogOutput::reset( void )
{
  QMutexLocker aolocker( mutex() );
  {
    QMutexLocker corelocker( DAQFlexDevice->mutex() );

    // clear underrun condition:
    DAQFlexDevice->sendMessageUnlocked( "AOSCAN:RESET" );
    if ( DAQFlexDevice->failed() )
      cerr << "RESET: FAILED TO RESET ANALOG OUTPUT " << DAQFlexDevice->daqflexErrorStr() << "\n";

    DAQFlexDevice->clearWrite();
  }
  
  Sigs.clear();
  if ( Buffer != 0 )
    delete [] Buffer;
  Buffer = 0;
  BufferSize = 0;
  NBuffer = 0;

  Settings.clear();
  IsPrepared = false;

  return 0;
}


AnalogOutput::Status DAQFlexAnalogOutput::statusUnlocked( void ) const
{
  Status r = Idle;
  int ern = DAQFlexCore::Success;
  string response = DAQFlexDevice->getValue( "AOSCAN:STATUS", ern );
  if ( response == "RUNNING" )
    r = Running;
  else if ( response == "UNDERRUN" ) {
    Sigs.addError( DaqError::OverflowUnderrun );
    setErrorStr( "underrun" );
    r = Underrun;
  }
  return r;
}


bool DAQFlexAnalogOutput::useAIRate( void ) const
{
  return UseAIClock;
}


bool DAQFlexAnalogOutput::prepared( void ) const 
{ 
  lock();
  bool ip = IsPrepared;
  unlock();
  return ip;
}


bool DAQFlexAnalogOutput::noMoreData( void ) const
{
  lock();
  bool nmd = NoMoreData;
  unlock();
  return nmd;
}


}; /* namespace daqflex */
