/*
  repro.cc
  Parent class of all research programs.

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

#include <ctype.h>
#include <cmath>
#include <QDir>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QApplication>
#include <relacs/str.h>
#include <relacs/acquire.h>
#include <relacs/savefiles.h>
#include <relacs/relacswidget.h>
#include <relacs/repros.h>
#include <relacs/repro.h>

namespace relacs {


RePro::RePro( const string &name, const string &pluginset,
	      const string &author,
	      const string &version, const string &date )
  : RELACSPlugin( "RePro: " + name, RELACSPlugin::Plugins,
		  name, pluginset, author, version, date ),
    OverwriteOpt()
{
  addDialogStyle( OptWidget::TabSectionStyle | OptWidget::HideStyle );

  LastState = Continue;
  CompleteRuns = 0;
  TotalRuns = 0;
  AllRuns = 0;
  FailedRuns = 0;
  ReProStartTime = 0.0;

  SoftStop = 0;
  SoftStopKey = Qt::Key_Space;

  GrabKeys.clear();
  GrabKeys.reserve( 20 );
  GrabKeysBaseSize = 0;
  GrabKeysInstalled = false;

  Thread = new ReProThread( this );
  Interrupt = 0;

  // start timer:
  ReProTime.start();

  PrintMessage = true;
}


RePro::~RePro( void )
{
  // XXX auto remove from RePros?
}


void RePro::setName( const string &name )
{
  RELACSPlugin::setName( name );
  setConfigIdent( "RePro: " + name );
}


void RePro::readConfig( StrQueue &sq )
{
  ConfigClass::readConfig( sq );
  setToDefaults();
}


void RePro::saveConfig( ofstream &str )
{
  setDefaults();
  ConfigClass::saveConfig( str );
}


void RePro::run( void )
{
  // start timer:
  ReProTime.start();
  timeStamp();

  // write message:
  if ( PrintMessage )
    message( "Running <b>" + name() + "</b> ..." );

  // init:
  ReProStartTime = sessionTime();
  SoftStopLock.lock();
  SoftStop = 0;
  SoftStopLock.unlock();
  setSettings();
  InterruptLock.lock();
  Interrupt = 0;
  InterruptLock.unlock();
  GrabKeysBaseSize = GrabKeys.size();
  enable();
  getData();
  lock();
  RW->SF->holdOff();

  // run RePro:
  LastState = main();

  // update statistics:
  if ( LastState == Completed )
    CompleteRuns++;
  if ( LastState == Completed || LastState == Aborted )
    TotalRuns++;
  AllRuns++;
  if ( LastState == Failed )
    FailedRuns++;

  unlock();

  RW->KeyTime->unsetNoFocusWidget();
  GrabKeys.resize( GrabKeysBaseSize );
  disable();

  // write message:
  if ( PrintMessage ) {
    if ( completed() )
      message( "<b>" + name() + "</b> successfully completed after <b>" + reproTimeStr() + "</b>" );
    else if ( failed() )
      message( "<b>" + name() + "</b> stopped after <b>" + reproTimeStr() + "</b>" );
    else
      message( "<b>" + name() + "</b> aborted after <b>" + reproTimeStr() + "</b>" );
  }

  // start next RePro:
  if ( ! interruptWrite() )
    QApplication::postEvent( RW, new QEvent( QEvent::Type( QEvent::User+1 ) ) );
}


bool RePro::interrupt( void ) const
{
  InterruptLock.lock();
  bool ir = ( Interrupt >= 2 ); 
  InterruptLock.unlock();
  return ir;
}


bool RePro::interruptWrite( void )
{
  InterruptLock.lock();
  bool ir = ( Interrupt >= 1 ); 
  if ( ir )
    Interrupt = 2;
  InterruptLock.unlock();
  return ir;
}


void RePro::start( QThread::Priority priority )
{
  Thread->start( priority );
}


void RePro::requestStop( void )
{
  if ( Thread->isRunning() ) {
    InterruptLock.lock();
    int ir = Interrupt;
    Interrupt = 1;
    // stop analog output,
    // before the repro gets any chance to delete the output signal:
    if ( ir < 2 )
      stopWrite();

    // tell the RePro to interrupt:
    Interrupt = 2;
    InterruptLock.unlock();
    
    // wake up the RePro from sleeping:
    SleepWait.wakeAll();
  }
}


bool RePro::isRunning( void ) const
{
  return Thread->isRunning();
}


bool RePro::wait( double time )
{
  if ( time > 0.0 )
    return Thread->wait( (unsigned long)::floor( 1000.0*time ) );
  else
    return Thread->wait();
}


bool RePro::sleep( double t, double tracetime )
{
  if ( tracetime < 0.0 )
    tracetime = currentTime() + t;

  RW->updateRePro();

  // interrupt RePro:
  if ( interrupt() )
    return true;

  // sleep:
  if ( t > 0.0 ) {
    unsigned long ms = (unsigned long)::rint(1.0e3*t);
    if ( ms < 1 )
      ms = 1;
    SleepWait.wait( mutex(), ms );
  }

  // force data updates:
  getData( interrupt() ? 0.0 : tracetime );

  return interrupt();
}


void RePro::timeStamp( void )
{
  SleepTime.start();
  TraceTime = currentTime();
}


bool RePro::sleepOn( double t )
{
  double st = 0.001 * SleepTime.elapsed();
  return RePro::sleep( t - st, TraceTime + t );
}


bool RePro::sleepWait( double time )
{
  bool r = false;
  if ( time <= 0.0 )
    r = SleepWait.wait( mutex() );
  else {
    unsigned long ms = (unsigned long)::rint(1.0e3*time);
    if ( ms < 1 )
      ms = 1;
    r = SleepWait.wait( mutex(), ms );
  }
  // make new data available:
  getData();
  return r;
}


void RePro::wake( void )
{
  SleepWait.wakeAll();
}


void RePro::enable( void )
{
  postCustomEvent( 8 );
}


void RePro::disable( void )
{
  postCustomEvent( 9 );
}


void RePro::sessionStarted( void )
{
  CompleteRuns = 0;
  TotalRuns = 0;
  AllRuns = 0;
  FailedRuns = 0;
}


void RePro::sessionStopped( bool saved )
{
  CompleteRuns = 0;
  TotalRuns = 0;
  AllRuns = 0;
  FailedRuns = 0;
}


bool RePro::completed( void ) const
{
  return ( LastState == Completed );
}


bool RePro::aborted( void ) const
{
  return ( LastState == Aborted );
}


bool RePro::failed( void ) const
{
  return ( LastState == Failed );
}


int RePro::completeRuns( void ) const
{
  return CompleteRuns;
}


int RePro::totalRuns( void ) const
{
  return TotalRuns;
}


int RePro::allRuns( void ) const
{
  return AllRuns;
}


int RePro::failedRuns( void ) const
{
  return FailedRuns;
}


double RePro::reproTime( void ) const
{
  return 0.001*ReProTime.elapsed();
}


string RePro::reproTimeStr( void ) const
{
  double sec = reproTime();
  double min = floor( sec/60.0 );
  sec -= min*60.0;
  double hour = floor( min/60.0 );
  min -= hour*60.0;

  struct tm time = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  time.tm_sec = (int)sec;
  time.tm_min = (int)min;
  time.tm_hour = (int)hour;

  lockRelacsSettings();
  Str rts = relacsSettings().text( "reprotimeformat" );
  unlockRelacsSettings();
  rts.format( &time );
  return rts;

}


double RePro::reproStartTime( void ) const
{
  return ReProStartTime;
}


int RePro::testWrite( OutData &signal )
{
  return RW->AQ->testWrite( signal );
}


int RePro::testWrite( OutList &signal )
{
  return RW->AQ->testWrite( signal );
}


int RePro::write( OutData &signal, bool setsignaltime )
{
  if ( interruptWrite() )
    return -1;

  unlock();

  double st = signalTime();

  int r = RW->write( signal, setsignaltime, true );

  // force data updates:
  if ( r < 0 || interruptWrite() )
    getData();
  else
    getData( signal.length(), st );

  lock();

  return r;
}


int RePro::write( OutList &signal, bool setsignaltime )
{
  if ( interruptWrite() )
    return -1;

  unlock();

  double st = signalTime();

  int r = RW->write( signal, setsignaltime, true );

  // force data updates:
  if ( r < 0 || interruptWrite() )
    getData();
  else
    getData( signal.maxLength(), st );

  lock();

  return r;
}


int RePro::startWrite( OutData &signal, bool setsignaltime )
{
  if ( interruptWrite() )
    return -1;
  return RW->write( signal, setsignaltime, false );
}


int RePro::startWrite( OutList &signal, bool setsignaltime )
{
  if ( interruptWrite() )
    return -1;
  return RW->write( signal, setsignaltime, false );
}


int RePro::directWrite( OutData &signal, bool setsignaltime )
{
  return RW->directWrite( signal, setsignaltime );
}


int RePro::directWrite( OutList &signal, bool setsignaltime )
{
  return RW->directWrite( signal, setsignaltime );
}


int RePro::writeZero( int channel, int device )
{
  return RW->AQ->writeZero( channel, device );
}


int RePro::writeZero( int index )
{
  return RW->AQ->writeZero( index );
}


int RePro::writeZero( const string &trace )
{
  return RW->AQ->writeZero( trace );
}


int RePro::stopWrite( void )
{
  return RW->stopWrite();
}


double RePro::minLevel( int trace ) const
{
  return RW->AQ->minLevel( trace );
}


double RePro::minLevel( const string &trace ) const
{
  return RW->AQ->minLevel( trace );
}


double RePro::maxLevel( int trace ) const
{
  return RW->AQ->maxLevel( trace );
}


double RePro::maxLevel( const string &trace ) const
{
  return RW->AQ->maxLevel( trace );
}


void RePro::levels( int trace, vector<double> &l ) const
{
  return RW->AQ->levels( trace, l );
}


void RePro::levels( const string &trace, vector<double> &l ) const
{
  return RW->AQ->levels( trace, l );
}


double RePro::minIntensity( int trace, double frequency ) const
{
  return RW->AQ->minIntensity( trace, frequency );
}


double RePro::minIntensity( const string &trace, double frequency ) const
{
  return RW->AQ->minIntensity( trace, frequency );
}


double RePro::maxIntensity( int trace, double frequency ) const
{
  return RW->AQ->maxIntensity( trace, frequency );
}


double RePro::maxIntensity( const string &trace, double frequency ) const
{
  return RW->AQ->maxIntensity( trace, frequency );
}


void RePro::intensities( int trace, vector<double> &ints, double frequency ) const
{
  return RW->AQ->intensities( trace, ints, frequency );
}


void RePro::intensities( const string &trace, vector<double> &ints, double frequency ) const
{
  return RW->AQ->intensities( trace, ints, frequency );
}


void RePro::setMessage( bool message )
{
  PrintMessage = message;
}


void RePro::noMessage( void )
{
  PrintMessage = false;
}


void RePro::message( const string &msg )
{
  if ( repros() != 0 )
    repros()->message( msg );
}


string RePro::addPath( const string &file ) const
{
  RW->SF->storeFile( file );
  return RW->SF->addPath( file );
}


void RePro::keepFocus( void )
{
  RW->KeyTime->setNoFocusWidget( widget() );
}


void RePro::keyPressEvent( QKeyEvent *event )
{
  if ( event->key() == SoftStopKey ) {
    SoftStopLock.lock();
    SoftStop++;
    SoftStopLock.unlock();
    event->accept();
  }
  else
    event->ignore();
}


void RePro::keyReleaseEvent( QKeyEvent *event )
{
  event->ignore();
}


void RePro::grabKey( int key )
{
  GrabKeyLock.lock();
  GrabKeys.push_back( key );
  GrabKeyLock.unlock();
}


void RePro::grabKeys( void )
{
  GrabKeyLock.lock();
  if ( ! GrabKeysInstalled ) {
    qApp->installEventFilter( this );
    GrabKeysInstalled = true;
  }
  GrabKeyLock.unlock();
}


void RePro::releaseKey( int key )
{
  GrabKeyLock.lock();
  int inx = 0;
  vector<int>::iterator kp = GrabKeys.begin();
  while ( kp != GrabKeys.end() ) {
    if ( *kp == key ) {
      kp = GrabKeys.erase( kp );
      if ( inx < GrabKeysBaseSize )
	GrabKeysBaseSize--;
    }
    else {
      ++kp;
      ++inx;
    }
  }
  GrabKeyLock.unlock();
}


void RePro::releaseKeys( void )
{
  GrabKeyLock.lock();
  if ( GrabKeysInstalled ) {
    qApp->removeEventFilter( this );
    GrabKeysInstalled = false;
  }
  GrabKeyLock.unlock();
}


bool RePro::eventFilter( QObject *watched, QEvent *e )
{
  if ( watched == widget() )
    return RELACSPlugin::eventFilter( watched, e );   // this passes the keyevents to the RePros...
  
  if ( e->type() == QEvent::Shortcut ) {
    // check for grabbed keys:
    QShortcutEvent *se = dynamic_cast<QShortcutEvent *>(e);
    vector<int>::iterator kp = GrabKeys.begin();
    while ( kp != GrabKeys.end() ) {
      if ( se->key() == QKeySequence( *kp ) ) {
	int mask = Qt::META | Qt::SHIFT | Qt::CTRL | Qt::ALT;
	QKeyEvent ke( QEvent::KeyPress, se->key()[0] & ~mask,
		      Qt::KeyboardModifiers( se->key()[0] & mask ) );
	keyPressEvent( &ke );
	return true;
      }
      ++kp;
    }
  }
  return false;
}


int RePro::softStop( void )
{
  SoftStopLock.lock();
  int ss = SoftStop;
  SoftStopLock.unlock();
  return ss;
}


void RePro::setSoftStop( int s )
{
  SoftStopLock.lock();
  SoftStop = s;
  SoftStopLock.unlock();
}


void RePro::clearSoftStop( void )
{
  SoftStopLock.lock();
  SoftStop = 0;
  SoftStopLock.unlock();
}


void RePro::setSoftStopKey( int keycode )
{
  SoftStopKey = keycode;
}


void RePro::tracePlotOn( bool on )
{
  RW->PT->setPlotOn( on );
}


void RePro::tracePlotOff( void )
{
  RW->PT->setPlotOff();
}


void RePro::tracePlotSignal( double length, double offs )
{
  RW->PT->setPlotNextSignal( length, offs );
}



void RePro::tracePlotSignal( void )
{
  RW->PT->setPlotNextSignal();
}


void RePro::tracePlotContinuous( double length )
{
  RW->PT->setPlotContinuous( length );
}


void RePro::tracePlotContinuous( void )
{
  RW->PT->setPlotContinuous();
}


string RePro::macroName( void )
{
  return repros()->macroName();
}


string RePro::macroParam( void )
{
  return repros()->macroParam();
}


string RePro::reproPath( bool v )
{
  // construct path:
  lockRelacsSettings();
  Str path = relacsSettings().text( "repropath" );
  unlockRelacsSettings();
  path.provideSlash();
  path += Str( name() ).lower();
  path.provideSlash();
  if ( v ) {
    path += "v" + Str( version() ).lower();
    path.provideSlash();
  }

  // create path:
  QDir dir;  // current directory
  int sp = path.find( '/', 1 );
  while ( sp > 0 ) {
    string dirpath = path.left( sp );
    if ( ! dir.exists( dirpath.c_str() ) ) {
      if ( ! dir.mkdir( dirpath.c_str() ) )
	return "";
    }
    sp = path.find( '/', sp+1 );
  }

  return path;
}


string RePro::addReProPath( const string &file, bool v )
{
  return reproPath( v ) + file;
}


void RePro::modifyDialog( OptDialog *od )
{
}


void RePro::dialog( void )
{ 
  if ( dialogOpen() )
    return;
  setDialogOpen();

  OptDialog *od = new OptDialog( false, RW );
  od->setCaption( dialogCaption() );
  dialogHeaderWidget( od );
  if ( Options::size( dialogSelectMask() ) <= 0 )
    dialogEmptyMessage( od );
  else {
    // repro options:
    Options::addStyles( OptWidget::LabelBlue, MacroFlag );
    Options::delStyles( OptWidget::LabelBlue, OverwriteFlag );
    Options::addStyles( OptWidget::LabelGreen, OverwriteFlag );
    Options::delStyles( OptWidget::LabelGreen, CurrentFlag );
    Options::addStyles( OptWidget::LabelRed, CurrentFlag );
    DialogOptions = *this;
    string tabhotkeys = "oarc";
    if ( dialogHeader() )
      tabhotkeys += 'h';
    OptWidget *roptw = od->addOptions( *this, dialogSelectMask(), 
				       dialogReadOnlyMask(), dialogStyle(),
				       mutex(), &tabhotkeys );
    connect( roptw, SIGNAL( valueChanged( const Parameter& ) ),
	     this, SLOT( notificationFromDialog( const Parameter& ) ) );
    if ( ! roptw->tabs() ) {
      roptw->setMargins( 2 );
      od->addSeparator();
    }
    modifyDialog( od );
    OptWidget *doptw = od->addOptions( reprosDialogOpts() );
    doptw->setMargins( 2 );
    doptw->setVerticalSpacing ( 4 );
    // buttons:
    od->setRejectCode( 0 );
    od->addButton( "&Ok", OptDialog::Accept, 1 );
    od->addButton( "&Apply", OptDialog::Accept, 1, false );
    od->addButton( "&Run", OptDialog::Accept, 2, false );
    //    od->addButton( "Rese&t", OptDialog::Defaults, 3, false );
    od->addButton( "&Cancel" );
    connect( od, SIGNAL( dialogClosed( int ) ),
	     this, SLOT( dClosed( int ) ) );
    connect( od, SIGNAL( buttonClicked( int ) ),
	     this, SIGNAL( dialogAction( int ) ) );
    connect( od, SIGNAL( valuesChanged( void ) ),
	     this, SIGNAL( dialogAccepted( void ) ) );
  }
  od->exec();
}


void RePro::dClosed( int r )
{
  ConfigDialog::dClosed( r );
  Options::delStyles( OptWidget::LabelBlue, MacroFlag );
  Options::delStyles( OptWidget::LabelGreen, OverwriteFlag );
  Options::delStyles( OptWidget::LabelRed, CurrentFlag );
  Options::delFlags( MacroFlag + OverwriteFlag + CurrentFlag );
}


Options &RePro::overwriteOptions( void )
{
  return OverwriteOpt;
}


string RePro::checkOptions( const string &opttxt )
{
  Options opt = *(Options*)(this);
  opt.read( opttxt );
  return opt.warning();
}


void RePro::setSaving( bool saving )
{
  RW->SF->save( saving );
}


void RePro::noSaving( void )
{
  setSaving( false );
}


void RePro::customEvent( QEvent *qce )
{
  switch ( qce->type() - QEvent::User ) {
  case 8: {
    if ( widget() != 0 )
      widget()->setEnabled( true );
    grabKeys();
    break;
  }
  case 9: {
    if ( widget() != 0 )
      widget()->setEnabled( false );
    releaseKeys();
    break;
  }
  default:
    RELACSPlugin::customEvent( qce );
  }
}


ReProThread::ReProThread( RePro *r )
  : QThread( r ),
    R( r )
{
}


void ReProThread::run( void )
{
  R->run();
}


void ReProThread::usleep( unsigned long usecs )
{
  QThread::usleep( usecs );
}


}; /* namespace relacs */

#include "moc_repro.cc"

