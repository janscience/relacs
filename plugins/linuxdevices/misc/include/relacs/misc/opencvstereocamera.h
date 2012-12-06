/*
  misc/opencvstereocamera.h
  The Opencvstereocamera module 

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

#ifndef _RELACS_MISC_OPENCVSTEREOCAMERA_H_
#define _RELACS_MISC_OPENCVSTEREOCAMERA_H_ 1

#include <string>
#include <vector>
#include <cv.h>
#include <highgui.h>
#include <relacs/camera.h>
#include <QGraphicsScene>
#include <QGraphicsView>
using namespace std;
using namespace relacs;
using namespace cv;

namespace misc {




/*!
\class OpenCVStereoCamera
\author Fabian Sinz
\version 1.0
\brief [Camera] The %OpenCVStereoCamera module 
*/

class OpenCVStereoCamera : public Camera
{

 public:

  OpenCVStereoCamera( const string &device );
  OpenCVStereoCamera( void );
  ~OpenCVStereoCamera( void );

  

  virtual int open( const string &device, const Options &opts );
  virtual bool isOpen( void ) const {return Opened; };
  virtual void close( void );
  virtual int reset( void );

  bool isCalibrated( void ) const {return Calibrated; };
  string getLeftCamDevice( void ) const { return LeftCamDevice;};
  string getRightCamDevice( void ) const {return RightCamDevice;};
  Mat getFundamentalMatrix(void) const {return FundamentalMatrix;};


  void saveParameters(void);
  void setCalibrated(bool toWhat);

  void transformLeftToRight( Mat & q);
  void transformRightToLeft( Mat & q);

  int calibrate(vector< vector<Point3f> > ObjectPoints, vector< vector<Point2f> > ImagePoints[], Size sz);
  


 protected:
  bool Opened, Calibrated;
  string ParamFile,LeftCamDevice, RightCamDevice, LeftParamFile, RightParamFile;
 
  Mat IntrinsicMatrix[2];
  Mat DistortionCoeffs[2];
  Mat EssentialMatrix, FundamentalMatrix, RotationMatrix, TranslationMatrix;

 private:

  

};


}; /* namespace misc */

#endif /* ! _RELACS_MISC_OPENCVSTEREOCAMERA_H_ */