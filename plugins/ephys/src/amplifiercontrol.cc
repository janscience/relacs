/*
  ephys/amplifiercontrol.cc
  Controls an amplifier: buzzer and resistance measurement.

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

#include <qlabel.h>
#include <relacs/ephys/amplifiercontrol.h>
using namespace relacs;

namespace ephys {


AmplifierControl::AmplifierControl( void )
  : Control( "AmplifierControl", "AmplifierControl", "ephys",
	     "Jan Benda", "1.0", "Apr 16, 2010" )
{

  AmplBox = new QHBox( this );
  BuzzerButton = 0;
  ResistanceButton = 0;
  ResistanceLabel = 0;

  Ampl = 0;
  RMeasure = false;

  MaxResistance = 100.0;
  ResistanceScale = 1.0;

  addNumber( "resistancescale", "Scaling factor for computing R from stdev of voltage trace", ResistanceScale, 0.0, 100000.0, 0.01 );
  addNumber( "maxresistance", "Maximum resistance to be expected for scaling voltage trace", MaxResistance, 0.0, 1000000.0, 10.0, "MOhm" );
}


void AmplifierControl::notify( void )
{
  ResistanceScale = number( "resistancescale" );
  MaxResistance = number( "maxresistance" );
}


void AmplifierControl::initDevices( void )
{
  Ampl = dynamic_cast< misc::AmplMode* >( device( "ampl-1" ) );
  if ( Ampl != 0 ) {
    lockMetaData();
    Options &mo = metaData( "Electrode" );
    mo.unsetNotify();
    if ( ! mo.exist( "Resistance" ) )
      mo.addNumber( "Resistance", "Resistance", 0.0, "MOhm", "%.0f" );
    mo.setNotify();
    unlockMetaData();
    if ( ResistanceButton == 0 && BuzzerButton == 0 ) {
    
      new QLabel( AmplBox );

      BuzzerButton = new QPushButton( "Buzz", AmplBox );
      connect( BuzzerButton, SIGNAL( clicked() ),
	       this, SLOT( buzz() ) );

      new QLabel( AmplBox );

      ResistanceButton = new QPushButton( "R", AmplBox );
      connect( ResistanceButton, SIGNAL( pressed() ),
	       this, SLOT( startResistance() ) );
      connect( ResistanceButton, SIGNAL( released() ),
	       this, SLOT( stopResistance() ) );
    
      QLabel *label = new QLabel( "=", AmplBox );
      label->setAlignment( AlignHCenter | AlignVCenter );

      ResistanceLabel = new QLabel( "000", AmplBox );
      ResistanceLabel->setAlignment( AlignRight | AlignVCenter );
      ResistanceLabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
      ResistanceLabel->setLineWidth( 2 );
      QFont nf( ResistanceLabel->font() );
      nf.setPointSizeFloat( 1.6 * nf.pointSizeFloat() );
      nf.setBold( true );
      ResistanceLabel->setFont( nf );
      QPalette qp( ResistanceLabel->palette() );
      qp.setColor( QColorGroup::Background, black );
      qp.setColor( QColorGroup::Foreground, green );
      ResistanceLabel->setPalette( qp );
      ResistanceLabel->setFixedHeight( ResistanceLabel->sizeHint().height() );
      ResistanceLabel->setFixedWidth( ResistanceLabel->sizeHint().width() );
      ResistanceLabel->setText( "0" );

      new QLabel( "MOhm", AmplBox );
    
      new QLabel( AmplBox );
    }
    AmplBox->show();
  }
  else {
    AmplBox->hide();
  }

  updateGeometry();
}


void AmplifierControl::startResistance( void )
{
  if ( Ampl != 0 && SpikeTrace[0] >= 0 && ! RMeasure ) {
    readLockData();
    DGain = trace( SpikeTrace[0] ).gainIndex();
    adjustGain( trace( SpikeTrace[0] ), MaxResistance / ResistanceScale );
    unlockData();
    activateGains();
    Ampl->resistance();
    RMeasure = true;
  }
}


void AmplifierControl::measureResistance( void )
{
  if ( Ampl != 0 && SpikeTrace[0] >= 0 && RMeasure ) {
    readLockData();
    double r = trace( SpikeTrace[0] ).stdev( trace( SpikeTrace[0] ).currentTime() - 0.05,
					     trace( SpikeTrace[0] ).currentTime() );
    unlockData();
    r *= ResistanceScale;
    ResistanceLabel->setText( Str( r, "%.0f" ).c_str() );
    lockMetaData();
    metaData( "Electrode" ).setNumber( "Resistance", r );
    unlockMetaData();
  }
}


void AmplifierControl::stopResistance( void )
{
  if ( Ampl != 0 && SpikeTrace[0] >= 0 && RMeasure ) {
    Ampl->manual();
    readLockData();
    setGain( trace( SpikeTrace[0] ), DGain );
    unlockData();
    activateGains();
    RMeasure = false;
  }
}


void AmplifierControl::buzz( void )
{
  if ( Ampl != 0 ) {
    Ampl->buzzer( );
  }
}


void AmplifierControl::keyPressEvent( QKeyEvent *e )
{
  switch ( e->key() ) {

  case Key_O:
    if ( Ampl != 0 && ResistanceButton != 0 ) {
      ResistanceButton->setDown( true );
      startResistance();
    }
    else
      e->ignore();
    break;

  case Key_Z:
    if ( Ampl != 0 && BuzzerButton != 0 ) {
      BuzzerButton->animateClick();
    }
    else
      e->ignore();
    break;

  default:
    Control::keyPressEvent( e );

  }
}


void AmplifierControl::keyReleaseEvent( QKeyEvent *e )
{
  switch ( e->key() ) {

  case Key_O: 
    if ( Ampl != 0 && ResistanceButton != 0 ) {
      measureResistance();
      if ( ! e->isAutoRepeat() ) {
	ResistanceButton->setDown( false );
	stopResistance();
      }
    }
    else
      e->ignore();
    break;

  default:
    Control::keyReleaseEvent( e );

  }
}



addControl( AmplifierControl );

}; /* namespace ephys */

#include "moc_amplifiercontrol.cc"