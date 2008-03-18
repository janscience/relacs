/*
  loudspeaker.h
  Direct conversion to attenuation level 

  RELACS - RealTime ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2007 Jan Benda <j.benda@biologie.hu-berlin.de>

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

#ifndef _LOUDSPEAKER_H_
#define _LOUDSPEAKER_H_


#include <vector>
#include <relacs/configclass.h>
#include <relacs/attenuate.h>

using namespace std;


/*!
\class LoudSpeaker
\brief Direct conversion to attenuation level 
       depending on carrier frequency.
\author Jan Benda
\version 1.1
*/


class LoudSpeaker : public Attenuate, public ConfigClass
{

public:

    /*! Constructor. */
  LoudSpeaker( void );
    /*! Destructor. */
  virtual ~LoudSpeaker( void );

    /*! Returns the gain factor used for a signal 
        with carrier frequency \a frequency. */
  double gain( double frequency ) const;
    /*! Returns the offset used for a signal 
        with carrier frequency \a frequency. */
  double offset( double frequency ) const;
    /*! Returns the gain factor in \a gain and the offset in \a offset 
        used for a signal with carrier frequency \a frequency.
        \a frequency is set to the closest value found 
	in the calibration table.
        \a gain and \a offset are linearly interpolated. */
  void gain( double &gain, double &offset, double &frequency ) const;
    /*! Set the gain factor for a signal 
        with carrier frequency \a frequency
	to \a gain and the offset to \a offset. */
  void setGain( double gain, double offset, double frequency );
    /*! Reset the gain factor and the offset for a signal 
        with carrier frequency \a frequency
	to the default values. */
  void reset( double frequency );

    /*! Set the sampling rate that was used while determining the gain()
        and offset() to \a rate. 
        It is just saved in the calibration file. */
  void setSamplingRate( double rate );

    /*! Load the calibration data. */
  void load( void );
    /*! Save the calibration data to \a file, using \a date. */
  void saveCalibration( const string &file, const string &date ) const;
    /*! Save the calibration data in directory \a path. */
  virtual void save( const string &path ) const;
    /*! Save the calibration data to the default file. */
  void save( void ) const;
    /*! Clear the calibration table. */
  void clear( void );

    /*! Set the device and the config identifier string to \a ident. */
  virtual void setDeviceIdent( const string &ident );

    /*! Load the configuration file. */
  virtual void config( void );


protected:

    /*! Transform the requested intensity \a intensity
        for the carrier frequency \a frequency of the signal into
        \a db decibel which are ued to set the attenuation level. */
  virtual int decibel( double intensity, double frequency, double &db );
    /*! Transform the attenuation level \a decibel
        for the carrier frequency \a frequency of the signal into
        the intesity \a intens. 
        This is the inverse function of decibel(). */
  virtual void intensity( double &intens, double frequency, double decibel );


private:

  vector< double > Frequency;
  vector< double > Gain;
  vector< double > Offset;

  string CalibFile;
  double DefaultGain;
  double DefaultOffset;
  double SamplingRate;
  mutable string CalibDate;

};

#endif
