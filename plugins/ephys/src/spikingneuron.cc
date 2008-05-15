/*
  ephys/spikingneuron.cc
  Base class for a spiking (point-) neuron.

  RELACS - RealTime ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2008 Jan Benda <j.benda@biologie.hu-berlin.de>

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
#include <relacs/ephys/spikingneuron.h>

namespace ephys {


SpikingNeuron::SpikingNeuron( void )
  : ConfigClass( "" ),
    Gain( 1.0 ),
    Offset( 0.0 )
{
  setConfigIdent( name() );
}


SpikingNeuron::~SpikingNeuron( void )
{
}


string SpikingNeuron::name( void ) const
{
  return "";
}


void SpikingNeuron::conductances( vector< string > &conductancenames ) const
{
  conductancenames.clear();
}


void SpikingNeuron::conductances( double *g ) const
{
}


string SpikingNeuron::conductanceUnit( void ) const
{
  return "mS/cm^2";
}


void SpikingNeuron::currents( vector< string > &currentnames ) const
{
  currentnames.clear();
}


void SpikingNeuron::currents( double *c ) const
{
}


string SpikingNeuron::currentUnit( void ) const
{
  return "uA/cm^2";
}


string SpikingNeuron::inputUnit( void ) const
{
  return "uA/cm^2";
}


void SpikingNeuron::add( void )
{
  addLabel( "Input", ScalingFlag );
  addNumber( "gain", "Gain", Gain, 0.0, 10000.0, 0.1 ).setFlags( ScalingFlag );
  addNumber( "offset", "Offset", Offset, -100000.0, 100000.0, 1.0, "muA/cm^2" ).setFlags( ScalingFlag );
}


void SpikingNeuron::notify( void )
{
  Gain = number( "gain" );
  Offset = number( "offset" );
}


double SpikingNeuron::gain( void ) const
{
  return Gain;
}


double SpikingNeuron::offset( void ) const
{
  return Offset;
}


Stimulus::Stimulus( void )
  : SpikingNeuron()
{
}


string Stimulus::name( void ) const
{
  return "Stimulus";
}


int Stimulus::dimension( void ) const
{
  return 1;
}


void Stimulus::variables( vector< string > &varnames ) const
{
  varnames.clear();
  varnames.reserve( dimension() );
  varnames.push_back( "Stimulus" );
}


void Stimulus::units( vector< string > &u ) const
{
  u.clear();
  u.reserve( dimension() );
  u.push_back( "" );
}


void Stimulus::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  x[0] = s;
  dxdt[0] = 0.0;
}


void Stimulus::init( double *x ) const
{
  x[0] = 0.0;
}



FitzhughNagumo::FitzhughNagumo( void )
  : SpikingNeuron()
{
  // from Koch, Biophysics of Computation, Chap.7.1
  Phi = 0.08; 
  A = 0.7;
  B = 0.8;
  TimeScale = 1.0;

  /*
  TimeScale = 0.2;
  Gain = 0.02;
  Offset = -5.0;
  Scale = 10.0;
  */

}


string FitzhughNagumo::name( void ) const
{
  return "Fitzhugh-Nagumo";
}


int FitzhughNagumo::dimension( void ) const
{
  return 2;
}


void FitzhughNagumo::variables( vector< string > &varnames ) const
{
  varnames.clear();
  varnames.reserve( dimension() );
  varnames.push_back( "V" );
  varnames.push_back( "W" );
}


void FitzhughNagumo::units( vector< string > &u ) const
{
  u.clear();
  u.reserve( dimension() );
  u.push_back( "1" );
  u.push_back( "1" );
}


void FitzhughNagumo::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  /* V */ dxdt[0] = (x[0]-x[0]*x[0]*x[0]/3.0-x[1]+s)/TimeScale;
  /* W */ dxdt[1] = Phi*(x[0]+A-B*x[1])/TimeScale;
}


void FitzhughNagumo::init( double *x ) const
{
  x[0] = -1.2;
  x[1] = -0.62;
}


void FitzhughNagumo::add( void )
{
  addLabel( "Parameter", ModelFlag );
  addNumber( "phi", "Phi", Phi, 0.0, 100.0, 0.1 ).setFlags( ModelFlag );
  addNumber( "a", "a", A, -100.0, 100.0, 0.1 ).setFlags( ModelFlag );
  addNumber( "b", "b", B, -100.0, 100.0, 0.1 ).setFlags( ModelFlag );

  SpikingNeuron::add();
  insertNumber( "timescale", "gain", "Timescale", TimeScale, 0.0, 1000.0, 0.001, "ms" ).setFlags( ScalingFlag );
}


void FitzhughNagumo::notify( void )
{
  SpikingNeuron::notify();
  Phi = number( "phi" );
  A = number( "a" );
  B = number( "b" );
  TimeScale = number( "timescale" );
}



MorrisLecar::MorrisLecar( void )
  : SpikingNeuron()
{
  // source??
  GCa = 4.0;
  GK = 8.0;
  GL = 2.0;

  ECa = +120.0;
  EK = -80.0;
  EL = -60.0;

  MVCa = -1.2;
  MKCa = 18.0;
  MVK = 12.0;
  MKK = 17.4;
  MPhiK = 0.067;

  C = 20.0;
  TimeScale = 10.0;
  Gain = 1.0;
  Offset = 40.0;

  GCaGates = GCa;
  GKGates = GK;

  ICa = 0.0;
  IK = 0.0;
  IL = 0.0;
}


string MorrisLecar::name( void ) const
{
  return "Morris-Lecar";
}


int MorrisLecar::dimension( void ) const
{
  return 2;
}


void MorrisLecar::variables( vector< string > &varnames ) const
{
  varnames.clear();
  varnames.reserve( dimension() );
  varnames.push_back( "V" );
  varnames.push_back( "w" );
}


void MorrisLecar::units( vector< string > &u ) const
{
  u.clear();
  u.reserve( dimension() );
  u.push_back( "mV" );
  u.push_back( "1" );
}


void MorrisLecar::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double m = 1.0/(1.0+exp(-2.0*(x[0]-MVCa)/MKCa));
  double w = 1.0/(1.0+exp(-2.0*(x[0]-MVK)/MKK));
  double tau = 1.0/(MPhiK*cosh((x[0]-MVK)/MKK));

  GCaGates = GCa*m;
  GKGates = GK*x[1];

  ICa = GCaGates*(x[0]-ECa);
  IK = GKGates*(x[0]-EK);
  IL = GL*(x[0]-EL);

  /* V */ dxdt[0] = TimeScale*(-ICa-IK-IL+s)/C;
  /* w */ dxdt[1] = TimeScale*(w-x[1])/tau;
}


void MorrisLecar::init( double *x ) const
{
  x[0] = -59.469;
  x[1] = 0.00027;
}


void MorrisLecar::conductances( vector< string > &conductancenames ) const
{
  conductancenames.clear();
  conductancenames.reserve( 3 );
  conductancenames.push_back( "g_Ca" );
  conductancenames.push_back( "g_K" );
  conductancenames.push_back( "g_l" );
}


void MorrisLecar::conductances( double *g ) const
{
  g[0] = GCaGates;
  g[1] = GKGates;
  g[2] = GL;
}


void MorrisLecar::currents( vector< string > &currentnames ) const
{
  currentnames.clear();
  currentnames.reserve( 3 );
  currentnames.push_back( "I_Ca" );
  currentnames.push_back( "I_K" );
  currentnames.push_back( "I_l" );
}


void MorrisLecar::currents( double *c ) const
{
  c[0] = ICa;
  c[1] = IK;
  c[2] = IL;
}


void MorrisLecar::add( void )
{
  addLabel( "General", ModelFlag );
  addSelection( "params", "Parameter set", "Custom|Type I|Type II" ).setFlags( ModelFlag );

  addLabel( "Calcium current", ModelFlag );
  addNumber( "gca", "Ca conductivity", GCa, 0.0, 10000.0, 0.1, "nS" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );
  addNumber( "eca", "Ca reversal potential", ECa, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );
  addNumber( "mvca", "Midpoint potential of Ca activation", MVCa, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );
  addNumber( "mkca", "Width of Ca activation", MKCa, 0.0, 1000.0, 1.0, "mV" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );

  addLabel( "Potassium current", ModelFlag );
  addNumber( "gk", "K conductivity", GK, 0.0, 10000.0, 0.1, "nS" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );
  addNumber( "ek", "K reversal potential", EK, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );
  addNumber( "mvk", "Midpoint potential of K activation", MVK, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );
  addNumber( "mkk", "Width of K activation", MKK, 0.0, 1000.0, 1.0, "mV" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );
  addNumber( "mphik", "Rate of K activation", MPhiK, 0.0, 10.0, 0.001, "kHz" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );

  addLabel( "Leak current", ModelFlag );
  addNumber( "gl", "Leak conductivity", GL, 0.0, 10000.0, 0.1, "nS" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );
  addNumber( "el", "Leak reversal potential", EL, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );
  addNumber( "c", "Capacitance", C, 0.0, 100.0, 0.1, "pF" ).setFlags( ModelFlag ).setActivation( "params", "Custom" );

  SpikingNeuron::add();
  insertNumber( "timescale", "gain", "Timescale", TimeScale, 0.0, 1000.0, 0.001 ).setFlags( ScalingFlag );
}


void MorrisLecar::notify( void )
{
  SpikingNeuron::notify();

  int params = index( "params" );

  if ( params == 1 ) {
    // Rinzel & Ermentrout, 1999 in Methods of Neural Modeling by Koch & Segev
    ECa = +120.0;
    GCa = 4.4;
    MVCa = -1.2;
    MKCa = 18.0;
    EK = -84.0;
    GK = 8.0;
    MVK = 12.0;
    MKK = 17.4;
    MPhiK = 0.0667;
    EL = -60.0;
    GL = 2.0;
    C = 20.0;
  }
  else if ( params == 2 ) {
    // Rinzel & Ermentrout, 1999 in Methods of Neural Modeling by Koch & Segev
    ECa = +120.0;
    GCa = 4.0;
    MVCa = -1.2;
    MKCa = 18.0;
    EK = -84.0;
    GK = 8.0;
    MVK = 2.0;
    MKK = 30.0;
    MPhiK = 0.04;
    EL = -60.0;
    GL = 2.0;
    C = 20.0;
  }
  else {
    ECa = number( "eca" );
    GCa = number( "gca" );
    MVCa = number( "mvca" );
    MKCa = number( "mkca" );
    EK = number( "ek" );
    GK = number( "gk" );
    MVK = number( "mvk" );
    MKK = number( "mkk" );
    MPhiK = number( "mphik" );
    EL = number( "el" );
    GL = number( "gl" );
    C = number( "c" );
  }
  TimeScale = number( "timescale" );
}


HodgkinHuxley::HodgkinHuxley( void )
  : SpikingNeuron()
{
  GNa = 120.0;
  GK = 36.0;
  GL = 0.3;

  ENa = +50.0;
  EK = -77.0;
  EL = -54.384;

  C = 1.0;
  PT = 1.0;

  GNaGates = GNa;
  GKGates = GK;

  INa= 0.0;
  IK= 0.0;
  IL= 0.0;
}


string HodgkinHuxley::name( void ) const
{
  return "Hodgkin-Huxley";
}


int HodgkinHuxley::dimension( void ) const
{
  return 4;
}


void HodgkinHuxley::variables( vector< string > &varnames ) const
{
  varnames.clear();
  varnames.reserve( dimension() );
  varnames.push_back( "V" );
  varnames.push_back( "m" );
  varnames.push_back( "h" );
  varnames.push_back( "n" );
}


void HodgkinHuxley::units( vector< string > &u ) const
{
  u.clear();
  u.reserve( dimension() );
  u.push_back( "mV" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
}


void HodgkinHuxley::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  GNaGates = GNa*x[1]*x[1]*x[1]*x[2];
  GKGates = GK*x[3]*x[3]*x[3]*x[3];

  INa = GNaGates*(x[0]-ENa);
  IK = GKGates*(x[0]-EK);
  IL = GL*(x[0]-EL);

  /* V */ dxdt[0] = (-INa-IK-IL+s)/C;
  /* m */ dxdt[1] = PT*( 0.1*(x[0]+40.0)/(1.0-exp(-(x[0]+40.0)/10.0))*(1.0-x[1]) - x[1]*4.0*exp(-(x[0]+65.0)/18.0) );
  /* h */ dxdt[2] = PT*( 0.07*exp(-(x[0]+65)/20.0)*(1.0-x[2]) - x[2]*1.0/(1.0+exp(-(x[0]+35.0)/10.0)) );
  /* n */ dxdt[3] = PT*( 0.01*(x[0]+55.0)/(1.0-exp(-(x[0]+55.0)/10.0))*(1.0-x[3]) - x[3]*0.125*exp(-(x[0]+65.0)/80.0) );
}


void HodgkinHuxley::init( double *x ) const
{
  x[0] = -65.0;
  x[1] = 0.053;
  x[2] = 0.596;
  x[3] = 0.318;
}


void HodgkinHuxley::conductances( vector< string > &conductancenames ) const
{
  conductancenames.clear();
  conductancenames.reserve( 3 );
  conductancenames.push_back( "g_Na" );
  conductancenames.push_back( "g_K" );
  conductancenames.push_back( "g_l" );
}


void HodgkinHuxley::conductances( double *g ) const
{
  g[0] = GNaGates;
  g[1] = GKGates;
  g[2] = GL;
}


void HodgkinHuxley::currents( vector< string > &currentnames ) const
{
  currentnames.clear();
  currentnames.reserve( 3 );
  currentnames.push_back( "I_Na" );
  currentnames.push_back( "I_K" );
  currentnames.push_back( "I_l" );
}


void HodgkinHuxley::currents( double *c ) const
{
  c[0] = INa;
  c[1] = IK;
  c[2] = IL;
}


void HodgkinHuxley::add( void )
{
  addLabel( "Sodium current", ModelFlag );
  addNumber( "gna", "Na conductivity", GNa, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "ena", "Na reversal potential", ENa, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );

  addLabel( "Potassium current", ModelFlag );
  addNumber( "gk", "K conductivity", GK, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "ek", "K reversal potential", EK, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );

  addLabel( "Leak current", ModelFlag );
  addNumber( "gl", "Leak conductivity", GL, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "el", "Leak reversal potential", EL, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  addNumber( "c", "Capacitance", C, 0.0, 100.0, 0.1, "muF/cm^2" ).setFlags( ModelFlag );
  addNumber( "phi", "Phi", PT, 0.0, 100.0, 1.0 ).setFlags( ModelFlag );

  SpikingNeuron::add();
}


void HodgkinHuxley::notify( void )
{
  SpikingNeuron::notify();
  ENa = number( "ena" );
  GNa = number( "gna" );
  EK = number( "ek" );
  GK = number( "gk" );
  EL = number( "el" );
  GL = number( "gl" );
  C = number( "c" );
  PT = number( "phi" );
}


Connor::Connor( void )
  : HodgkinHuxley()
{
  GNa = 120.0;
  GK = 20.0;
  GKA = 47.0;
  GL = 0.3;

  ENa = +50.0;
  EK = -77.0;
  EKA = -80.0;
  EL = -22.0;

  C = 1.0;
  PT = 1.0;

  GNaGates = GNa;
  GKGates = GK;
  GKAGates = GKA;

  INa= 0.0;
  IK= 0.0;
  IKA = 0.0;
  IL= 0.0;
}


string Connor::name( void ) const
{
  return "Connor";
}


int Connor::dimension( void ) const
{
  return 6;
}


void Connor::variables( vector< string > &varnames ) const
{
  HodgkinHuxley::variables( varnames );
  varnames.push_back( "a" );
  varnames.push_back( "b" );
}


void Connor::units( vector< string > &u ) const
{
  HodgkinHuxley::units( u );
  u.push_back( "1" );
  u.push_back( "1" );
}


void Connor::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double V = x[0];

  GNaGates = GNa*x[1]*x[1]*x[1]*x[2];
  GKGates = GK*x[3]*x[3]*x[3]*x[3];
  GKAGates = GKA*x[4]*x[4]*x[4]*x[5];

  INa = GNaGates*(V-ENa);
  IK = GKGates*(V-EK);
  IKA = GKAGates*(V-EKA);
  IL = GL*(V-EL);

  /* V */ dxdt[0] = (-INa-IK-IKA-IL+s)/C;
  /* m */ dxdt[1] = PT*( 0.1*(V+40.0)/(1.0-exp(-(V+40.0)/10.0))*(1.0-x[1]) - x[1]*4.0*exp(-(V+65.0)/18.0) );
  /* h */ dxdt[2] = PT*( 0.07*exp(-(V+65)/20.0)*(1.0-x[2]) - x[2]*1.0/(1.0+exp(-(V+35.0)/10.0)) );
  /* n */ dxdt[3] = PT*( 0.01*(V+55.0)/(1.0-exp(-(V+55.0)/10.0))*(1.0-x[3]) - x[3]*0.125*exp(-(V+65.0)/80.0) );
  /* a */ dxdt[4] = (pow(0.0761*exp((V+99.22)/31.84)/(1.0+exp((V+6.17)/28.93)),1.0/3.0)-x[4])/(0.3632+1.158/(1.0+exp((V+60.96)/20.12)));
  /* b */ dxdt[5] = (1.0/(pow(1.0+exp((V+58.3)/14.54),4.0))-x[5])/(1.24+2.678/(1.0+exp((V+55.0)/16.072)));
}


void Connor::init( double *x ) const
{
  x[0] = -72.975;
  x[1] = 0.01008;
  x[2] = 0.9659;
  x[3] = 0.15589;
  x[4] = 0.54044;
  x[5] = 0.28848;
}


void Connor::conductances( vector< string > &conductancenames ) const
{
  conductancenames.clear();
  conductancenames.reserve( 4 );
  conductancenames.push_back( "g_Na" );
  conductancenames.push_back( "g_K" );
  conductancenames.push_back( "g_KA" );
  conductancenames.push_back( "g_l" );
}


void Connor::conductances( double *g ) const
{
  g[0] = GNaGates;
  g[1] = GKGates;
  g[2] = GKAGates;
  g[3] = GL;
}


void Connor::currents( vector< string > &currentnames ) const
{
  currentnames.clear();
  currentnames.reserve( 4 );
  currentnames.push_back( "I_Na" );
  currentnames.push_back( "I_K" );
  currentnames.push_back( "I_KA" );
  currentnames.push_back( "I_l" );
}


void Connor::currents( double *c ) const
{
  c[0] = INa;
  c[1] = IK;
  c[2] = IKA;
  c[3] = IL;
}


void Connor::add( void )
{
  HodgkinHuxley::add();

  insertLabel( "A current", "Leak current", ModelFlag );
  insertNumber( "gka", "Leak current", "A conductivity", GKA, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  insertNumber( "eka", "Leak current", "A reversal potential", EKA, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );

  SpikingNeuron::add();
}


void Connor::notify( void )
{
  SpikingNeuron::notify();
  HodgkinHuxley::notify();
  EKA = number( "eka" );
  GKA = number( "gka" );
}


RushRinzel::RushRinzel( void )
  : Connor()
{
  GNa = 120.0;
  GK = 20.0;
  GKA = 60.0;
  GL = 0.315; // ????

  ENa = +50.0;
  EK = -72.0;
  EKA = -72.0;
  EL = -17.0;

  AV0 = -75.0;
  ADV = -50.0;
  BV0 = -70.0;
  BDV = 6.0;
  BTau = 1.0;

  C = 1.0;
  PT = 3.82;

  GNaGates = GNa;
  GKGates = GK;
  GKAGates = GKA;

  INa= 0.0;
  IK= 0.0;
  IKA = 0.0;
  IL= 0.0;
}


string RushRinzel::name( void ) const
{
  return "Rush-Rinzel";
}


int RushRinzel::dimension( void ) const
{
  return 3;
}


void RushRinzel::variables( vector< string > &varnames ) const
{
  varnames.clear();
  varnames.reserve( dimension() );
  varnames.push_back( "V" );
  varnames.push_back( "n" );
  varnames.push_back( "b" );
}


void RushRinzel::units( vector< string > &u ) const
{
  u.clear();
  u.reserve( dimension() );
  u.push_back( "mV" );
  u.push_back( "1" );
  u.push_back( "1" );
}


void RushRinzel::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double V = x[0];

  double am = 0.1*(V+35.0)/(1.0-exp(-0.1*(V+35)));
  double bm = 4.0*exp(-0.05*(V+60.0));
  double m0 = am/(am+bm);

  double an = 0.01*(V+50.01)/(1.0-exp(-0.1*(V+50.01)));
  double bn = 0.125*exp( -0.0125*(V+60));
  double n0 = an/(an+bn);
  double tn = 1.0/(an+bn);

  double a0 = 1.0/(1.0+exp((V-AV0)/ADV));
  double b0 = 1.0/(1.0+exp((V-BV0)/BDV));
  double tb = BTau;

  GNaGates = GNa*m0*m0*m0*(0.9-1.2*x[1]);
  GKGates = GK*x[1]*x[1]*x[1]*x[1];
  GKAGates = GKA*a0*a0*a0*x[2];

  INa = GNaGates*(V-ENa);
  IK = GKGates*(V-EK);
  IKA = GKAGates*(V-EKA);
  IL = GL*(V-EL);

  /* V */ dxdt[0] = (-INa-IK-IKA-IL+s)/C;
  /* n */ dxdt[1] = PT*( n0 - x[1] )/tn;
  /* b */ dxdt[2] = (b0 - x[2])/tb;
}


void RushRinzel::init( double *x ) const
{
  x[0] = -67.78;
  x[1] = 0.207863;
  x[2] = 0.408565;
}


void RushRinzel::add( void )
{
  Connor::add();

  insertNumber( "av0", "Leak current", "Midpoint potential of a-gate", AV0, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  insertNumber( "adv", "Leak current", "Dynamic range of a-gate", ADV, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  insertNumber( "bv0", "Leak current", "Midpoint potential of b-gate", BV0, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  insertNumber( "bdv", "Leak current", "Dynamic range of b-gate", BDV, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  insertNumber( "btau", "Leak current", "Time constant of b-gate", BTau, 0.0, 1000.0, 0.2, "ms" ).setFlags( ModelFlag );

  SpikingNeuron::add();
}


void RushRinzel::notify( void )
{
  SpikingNeuron::notify();
  Connor::notify();
  AV0 = number( "av0" );
  ADV = number( "adv" );
  BV0 = number( "bv0" );
  BDV = number( "bdv" );
  BTau = number( "btau" );
}


Awiszus::Awiszus( void )
  : Connor()
{
  GNa = 240.0;
  GK = 36.0;
  GKA = 61.0;
  GL = 0.068;

  ENa = +64.7;
  EK = -95.2;
  EKA = -95.2;
  EL = -51.3;

  C = 1.0;

  GNaGates = GNa;
  GKGates = GK;
  GKAGates = GKA;

  INa= 0.0;
  IK= 0.0;
  IKA = 0.0;
  IL= 0.0;
}


string Awiszus::name( void ) const
{
  return "Awiszus";
}


void Awiszus::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double V = x[0];
  
  double z = -(53.0+V)/6.0;
  double am = fabs( z ) < 1e-4 ? 11.3 : 11.3*z/(exp(z)-1.0);
  z = (57.0+V)/9.0;
  double bm = fabs( z ) < 1e-4 ? 37.4 : 37.4*z/(exp(z)-1.0);
  z = (V+106.0)/9.0;
  double ah = fabs( z ) < 1e-4 ? 5.0 : 5.0*z/(exp(z)-1.0);

  GNaGates = GNa*x[1]*x[1]*x[1]*x[2];
  GKGates = GK*x[3]*x[3]*x[3];
  GKAGates = GKA*x[4]*x[4]*x[4]*x[4]*x[5];

  INa = GNaGates*(V-ENa);
  IK = GKGates*(V-EK);
  IKA = GKAGates*(V-EKA);
  IL = GL*(V-EL);

  /* V */ dxdt[0] = (-INa-IK-IKA-IL+s)/C;
  /* m */ dxdt[1] = (1-x[1])*am-x[1]*bm;
  /* h */ dxdt[2] = (1-x[2])*ah-x[2]*22.6/(exp(-(V+22.0)/12.5)+1.0);
  /* n */ dxdt[3] = (1.0/(1.0+exp((1.7-V)/11.4))-x[3])/(0.24+0.7/(1.0+exp((V+12.0)/16.4)));
  /* a */ dxdt[4] = (1.0/(1.0+exp(-(55+V)/13.8))-x[4])/(0.12+0.6/(1.0+exp((V+24)/16.5)));
  /* b */ dxdt[5] = (1.0/(1.0+exp((77.0+V)/7.8))-x[5])/(2.1+1.8/(1.0+exp((V-18.0)/5.7)));
}


void Awiszus::init( double *x ) const
{
  x[0] = -67.78;
  x[1] = 0.207863;
  x[2] = 0.408565;
}


FleidervishSI::FleidervishSI( void )
  : HodgkinHuxley()
{
  GNa = 10.0;
  GK = 1.5;
  GL = 0.008;

  ENa = +50.0;
  EK = -90.0;
  EL = -70.0;

  GNa = 120.0;
  GK = 36.0;
  GL = 0.3;

  ENa = +50.0;
  EK = -77.0;
  EL = -54.384;

  C = 1.0;
  PT = 1.0;

  GNaGates = GNa;
  GKGates = GK;

  INa = 0.0;
  IK = 0.0;
  IL = 0.0;
}


string FleidervishSI::name( void ) const
{
  return "Fleidervish";
}


int FleidervishSI::dimension( void ) const
{
  return 5;
}


void FleidervishSI::variables( vector< string > &varnames ) const
{
  HodgkinHuxley::variables( varnames );
  varnames.push_back( "s" );
}


void FleidervishSI::units( vector< string > &u ) const
{
  HodgkinHuxley::units( u );
  u.push_back( "1" );
}


void FleidervishSI::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double V = x[0];

  GNaGates = GNa*x[1]*x[1]*x[1]*x[2]*x[4];
  GKGates = GK*x[3]*x[3]*x[3]*x[3];

  INa = GNaGates*(V-ENa);
  IK = GKGates*(V-EK);
  IL = GL*(V-EL);

  /* V */ dxdt[0] = ( - INa - IK - IL + s )/C;
  /* m */ dxdt[1] = (1-x[1])*0.091*(V+40.0)/(1.0-exp(-(V+40.0)/5.0))-x[1]*(-0.062)*(V+40.0)/(1.0-exp((V+40)/5.0));
  /* h */ dxdt[2] = (1-x[2])*0.06*exp(-(55.0+V)/15.0)-x[2]*6.01/(exp((17.0-V)/21.0)+1.0);
  /* n */ dxdt[3] = (1-x[3])*0.034*(V+45)/(1.0-exp(-(V+45.0)/5.0))-x[3]*0.54*exp(-(75+V)/40);
  /* s */ dxdt[4] = (1-x[4])*0.001*exp(-(85.0+V)/30.0)-x[4]*0.0034/(exp(-(17.0+V)/10.0)+1.0);
}


void FleidervishSI::init( double *x ) const
{
  x[0] = -72.975;
  x[1] = 0.01008;
  x[2] = 0.9659;
  x[3] = 0.15589;
  x[4] = 0.54044;
}


TraubHH::TraubHH( void )
  : HodgkinHuxley()
{
  GNa = 100.0;
  GK = 200.0;
  GL = 0.1;

  ENa = +48.0;
  EK = -82.0;
  EL = -67.0;

  C = 1.0;
  PT = 1.0;

  GNaGates = GNa;
  GKGates = GK;

  INa = 0.0;
  IK = 0.0;
  IL = 0.0;
}


string TraubHH::name( void ) const
{
  return "Traub-Miles, HH currents only";
}


void TraubHH::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double V = x[0];

  double am = 0.32*(V+54.0)/(1.0-exp(-(V+54.0)/4));
  double bm = 0.28*(V+27.0)/(exp((V+27.0)/5.0)-1.0);

  double ah = 0.128*exp(-(V+50.0)/18.0);
  double bh = 4.0/(1.0+exp(-(V+27.0)/5.0));
  
  double an = 0.032*(V+52.0)/(1-exp(-(V+52.0)/5.0));
  double bn = 0.5*exp(-(V+57.0)/40.0);

  GNaGates = GNa*x[1]*x[1]*x[1]*x[2];
  GKGates = GK*x[3]*x[3]*x[3]*x[3];

  INa = GNaGates*(V-ENa);
  IK = GKGates*(V-EK);
  IL = GL*(V-EL);

  /* V */ dxdt[0] = ( - INa - IK - IL + s )/C;
  /* m */ dxdt[1] = PT*( am*(1.0-x[1]) - x[1]*bm );
  /* h */ dxdt[2] = PT*( ah*(1.0-x[2]) - x[2]*bh );
  /* n */ dxdt[3] = PT*( an*(1.0-x[3]) - x[3]*bn );
}


void TraubHH::init( double *x ) const
{
  x[0] = -38.6761;
  x[1] = 0.580671;
  x[2] = 0.161987;
  x[3] = 0.591686;
}


TraubMiles::TraubMiles( void )
  : HodgkinHuxley()
{
  GNa = 100.0;
  GK = 200.0;
  GL = 0.1;
  GCa = 119.9;
  GAHP = 3.01;

  ENa = +48.0;
  EK = -82.0;
  EL = -67.0;
  ECa = +73.0;
  EAHP = -82.0;

  C = 1.0;
  PT = 1.0;

  GNaGates = GNa;
  GKGates = GK;
  GCaGates = GCa;
  GAHPGates = GAHP;

  INa = 0.0;
  IK = 0.0;
  IL = 0.0;
  ICa = 0.0;
  IAHP = 0.0;
}


string TraubMiles::name( void ) const
{
  return "Traub-Miles";
}


int TraubMiles::dimension( void ) const
{
  return 9;
}


void TraubMiles::variables( vector< string > &varnames ) const
{
  varnames.clear();
  varnames.reserve( dimension() );
  varnames.push_back( "V" );
  varnames.push_back( "m" );
  varnames.push_back( "h" );
  varnames.push_back( "n" );
  varnames.push_back( "y" );
  varnames.push_back( "s" );
  varnames.push_back( "r" );
  varnames.push_back( "q" );
  varnames.push_back( "[Ca]" );
}


void TraubMiles::units( vector< string > &u ) const
{
  u.clear();
  u.reserve( dimension() );
  u.push_back( "mV" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "mM" );
}


void TraubMiles::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double V = x[0];
  double Ca = x[8];

  double am = 0.32*(V+54.0)/(1.0-exp(-(V+54.0)/4.0));
  double bm = 0.28*(V+27.0)/(exp((V+27.0)/5.0)-1.0);

  double ah = 0.128*exp(-(V+50.0)/18.0);
  double bh = 4.0/(1.0+exp(-(V+27.0)/5.0));
  
  double an = 0.032*(V+52.0)/(1-exp(-(V+52.0)/5.0));
  double bn = 0.5*exp(-(V+57.0)/40.0);

  double ay = 0.028*exp(-(V+52.0)/15.0)+2.0/(1.0+exp(-0.1*(V-18.0)));
  double by = 0.4/(1.0+exp(-0.1*(V+27.0)));

  double as = 0.04*(V+7.0)/(1.0-exp(-0.1*(V+7.0)));
  double bs = 0.005*(V+22.0)/(exp(0.1*(V+22.0))-1.0);

  double ar = 0.005;
  double br = 0.025*(200.0-Ca)/(exp((200.0-Ca)/20.0)-1.0);

  double aq = exp((V+67.0)/27.0)*0.005*(200.0-Ca)/(exp((200.0-Ca)/20.0)-1.0);
  double bq = 0.002;

  GNaGates = GNa*x[1]*x[1]*x[1]*x[2];
  GKGates = GK*x[3]*x[3]*x[3]*x[3]*x[4];
  GCaGates = GCa*x[5]*x[5]*x[5]*x[5]*x[5]*x[6];
  GAHPGates = GAHP*x[7];

  INa = GNaGates*(V-ENa);
  IK = GKGates*(V-EK);
  IL = GL*(V-EL);
  ICa = GCaGates*(V-ECa);
  IAHP = GAHPGates*(V-EAHP);

  /* V */ dxdt[0] = ( - INa - IK - IL - ICa - IAHP + s )/C;
  /* m */ dxdt[1] = am*(1.0-x[1]) - x[1]*bm;
  /* h */ dxdt[2] = ah*(1.0-x[2]) - x[2]*bh;
  /* n */ dxdt[3] = an*(1.0-x[3]) - x[3]*bn;
  /* y */ dxdt[4] = ay*(1.0-x[4]) - x[4]*by;
  /* s */ dxdt[5] = as*(1.0-x[5]) - x[5]*bs;
  /* r */ dxdt[6] = ar*(1.0-x[6]) - x[6]*br;
  /* q */ dxdt[7] = aq*(1.0-x[7]) - x[7]*bq;
  /* Ca */dxdt[8] = -0.002*ICa - 0.0125*x[6];
}


void TraubMiles::init( double *x ) const
{
  x[0] = -66.61;
  x[1] = 0.015995;
  x[2] = 0.995513;
  x[3] = 0.040180;
  x[4] = 0.908844;
  x[5] = 0.026259;
  x[6] = 0.138319;
  x[7] = 0.760006;
  x[8] = 115.0;
}


void TraubMiles::conductances( vector< string > &conductancenames ) const
{
  conductancenames.clear();
  conductancenames.reserve( 5 );
  conductancenames.push_back( "g_Na" );
  conductancenames.push_back( "g_K" );
  conductancenames.push_back( "g_l" );
  conductancenames.push_back( "g_Ca" );
  conductancenames.push_back( "g_AHP" );
}


void TraubMiles::conductances( double *g ) const
{
  g[0] = GNaGates;
  g[1] = GKGates;
  g[2] = GL;
  g[3] = GCaGates;
  g[4] = GAHPGates;
}


void TraubMiles::currents( vector< string > &currentnames ) const
{
  currentnames.clear();
  currentnames.reserve( 5 );
  currentnames.push_back( "I_Na" );
  currentnames.push_back( "I_K" );
  currentnames.push_back( "I_l" );
  currentnames.push_back( "I_Ca" );
  currentnames.push_back( "I_AHP" );
}


void TraubMiles::currents( double *c ) const
{
  c[0] = INa;
  c[1] = IK;
  c[2] = IL;
  c[3] = ICa;
  c[4] = IAHP;
}


void TraubMiles::add( void )
{
  HodgkinHuxley::add();

  insertLabel( "Calcium current", "Input", ModelFlag );
  insertNumber( "gca", "Input", "Ca conductivity", GCa, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  insertNumber( "eca", "Input", "Ca reversal potential", ECa, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );

  insertLabel( "AHP-type current", "Input", ModelFlag );
  insertNumber( "gahp", "Input", "AHP conductivity", GAHP, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  insertNumber( "eahp", "Input", "AHP reversal potential", EAHP, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
}


void TraubMiles::notify( void )
{
  HodgkinHuxley::notify();

  ECa = number( "eca" );
  GCa = number( "gca" );
  EAHP = number( "eahp" );
  GAHP = number( "gahp" );
}


TraubErmentrout::TraubErmentrout( void )
  : HodgkinHuxley()
{
  GNa = 100.0;
  GK = 80.0;
  GL = 0.1;
  GCa = 5.0;
  GM = 8.0;
  GAHP = 4.0;

  ENa = +50.0;
  EK = -100.0;
  EL = -67.0;
  ECa = +120.0;
  EM = -100.0;
  EAHP = -100.0;

  TauW = 100.0;
  C = 1.0;
  PT = 1.0;

  GNaGates = GNa;
  GKGates = GK;
  GCaGates = GCa;
  GMGates = GM;
  GAHPGates = GAHP;

  INa = 0.0;
  IK = 0.0;
  IL = 0.0;
  ICa = 0.0;
  IM = 0.0;
  IAHP = 0.0;
}


string TraubErmentrout::name( void ) const
{
  return "Traub-Miles-Ermentrout";
}


int TraubErmentrout::dimension( void ) const
{
  return 8;
}


void TraubErmentrout::variables( vector< string > &varnames ) const
{
  varnames.clear();
  varnames.reserve( dimension() );
  varnames.push_back( "V" );
  varnames.push_back( "m" );
  varnames.push_back( "h" );
  varnames.push_back( "n" );
  varnames.push_back( "s" );
  varnames.push_back( "w" );
  varnames.push_back( "q" );
  varnames.push_back( "[Ca]" );
}


void TraubErmentrout::units( vector< string > &u ) const
{
  u.clear();
  u.reserve( dimension() );
  u.push_back( "mV" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "mM" );
}


void TraubErmentrout::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double V = x[0];
  double Ca = x[7];

  double am = 0.32*(V+54.0)/(1.0-exp(-(V+54.0)/4));
  double bm = 0.28*(V+27.0)/(exp((V+27.0)/5.0)-1.0);

  double ah = 0.128*exp(-(V+50.0)/18.0);
  double bh = 4.0/(1.0+exp(-(V+27.0)/5.0));
  
  double an = 0.032*(V+52.0)/(1-exp(-(V+52.0)/5.0));
  double bn = 0.5*exp(-(V+57.0)/40.0);

  x[4] = 1.0/(1.0+exp(-(V+25.0)/5.0));
  x[6] = Ca/(30.0+Ca);

  GNaGates = GNa*x[1]*x[1]*x[1]*x[2];
  GKGates = GK*x[3]*x[3]*x[3]*x[3];
  GCaGates = GCa*x[4];
  GMGates = GM*x[5];
  GAHPGates = GAHP*x[6];

  INa = GNaGates*(V-ENa);
  IK = GKGates*(V-EK);
  IL = GL*(V-EL);
  ICa = GCaGates*(V-ECa);
  IM = GMGates*(V-EM);
  IAHP = GAHPGates*(V-EAHP);

  /* V */ dxdt[0] = ( - INa - IK - IL - ICa - IM - IAHP + s )/C;
  /* m */ dxdt[1] = am*(1.0-x[1]) - x[1]*bm;
  /* h */ dxdt[2] = ah*(1.0-x[2]) - x[2]*bh;
  /* n */ dxdt[3] = an*(1.0-x[3]) - x[3]*bn;
  /* s */ dxdt[4] = 0.0;
  /* w */ dxdt[5] = (1.0/(1.0+exp(-(V+20.0)/5.0)) - x[5])/TauW;
  /* q */ dxdt[6] = 0.0;
  /* Ca */ dxdt[7] = -0.002*ICa - 0.0125*Ca;
}


void TraubErmentrout::init( double *x ) const
{
  x[0] = -66.01;
  x[1] = 0.018030;
  x[2] = 0.994788;
  x[3] = 0.044163;
  x[4] = 0.000274;
  x[5] = 0.000137;
  x[6] = 0.001291;
  x[7] = 0.038781;
}


void TraubErmentrout::conductances( vector< string > &conductancenames ) const
{
  conductancenames.clear();
  conductancenames.reserve( 6 );
  conductancenames.push_back( "g_Na" );
  conductancenames.push_back( "g_K" );
  conductancenames.push_back( "g_l" );
  conductancenames.push_back( "g_Ca" );
  conductancenames.push_back( "g_M" );
  conductancenames.push_back( "g_AHP" );
}


void TraubErmentrout::conductances( double *g ) const
{
  g[0] = GNaGates;
  g[1] = GKGates;
  g[2] = GL;
  g[3] = GCaGates;
  g[4] = GMGates;
  g[5] = GAHPGates;
}


void TraubErmentrout::currents( vector< string > &currentnames ) const
{
  currentnames.clear();
  currentnames.reserve( 6 );
  currentnames.push_back( "I_Na" );
  currentnames.push_back( "I_K" );
  currentnames.push_back( "I_l" );
  currentnames.push_back( "I_Ca" );
  currentnames.push_back( "I_M" );
  currentnames.push_back( "I_AHP" );
}


void TraubErmentrout::currents( double *c ) const
{
  c[0] = INa;
  c[1] = IK;
  c[2] = IL;
  c[3] = ICa;
  c[4] = IM;
  c[5] = IAHP;
}


void TraubErmentrout::add( void )
{
  HodgkinHuxley::add();

  insertLabel( "Calcium current", "Input", ModelFlag );
  insertNumber( "gca", "Input", "Ca conductivity", GCa, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  insertNumber( "eca", "Input", "Ca reversal potential", ECa, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );

  insertLabel( "M-type current", "Input", ModelFlag );
  insertNumber( "gm", "Input", "M conductivity", GM, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  insertNumber( "em", "Input", "M reversal potential", EM, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  insertNumber( "tauw", "Input", "W time constant", TauW, 0.0, 1000.0, 1.0, "ms" ).setFlags( ModelFlag );

  insertLabel( "AHP-type current", "Input", ModelFlag );
  insertNumber( "gahp", "Input", "AHP conductivity", GAHP, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  insertNumber( "eahp", "Input", "AHP reversal potential", EAHP, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
}


void TraubErmentrout::notify( void )
{
  HodgkinHuxley::notify();

  ECa = number( "eca" );
  GCa = number( "gca" );
  EM = number( "em" );
  GM = number( "gm" );
  TauW = number( "tauw" );
  EAHP = number( "eahp" );
  GAHP = number( "gahp" );
}


TraubErmentroutNaSI::TraubErmentroutNaSI( void )
  : TraubErmentrout()
{
}


string TraubErmentroutNaSI::name( void ) const
{
  return "Traub-Miles-Ermentrout NaSI";
}


int TraubErmentroutNaSI::dimension( void ) const
{
  return 9;
}


void TraubErmentroutNaSI::variables( vector< string > &varnames ) const
{
  varnames.clear();
  varnames.reserve( dimension() );
  varnames.push_back( "V" );
  varnames.push_back( "m" );
  varnames.push_back( "h" );
  varnames.push_back( "l" );
  varnames.push_back( "n" );
  varnames.push_back( "s" );
  varnames.push_back( "w" );
  varnames.push_back( "q" );
  varnames.push_back( "[Ca]" );
}


void TraubErmentroutNaSI::units( vector< string > &u ) const
{
  u.clear();
  u.reserve( dimension() );
  u.push_back( "mV" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "mM" );
}


void TraubErmentroutNaSI::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double dl=0.3;
  double zl=-3.5;
  double vl=0.0;
  double tlmax=1700.0;
  double Vl=-53.0;
  double T=291.0;  /* K, 18GradCelsius */
  double e=1.60217653e-19; /* C   */
  double k=1.3806505e-23; /*  J/K */
  double eKT=0.001*e/k/T; /*  */

  double V = x[0];
  double Ca = x[8];

  double am = 0.32*(V+54.0)/(1.0-exp(-(V+54.0)/4));
  double bm = 0.28*(V+27.0)/(exp((V+27.0)/5.0)-1.0);

  double ah = 0.128*exp(-(V+50.0)/18.0);
  double bh = 4.0/(1.0+exp(-(V+27.0)/5.0));
  
  double an = 0.032*(V+52.0)/(1-exp(-(V+52.0)/5.0));
  double bn = 0.5*exp(-(V+57.0)/40.0);

  double tl = tlmax*(pow( (1.0-dl)/dl, dl ) + pow( (1.0-dl)/dl, dl-1.0 ))/(exp(dl*zl*eKT*(V-Vl))+exp((dl-1.0)*zl*eKT*(V-Vl)));
  double ls = vl+(1.0-vl)/(1.0+exp(-zl*eKT*(V-Vl)));

  x[5] = 1.0/(1.0+exp(-(V+25.0)/5.0));
  x[7] = Ca/(30.0+Ca);

  GNaGates = GNa*x[1]*x[1]*x[1]*x[2]*x[3];
  GKGates = GK*x[4]*x[4]*x[4]*x[4];
  GCaGates = GCa*x[5];
  GMGates = GM*x[6];
  GAHPGates = GAHP*x[7];

  INa = GNaGates*(V-ENa);
  IK = GKGates*(V-EK);
  IL = GL*(V-EL);
  ICa = GCaGates*(V-ECa);
  IM = GMGates*(V-EM);
  IAHP = GAHPGates*(V-EAHP);

  /* V */ dxdt[0] = ( - INa - IK - IL - ICa - IM - IAHP + s )/C;
  /* m */ dxdt[1] = am*(1.0-x[1]) - x[1]*bm;
  /* h */ dxdt[2] = ah*(1.0-x[2]) - x[2]*bh;
  /* l */ dxdt[3] = ( ls - x[3] ) / tl;
  /* n */ dxdt[4] = an*(1.0-x[4]) - x[4]*bn;
  /* s */ dxdt[5] = 0.0;
  /* w */ dxdt[6] = (1.0/(1.0+exp(-(V+20.0)/5.0)) - x[6])/TauW;
  /* q */ dxdt[7] = 0.0;
  /* Ca */ dxdt[8] = -0.002*ICa - 0.0125*Ca;
}


void TraubErmentroutNaSI::init( double *x ) const
{
  x[0] = -66.01;
  x[1] = 0.018030;
  x[2] = 0.994788;
  x[3] = 1.0;
  x[4] = 0.044163;
  x[5] = 0.000274;
  x[6] = 0.000137;
  x[7] = 0.001291;
  x[8] = 0.038781;
}


WangBuzsaki::WangBuzsaki( void )
  : HodgkinHuxley()
{
  GNa = 35.0;
  GK = 9.0;
  GL = 0.1;

  ENa = +55.0;
  EK = -90.0;
  EL = -65.0;

  C = 1.0;
  PT = 5.0;

  GNaGates = GNa;
  GKGates = GK;

  INa = 0.0;
  IK = 0.0;
  IL = 0.0;
}


string WangBuzsaki::name( void ) const
{
  return "Wang-Buzsaki";
}


int WangBuzsaki::dimension( void ) const
{
  return 3;
}


void WangBuzsaki::variables( vector< string > &varnames ) const
{
  varnames.clear();
  varnames.reserve( dimension() );
  varnames.push_back( "V" );
  varnames.push_back( "h" );
  varnames.push_back( "n" );
}


void WangBuzsaki::units( vector< string > &u ) const
{
  u.clear();
  u.reserve( dimension() );
  u.push_back( "mV" );
  u.push_back( "1" );
  u.push_back( "1" );
}


void WangBuzsaki::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double ms = 1.0/(1.0+4.0*exp(-(x[0]+60.0)/18.0)*(exp(-0.1*(x[0]+35.0))-1.0)/(-0.1*(x[0]+35.0)));

  GNaGates = GNa*ms*ms*ms*x[1];
  GKGates = GK*x[2]*x[2]*x[2]*x[2];

  INa = GNaGates*(x[0]-ENa);
  IK = GKGates*(x[0]-EK);
  IL = GL*(x[0]-EL);

  /* V */ dxdt[0] = (-INa-IK-IL+s)/C;
  /* h */ dxdt[1] = PT*(0.07*exp(-(x[0]+58)/20)*(1.0-x[1])-x[1]/(exp(-0.1*(x[0]+28))+1));
  /* n */ dxdt[2] = PT*(-0.01*(x[0]+34.0)*(1.0-x[2])/(exp(-0.1*(x[0]+34.0))-1)-0.125*exp(-(x[0]+44.0)/80.0)*x[2]);
}


void WangBuzsaki::init( double *x ) const
{
  x[0] = -64.018;
  x[1] = 0.7808;
  x[2] = 0.0891;
}


WangBuzsakiAdapt::WangBuzsakiAdapt( void )
  : WangBuzsaki()
{
  EA = -90.0;

  GA = 0.8;

  Atau = 100.0;

  GAGates = GA;
  IA = 0.0;
}


string WangBuzsakiAdapt::name( void ) const
{
  return "Wang-Buzsaki Adapt";
}


int WangBuzsakiAdapt::dimension( void ) const
{
  return 4;
}


void WangBuzsakiAdapt::variables( vector< string > &varnames ) const
{
  WangBuzsaki::variables( varnames );
  varnames.push_back( "a" );
}


void WangBuzsakiAdapt::units( vector< string > &u ) const
{
  WangBuzsaki::units( u );
  u.push_back( "1" );
}


void WangBuzsakiAdapt::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double ms = 1.0/(1.0+4.0*exp(-(x[0]+60.0)/18.0)*(exp(-0.1*(x[0]+35.0))-1.0)/(-0.1*(x[0]+35.0)));
  double w0 = 1.0/(exp(-(x[0]+35.0)/10.0)+1.0);

  GNaGates = GNa*ms*ms*ms*x[1];
  GKGates = GK*x[2]*x[2]*x[2]*x[2];
  GAGates = GA*x[3];

  INa = GNaGates*(x[0]-ENa);
  IK = GKGates*(x[0]-EK);
  IA = GAGates*(x[0]-EA);
  IL = GL*(x[0]-EL);

  /* V */ dxdt[0] = (-INa-IK-IL-IA+s)/C;
  /* h */ dxdt[1] = PT*(0.07*exp(-(x[0]+58)/20)*(1.0-x[1])-x[1]/(exp(-0.1*(x[0]+28))+1));
  /* n */ dxdt[2] = PT*(-0.01*(x[0]+34.0)*(1.0-x[2])/(exp(-0.1*(x[0]+34.0))-1)-0.125*exp(-(x[0]+44.0)/80.0)*x[2]);
  /* a */ dxdt[3] = ( w0 - x[3] )/Atau;
}


void WangBuzsakiAdapt::init( double *x ) const
{
  WangBuzsaki::init( x );
  x[3] = 0.0;
}


void WangBuzsakiAdapt::conductances( vector< string > &conductancenames ) const
{
  WangBuzsaki::conductances( conductancenames );
  conductancenames.push_back( "g_A" );
}


void WangBuzsakiAdapt::conductances( double *g ) const
{
  g[0] = GNaGates;
  g[1] = GKGates;
  g[2] = GL;
  g[3] = GAGates;
}


void WangBuzsakiAdapt::currents( vector< string > &currentnames ) const
{
  WangBuzsaki::currents( currentnames );
  currentnames.push_back( "I_A" );
}


void WangBuzsakiAdapt::currents( double *c ) const
{
  c[0] = INa;
  c[1] = IK;
  c[2] = IL;
  c[3] = IA;
}


void WangBuzsakiAdapt::add( void )
{
  WangBuzsaki::add();

  insertLabel( "Adaptation current", "Input", ModelFlag );
  insertNumber( "ga", "Input", "A conductivity", GA, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  insertNumber( "ea", "Input", "A reversal potential", EA, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  insertNumber( "atau", "Input", "A time constant", Atau, 0.0, 1000.0, 1.0, "ms" ).setFlags( ModelFlag );
}


void WangBuzsakiAdapt::notify( void )
{
  WangBuzsaki::notify();
  EA = number( "ea" );
  GA = number( "ga" );
  Atau = number( "atau" );
}


Crook::Crook( void )
  : HodgkinHuxley()
{
  GNa = 221.0;
  GK = 47.0;
  GL = 2.0;
  GCa = 8.5;
  GKAHP = 7.0;
  GKM = 10.0;
  GLD = 0.05;
  GDS = 1.1;

  ENa = +55.0;
  EK = -90.0;
  EL = -70.0;
  ECa = +120.0;

  C = 0.8;
  SFrac = 0.05;
  CaA = 3.0;
  CaTau = 60.0;

  GNaGates = GNa;
  GKGates = GK;
  GCaGates = GCa;
  GKAHPGates = GKAHP;
  GKMGates = GKM;
  GDSGates = GDS;
  GSDGates = GDS;

  INa = 0.0;
  IK = 0.0;
  IL = 0.0;
  ICa = 0.0;
  IKAHP = 0.0;
  IKM = 0.0;
  IDS = 0.0;
  ILD = 0.0;
  ISD = 0.0;
}


string Crook::name( void ) const
{
  return "Crook";
}


int Crook::dimension( void ) const
{
  return 10;
}


void Crook::variables( vector< string > &varnames ) const
{
  HodgkinHuxley::variables( varnames );
  varnames[0] = "VS";
  varnames.push_back( "s" );
  varnames.push_back( "r" );
  varnames.push_back( "q" );
  varnames.push_back( "w" );
  varnames.push_back( "[Ca]" );
  varnames.push_back( "VD" );
}


void Crook::units( vector< string > &u ) const
{
  HodgkinHuxley::units( u );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "mM" );
  u.push_back( "mV" );
}


void Crook::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double VS = x[0];
  double VD = x[9];
  double Ca = x[8];

  double am = 0.32*(-47.1-VS)/(exp(0.25*(-47.1-VS))-1.0);
  double bm = 0.28*(VS+20.1)/(exp((VS+20.1)/5.0)-1.0);

  double ah = 0.128*exp((-43.0-VS)/18.0);
  double bh = 4.0/(exp((-20.0-VS)/5.0)+1.0);

  double an = 0.59*(-25.1-VS)/(exp((-25.1-VS)/5.0)-1.0);
  double bn = 0.925*exp(0.925-0.025*(VS+77));

  double as = 0.912/(exp(-0.072*(VS-5.0))+1.0);
  double bs = 0.0114*(VS+8.9)/(exp((VS+8.9)/5.0)-1.0);

  double r0 = (exp(-(VS+60.0)/20.0))<1.0 ? (exp(-(VS+60.0)/20.0)) : 1.0;
  double tr = 1.0/0.005;

  double q0 = (0.0005*Ca)*(0.0005*Ca);
  double tq = 0.0338/((0.00001*Ca < 0.01 ? 0.00001*Ca : 0.01)+0.001);

  double w0 = 1.0/(exp(-(VS+35.0)/10.0)+1.0);
  double tw = 92.0*exp(-(VS+35.0)/20.0)/(1.0+0.3*exp(-(VS+35.0)/10.0));

  GNaGates = GNa*x[1]*x[1]*x[2];
  GKGates = GK*x[3];
  GCaGates = GCa*x[4]*x[4]*x[5];
  GKAHPGates = GKAHP*x[6];
  GKMGates = GKM*x[7];
  GDSGates = GDS/SFrac;
  GSDGates = GDS/SFrac;

  INa = GNaGates*(VS-ENa);
  IK = GKGates*(VS-EK);
  ICa = GCaGates*(VS-ECa);
  IKAHP = GKAHPGates*(VS-EK);
  IKM = GKMGates*(VS-EK);
  IL = GL*(VS-EL);
  IDS = GDSGates*(VS-VD);
  ILD = GLD*(VD-EL);
  ISD = -GSDGates*(VS-VD);

  /* VS */ dxdt[0] = ( - INa - IK - ICa - IKAHP - IKM - IL - IDS + s/SFrac )/C;
  /* m */  dxdt[1] = am*(1.0-x[1]) - x[1]*bm;
  /* h */  dxdt[2] = ah*(1.0-x[2]) - x[2]*bh;
  /* n */  dxdt[3] = an*(1.0-x[3]) - x[3]*bn;
  /* s */  dxdt[4] = as*(1.0-x[4]) - x[4]*bs;
  /* r */  dxdt[5] = (r0-x[5])/tr;
  /* q */  dxdt[6] = (q0-x[6])/tq;
  /* w */  dxdt[7] = (w0-x[7])/tw;
  /* Ca */ dxdt[8] = -CaA*ICa - x[8]/CaTau;
  /* VD */ dxdt[9] = ( - ILD - ISD )/C;
}


void Crook::init( double *x ) const
{
  x[0] = -71.41;
  x[1] = 0.001243;
  x[2] = 0.999779;
  x[3] = 0.001277;
  x[4] = 0.005174;
  x[5] = 1.0;
  x[6] = 0.000014;
  x[7] = 0.025478;
  x[8] = 7.615;
  x[9] = -71.35;
}


void Crook::conductances( vector< string > &conductancenames ) const
{
  conductancenames.clear();
  conductancenames.reserve( 9 );
  conductancenames.push_back( "g_Na" );
  conductancenames.push_back( "g_K" );
  conductancenames.push_back( "g_lS" );
  conductancenames.push_back( "g_Ca" );
  conductancenames.push_back( "g_M" );
  conductancenames.push_back( "g_AHP" );
  conductancenames.push_back( "g_DS" );
  conductancenames.push_back( "g_lD" );
  conductancenames.push_back( "g_SD" );
}


void Crook::conductances( double *g ) const
{
  g[0] = GNaGates;
  g[1] = GKGates;
  g[2] = GL;
  g[3] = GCaGates;
  g[4] = GKMGates;
  g[5] = GKAHPGates;
  g[6] = GDSGates;
  g[7] = GLD;
  g[8] = GSDGates;
}


void Crook::currents( vector< string > &currentnames ) const
{
  currentnames.clear();
  currentnames.reserve( 9 );
  currentnames.push_back( "I_Na" );
  currentnames.push_back( "I_K" );
  currentnames.push_back( "I_lS" );
  currentnames.push_back( "I_Ca" );
  currentnames.push_back( "I_M" );
  currentnames.push_back( "I_AHP" );
  currentnames.push_back( "I_DS" );
  currentnames.push_back( "I_lD" );
  currentnames.push_back( "I_SD" );
}


void Crook::currents( double *c ) const
{
  c[0] = INa;
  c[1] = IK;
  c[2] = IL;
  c[3] = ICa;
  c[4] = IKM;
  c[5] = IKAHP;
  c[6] = IDS;
  c[7] = ILD;
  c[8] = ISD;
}


void Crook::add( void )
{
  addLabel( "Soma Sodium current", ModelFlag );
  addNumber( "gna", "Na conductivity", GNa, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "ena", "Na reversal potential", ENa, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );

  addLabel( "Soma Potassium current", ModelFlag );
  addNumber( "gk", "K conductivity", GK, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "ek", "K reversal potential", EK, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );

  addLabel( "Soma Calcium current", ModelFlag );
  addNumber( "gcas", "Ca conductivity", GCa, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "eca", "Ca reversal potential", ECa, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  addNumber( "caa", "Ca activation", CaA, 0.0, 200.0, 1.0, "1" ).setFlags( ModelFlag );
  addNumber( "catau", "Ca removal time constant", CaTau, 0.0, 10000.0, 1.0, "ms" ).setFlags( ModelFlag );

  addLabel( "Soma Leak current", ModelFlag );
  addNumber( "gl", "Leak conductivity", GL, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "el", "Leak reversal potential", EL, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  addNumber( "c", "Capacitance", C, 0.0, 100.0, 0.1, "muF/cm^2" ).setFlags( ModelFlag );
  addNumber( "phi", "Phi", PT, 0.0, 100.0, 1.0 ).setFlags( ModelFlag );

  addLabel( "Other currents", ModelFlag );
  addNumber( "gahp", "Soma AHP-type K conductivity", GKAHP, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "gm", "Soma M-type K conductivity", GKM, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "gld", "Dendrite leak conductivity", GLD, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "gds", "Soma-dendrite conductivity", GDS, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "sfrac", "Soma coupling fraction", SFrac, 0.0, 1.0, 0.01 ).setFlags( ModelFlag );

  SpikingNeuron::add();
}


void Crook::notify( void )
{
  SpikingNeuron::notify();
  HodgkinHuxley::notify();
  ECa = number( "eca" );
  GCa = number( "gcas" );
  CaA = number( "caa" );
  CaTau = number( "catau" );
  GKAHP = number( "gahp" );
  GKM = number( "gm" );
  GLD = number( "gld" );
  SFrac = number( "sfrac" );
}


WangIKNa::WangIKNa( void )
  : HodgkinHuxley()
{
  GNa = 45.0;
  GK = 18.0;
  GL = 0.1;
  GCaS = 1.0;
  GKCaS = 5.0;
  GKNa = 5.0;
  GDS = 2.0;
  GLD = 0.1;
  GCaD = 1.0;
  GKCaD = 5.0;

  ENa = +55.0;
  EK = -80.0;
  EL = -65.0;
  ECa = +120.0;

  C = 1.0;
  PT = 4.0;

  CaSA = 0.002;
  CaSTau = 240.0;
  CaDA = 0.00067;
  CaDTau = 80.0;

  GNaGates = GNa;
  GKGates = GK;
  GCaSGates = GCaS;
  GKCaSGates = GKCaS;
  GKNaGates = GKNa;
  GDSGates = GDS;
  GCaDGates = GCaD;
  GKCaDGates = GKCaD;
  GSDGates = GDS;

  INa = 0.0;
  IK = 0.0;
  IL = 0.0;
  ICaS = 0.0;
  IKCaS = 0.0;
  IKNa = 0.0;
  IDS = 0.0;
  ILD = 0.0;
  ICaD = 0.0;
  IKCaD = 0.0;
  ISD = 0.0;
}


string WangIKNa::name( void ) const
{
  return "Wang I_KNa";
}


int WangIKNa::dimension( void ) const
{
  return 11;
}


void WangIKNa::variables( vector< string > &varnames ) const
{
  HodgkinHuxley::variables( varnames );
  varnames[0] = "VS";
  varnames.push_back( "vs" );
  varnames.push_back( "[CaS]" );
  varnames.push_back( "ws" );
  varnames.push_back( "[Na]" );
  varnames.push_back( "VD" );
  varnames.push_back( "vd" );
  varnames.push_back( "[CaD]" );
}


void WangIKNa::units( vector< string > &u ) const
{
  HodgkinHuxley::units( u );
  u.push_back( "1" );
  u.push_back( "mM" );
  u.push_back( "1" );
  u.push_back( "mM" );
  u.push_back( "mV" );
  u.push_back( "1" );
  u.push_back( "mM" );
}


void WangIKNa::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double VS = x[0];
  double CaS = x[5];
  double Na = x[7];
  double Na3 = Na*Na*Na;
  double Nae3 = 8.0*8.0*8.0;
  double CaD = x[10];
  double VD = x[8];

  double ms = 1.0/(1.0+4.0*exp(-(VS+58.0)/12.0)*(exp(-0.1*(VS+33.0))-1.0)/(-0.1*(VS+33.0)));
  x[1] = ms;
  double vs = 1.0/(1.0+exp(-(VS+20.0)/9.0));
  x[4] = vs;
  double ws = 0.37/(1.0+pow(38.7/Na,3.5));
  x[6] = ws;
  double vd = 1.0/(1.0+exp(-(VD+20.0)/9.0));
  x[9] = vd;

  GNaGates = GNa*ms*ms*ms*x[2];
  GKGates = GK*x[3]*x[3]*x[3]*x[3];
  GCaSGates = GCaS*vs*vs;
  GKCaSGates = GKCaS*CaS/(CaS+30.0);
  GKNaGates = GKNa*ws;
  GDSGates = GDS/0.5;
  GCaDGates = GCaD*vd*vd;
  GKCaDGates = GKCaD*CaD/(CaD+30.0);
  GSDGates = GDS/(1.0-0.5);

  INa = GNaGates*(VS-ENa);
  IK = GKGates*(VS-EK);
  IL = GL*(VS-EL);
  ICaS = GCaSGates*(VS-ECa);
  IKCaS = GKCaSGates*(VS-EK);
  IKNa = GKNaGates*(VS-EK);
  IDS = GDSGates*(VS-VD);
  ILD = GLD*(VD-EL);
  ICaD = GCaDGates*(VD-ECa);
  IKCaD = GKCaDGates*(VD-EK);
  ISD = GSDGates*(VD-VS);

  /* VS */  dxdt[0] = ( - INa - IK - IL - ICaS - IKCaS - IKNa - IDS + s )/C;
  /* m */   dxdt[1] = 0.0;
  /* h */   dxdt[2] = PT*(0.07*exp(-(VS+50.0)/10.0)*(1.0-x[2])-x[2]/(exp(-0.1*(VS+20.0))+1.0));
  /* n */   dxdt[3] = PT*(-0.01*(VS+34.0)*(1.0-x[3])/(exp(-0.1*(VS+34.0))-1.0)-0.125*exp(-(VS+44.0)/25.0)*x[3]);
  /* vs */  dxdt[4] = 0.0;
  /* CaS */ dxdt[5] = -CaSA*ICaS - x[5]/CaSTau;
  /* ws */  dxdt[6] = 0.0;
  /* Na */  dxdt[7] = -0.0003*INa - 3.0*0.0006*( Na3/(Na3+15.0*15.0*15.0) - Nae3/(Nae3+15.0*15.0*15.0));
  /* VD */  dxdt[8] = ( - ILD - ICaD - IKCaD - ISD )/C;
  /* vd */  dxdt[9] = 0.0;
  /* CaD */ dxdt[10] = -CaDA*ICaD - x[10]/CaDTau;
}


void WangIKNa::init( double *x ) const
{
  x[0] = -64.86;
  x[1] = 0.019024;
  x[2] = 0.965235;
  x[3] = 0.048809;
  x[4] = 0.006798;
  x[5] = 0.004101;
  x[6] = 0.0;
  x[7] = 2.5494;
  x[8] = -64.86;
  x[9] = 0.0068;
  x[10] = 0.00046;
}


void WangIKNa::conductances( vector< string > &conductancenames ) const
{
  conductancenames.clear();
  conductancenames.reserve( 11 );
  conductancenames.push_back( "g_Na" );
  conductancenames.push_back( "g_K" );
  conductancenames.push_back( "g_lS" );
  conductancenames.push_back( "g_CaS" );
  conductancenames.push_back( "g_KCaS" );
  conductancenames.push_back( "g_KNa" );
  conductancenames.push_back( "g_DS" );
  conductancenames.push_back( "g_lD" );
  conductancenames.push_back( "g_CaD" );
  conductancenames.push_back( "g_KCaD" );
  conductancenames.push_back( "g_SD" );
}


void WangIKNa::conductances( double *g ) const
{
  g[0] = GNaGates;
  g[1] = GKGates;
  g[2] = GL;
  g[3] = GCaSGates;
  g[4] = GKCaSGates;
  g[5] = GKNaGates;
  g[6] = GDSGates;
  g[7] = GLD;
  g[8] = GCaDGates;
  g[9] = GKCaDGates;
  g[10] = GSDGates;
}


void WangIKNa::currents( vector< string > &currentnames ) const
{
  currentnames.clear();
  currentnames.reserve( 11 );
  currentnames.push_back( "I_Na" );
  currentnames.push_back( "I_K" );
  currentnames.push_back( "I_lS" );
  currentnames.push_back( "I_CaS" );
  currentnames.push_back( "I_KCaS" );
  currentnames.push_back( "I_KNa" );
  currentnames.push_back( "I_DS" );
  currentnames.push_back( "I_lD" );
  currentnames.push_back( "I_CaD" );
  currentnames.push_back( "I_KCaD" );
  currentnames.push_back( "I_SD" );
}


void WangIKNa::currents( double *c ) const
{
  c[0] = INa;
  c[1] = IK;
  c[2] = IL;
  c[3] = ICaS;
  c[4] = IKCaS;
  c[5] = IKNa;
  c[6] = IDS;
  c[7] = ILD;
  c[8] = ICaD;
  c[9] = IKCaD;
  c[10] = ISD;
}


void WangIKNa::add( void )
{
  addLabel( "Soma Sodium current", ModelFlag );
  addNumber( "gna", "Na conductivity", GNa, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "ena", "Na reversal potential", ENa, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );

  addLabel( "Soma Potassium current", ModelFlag );
  addNumber( "gk", "K conductivity", GK, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "ek", "K reversal potential", EK, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );

  addLabel( "Soma Calcium current", ModelFlag );
  addNumber( "gcas", "Ca conductivity", GCaS, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "eca", "Ca reversal potential", ECa, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  addNumber( "casa", "Ca activation", CaSA, 0.0, 200.0, 1.0, "1" ).setFlags( ModelFlag );
  addNumber( "castau", "Ca removal time constant", CaSTau, 0.0, 10000.0, 1.0, "ms" ).setFlags( ModelFlag );

  addLabel( "Soma Leak current", ModelFlag );
  addNumber( "gl", "Leak conductivity", GL, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "el", "Leak reversal potential", EL, -200.0, 200.0, 1.0, "mV" ).setFlags( ModelFlag );
  addNumber( "c", "Capacitance", C, 0.0, 100.0, 0.1, "muF/cm^2" ).setFlags( ModelFlag );
  addNumber( "phi", "Phi", PT, 0.0, 100.0, 1.0 ).setFlags( ModelFlag );

  addLabel( "Dendrite Calcium current", ModelFlag );
  addNumber( "gcad", "Ca conductivity", GCaD, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "cada", "Ca activation", CaDA, 0.0, 200.0, 1.0, "1" ).setFlags( ModelFlag );
  addNumber( "cadtau", "Ca removal time constant", CaDTau, 0.0, 10000.0, 1.0, "ms" ).setFlags( ModelFlag );

  addLabel( "Other currents", ModelFlag );
  addNumber( "gkcas", "Soma Ca dependent K conductivity", GKCaS, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "gkna", "Soma Na dependent K conductivity", GKNa, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "gds", "Soma-dendrite conductivity", GDS, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "gld", "Dendrite leak conductivity", GLD, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "gkcad", "Dendrite Ca dependent K conductivity", GKCaD, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );

  SpikingNeuron::add();
}


void WangIKNa::notify( void )
{
  SpikingNeuron::notify();
  HodgkinHuxley::notify();
  ECa = number( "eca" );
  GCaS = number( "gcas" );
  CaSA = number( "casa" );
  CaSTau = number( "castau" );
  GCaD = number( "gcad" );
  CaDA = number( "cada" );
  CaDTau = number( "cadtau" );
  GKCaS = number( "gkcas" );
  GKNa = number( "gkna" );
  GDS = number( "gds" );
  GLD = number( "gld" );
  GKCaD = number( "gkcad" );
}


Edman::Edman( void )
  : SpikingNeuron()
{
  GNa = 5.6e-4;
  GNa = 0.004;
  GNa = 0.015;
  GNa = 0.02;
  GK = 2.4e-4;
  GLNa = 5.8e-8;
  GLK = 1.8e-6;
  GLK = 1.0e-5;
  GLCl = 1.1e-7;
  GP = 3.0e-10;

  C = 7.8;

  GNaGates = GNa;
  GKGates = GK;
  GLNaA = GLNa;
  GLKA = GLK;
  GLClA = GLCl;
  GPA = GP;

  INa= 0.0;
  IK= 0.0;
  ILNa = 0.0;
  ILK = 0.0;
  ILCl = 0.0;
  IP = 0.0;
}


string Edman::name( void ) const
{
  return "Edman";
}


int Edman::dimension( void ) const
{
  return 7;
}


void Edman::variables( vector< string > &varnames ) const
{
  varnames.clear();
  varnames.reserve( dimension() );
  varnames.push_back( "V" );
  varnames.push_back( "m" );
  varnames.push_back( "h" );
  varnames.push_back( "l" );
  varnames.push_back( "n" );
  varnames.push_back( "r" );
  varnames.push_back( "[Na]" );
}


void Edman::units( vector< string > &u ) const
{
  u.clear();
  u.reserve( dimension() );
  u.push_back( "mV" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "1" );
  u.push_back( "mM" );
}


void Edman::operator()(  double t, double s, double *x, double *dxdt, int n )
{
  double A=1.0e-3;
  double vol=1.25e-6; /* cm^3 */
  double Narest=0.001*10.0, Krest=0.001*160.0, Clrest=0.001*46.0;
  double NaO=0.001*325.0, KO=0.001*5.0, ClO=0.001*(325.0+5.0+2.0*25.0+2.0*4.0+26.0);
  double dm=0.3, dh=0.5, dl=0.3, dn=0.3, dr=0.5;
  double zm=3.1, zh=-4.0, zl=-3.5, zn=2.6, zr=-4.0;
  double vm=0.0, vh=0.0, vl=0.0, vn=0.03, vr=0.3;
  double tmmax=0.3, thmax=5.0, tlmax=1700.0, tnmax=6.0, trmax=1200.0;
  double Vm=-13.0, Vh=-35.0, Vl=-53.0, Vn=-18.0, Vr=-61.0;
  double Km=7.7;
  double F=96485.0; /* C/mol */
  double R=8.3144; /* J/K/mol */
  double T=291.0;  /* K, 18GradCelsius */
  double FRT=0.001*F/R/T;
  double F2RT=F*F/R/T;
  double e=1.60217653e-19; /* C   */
  double k=1.3806505e-23; /*  J/K */
  double eKT=0.001*e/k/T; /*  */

  double V = x[0];
  double Na = x[6];
  double K = Krest - ( Na - Narest );
  double Cl = Clrest;

  double ms = vm+(1.0-vm)/(1.0+exp(-zm*eKT*(V-Vm)));
  double hs = vh+(1.0-vh)/(1.0+exp(-zh*eKT*(V-Vh)));
  double ls = vl+(1.0-vl)/(1.0+exp(-zl*eKT*(V-Vl)));
  double ns = vn+(1.0-vn)/(1.0+exp(-zn*eKT*(V-Vn)));
  double rs = vr+(1.0-vr)/(1.0+exp(-zr*eKT*(V-Vr)));

  double tm = tmmax*(pow( (1.0-dm)/dm, dm ) + pow( (1.0-dm)/dm, dm-1.0 ))/(exp(dm*zm*eKT*(V-Vm))+exp((dm-1.0)*zm*eKT*(V-Vm)));
  double th = thmax*(pow( (1.0-dh)/dh, dh ) + pow( (1.0-dh)/dh, dh-1.0 ))/(exp(dh*zh*eKT*(V-Vh))+exp((dh-1.0)*zh*eKT*(V-Vh)));
  double tl = tlmax*(pow( (1.0-dl)/dl, dl ) + pow( (1.0-dl)/dl, dl-1.0 ))/(exp(dl*zl*eKT*(V-Vl))+exp((dl-1.0)*zl*eKT*(V-Vl)));
  double tn = tnmax*(pow( (1.0-dn)/dn, dn ) + pow( (1.0-dn)/dn, dn-1.0 ))/(exp(dn*zn*eKT*(V-Vn))+exp((dn-1.0)*zn*eKT*(V-Vn)));
  double tr = trmax*(pow( (1.0-dr)/dr, dr ) + pow( (1.0-dr)/dr, dr-1.0 ))/(exp(dr*zr*eKT*(V-Vr))+exp((dr-1.0)*zr*eKT*(V-Vr)));

  GNaGates = A*GNa*x[1]*x[1]*x[2]*x[3];
  GKGates = A*GK*x[4]*x[4]*x[5];
  GLNaA = A*GLNa;
  GLKA = A*GLK;
  GLClA = A*GLCl;
  GPA = A*GP;

  INa = GNaGates*V*F2RT*(NaO-Na*exp(V*FRT))/(1.0-exp(V*FRT));
  IK = GKGates*V*F2RT*(KO-K*exp(V*FRT))/(1.0-exp(V*FRT));
  ILNa = GLNaA*V*F2RT*(NaO-Na*exp(V*FRT))/(1.0-exp(V*FRT));
  ILK = GLKA*V*F2RT*(KO-K*exp(V*FRT))/(1.0-exp(V*FRT));
  ILCl = GLClA*V*F2RT*(ClO-Cl*exp(V*FRT))/(1.0-exp(V*FRT));
  IP = 1.0e6*GPA*F/3.0/pow( 1.0+Km/Na, 3.0 );

  /* V */ dxdt[0] = ( - INa - IK - ILNa - ILK - ILCl - IP + 0.001*s )/C/A;
  /* m */ dxdt[1] = ( ms - x[1] ) / tm;
  /* h */ dxdt[2] = ( hs - x[2] ) / th;
  /* l */ dxdt[3] = ( ls - x[3] ) / tl;
  /* n */ dxdt[4] = ( ns - x[4] ) / tn;
  /* r */ dxdt[5] = ( rs - x[5] ) / tr;
  /* Na */ dxdt[6] = - 1.0e-6 * ( INa + ILNa + 3.0*IP ) / F / vol;
}


void Edman::init( double *x ) const
{
  x[0] = -60.86;
  x[1] = 0.002688;
  x[2] = 0.984;
  x[3] = 0.8609;
  x[4] = 0.0412;
  x[5] = 0.8117;
  x[6] = 0.001*10.022;
}


void Edman::conductances( vector< string > &conductancenames ) const
{
  conductancenames.clear();
  conductancenames.reserve( 6 );
  conductancenames.push_back( "g_Na" );
  conductancenames.push_back( "g_K" );
  conductancenames.push_back( "g_lNa" );
  conductancenames.push_back( "g_lK" );
  conductancenames.push_back( "g_lCl" );
  conductancenames.push_back( "g_P" );
}


void Edman::conductances( double *g ) const
{
  g[0] = GNaGates;
  g[1] = GKGates;
  g[2] = GLNaA;
  g[3] = GLKA;
  g[4] = GLClA;
  g[5] = GPA;
}


void Edman::currents( vector< string > &currentnames ) const
{
  currentnames.clear();
  currentnames.reserve( 6 );
  currentnames.push_back( "I_Na" );
  currentnames.push_back( "I_K" );
  currentnames.push_back( "I_lNa" );
  currentnames.push_back( "I_lK" );
  currentnames.push_back( "I_lCl" );
  currentnames.push_back( "I_P" );
}


void Edman::currents( double *c ) const
{
  c[0] = INa;
  c[1] = IK;
  c[2] = ILNa;
  c[3] = ILK;
  c[4] = ILCl;
  c[5] = IP;
}


void Edman::add( void )
{
  addLabel( "Conductivities", ModelFlag );
  addNumber( "gna", "Na conductivity", GNa, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "gk", "K conductivity", GK, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "glna", "Na leak conductivity", GLNa, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "glk", "K leak conductivity", GLK, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "glcl", "Cl leak conductivity", GLCl, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "gp", "Pump conductivity", GP, 0.0, 10000.0, 0.1, "mS/cm^2" ).setFlags( ModelFlag );
  addNumber( "c", "Capacitance", C, 0.0, 100.0, 0.1, "muF/cm^2" ).setFlags( ModelFlag );

  SpikingNeuron::add();
}


void Edman::notify( void )
{
  SpikingNeuron::notify();
  GNa = number( "gna" );
  GK = number( "gk" );
  GLNa = number( "glna" );
  GLK = number( "glk" );
  GLCl = number( "glcl" );
  GP = number( "gp" );
  C = number( "c" );
}


}; /* namespace ephys */