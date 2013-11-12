/*
  plottrace.h
  Plot trace and spikes.

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

#ifndef _RELACS_PLOTTRACE_H_
#define _RELACS_PLOTTRACE_H_ 1

#include <QWidget>
#include <QPushButton>
#include <QMenu>
#include <QTimer>
#include <vector>
#include <relacs/relacsplugin.h>
#include <relacs/multiplot.h>

namespace relacs {


class RELACSWidget;

/*!
\class PlotTrace
\author Jan Benda
\author Christian Machens
\version 2.0
\brief Plot trace and spikes.
\todo init(): user configurabel plot modes (paged, fixed, contiuous): origin and time units
*/

  /*! Flag for the modes of traces or events, indicating that they should be plotted. */
static const int PlotTraceMode = 0x0008;
  /*! Flag for the modes of events to be used as a trigger signal. */
static const int PlotTriggerMode = 0x0800;
  /*! Flag for the modes of traces, indicating that it should be centered vertically. */
static const int PlotTraceCenterVertically = 0x0100;

class PlotTrace : public RELACSPlugin
{
  Q_OBJECT

  friend class RELACSWidget;

public:

  struct TraceStyle
  {
    TraceStyle( void )
      : PlotWindow( 0 ),
	Line( Plot::Green, 2, Plot::Solid ),
	Point( Plot::Circle, 0, Plot::Green, Plot::Green )
    {};
    int PlotWindow;
    Plot::LineStyle Line;
    Plot::PointStyle Point;
  };

  struct EventStyle
  {
    EventStyle( void )
      : PlotWindow( 0 ),
	Line( Plot::Transparent, 0, Plot::Solid ),
	Point( Plot::Circle, 1, Plot::Yellow, Plot::Yellow ),
	YPos( 0.1 ),
	YCoor( Plot::Graph ),
	YData( false ),
	Size( 6.0 ),
	SizeCoor( Plot::Pixel )
    {};
    int PlotWindow;
    Plot::LineStyle Line;
    Plot::PointStyle Point;
    double YPos;
    Plot::Coordinates YCoor;
    bool YData;
    double Size;
    Plot::Coordinates SizeCoor;
  };

    /*! Construct a PlotTrace. */
  PlotTrace( RELACSWidget *rw, QWidget* parent=0 );
    /*! Destruct a PlotTrace. */
  ~PlotTrace( );

    /*! Switch plotting of raw traces on or off. */
  void setPlotOn( bool on=true );
    /*! Switch plotting of raw traces off. */
  void setPlotOff( void );
    /*! Plot raw traces relative to signal in a window of width \a length seconds
        and the start of the signal \a offs seconds from the left margin. */
  void setPlotSignal( double length, double offs=0.0 );
    /*! Plot raw traces relative to signal while leaving the window size unchanged. */
  void setPlotSignal( void );
    /*! Plot raw traces continuously in a window of width \a length seconds. */
  void setPlotContinuous( double length );
    /*! Plot raw traces continuously while leaving the window size unchanged. */
  void setPlotContinuous( void );

    /*! Set the number of plots necessary for the input traces and events. */
  void resize( void );
    /*! Initialize the plots with the data \a data and \a events. */
  void init( void );
    /*! Add menu entries controlling the time window to \a menu. */
  void addMenu( QMenu *menu );
    /*! Update menu entries toggeling the traces. */
  void updateMenu( void );
    /*! Start plotting with time interval \a time. */
  void start( double time );
    /*! Stop plotting. */
  void stop( void );


public slots:

    /*! Plot voltage traces and events. */
  void plot( void );

    /*! Toggle plot of trace \a trace. */
  void toggle( QAction *trace );

  void zoomOut( void );
  void zoomIn( void );
  void moveLeft( void );
  void moveRight( void );
  void moveStart( void );
  void moveEnd( void );
  void moveToSignal( void );
  void viewSignal( void );
  void moveSignalOffsLeft( void );
  void moveSignalOffsRight( void );
  void viewEnd( void );
  void viewWrapped( void );
  void toggleTrigger( void );
  void manualRange( void );
  void autoRange( void );
  void centerVertically( void );

  void plotOnOff( void );
  void viewToggle( void );
  void toggleManual( void );

  void displayIndex( const deque<int> &traceindex, const deque<int> &eventsindex, double time );


protected:

  virtual void resizeLayout( void );
  virtual void resizeEvent( QResizeEvent *qre );
  virtual void keyPressEvent( QKeyEvent *event );
  virtual void customEvent( QEvent *qce );

  QWidget *ButtonBox;
  QHBoxLayout *ButtonBoxLayout;
  QPushButton *OnOffButton;
  QPushButton *ViewButton;
  QPushButton *ManualButton;

  QPixmap SignalViewIcon;
  QPixmap EndViewIcon;


protected slots:

  void updateRanges( int id );
  void resizePlots( QResizeEvent *qre );


private:

    /*! Different view modes. */
  enum Views {
      /*! Keep the display fixed. */
    FixedView,
      /*! Show the traces relative to the current signal time. */
    SignalView,
      /*! Show the traces relative to the current data. */
    EndView,
      /*! Show the traces wrapped relative to the current data. */
    WrapView,
      /*! Show the traces either in EndView or WrapView according to
	  the ContinuousView variable. */
    ContView,
  };

  void setView( Views mode );

  Views ContinuousView;

  double TimeWindow;
  double TimeOffs;

  Views ViewMode;
  bool PlotChanged;
  double LeftTime;
  double Offset;
  bool Trigger;
  int TriggerSource;
  bool Manual;

  bool Plotting;

  bool AutoOn;
  bool AutoFixed;
  double AutoTime;
  double AutoOffs;

  vector< int > PlotElements;

  vector< QAction* > PlotActions;

  QMenu *Menu;

  QTimer PlotTimer;

  MultiPlot P;
  vector< int > VP;  // the indices of the visible plots

};


}; /* namespace relacs */

#endif /* ! _RELACS_PLOTTRACE_H_ */

