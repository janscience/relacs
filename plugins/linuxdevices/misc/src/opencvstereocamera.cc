/*
  misc/opencvstereocamera.cc
  The Opencvstereocamera module linear robot from MPH

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

// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <cmath>
#include <cstdio>
// #include <cstring>
#include <iostream>
#include <relacs/misc/opencvstereocamera.h>

#define length(a) ( sizeof ( a ) / sizeof ( *a ) )

using namespace std;
using namespace relacs;
using namespace cv;

namespace misc {


/*************************************************************************/


OpenCVStereoCamera::OpenCVStereoCamera( void )
  : Camera( "OpenCVStereoCamera" ){
  Opened = false;

  initOptions();
}



OpenCVStereoCamera::~OpenCVStereoCamera( void )
{
  close();
  Opened = false;
}


void OpenCVStereoCamera::initOptions()
{
  Camera::initOptions();

  addText("leftcam", "Left camera", "");
  addText("rightcam", "Right camera", "");
  addText("parameters", "Parameters", "");
  addText("leftparam", "Left parameters", "");
  addText("rightparam", "Right parameters", "");
}


int OpenCVStereoCamera::open( const string &device )
{
  clearError();
  Info.clear();
  Settings.clear();

  Opened = true;
  // // load camera number
  // CameraNo = atoi(opts.text("device").c_str());
  // Source = cvCaptureFromCAM( CameraNo );
  
  // Info.addInteger("device",CameraNo);
 
  LeftCamDevice =  text( "leftcam" );
  Info.addText("leftcam", LeftCamDevice);

  RightCamDevice =  text( "rightcam" );
  Info.addText("rightcam", RightCamDevice);
  
  ParamFile = text( "parameters" );
  Info.addText("parameters", ParamFile);

  LeftParamFile = text("leftparam");
  Info.addText("leftparam", LeftParamFile);

  RightParamFile = text("rightparam");
  Info.addText("rightparam", RightParamFile);

  // load all intrinsics and fundamental parameters 
  // if they all exist set to calibrated, else set to uncalibrated
  FileStorage fs;
  if (fs.open(ParamFile, FileStorage::READ)){
    fs["rotation"] >> RotationMatrix;
    fs["essential"] >> EssentialMatrix;
    fs["fundamental"] >> FundamentalMatrix;
    fs["translation"] >> TranslationMatrix;
    fs.release();
    Calibrated = true;

  }else{
    EssentialMatrix = Mat(3, 3, CV_64FC1);
    FundamentalMatrix =  Mat(3, 3, CV_64FC1);
    RotationMatrix  = Mat(3, 3, CV_64FC1);
    TranslationMatrix = Mat(3, 1, CV_64FC1);

    Calibrated = false;
  }
 


  return 0;
}


void OpenCVStereoCamera::close( void ){
  Opened = false;
  Info.clear();
  Settings.clear();
}


void OpenCVStereoCamera::transformLeftToRight( Mat& q){
  if (Calibrated){
    Mat R = RotationMatrix;
    R.convertTo(R,q.type());

    Mat T = TranslationMatrix;
    T.convertTo(T,q.type());

    q = q*R.t();
    Mat tmp;
    for (int j = 0; j != q.rows; ++j){
      tmp = q.row(j);
      tmp = tmp + T.t();
    }
  }else{
    cerr << "Stereo Camera must be calibrated to transform coordinates!" << endl;
  }

}


void OpenCVStereoCamera::transformRightToLeft( Mat& q){
  if (Calibrated){
    Mat R = RotationMatrix;
    R.convertTo(R,q.type());


    Mat T = TranslationMatrix;
    T.convertTo(T,q.type());

    Mat tmp;
    for (int j = 0; j != q.rows; ++j){
      tmp = q.row(j);
      tmp = tmp - T.t();
    }

    q = q*R;
  }else{
    cerr << "Stereo Camera must be calibrated to transform coordinates!" << endl;
  }

}


int OpenCVStereoCamera::calibrate(vector< vector<Point3f> > ObjectPoints, 
				  vector< vector<Point2f> > ImagePoints[], Size sz){

  FileStorage fs;
  if (fs.open(LeftParamFile, FileStorage::READ)){
    fs["intrinsic"] >> IntrinsicMatrix[0];
    fs["distortion"] >> DistortionCoeffs[0];
    fs.release();
  }
  if (fs.open(RightParamFile, FileStorage::READ)){
    fs["intrinsic"] >> IntrinsicMatrix[1];
    fs["distortion"] >> DistortionCoeffs[1];
    fs.release();
  }


  stereoCalibrate(ObjectPoints, ImagePoints[0], ImagePoints[1], IntrinsicMatrix[0], DistortionCoeffs[0], 
		  IntrinsicMatrix[1], DistortionCoeffs[1], sz, RotationMatrix, TranslationMatrix, 
		  EssentialMatrix, FundamentalMatrix, 
		  TermCriteria(TermCriteria::COUNT+  TermCriteria::EPS, 10000, 1e-15), CV_CALIB_FIX_INTRINSIC);
  saveParameters();

  return 0;
}
  
  



int OpenCVStereoCamera::reset( void ){
  return 0;
}

void OpenCVStereoCamera::setCalibrated(bool toWhat){
  Calibrated = toWhat;
}

void OpenCVStereoCamera::saveParameters(void){
  cerr << "OpenCVStereoCamera: Saving parameters to " << ParamFile << endl;

  FileStorage fs(ParamFile, FileStorage::WRITE);
  fs << "fundamental" << FundamentalMatrix;
  fs << "essential" << EssentialMatrix;
  fs << "rotation" << RotationMatrix;
  fs << "translation" << TranslationMatrix;
  fs.release();
}





}; /* namespace misc */
