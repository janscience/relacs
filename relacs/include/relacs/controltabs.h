/*
  controltabs.h
  Container organizing Control plugins.

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

#ifndef _RELACS_CONTROLTABS_H_
#define _RELACS_CONTROLTABS_H_ 1

#include <deque>
#include <relacs/plugintabs.h>

namespace relacs {


class Control;
class RELACSWidget;


/*! 
\class ControlTabs
\author Jan Benda
\brief Container organizing Control plugins.
*/


class ControlTabs : public PluginTabs
{
  Q_OBJECT

public:

  ControlTabs( RELACSWidget *rw, QWidget *parent=0 );
  ~ControlTabs( void );

    /*! Creates Control plugins from configuration. */
  void createControls( void );

    /*! Add the menu for configuring Controls to \a menu. */
  void addMenu( QMenu *menu, bool doxydoc );

    /*! Calls initialize() of each Control. */
  void initialize( void );
    /*! Calls setSettings() and initDevices() of each Control. */
  void initDevices( void );

    /*! Start all Control threads. */
  void start( void );
    /*! Kindly requests all Control threads to be stopped. */
  void requestStop( void );
    /*! Wait for the Control threads to finish execution (\a time < 0) 
        or for \a time seconds to be elapsed. */
  void wait( double time=-1.0 );

    /*! Calles modeChanged() of each %Control whenever the mode is changed. */
  void modeChanged( void );
    /*! Inform each Control that some stimulus data have been changed. */
  void notifyStimulusData( void );
    /*! Inform each Control that some meta data have been changed
        in MetaDataSection \a section. */
  void notifyMetaData( const string &section );
    /*! Inform each Control that a new session is started. */
  void sessionStarted( void );
    /*! Inform each Control that the session is stopped. */
  void sessionStopped( bool saved );

    /*! Return the control with index \a index. */
  Control *control( int index );
    /*! Return the control with name \a name. */
  Control *control( const string &name );

protected:

  void keyPressEvent( QKeyEvent *event );
  void keyReleaseEvent( QKeyEvent *event );


private:

  deque< Control* > CN;
  bool HandlingEvent;

};


}; /* namespace relacs */

#endif /* ! _RELACS_CONTROLTABS_H_ */
