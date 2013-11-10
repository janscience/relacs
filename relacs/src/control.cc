/*
  control.cc
  Base class for widgets that control hardware or analyze data.

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

#include <QAction>
#include <relacs/str.h>
#include <relacs/relacswidget.h>
#include <relacs/control.h>

namespace relacs {


Control::Control( const string &name, const string &pluginset,
		  const string &author, const string &version, 
		  const string &date )
  : RELACSPlugin( "Control: " + name, RELACSPlugin::Plugins,
		  name, pluginset, author, version, date )
{
  Thread = new ControlThread( this );
  Interrupt = false;
}


Control::~Control( void )
{
}


void Control::setName( const string &name )
{
  RELACSPlugin::setName( name );
  setConfigIdent( "Control: " + name );
}


void Control::initDevices( void )
{
}


void Control::addActions( QMenu *menu, bool doxydoc )
{
  menu->addAction( string( uniqueName() + " Dialog..." ).c_str(),
		   this, SLOT( dialog() ) );
  menu->addAction( string( uniqueName() + " Help..." ).c_str(),
		   this, SLOT( help() ) );
  menu->addAction( string( uniqueName() + " Screenshot" ).c_str(),
		   this, SLOT( saveWidget() ) );
  if ( doxydoc )
    menu->addAction( string( uniqueName() + " Doxygen" ).c_str(),
		     this, SLOT( saveDoxygenOptions() ) );
}


void Control::run( void )
{
  timeStamp();
  updateData();
  lockAll();
  setSettings();
  main();
  unlockAll();
}


void Control::main( void )
{
}


bool Control::interrupt( void ) const
{
  InterruptLock.lock();
  bool ir = Interrupt; 
  InterruptLock.unlock();
  return ir;
}


bool Control::waitOnData( double time )
{
  unsigned long t = time == MAXDOUBLE ? ULONG_MAX : (unsigned long)::rint(1.0e3*time);
  unlockStimulusData();
  unlockMetaData();
  RW->UpdateDataWait.wait( mutex(), t );
  readLockData();
  updateTracesEvents();
  unlockData();
  lockMetaData();
  lockStimulusData();
  return interrupt();
}


bool Control::waitOnReProSleep( double time )
{
  unsigned long t = time == MAXDOUBLE ? ULONG_MAX : (unsigned long)::rint(1.0e3*time);
  unlockStimulusData();
  unlockMetaData();
  RW->ReProSleepWait.wait( mutex(), t );
  readLockData();
  updateTracesEvents();
  unlockData();
  lockMetaData();
  lockStimulusData();
  return interrupt();
}


bool Control::waitOnReProFinished( double time )
{
  unsigned long t = time == MAXDOUBLE ? ULONG_MAX : (unsigned long)::rint(1.0e3*time);
  unlockStimulusData();
  unlockMetaData();
  RW->ReProAfterWait.wait( mutex(), t );
  updateData();
  lockMetaData();
  lockStimulusData();
  return interrupt();
}


bool Control::waitOnSessionStart( double time )
{
  unsigned long t = time == MAXDOUBLE ? ULONG_MAX : (unsigned long)::rint(1.0e3*time);
  unlockStimulusData();
  unlockMetaData();
  RW->SessionStartWait.wait( mutex(), t );
  updateData();
  lockMetaData();
  lockStimulusData();
  return interrupt();
}


bool Control::waitOnSessionPrestop( double time )
{
  unsigned long t = time == MAXDOUBLE ? ULONG_MAX : (unsigned long)::rint(1.0e3*time);
  unlockStimulusData();
  unlockMetaData();
  RW->SessionPrestopWait.wait( mutex(), t );
  updateData();
  lockMetaData();
  lockStimulusData();
  return interrupt();
}


bool Control::waitOnSessionStop( double time )
{
  unsigned long t = time == MAXDOUBLE ? ULONG_MAX : (unsigned long)::rint(1.0e3*time);
  unlockStimulusData();
  unlockMetaData();
  RW->SessionStopWait.wait( mutex(), t );
  updateData();
  lockMetaData();
  lockStimulusData();
  return interrupt();
}


bool Control::sleep( double t )
{
  // sleep:
  unlockStimulusData();
  unlockMetaData();
  if ( t > 0.0 ) {
    unsigned long ms = (unsigned long)(1.0e3*t);
    if ( t < 0.001 || ms < 1 ) {
      unlock();
      Thread->usleep( (unsigned long)(1.0e6*t) );
      lock();
    }
    else
      SleepWait.wait( mutex(), ms );
  }
  updateData();
  lockMetaData();
  lockStimulusData();

  // interrupt Control:
  return interrupt();
}


void Control::timeStamp( void )
{
  SleepTime.start();
}


bool Control::sleepOn( double t )
{
  double st = 0.001 * SleepTime.elapsed();
  return sleep( t - st );
}


void Control::start( void )
{
  Thread->start();
}


void Control::requestStop( void )
{
  // terminate thread:
  if ( Thread->isRunning() ) {
    InterruptLock.lock();
    Interrupt = true;
    InterruptLock.unlock();

    // wake up the Control from sleeping:
    SleepWait.wakeAll();
  }
}


bool Control::wait( double time )
{
  if ( time < 0.0 )
    return Thread->wait();
  else
    return Thread->wait( (unsigned long)::floor( 1000.0*time ) );
}


ControlThread::ControlThread( Control *c )
  : QThread( c ),
    C( c )
{
}


void ControlThread::run( void )
{
  C->run();
}


void ControlThread::usleep( unsigned long usecs )
{
  QThread::usleep( usecs );
}


}; /* namespace relacs */

#include "moc_control.cc"

