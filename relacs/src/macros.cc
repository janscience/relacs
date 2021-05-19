/*
  macros.cc
  Macros execute RePros

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

#include <deque>
#include <QApplication>
#include <QPainter>
#include <QBitmap>
#include <QTextBrowser>
#include <QFileDialog>
#include <relacs/outdata.h>
#include <relacs/str.h>
#include <relacs/strqueue.h>
#include <relacs/optdialog.h>
#include <relacs/messagebox.h>
#include <relacs/rangeloop.h>
#include <relacs/relacswidget.h>
#include <relacs/repro.h>
#include <relacs/repros.h>
#include <relacs/control.h>
#include <relacs/controltabs.h>
#include <relacs/session.h>
#include <relacs/filter.h>
#include <relacs/filterdetectors.h>
#include <relacs/macros.h>

namespace relacs {


const string Macro::StartUpIdent = "startup";
const string Macro::ShutDownIdent = "shutdown";
const string Macro::FallBackIdent = "fallback";
const string Macro::StartSessionIdent = "startsession";
const string Macro::StopSessionIdent = "stopsession";
const string Macro::NoButtonIdent = "nobutton";
const string Macro::NoKeyIdent = "nokey";
const string Macro::NoMenuIdent = "nomenu";
const string Macro::KeepIdent = "keep";
const string Macro::OverwriteIdent = "overwrite";

QPixmap *Macro::BaseIcon = 0;
QPixmap *Macro::StackIcon = 0;
QPixmap *Macro::RunningIcon = 0;
QPixmap *Macro::IdleIcon = 0;
QPixmap *Macro::SessionIcon = 0;

QPixmap *MacroCommand::EnabledIcon = 0;
QPixmap *MacroCommand::DisabledIcon = 0;
QPixmap *MacroCommand::RunningIcon = 0;


Macros::Macros( RELACSWidget *rw, QWidget *parent )
  : QWidget( parent ),
    ConfigClass( "Macros", RELACSPlugin::Core ),
    RW( rw ),
    RPs( 0 ),
    CTs( 0 ),
    MCs(),
    CurrentMacro( -1 ),
    CurrentCommand( 0 ),
    Stack(),
    ResumePos(),
    ResumeMacroOnly( false ),
    ThisCommandOnly( false ),
    ThisMacroOnly( false ),
    Warnings( "" ),
    StartUpIndex( 0 ),
    ShutDownIndex( -1 ),
    FallBackIndex( 0 ),
    StartSessionIndex( -1 ),
    StopSessionIndex( -1 ),
    MacroFile( "" ),
    Menu( 0 ),
    SwitchMenu( 0 ),
    ButtonLayout( 0 ),
    Fatal( false )
{
  ButtonLayout = new QGridLayout( this );
  ButtonLayout->setContentsMargins( 0, 0, 0, 0 );
  ButtonLayout->setSpacing( 0 );
  setLayout( ButtonLayout );

  addText( "file", "Configuration file", "macros.cfg" );
  addText( "mainfile", "Main configuration file", "" );
  addBoolean( "fallbackonreload", "Start fallback macro when loading macros", true );

  Macro::createIcons( 2*fontInfo().pixelSize() );
  MacroCommand::createIcons( 2*fontInfo().pixelSize() );
}


Macros::~Macros( void )
{
  clear( false );
  Macro::destroyIcons();
  MacroCommand::destroyIcons();
}


int Macros::index( const string &macro ) const
{
  if ( macro.empty() )
    return -1;

  Str id = macro;
  id.lower();

  for ( unsigned int k=0; k<MCs.size(); k++ ) {
    Str idr = MCs[k]->name();
    idr.lower();
    if ( idr == id )
      return k;
  }

  return -1;
}


string Macros::macro( void ) const
{
  return MCs[CurrentMacro]->name();
}


string Macros::options( void ) const
{
  if ( CurrentMacro >= 0 && CurrentCommand >= 0 ) {
    return MCs[CurrentMacro]->expandParameter( MCs[CurrentMacro]->command( CurrentCommand )->parameter() );
  }
  else {
    return "";
  }
}


int Macros::size( void ) const
{
  return MCs.size();
}


void Macros::clear( bool keep )
{
  // clear buttons:
  for ( unsigned int k=0; k<MCs.size(); k++ ) {
    if ( MCs[k]->pushButton() != 0 ) {
      MCs[k]->pushButton()->hide();
      ButtonLayout->removeWidget( MCs[k]->pushButton() );
    }
  }

  // clear menu:
  if ( Menu != 0 )
    Menu->clear();
  SwitchMenu = 0;
  SwitchActions.clear();

  // clear macros:
  MacrosType::iterator mp = MCs.begin();
  while ( mp != MCs.end() ) {
    if ( ! ( (*mp)->keep() && keep ) ) {
      delete (*mp);
      mp = MCs.erase( mp );
    }
    else
      ++mp;
  }
}


void Macros::load( const string &file, bool main )
{
  clear();

  // which file to load?
  string macrofile = file;
  if ( main ) {
    macrofile = text( "mainfile" );
    if ( macrofile.empty() ||
	 macrofile == "none" )
      return;
  }
  if ( macrofile.empty() )
    macrofile = text( "file" );

  // open file:
  ifstream macrostream( macrofile.c_str() );
  if ( !macrostream ) {
    Warnings += "Can't read macro configuration file \"<b>";
    Warnings += macrofile + "</b>\".\n";
    return;
  }

  // start to read in file:
  MacroFile = macrofile;
  int linenum = 0;
  string line = "";
  Str strippedline = "";
  while ( getline( macrostream, line ) ) {
    linenum++;
    strippedline = line;
    strippedline.strip( Str::WhiteSpace, "#" );
    // line empty:
    if ( strippedline.empty() )
      continue;
    else {
      if ( strippedline[0] == '$' ) {
	// macro command definition:
	break;
      }
      else {
	Warnings += "First entry needs to be a macro definition starting with '$' in line <b>"
	  + Str( linenum ) + "</b>: \"<b>" + line + "</b>\"";
	macrostream.close();
	return;
      }
    }
  }

  // read in the file:
  while ( ! strippedline.empty() ) {
    Str lineerror = "in line <b>" + Str( linenum )
      + "</b>: \"<b>" + line + "</b>\"";
    if ( strippedline[0] == '$' ) {
      // new macro:
      strippedline.erase( 0, 1 );
      strippedline.strip();
      if ( !strippedline.empty() ) {
	// add new macro:
	MCs.push_back( new Macro( strippedline, this ) );
      }
      else {
	Warnings += "Macro name expected " + lineerror + ".\n";
      }
    }
    else {
      if ( MCs.empty() ) {
	Warnings += "Cannot add parameter to a not existing macro " + lineerror + ".\n";
	return;
      }
      // append parameter:
      MCs.back()->addParameter( strippedline );
    }
    if ( MCs.empty() ) {
      Warnings += "Cannot read commands for a not existing macro " + lineerror + ".\n";
      return;
    }
    // load macro commands:
    strippedline = MCs.back()->load( macrostream, line, linenum, Warnings );
  }

  macrostream.close();
}


bool Macros::check( void )
{
  Fatal = false;

  // check all macros:
  for ( MacrosType::iterator mp = MCs.begin();
	mp != MCs.end(); ) {

    (*mp)->check( Warnings );

    if ( (*mp)->size() == 0 ) {
      Warnings += "Removed empty Macro \"<b>" + (*mp)->name() + "</b>\".\n";
      mp = MCs.erase( mp );
    }
    else {
      // check doublets of overwrite macro:
      if ( (*mp)->overwrite() ) {
	MacrosType::iterator mpp = MCs.begin();
	while ( mpp != mp && mpp != MCs.end() ) {
	  if ( (*mpp)->name() == (*mp)->name() ) {
	    int mk = mp - MCs.begin();
	    mpp = MCs.erase( mpp );
	    mp = MCs.begin() + mk - 1;
	  }
	  ++mpp;
	}
      }

      // check doublets of keep macros:
      MacrosType::iterator mpp = MCs.begin();
      while ( mpp != mp && mpp != MCs.end() && (*mpp)->keep() ) {
	if ( (*mpp)->name() == (*mp)->name() &&
	     (*mp)->keep() ) {
	  mp = MCs.erase( mp );
	  --mp;
	  break;
	}
	++mpp;
      }

      ++mp;
    }

  }  // macros

  // set macros indices:
  for ( MacrosType::iterator mp = MCs.begin();
	mp != MCs.end(); ++mp )
    (*mp)->setMacroIndices();

  // set RePros back to default values:
  for ( int k=0; k<RPs->size(); k++ )
    RPs->repro( k )->setDefaults();

  // no macros?
  if ( MCs.empty() ) {

    if ( !Warnings.empty() )
      Warnings += "\n";
    Warnings += "No Macros specified! Trying to create Macros from RePros...\n";

    for ( int k=0; k<RPs->size(); k++ ) {
      MCs.push_back( new Macro( RPs->repro( k )->name(), this ) );
      MCs.back()->push( new MacroCommand( RPs->repro( k ), "",
					  this, MCs.back() ) );
    }

  }

  // no macros?
  if ( MCs.empty() ) {
    if ( !Warnings.empty() )
      Warnings += "\n";
    Warnings += "No Macros!\n";
    Fatal = true;
  }

  // what about the default startup, etc. settings?
  StartUpIndex = -1;
  ShutDownIndex = -1;
  FallBackIndex = -1;
  StartSessionIndex = -1;
  StopSessionIndex = -1;
  for ( unsigned int k=0; k<MCs.size(); k++ ) {
    if ( MCs[k]->action() & Macro::StartUp ) {
      if ( StartUpIndex >= 0 && StartUpIndex < (int)k )
	MCs[StartUpIndex]->delAction( Macro::StartUp );
      StartUpIndex = k;
    }
    if ( MCs[k]->action() & Macro::ShutDown ) {
      if ( ShutDownIndex >= 0 && ShutDownIndex < (int)k )
	MCs[ShutDownIndex]->delAction( Macro::ShutDown );
      ShutDownIndex = k;
    }
    if ( MCs[k]->action() & Macro::FallBack ) {
      if ( FallBackIndex >= 0 && FallBackIndex < (int)k )
	MCs[FallBackIndex]->delAction( Macro::FallBack );
      FallBackIndex = k;
    }
    if ( MCs[k]->action() & Macro::StartSession ) {
      if ( StartSessionIndex >= 0 && StartSessionIndex < (int)k )
	MCs[StartSessionIndex]->delAction( Macro::StartSession );
      StartSessionIndex = k;
    }
    if ( MCs[k]->action() & Macro::StopSession ) {
      if ( StopSessionIndex >= 0 && StopSessionIndex < (int)k )
	MCs[StopSessionIndex]->delAction( Macro::StopSession );
      StopSessionIndex = k;
    }
  }

  // no fallback macro?
  for ( unsigned int k=0; k<MCs.size() && FallBackIndex < 0; k++ ) {
    for ( Macro::iterator cp = MCs[k]->begin();
	  cp != MCs[k]->end();
	  ++cp ) {
      if ( (*cp)->repro() != 0 ) {
	MCs[k]->setAction( Macro::FallBack );
	FallBackIndex = k;
	break;
      }
    }
  }

  // no RePro within fallback macro?
  if ( FallBackIndex >= 0 && (int)MCs.size() > FallBackIndex ) {
    RePro *lr = 0;
    for ( Macro::iterator cp = MCs[FallBackIndex]->begin();
	  cp != MCs[FallBackIndex]->end();
	  ++cp ) {
      if ( (*cp)->repro() != 0 )
	lr = (*cp)->repro();
    }
    if ( lr == 0 ) {
      if ( !Warnings.empty() )
	Warnings += "\n";
      Warnings += "No RePro found in FallBack Macro!\n";
      MCs[FallBackIndex]->delAction( Macro::FallBack );
      FallBackIndex = -1;
    }
  }

  // no fallback macro?
  if ( FallBackIndex < 0 ) {
    if ( !Warnings.empty() )
      Warnings += "\n";
    Warnings += "No FallBack macro found!\n";
    Fatal = true;
  }

  // no startup macro?
  if ( StartUpIndex < 0 ) {
    StartUpIndex = FallBackIndex;
  }

  // no startsession macro?
  if ( StartSessionIndex < 0 ) {
    StartSessionIndex = FallBackIndex;
  }

  // set macro and command indices:
  for ( unsigned int m=0; m<MCs.size(); m++ )
    MCs[m]->init( m );

  return Fatal;
}


void Macros::checkOptions( void )
{
  Warnings = "";
  for ( MacrosType::iterator mp = MCs.begin();
	mp != MCs.end();
	++mp ) {
      (*mp)->checkOptions( Warnings );
  }
}


void Macros::warning( void )
{
  if ( !Warnings.empty() ) {
    string s = Warnings;
    s.insert( 0, "<li>" );
    string::size_type p = s.find( "\n" );
    while ( p != string::npos ) {
      s.insert( p, "</li>" );
      p += 6;
      string::size_type n = s.find( "\n", p );
      if ( n == string::npos )
	break;
      s.insert( p, "<li>" );
      p = n + 4;
    }
    MessageBox::warning( "RELACS: Macros", "<ul>" + s + "</ul>", 0.0 );

    Warnings.eraseMarkup();
    RW->printlog( "! warning in Macros: " + Warnings );
  }
  Warnings = "";
}


bool Macros::fatal( void ) const
{
  return Fatal;
}


void Macros::create( void )
{
  // base menu:
  Menu->addAction( "&Reload", this, SLOT( reload( void ) ) );
  Menu->addAction( "&Load...", this, SLOT( selectMacros( void ) ) );
  if ( Options::size( "file" ) > 1 ) {
    SwitchMenu = Menu->addMenu( "&Switch" );
    for ( int k=0; k<Options::size( "file" ); k++ )
      SwitchActions.push_back( SwitchMenu->addAction( text( "file", k ).c_str() ) );
    connect( SwitchMenu, SIGNAL( triggered( QAction* ) ),
	     this, SLOT( switchMacro( QAction* ) ) );
  }
  Menu->addAction( "&Skip RePro", this, SLOT( startNextRePro( void ) ),
		   QKeySequence( Qt::Key_S ) );
  Menu->addAction( "&Break", this, SLOT( softBreak( void ) ),
		   QKeySequence( Qt::Key_B ) );
  ResumeAction = Menu->addAction( "Resume", this, SLOT( resume( void ) ),
				  QKeySequence( Qt::Key_R ) );
  ResumeNextAction = Menu->addAction( "Resume next", this,
				      SLOT( resumeNext( void ) ),
				      QKeySequence( Qt::Key_N ) );
  ResumeAction->setEnabled( false );
  ResumeNextAction->setEnabled( false );
  Menu->addSeparator();

  // count macro buttons:
  int nb = 0;
  for ( unsigned int k=0; k<MCs.size(); k++ )
    if ( MCs[k]->button() )
      nb++;

  // number of buttons in a row:
  int cols = nb;
  const int maxcols = 6;
  if ( nb > maxcols ) {
    int r = (nb-1)/maxcols + 1;
    cols = (nb-1)/r + 1;
  }

  // create buttons and menus:
  int fkc = 0;
  int mk = 0;
  int row=0;
  int col=0;
  for ( unsigned int k=0; k<MCs.size(); k++ ) {

    // key code:
    Str keys = MCs[k]->setKey( fkc );

    // menu:
    MCs[k]->addMenu( Menu, mk );

    // button:
    MCs[k]->addButton( keys );
    if ( MCs[k]->pushButton() != 0 ) {
      ButtonLayout->addWidget( MCs[k]->pushButton(), row, col );
      col++;
      if ( col >= cols ) {
	col=0;
	row++;
      }
    }

  }

  setFixedHeight( minimumSizeHint().height() );
  update();
}


void Macros::setMenu( QMenu *menu )
{
  Menu = menu;
}


void Macros::startNextRePro( bool saving, bool enable )
{
  if ( RW->idle() )
    return;

  RW->stopRePro();

  do {

    // clear running icon:
    if ( CurrentMacro >= 0 && CurrentMacro < (int)MCs.size() &&
	 CurrentCommand >= 0 && CurrentCommand < (int)MCs[CurrentMacro]->size() )
	MCs[CurrentMacro]->command( CurrentCommand )->clearIcon();

    CurrentCommand++;

    do {
      if ( ThisCommandOnly || CurrentMacro < 0 ) {
	clearStackButtons();
	CurrentMacro = FallBackIndex;
	CurrentCommand = 0;
	ThisCommandOnly = false;
	setRunButton();
	RW->startedMacro( MCs[CurrentMacro]->name(),
			  MCs[CurrentMacro]->variablesStr() );
	enable = false;
      }
      else if ( CurrentCommand >= (int)MCs[CurrentMacro]->size() ) {
	if ( ThisMacroOnly && Stack.size() == 1 ) {
	  ThisMacroOnly = false;
	  clearStackButtons();
	  if ( CurrentMacro == ShutDownIndex )
	    return;
	  CurrentMacro = FallBackIndex;
	  CurrentCommand = 0;
	  setRunButton();
	  RW->startedMacro( MCs[CurrentMacro]->name(),
			    MCs[CurrentMacro]->variablesStr() );
	}
	else if ( Stack.size() > 0 ) {
	  clearButton();
	  MacroPos mc( Stack.back() );
	  CurrentMacro = mc.MacroID;
	  CurrentCommand = mc.CommandID;
	  MCs[CurrentMacro]->variables() = mc.MacroVariables;
	  Stack.pop_back();
	  setRunButton();
	}
	else {
	  clearButton();
	  if ( CurrentMacro == ShutDownIndex )
	    return;
	  CurrentMacro = FallBackIndex;
	  CurrentCommand = 0;
	  setRunButton();
	  RW->startedMacro( MCs[CurrentMacro]->name(),
			    MCs[CurrentMacro]->variablesStr() );
	}
	enable = false;
      }
    } while ( CurrentCommand < 0 || 
	      CurrentCommand >= (int)MCs[CurrentMacro]->size() );
    
    if ( MCs[CurrentMacro]->command( CurrentCommand )->enabled() || enable ) {
      if ( MCs[CurrentMacro]->command( CurrentCommand )->execute( saving ) )
	break;
      enable = false;
    }

  } while ( CurrentCommand < 0 ||
	    ! MCs[CurrentMacro]->command( CurrentCommand )->enabled() ||
	    MCs[CurrentMacro]->command( CurrentCommand )->repro() == 0 );
}


void Macros::startNextRePro( void )
{
  startNextRePro( true, false );
}


void Macros::startMacro( int macro, int command, bool saving, 
			 bool enable, deque<MacroPos> *newstack )
{
  clearStackButtons();
  clearButton();

  if ( macro >= 0 && macro < (int)MCs.size() )
    CurrentMacro = macro;
  else
    CurrentMacro = FallBackIndex;

  CurrentCommand = command - 1;

  if ( newstack != 0 ) {
    Stack = *newstack;
    setStackButtons();
  }

  ThisCommandOnly = false;
  ThisMacroOnly = false;

  setRunButton();

  RW->startedMacro( MCs[CurrentMacro]->name(),
		    MCs[CurrentMacro]->variablesStr() );

  startNextRePro( saving, enable );
}


void Macros::startUp( void )
{
  if ( StartUpIndex >= 0 )
    startMacro( StartUpIndex, 0, false );
}


void Macros::shutDown( void )
{
  if ( ShutDownIndex >= 0 )
    startMacro( ShutDownIndex, 0, false );
}


void Macros::fallBack( bool saving )
{
  if ( FallBackIndex >= 0 )
    startMacro( FallBackIndex, 0, saving );
}


void Macros::startSession( void )
{
  if ( StartSessionIndex >= 0 )
    startMacro( StartSessionIndex );
}


void Macros::stopSession( void )
{
  if ( StopSessionIndex >= 0 )
    startMacro( StopSessionIndex, 0, false );
}


void Macros::executeMacro( int newmacro, const Str &params )
{
  // put next command on the stack:
  if ( MCs[newmacro]->button() )
    setStackButton();
  // put current macro on stack only if the new macro is a different one or
  // the currentcommand is not the last one:
  if ( newmacro != CurrentMacro || CurrentCommand+1 < MCs[CurrentMacro]->size() )
    Stack.push_back( MacroPos( CurrentMacro, CurrentCommand + 1,
			       MCs[CurrentMacro]->variables() ) );
  // execute the requested macro:
  MCs[newmacro]->variables().setDefaults();
  MCs[newmacro]->variables().read( MCs[CurrentMacro]->expandParameter( params ) );
  CurrentMacro = newmacro;
  CurrentCommand = -1;
  setRunButton();
}


void Macros::setThisOnly( bool macro )
{
  if ( macro )
    ThisMacroOnly = true;
  else
    ThisCommandOnly = true;
}


void Macros::macroStack( Options &stack )
{
  for ( int k = 0; k < (int)Stack.size(); k++ ) {
    stack.newSection( Stack[k].MacroVariables, MCs[Stack[k].MacroID]->name() );
    stack.clearSections();
  }
  stack.addSection( MCs[CurrentMacro]->variables(), MCs[CurrentMacro]->name() );
  stack.clearSections();
}


void Macros::saveConfig( ofstream &str )
{
  string sm = MacroFile;
  for ( int k=0; k<Options::size( "file" ); k++ )
    if ( MacroFile != text( "file", k ) )
      sm += '|' + text( "file", k );
  setText( "file", sm );
  setToDefault( "file" );
  if ( SwitchMenu != 0 ) {
    SwitchMenu->clear();
    SwitchActions.clear();
    for ( int k=0; k<Options::size( "file" ); k++ )
      SwitchActions.push_back( SwitchMenu->addAction( text( "file", k ).c_str() ) );
  }
  ConfigClass::saveConfig( str );
}


void Macros::setRePros( RePros *repros )
{
  RPs = repros;
}


void Macros::setControls( ControlTabs *controls )
{
  CTs = controls;
}


ostream &operator<< ( ostream &str, const Macros &macros )
{
  for ( unsigned int k=0; k<macros.MCs.size(); k++ )
    str << *macros.MCs[k];
  return str;
}


void Macros::loadMacros( const string &file )
{
  load( file );
  check();
  checkOptions();
  warning();
  create();

  if ( boolean( "fallbackonreload" ) ) {
    ResumePos.clear();
    ResumeStack.clear();
    fallBack();
  }
}


void Macros::selectMacros( void )
{
  QFileDialog* fd = new QFileDialog( 0 );
  fd->setFileMode( QFileDialog::ExistingFile );
  fd->setWindowTitle( "Open Macro File" );
  //  fd->setDirectory( Str( (*OP).text( 0 ) ).dir().c_str() );
  QStringList filters;
  filters << "Macro files (m*.cfg)" << "Config files (*.cfg)" << "Any files (*)";
  fd->setNameFilters( filters );
  fd->setViewMode( QFileDialog::List );
  if ( fd->exec() == QDialog::Accepted ) {
    Str filename = "";
    QStringList qsl = fd->selectedFiles();
    if ( qsl.size() > 0 )
      loadMacros( qsl[0].toStdString() );
  }
}


void Macros::switchMacro( QAction *action )
{
  for ( unsigned int k=0;
	k<SwitchActions.size() && (int)k < Options::size( "file" );
	k++ ) {
    if ( action == SwitchActions[k] ) {
      loadMacros( text( "file", k ) );
      break;
    }
  }
}


void Macros::reload( void )
{
  loadMacros( MacroFile );
}


void Macros::reloadRePro( const string &name )
{
  // find RePro:
  RePro *repro = RPs->nameRepro( name );
  if ( repro == 0 ) {
    RW->printlog( "! warning: Macros::reloadRePro() -> RePro " +
		  name + " not found!" );
  }

  for ( MacrosType::iterator mp = MCs.begin();
	mp != MCs.end(); ++mp )
    (*mp)->reloadRePro( repro );
}


void Macros::store( void )
{
  // store macro position:
  if ( CurrentMacro >= 0 && CurrentMacro < (int)MCs.size() ) {
    ResumePos.set( CurrentMacro, CurrentCommand,
		   MCs[CurrentMacro]->variables() );
    ResumeStack = Stack;
    ResumeMacroOnly = ThisMacroOnly;
    ResumeAction->setEnabled( true );
    ResumeNextAction->setEnabled( true );
  }
}


void Macros::softBreak( void )
{
  if ( RW->idle() )
    return;

  if ( CurrentMacro != FallBackIndex && CurrentMacro >= 0 ) {
    store();
    // request stop of current repro:
    MCs[CurrentMacro]->command( CurrentCommand )->repro()->setSoftStop();
    ThisCommandOnly = true;
  }
}


void Macros::hardBreak( void )
{
  if ( CurrentMacro != FallBackIndex && CurrentMacro >= 0 ) {
    store();
    // start fallback macro:
    fallBack();
  }
}


void Macros::resume( void )
{
  if ( RW->idle() )
    return;

  if ( ResumePos.defined() ) {
    // resume:
    MCs[ResumePos.MacroID]->variables() = ResumePos.MacroVariables;

    startMacro( ResumePos.MacroID, ResumePos.CommandID, 
		true, false, &ResumeStack );
    ThisMacroOnly = ResumeMacroOnly;

    // clear resume:
    ResumePos.clear();
    ResumeAction->setEnabled( false );
    ResumeNextAction->setEnabled( false );
  }
}


void Macros::resumeNext( void )
{
  if ( RW->idle() )
    return;

  if ( ResumePos.defined() ) {
    // resume:
    MCs[ResumePos.MacroID]->variables() = ResumePos.MacroVariables;

    startMacro( ResumePos.MacroID, ResumePos.CommandID + 1, 
		true, false, &ResumeStack );
    ThisMacroOnly = ResumeMacroOnly;

    // clear resume:
    ResumePos.clear();
    ResumeAction->setEnabled( false );
    ResumeNextAction->setEnabled( false );
  }
}


void Macros::noMacro( RePro *repro )
{
  clearStackButtons();
  clearButton();

  CurrentMacro = -1;

  RW->startedMacro( "RePro", "" );
}


void Macros::clearButton( void )
{
  if ( CurrentMacro >= 0 && CurrentMacro < (int)MCs.size() ) {
    MCs[CurrentMacro]->clearButton();
    if ( CurrentCommand >= 0 && CurrentCommand < (int)MCs[CurrentMacro]->size() )
      MCs[CurrentMacro]->command( CurrentCommand )->clearIcon();
  }
}


void Macros::setRunButton( void )
{
  if ( CurrentMacro >= 0 && CurrentMacro < (int)MCs.size() )
    MCs[CurrentMacro]->setRunButton();
}


void Macros::setStackButton( void )
{
  if ( CurrentMacro >= 0 && CurrentMacro < (int)MCs.size()  )
    MCs[CurrentMacro]->setStackButton( Stack.empty() );
}


void Macros::setStackButtons( void )
{
  for ( int k = 0; k < (int)Stack.size(); k++ ) {
    int macro = Stack[k].MacroID;
    if ( macro >= 0 && macro < (int)MCs.size() )
      MCs[macro]->setStackButton( (k==0) );
  }
}


void Macros::clearStackButtons( void )
{
  for ( int k = (int)Stack.size()-1; k >= 0; k-- ) {
    int macro = Stack[k].MacroID;
    if ( macro >= 0 && macro < (int)MCs.size() ) {
      MCs[macro]->clearButton();
      int com = Stack[k].CommandID;
      if ( com >= 0 && com < (int)MCs[macro]->size() )
	MCs[macro]->command( com )->clearIcon();
    }
  }
  Stack.clear();
  clearButton();
}


Macros::MacroPos::MacroPos( void )
  : MacroID( -1 ),
    CommandID( -1 ),
    MacroVariables()
{
}


Macros::MacroPos::MacroPos( int macro, int command, Options &var )
  : MacroID( macro ),
    CommandID( command ),
    MacroVariables( var )
{
}


Macros::MacroPos::MacroPos( const MacroPos &mp )
  : MacroID( mp.MacroID ),
    CommandID( mp.CommandID ), 
    MacroVariables( mp.MacroVariables )
{
}


void Macros::MacroPos::set( int macro, int command, Options &var ) 
{
  MacroID = macro;
  CommandID = command;
  MacroVariables = var;
}


void Macros::MacroPos::clear( void ) 
{
  MacroID = -1;
  CommandID = -1;
  MacroVariables.clear();
}


bool Macros::MacroPos::defined( void )
{
  return ( MacroID >= 0 && CommandID >= 0 );
}


Macro::Macro( void )
  : Name( "" ),
    Action( 0 ), 
    Button( true ),
    Menu( true ),
    Key( true ),
    Keep( false ),
    Overwrite( false ),
    KeyCode( 0 ),
    PushButton( 0 ),
    MenuAction( 0 ),
    RunAction( 0 ),
    BottomAction( 0 ),
    MacroNum( -1 ),
    MCs( 0 ),
    DialogOpen( false ),
    Commands()
{
}


Macro::Macro( Str name, Macros *mcs ) 
  : Action( 0 ),
    Button( true ),
    Menu( true ),
    Key( true ), 
    Keep( false ),
    Overwrite( false ),
    KeyCode( 0 ),
    PushButton( 0 ),
    MenuAction( 0 ),
    RunAction( 0 ),
    BottomAction( 0 ),
    MacroNum( -1 ),
    MCs( mcs ),
    DialogOpen( false ),
    Commands()
{
  Variables.clear();
  int cp = name.find( ':' );
  if ( cp > 0 ) {
    addParameter( name.substr( cp+1 ) );
    name.erase( cp );
  }

  if ( name.erase( StartUpIdent, 0, false, 3, Str::WordSpace ) > 0 )
    Action |= StartUp;
  if ( name.erase( ShutDownIdent, 0, false, 3, Str::WordSpace ) > 0 )
    Action |= ShutDown;
  if ( name.erase( FallBackIdent, 0, false, 3, Str::WordSpace ) > 0 ) {
    Action |= FallBack;
    Action |= ExplicitFallBack;
  }
  if ( name.erase( StartSessionIdent, 0, false, 3, Str::WordSpace ) > 0 )
    Action |= StartSession;
  if ( name.erase( StopSessionIdent, 0, false, 3, Str::WordSpace ) > 0 )
    Action |= StopSession;
  if ( name.erase( NoButtonIdent, 0, false, 3, Str::WordSpace ) > 0 ) {
    Button = false;
    Key = false;
  }
  if ( name.erase( NoKeyIdent, 0, false, 3, Str::WordSpace ) > 0 )
    Key = false;
  if ( name.erase( NoMenuIdent, 0, false, 3, Str::WordSpace ) > 0 ) {
    Menu = false;
    Button = false;
    Key = false;
  }
  if ( name.erase( KeepIdent, 0, false, 3, Str::WordSpace ) > 0 )
    Keep = true;
  if ( name.erase( OverwriteIdent, 0, false, 3, Str::WordSpace ) > 0 )
    Overwrite = true;
  Name = name.stripped( Str::WordSpace );
}


Macro::Macro( const Macro &macro ) 
  : Name( macro.Name ), 
    Variables( macro.Variables ),
    Action( macro.Action ), 
    Button( macro.Button ),
    Menu( macro.Menu ),
    Key( macro.Key ),
    Keep( false ),
    Overwrite( false ),
    KeyCode( macro.KeyCode ),
    PushButton( macro.PushButton ),
    MenuAction( macro.MenuAction ), 
    RunAction( macro.RunAction ),
    BottomAction( macro.BottomAction ),
    MacroNum( macro.MacroNum ),
    MCs( macro.MCs ),
    DialogOpen( macro.DialogOpen ),
    Commands( macro.Commands )
{
}


string Macro::name( void ) const
{
  return Name;
}


Options &Macro::variables( void )
{
  return Variables;
}


string Macro::variablesStr( void ) const
{
  string s = "";
  for ( int k=0; k<Variables.size(); k++ ) {
    if ( k > 0 )
      s += "; ";
    s += Variables[k].save();
  }
  return s;
}


void Macro::addParameter( const Str &param )
{
  Variables.load( param, "=", ";" );
  Variables.setToDefaults();
}


string Macro::expandParameter( const Str &params ) const
{
  StrQueue sq( params.stripped().preventLast( ";" ), ";" );
  for ( StrQueue::iterator sp=sq.begin(); sp != sq.end(); ++sp ) {
    // get identifier:
    string name = (*sp).ident( 0, "=", Str::WhiteSpace );
    if ( ! name.empty() ) {
      string value = (*sp).value();
      if ( value[0] == '$' ) {
	const Parameter &p = Variables[ value.substr( 1 ) ];
	if ( p.isNotype() ) {
	  if ( value.find( "rand" ) == 1 ) {
	    Str range = value.substr( 6 );
	    int p = range.find( ')' );
	    string unit;
	    if ( p >=0 ) {
	      unit = range.substr( p+1 );
	      range.erase( p );
	    }
	    double rnd = double( rand() ) / double( RAND_MAX );
	    p = range.find( ".." );
	    if ( p > 0 ) {
	      double min = range.number();
	      range.erase( 0, p+2 );
	      double max = range.number();
	      rnd = (max-min) * rnd + min;
	    }
	    else {
	      StrQueue sq( range, "," );
	      rnd = sq[ (int)floor( rnd*(sq.size()-1.0e-8) ) ].number();
	    }
	    (*sp) = name + "=" + Str( rnd ) + unit;
	  }
	  else
	    MCs->RW->printlog( "! warning in Macro::expandParameter(): " + value +
			      " is not defined as a variable!" );
	}
	else if ( p.isNumber() )
	  (*sp) = name + "=" + p.text( "%g%u" );
	else
	  (*sp) = name + "=" + p.text();
      }
    }
  }
  string newparams;
  sq.copy( newparams, ";" );
  return newparams;
}


int Macro::action( void ) const
{
  return Action;
}


void Macro::setAction( int action )
{
  Action = action;
}


void Macro::delAction( int action )
{
  Action &= ~action;
}


bool Macro::button( void ) const
{
  return Button;
}


MacroButton *Macro::pushButton( void ) const
{
  return PushButton;
}


void Macro::addButton( const string &keys )
{
  if ( Button ) {
    PushButton = new MacroButton( Name + keys, MCs );
    PushButton->show();
    clearButton();
    PushButton->setMinimumSize( PushButton->sizeHint() );
    connect( PushButton, SIGNAL( clicked() ), this, SLOT( launch() ) );
    connect( PushButton, SIGNAL( rightClicked() ), this, SLOT( popup() ) );
    if ( Key ) {
      MenuAction->setShortcut( Qt::SHIFT + KeyCode );
      connect( MenuAction, SIGNAL( triggered() ), this, SLOT( popup() ) );
    }
  }
  else
    PushButton = 0;
}


bool Macro::menu( void ) const
{
  return Menu;
}


void Macro::addMenu( QMenu *menu, int &num )
{
  if ( Menu ) {
    string mt = "";
    if ( num < 36 ) {
      mt += "&";
      if ( num == 0 )
	mt += '0';
      else if ( num < 10 )
	mt += ( '1' + num - 1 );
      else
	mt += ( 'a' + num - 10 );
      mt += " ";
    }
    else
      mt = "  ";
    mt += name();
    QMenu *firstpop = menu->addMenu( mt.c_str() );
    num++;
    MenuAction = firstpop->menuAction();
    RunAction = firstpop->addAction( menuStr().c_str(), this, SLOT( run() ) );
    if ( Key )
      RunAction->setShortcut( KeyCode );
    int n=2;
    if ( ! Variables.empty() ) {
      firstpop->addAction( "&Options", this, SLOT( dialog() ) );	
      n++;
    }
    firstpop->addSeparator();
    BottomAction = 0;

    QMenu *pop = firstpop;

    for ( unsigned int j=0; j<Commands.size(); j++, n++ ) {

      Commands[j]->addMenu( pop );

      if ( n > 20 ) {
	pop->addSeparator();
	QMenu *nextpop = pop->addMenu( "More..." );
	pop = nextpop;
	if ( BottomAction == 0 )
	  BottomAction = nextpop->menuAction();
	n = 0;
      }
    }

    if ( BottomAction == 0 && Commands.size() > 0 )
      BottomAction = Commands.back()->menu()->menuAction();

  }
}


string Macro::menuStr( void ) const
{
  string s = "&Run Macro " + Name;
  int nc = 10 + Name.size();
  int k = 0;
  int i = 0;
  for ( i=0; nc < Macros::MenuWidth && i<Variables.size(); i++, k++ ) {
    if ( k>0 ) {
      s += "; ";
      nc += 2;
    }
    else {
      s += ": ";
      nc += 2;
    }
    string vs = Variables[i].save( Options::FirstOnly );
    s += vs;
    nc += vs.size();
  }
  if ( i < Variables.size() ) {
    s += " ...";
    return s;
  }
  return s;
}


bool Macro::key( void ) const
{
  return Key;
}


string Macro::setKey( int &index )
{
  string keys = "";
  if ( Key ) {
    if ( ( Action & FallBack ) > 0 ) {
      KeyCode = Qt::Key_Escape;
      keys = " (ESC)";
    }
    else if ( index < 12 ) {
      KeyCode = Qt::Key_F1 + index;
      index++;
      keys = Str( index, " (F%d)" );
    }
    else
      KeyCode = 0;
  }
  else
    KeyCode = 0;
  return keys;
}


void Macro::clear( void )
{
  MenuAction = 0;
  Key = 0;
  if ( PushButton != 0 )
    delete PushButton;
  PushButton = 0;
}


bool Macro::keep( void ) const
{
  return Keep;
}


bool Macro::overwrite( void ) const
{
  return Overwrite;
}


int Macro::size( void ) const
{
  return Commands.size();
}


MacroCommand *Macro::command( int index )
{
  return Commands[index];
}


void Macro::push( MacroCommand *mc )
{
  return Commands.push_back( mc );
}


Macro::iterator Macro::begin( void )
{
  return Commands.begin();
}


Macro::iterator Macro::end( void )
{
  return Commands.end();
}


string Macro::load( ifstream &macrostream, string &line, int &linenum,
		    string &warnings )
{
  // this is called after a macro definition was read in,
  // so there might be more parameter for that macro:
  bool appendable = true;
  bool appendmacro = true;
  bool appendparam = true;
  while ( getline( macrostream, line ) ) {
    linenum++;
    Str strippedline = line;
    strippedline.strip( Str::WhiteSpace, "#" );
    // line empty:
    if ( strippedline.empty() ) {
      appendable = false;
      continue;
    }

    // macro:
    if ( strippedline[0] == '$' )
      return strippedline;

    // line error message:
    Str lineerror = " in line <b>" + Str( linenum )
      + "</b>: \"<b>" + line + "</b>\"";

    // create MacroCommand:
    MacroCommand *mc = new MacroCommand( strippedline, MCs, this );
    if ( appendable &&
	 mc->command() == MacroCommand::UnknownCom &&
	 mc->parameter().empty() &&
	 ( ( appendparam && strippedline.find( '=' ) >= 0 ) ||
	   ( !appendparam && line.find_first_not_of( Str::WhiteSpace ) != string::npos ) ) ) {
      delete mc;
      if ( appendmacro )
	return strippedline;
      else
	Commands.back()->addParameter( strippedline, appendparam );
    }
    else if ( mc->command() != MacroCommand::StartSessionCom &&
	      mc->command() != MacroCommand::StopSessionCom &&
	      mc->command() != MacroCommand::ShutdownCom &&
	      mc->command() != MacroCommand::ShellCom &&
	      mc->command() != MacroCommand::FilterCom &&
	      mc->command() != MacroCommand::DetectorCom &&
	      mc->command() != MacroCommand::MessageCom &&
	      mc->command() != MacroCommand::BrowseCom &&
	      mc->name().empty() ) {
      delete mc;
      warnings += "Missing name of action" + lineerror + ".\n";
      appendable = false;
    }
    else {
      // unknown command with name is a RePro:
      if ( mc->command() == MacroCommand::UnknownCom &&
	   !mc->name().empty() )
	  mc->setReProCommand();
      // still unknown command:
      if ( mc->command() == MacroCommand::UnknownCom ) {
	delete mc;
	warnings += "Unknown command type" + lineerror + ".\n";
	appendable = false;
      }
      else if ( mc->command() != MacroCommand::StartSessionCom &&
		mc->command() != MacroCommand::StopSessionCom &&
		mc->command() != MacroCommand::ShutdownCom &&
		mc->name().empty() && mc->parameter().empty() ) {
	delete mc;
	warnings += "Incomplete or empty specification of action " + Str( mc->command() )
	  + lineerror + ".\n";
	appendable = false;
      }
      else {
	Commands.push_back( mc );
	if ( mc->command() == MacroCommand::StartSessionCom ||
	     mc->command() == MacroCommand::StopSessionCom ||
	     mc->command() == MacroCommand::ShutdownCom ||
	     mc->command() == MacroCommand::BrowseCom ||
	     mc->command() == MacroCommand::SwitchCom ) {
	  // these commands get nothing appended:
	  appendable = false;
	}
	else if ( mc->command() == MacroCommand::ShellCom ||
		  mc->command() == MacroCommand::MessageCom ) {
	  // these commands get normal strings appended:
	  appendable = true;
	  appendmacro = false;
	  appendparam = false;
	}
	else {
	  // all other commands get key=value pairs appended:
	  appendable = true;
	  appendmacro = false;
	  appendparam = true;
	}
      }
    }

  }
  return "";
}


void Macro::check( string &warnings )
{
  // check the commands:
  for ( iterator cp = Commands.begin();
	cp != Commands.end(); ) {

    if ( (*cp)->check( warnings ) ) {

      if ( (*cp)->command() == MacroCommand::ReProCom ) {
	// expand ranges:
	Str ps = (*cp)->parameter();
	deque <RangeLoop> rls;
	deque <int> lb;
	deque <int> rb;
	// find ranges:
	int o = ps.find( "(" );
	if ( o > 0 && ps[o-1] == 'd' )
	  o = -1;
	while ( o >= 0 ) {
	  int c = ps.findBracket( o, "(", "" );
	  if ( c > 0 ) {
	    lb.push_back( o );
	    rb.push_back( c );
	    rls.push_back( RangeLoop( ps.mid( o+1, c-1 ) ) );
	    o = ps.find( "(", c+1 );
	  }
	  else
	    o = -1;
	}
	if ( rls.empty() )
	  ++cp;
	else {
	  // erase original repro:
	  MacroCommand omc( *(*cp) );
	  cp = Commands.erase( cp );
	  // loop ranges:
	  for ( unsigned int k=0; k<rls.size(); k++ )
	    rls[k].reset();
	  while ( !rls[0] ) {
	    // create parameter:
	    Str np = ps;
	    for ( int j=int(rls.size())-1; j>=0; j-- ) {
	      int ei = rb[j] + 1;
	      int si = np.findFirstNot( Str::WhiteSpace, ei );
	      if ( si > 0 )
		si = np.findFirst( Str::WhiteSpace + ';', si );
	      if ( si > 0 )
		ei = si;
	      np.insert( ei, 1, '*' );
	      np.replace( lb[j], rb[j] - lb[j] + 1, Str( *rls[j] ) );
	    }
	    // add repro:
	    omc.setParameter( np );
	    if ( cp != Commands.end() ) {
	      cp = Commands.insert( cp, new MacroCommand( omc ) );
	      ++cp;
	    }
	    else {
	      Commands.push_back( new MacroCommand( omc ) );
	      cp = Commands.end();
	    }

	    // increment range looops:
	    for ( int k = int(rls.size())-1; k >= 0; k-- ) {
	      ++rls[k];
	      if ( !rls[k] )
		break;
	      else if ( k > 0 )
		rls[k].reset();
	    }
	  }
	}
      }
      else
	++cp;
    }
    else
      cp = Commands.erase( cp );

  }  // commands
}


void Macro::checkOptions( string &warnings )
{
  // check the commands:
  for ( iterator cp = Commands.begin();
	cp != Commands.end();
	++cp ) {
    (*cp)->checkOptions( warnings );
  }
}


void Macro::setMacroIndices( void )
{
  for ( iterator cp = Commands.begin();
	cp != Commands.end(); ++cp )
    if ( (*cp)->command() == MacroCommand::MacroCom )
      (*cp)->setMacroIndex( MCs->index( (*cp)->name() ) );
}


void Macro::init( int macronum )
{
  MacroNum = macronum;
  int c = 0;
  for ( iterator cp = Commands.begin();
	cp != Commands.end();
	++cp ) {
    (*cp)->init( macronum, c );
    c++;
  }
}


void Macro::reloadRePro( RePro *repro )
{
  for ( iterator cp = Commands.begin();
	cp != Commands.end();
	++cp )
    (*cp)->reloadRePro( repro );
}


void Macro::clearButton( void )
{
  if ( PushButton != 0 ) {
    if ( Action & StartSession )
      PushButton->setIcon( *SessionIcon );
    else
      PushButton->setIcon( *IdleIcon );
  }
}


void Macro::setRunButton( void )
{
  if ( PushButton != 0 )
    PushButton->setIcon( *RunningIcon );
}


void Macro::setStackButton( bool base )
{
  if ( PushButton != 0 ) {
    if ( base )
      PushButton->setIcon( *BaseIcon );
    else
      PushButton->setIcon( *StackIcon );
  }
}


void Macro::createIcons( int size )
{
  int my = size - 2;
  int mx = my;

  SessionIcon = new QPixmap( mx+2, my+2 );
  QPainter p;
  p.begin( SessionIcon );
  p.eraseRect( SessionIcon->rect() );
  p.setPen( QPen( Qt::black, 1 ) );
  p.setBrush( Qt::black );
  QPolygon pa( 3 );
  pa.setPoint( 0, mx/3, 0 );
  pa.setPoint( 1, mx/3, my );
  pa.setPoint( 2, mx, my/2 );
  p.drawPolygon( pa );
  p.end();
  SessionIcon->setMask( SessionIcon->createHeuristicMask() );

  BaseIcon = new QPixmap( mx+2, my+2 );
  p.begin( BaseIcon );
  p.eraseRect( BaseIcon->rect() );
  p.setPen( QPen( Qt::black, 1 ) );
  p.setBrush( Qt::red );
  p.drawEllipse( mx/4, (my-mx*3/4)/2, mx*3/4, mx*3/4 );
  p.end();
  BaseIcon->setMask( BaseIcon->createHeuristicMask() );

  StackIcon = new QPixmap( mx+2, my+2 );
  p.begin( StackIcon );
  p.eraseRect( StackIcon->rect() );
  p.setPen( QPen( Qt::black, 1 ) );
  p.setBrush( Qt::yellow );
  p.drawEllipse( mx/4, (my-mx*3/4)/2, mx*3/4, mx*3/4 );
  p.end();
  StackIcon->setMask( StackIcon->createHeuristicMask() );

  RunningIcon = new QPixmap( mx+2, my+2 );
  p.begin( RunningIcon );
  p.eraseRect( RunningIcon->rect() );
  p.setPen( QPen( Qt::black, 1 ) );
  p.setBrush( Qt::green );
  p.drawEllipse( mx/4, (my-mx*3/4)/2, mx*3/4, mx*3/4 );
  p.end();
  RunningIcon->setMask( RunningIcon->createHeuristicMask() );

  IdleIcon = new QPixmap( mx+2, my+2 );
  p.begin( IdleIcon );
  p.eraseRect( IdleIcon->rect() );
  p.end();
  IdleIcon->setMask( IdleIcon->createHeuristicMask() );
}


void Macro::destroyIcons( void )
{
  delete SessionIcon;
  delete BaseIcon;
  delete StackIcon;
  delete RunningIcon;
  delete IdleIcon;
  SessionIcon = 0;
  BaseIcon = 0;
  StackIcon = 0;
  RunningIcon = 0;
  IdleIcon = 0;
}


ostream &operator<< ( ostream &str, const Macro &macro )
{
  str << "Macro " << macro.MacroNum+1 << ": " << macro.Name 
      << ( macro.action() & Macro::StartUp ? " startup" : "" )
      << ( macro.action() & Macro::ShutDown ? " shutdown" : "" )
      << ( macro.action() & Macro::FallBack ? " fallback" : "" ) 
      << ( macro.action() & Macro::StartSession ? " startsession" : "" )
      << ( macro.action() & Macro::StopSession ? " stopsession" : "" )
      << ( macro.button() ? "" : " nobutton" )
      << ( macro.menu() ? "" : " nomenu" );
  if ( macro.MenuAction != 0 )
    str << "Action: " << macro.MenuAction->shortcut().toString().toStdString();
  str << " -> " << macro.Variables.save() << '\n';
  for ( int j=0; j<macro.size(); j++ )
    str << *macro.Commands[j];
  return str;
}


void Macro::run( void )
{
  if ( KeyCode == Qt::Key_Escape &&
       qApp->focusWidget() != MCs->window() ) {
    MCs->window()->setFocus();
    MCs->RW->displayData();
  }
  else
    launch();
}


void Macro::launch( void )
{
  MCs->window()->setFocus();
  if ( Action & FallBack )
    MCs->store();
  Variables.setDefaults();
  MCs->startMacro( MacroNum );
  // Note: in case of a switch command, *this does not exist anymore!
}


void Macro::popup( void )
{
  if ( BottomAction != 0 ) {
    QPoint p = PushButton->mapToGlobal( QPoint( 0, -30 ) );
    MenuAction->menu()->popup( p, BottomAction );
  }
}


void Macro::dialog( void )
{
  if ( DialogOpen )
    return;

  DialogOpen = true;
  // create and exec dialog:
  OptDialog *od = new OptDialog( false, MCs );
  od->setCaption( "Macro " + Name + " Variables" );
  if ( ! Variables.empty() ) {
    string tabhotkeys = "oarc";
    od->addOptions( Variables, 0, 0, 0, 0, &tabhotkeys );
    od->addSeparator();
  }
  od->setVerticalSpacing( int(9.0*exp(-double(Variables.size())/14.0))+1 );
  od->setRejectCode( 0 );
  od->addButton( "&Ok", OptDialog::Accept, 1 );
  od->addButton( "&Apply", OptDialog::Accept, 1, false );
  od->addButton( "&Run", OptDialog::Accept, 2, false );
  //  od->addButton( "&Reset", OptDialog::Defaults );
  od->addButton( "&Cancel" );
  connect( od, SIGNAL( dialogClosed( int ) ),
	   this, SLOT( dialogClosed( int ) ) );
  connect( od, SIGNAL( buttonClicked( int ) ),
	   this, SLOT( dialogAction( int ) ) );
  connect( od, SIGNAL( valuesChanged( void ) ),
	   this, SLOT( acceptDialog( void ) ) );
  od->exec();
}


void Macro::acceptDialog( void )
{
  Variables.setToDefaults();
  // update menu:
  RunAction->setText( menuStr().c_str() );
}


void Macro::dialogAction( int r )
{
  if ( r == 2 )
    MCs->startMacro( MacroNum );
  // Note: in case of a switch command, *this does not exist anymore!
}


void Macro::dialogClosed( int r )
{
  DialogOpen = false;
}


MacroCommand::MacroCommand( void ) 
  : Command( UnknownCom ),
    Name( "" ),
    Params( "" ),
    RP( 0 ),
    CO(),
    DO( 0 ),
    MacroIndex( 0 ),
    FilterCommand( 0 ),
    DetectorCommand( 0 ),
    AutoConfigureTime( 0.0 ),
    CT( 0 ),
    TimeOut( 0.0 ),
    Enabled( true ),
    EnabledAction( 0 ),
    MacroNum( 0 ),
    CommandNum( 0 ),
    MC( 0 ),
    MCs( 0 ),
    DialogOpen( false ),
    MacroVars(),
    MenuShortcut( "" ),
    SubMenu( 0 )
{
}


MacroCommand::MacroCommand( const string &line, Macros *mcs, Macro *mc ) 
  : Command( UnknownCom ),
    Name( "" ),
    Params( "" ),
    RP( 0 ),
    CO(), 
    DO( 0 ),
    MacroIndex( 0 ),
    FilterCommand( 0 ),
    DetectorCommand( 0 ),
    AutoConfigureTime( 0.0 ),
    CT( 0 ),
    TimeOut( 0.0 ),
    Enabled( true ),
    EnabledAction( 0 ),
    MacroNum( 0 ),
    CommandNum( 0 ),
    MC( mc ),
    MCs( mcs ),
    DialogOpen( false ),
    MacroVars(),
    MenuShortcut( "" ),
    SubMenu( 0 )
{
  // break string into name and parameter:
  size_t pos = line.find_first_of( ':' );
  if ( pos != string::npos ) {
    Name = line.substr( 0, pos );
    Params = line.substr( pos+1 );
    Params.strip();
  }
  else
    Name = line;

  // Command disabled?
  if ( line[0] == '!' ) {
    Enabled = false;
    Name.erase( 0, 1 );
  }

  // Command type:
  if ( Name.eraseFirst( "repro", 0, false, 3, Str::WhiteSpace ) )
    Command = ReProCom;
  else if ( Name.eraseFirst( "macro", 0, false, 3, Str::WhiteSpace ) )
    Command = MacroCom;
  else if ( Name.eraseFirst( "filter", 0, false, 3, Str::WhiteSpace ) ) {
    Command = FilterCom;
    if ( Params.eraseFirst( "save", 0, false, 3, Str::WhiteSpace ) )
      FilterCommand = 1;
    else if ( Params.eraseFirst( "autoconf", 0, false, 3, Str::WhiteSpace ) ) {
      FilterCommand = 2;
      AutoConfigureTime = Params.number( 1.0 );
      Params.clear();
    }
  }
  else if ( Name.eraseFirst( "detector", 0, false, 3, Str::WhiteSpace ) ) {
    Command = DetectorCom;
    if ( Params.eraseFirst( "save", 0, false, 3, Str::WhiteSpace ) )
      DetectorCommand = 1;
    else if ( Params.eraseFirst( "autoconf", 0, false, 3, Str::WhiteSpace ) ) {
      DetectorCommand = 2;
      AutoConfigureTime = Params.number( 1.0 );
      Params.clear();
    }
  }
  else if ( Name.eraseFirst( "control", 0, false, 3, Str::WhiteSpace ) )
    Command = ControlCom;
  else if ( Name.eraseFirst( "switch", 0, false, 3, Str::WhiteSpace ) )
    Command = SwitchCom;
  else if ( Name.eraseFirst( "startsession", 0, false, 3, Str::WhiteSpace ) )
    Command = StartSessionCom;
  else if ( Name.eraseFirst( "stopsession", 0, false, 3, Str::WhiteSpace ) )
      Command = StopSessionCom;
  else if ( Name.eraseFirst( "shutdown", 0, false, 3, Str::WhiteSpace ) )
      Command = ShutdownCom;
  else if ( Name.eraseFirst( "shell", 0, false, 3, Str::WhiteSpace ) )
    Command = ShellCom;
  else if ( Name.eraseFirst( "message", 0, false, 3, Str::WhiteSpace ) ) {
    Command = MessageCom;
    int n=0;
    TimeOut = Name.number( 0.0, 0, &n );
    if ( n > 0 )
      Name.erase( 0, n );
  }
  else if ( Name.eraseFirst( "browse", 0, false, 3, Str::WhiteSpace ) )
    Command = BrowseCom;

  Name.strip( Str::WhiteSpace );
}


MacroCommand::MacroCommand( RePro *repro, const string &params,
			    Macros *mcs, Macro *mc )
  : Command( ReProCom ),
    Name( repro->name() ),
    Params( params ),
    RP( repro ),
    CO(), 
    DO( 0 ),
    MacroIndex( 0 ),
    FilterCommand( 0 ),
    DetectorCommand( 0 ),
    AutoConfigureTime( 0.0 ),
    CT( 0 ),
    TimeOut( 0.0 ),
    Enabled( true ),
    EnabledAction( 0 ),
    MacroNum( 0 ),
    CommandNum( 0 ),
    MC( mc ),
    MCs( mcs ),
    DialogOpen( false ),
    MacroVars(),
    MenuShortcut( "" ),
    SubMenu( 0 )
{
}


MacroCommand::MacroCommand( const MacroCommand &com ) 
  : Command( com.Command ),
    Name( com.Name ),
    Params( com.Params ),
    RP( com.RP ),
    CO( com.CO ),
    DO( com.DO ),
    MacroIndex( com.MacroIndex ),
    FilterCommand( com.FilterCommand ), 
    DetectorCommand( com.DetectorCommand ), 
    AutoConfigureTime( com.AutoConfigureTime ),
    CT( com.CT ),
    TimeOut( com.TimeOut ),
    Enabled( com.Enabled ),
    EnabledAction( com.EnabledAction ),
    MacroNum( com.MacroNum ),
    CommandNum( com.CommandNum ),
    MC( com.MC ),
    MCs( com.MCs ),
    DialogOpen( com.DialogOpen ),
    MacroVars( com. MacroVars ),
    MenuShortcut( com.MenuShortcut ),
    SubMenu( com.SubMenu )
{
}


MacroCommand::CommandType MacroCommand::command( void ) const
{
  return Command;
}


string MacroCommand::name( void ) const
{
  return Name;
}


void MacroCommand::setName( const string &name )
{
  Name = name;
}


string MacroCommand::parameter( void ) const
{
  return Params;
}


void MacroCommand::setParameter( const string &parameter )
{
  Params = parameter;
}


void MacroCommand::addParameter( const string &s, bool addsep )
{
  if ( addsep && ! Params.empty() )
    Params.provideLast( ';' );
  Params.provideLast( ' ' );
  Params += s;
}


bool MacroCommand::enabled( void ) const
{
  return Enabled;
}


void MacroCommand::clearIcon( void )
{
  SubMenu->menuAction()->setIcon( Enabled ? *EnabledIcon : *DisabledIcon );
}


void MacroCommand::setRunIcon( void )
{
  SubMenu->menuAction()->setIcon( *RunningIcon );
}


RePro *MacroCommand::repro( void )
{
  return RP;
}


void MacroCommand::setReProCommand( void )
{
  Command = ReProCom;
}


void MacroCommand::setMacroIndex( int index )
{
  MacroIndex = index;
}


void MacroCommand::init( int macronum, int commandnum )
{
  MacroNum = macronum;
  CommandNum = commandnum;
}


QMenu *MacroCommand::menu( void )
{
  return SubMenu;
}


void MacroCommand::addMenu( QMenu *menu )
{
  string s = "";
  if ( CommandNum < 36 ) {
    s = "&";
    if ( CommandNum == 0 )
      s += '0';
    else if ( CommandNum < 10 )
      s += ( '1' + CommandNum - 1 );
    else
      s += ( 'a' + CommandNum - 10 );
    s += " ";
  }
  else
    s = "  ";
  MenuShortcut = s;
  if ( Command == MacroCom ) {
    s += "Macro " + Name;
    if ( !Params.empty() ) {
      s += ": ";
      int index = 0;
      int nc = 10 + Name.size();
      for ( int i=0; nc+index < Macros::MenuWidth && index >= 0; i++ ) {
	index = Params.find( ';', index+1 );
      }
      if ( index < 0 || Params.substr( index+1 ).stripped().empty() )
	s += Params.preventedLast( ';' );
      else
	s += Params.substr( 0, index ) + " ...";
    }
  }
  else if ( Command == ShellCom )
    s += "Shell " + Name;
  else if ( Command == FilterCom ) {
    s += "Filter " + Name + ": ";
    if ( FilterCommand == 1 )
      s += "save";
    else
      s += "auto-configure " + Str( AutoConfigureTime ) + "s";
  }
  else if ( Command == DetectorCom ) {
    s += "Detector " + Name + ": ";
    if ( DetectorCommand == 1 )
      s += "save";
    else
      s += "auto-configure " + Str( AutoConfigureTime ) + "s";
  }
  else if ( Command == SwitchCom )
    s += "Switch to " + Name;
  else if ( Command == StartSessionCom )
    s += "Start Session";
  else if ( Command == StopSessionCom )
    s += "Stop Session";
  else if ( Command == ShutdownCom )
    s += "Shut down";
  else if ( Command == MessageCom ) {
    s += "Message " + Name;
    if ( !Params.empty() ) {
      Str ps = Params;
      ps.eraseMarkup();
      if ( ps.size() > 40 ) {
	ps.erase( 36 );
	ps += " ...";
      }
      s += ": " + ps;
    }
  }
  else if ( Command == BrowseCom ) {
    s += "Browse " + Params;
  }
  else if ( Command == ControlCom ) {
    s += "Control " + Name + ": " + Params;
  }
  else {
    s += "RePro " + Name;
    if ( !Params.empty() ) {
      s += ": ";
      int index = 0;
      int nc = 10 + Name.size();
      for ( int i=0; nc+index < Macros::MenuWidth && index >= 0; i++ ) {
	index = Params.find( ';', index+1 );
      }
      if ( index < 0 || Params.substr( index+1 ).stripped().empty() )
	s += Params.preventedLast( ';' );
      else
	s += Params.substr( 0, index ) + " ...";
    }
  }

  SubMenu = menu->addMenu( s.c_str() );
  SubMenu->menuAction()->setIcon( Enabled ? *EnabledIcon : *DisabledIcon );

  if ( CommandNum+1 < (int)MC->size() ) {
    SubMenu->addAction( "&Start macro here", this, SLOT( start() ) );
    SubMenu->addAction( "&Run only this", this, SLOT( run() ) );
  }
  else {
    SubMenu->addAction( "&Run", this, SLOT( run() ) );
  }
  if ( RP != 0 ) {
    SubMenu->addAction( "&Options...", this, SLOT( dialog() ) );
    SubMenu->addAction( "&View", this, SLOT( view() ) );
    SubMenu->addAction( "&Load", this, SLOT( reload() ) );
    SubMenu->addAction( "&Help...", this, SLOT( help() ) );
  }
  else if ( Command == MacroCom &&
	    ! Params.empty() ) {
    SubMenu->addAction( "&Options...", this, SLOT( dialog() ) );
  }
  /*
  else if ( CT != 0 ) {
    SubMenu->addAction( "&Options...", this, SLOT( dialog() ) );
    SubMenu->addAction( "&Help...", this, SLOT( help() ) );
  }
  */
  EnabledAction = SubMenu->addAction( Enabled ? "&Disable" : "&Enable",
				      this, SLOT( enable() ) );

  if ( ( Command == MacroCom || Command == ReProCom || Command == ControlCom ) &&
       !Params.empty() ) {
    SubMenu->addSeparator();
    SubMenu->addAction( "Parameter:" );
    int index = 0;
    int nindex = 0;
    for ( int i=0; nindex >= 0 && i<20; i++ ) {
      nindex = Params.find( ';', index );
      string ms = Params.mid( index, nindex-1 ).stripped();
      if ( !ms.empty() )
	SubMenu->addAction( string( "  " + ms ).c_str() );
      index = nindex+1;
    }
  }

}


bool MacroCommand::check( string &warnings )
{
  if ( Command == MacroCom ) {
    if ( MCs->index( name() ) < 0 ) {
      warnings += "Removed unknown Macro \"<b>";
      warnings += name();
      warnings += "</b>\" in Macro \"<b>";
      warnings += MC->name();
      warnings += "</b>\".\n";
      return false;
    }
  }

  else if ( Command == FilterCom ) {
    if ( ! name().empty() &&
	 ! MCs->RW->filterDetectors()->exist( name() ) ) {
      warnings += "Removed unknown Filter \"<b>";
      warnings += name();
      warnings += "</b>\" in Macro \"<b>";
      warnings += MC->name();
      warnings += "</b>\".\n";
      return false;
    }
  }

  else if ( Command == DetectorCom ) {
    if ( ! name().empty() &&
	 ! MCs->RW->filterDetectors()->exist( name() ) ) {
      warnings += "Removed unknown Detector \"<b>";
      warnings += name();
      warnings += "</b>\" in Macro \"<b>";
      warnings += MC->name();
      warnings += "</b>\".\n";
      return false;
    }
  }

  else if ( Command == SwitchCom ) {
    ifstream f( name().c_str() );
    if ( ! f.good() ) {
      warnings += "Removed switch to unknown file \"<b>";
      warnings += name();
      warnings += "</b>\" in Macro \"<b>";
      warnings += MC->name();
      warnings += "</b>\".\n";
      return false;
    }
  }

  else if ( Command == StartSessionCom ) {
    return true;
  }

  else if ( Command == StopSessionCom ) {
    return true;
  }

  else if ( Command == ShutdownCom ) {
    return true;
  }

  else if ( Command == ShellCom ) {
    return true;
  }

  else if ( Command == MessageCom ) {
    if ( parameter().empty() ) {
      setParameter( name() );
      setName( "RELACS Message" );
    }
    if ( name().empty() ) {
      setName( "RELACS Message" );
    }
  }

  else if ( Command == BrowseCom ) {
    if ( parameter().empty() ) {
      setParameter( name() );
      setName( "RELACS Info" );
    }
    if ( name().empty() ) {
      setName( "RELACS Info" );
    }
  }

  else if ( Command == ReProCom ) {
    // find RePro:
    RePro *repro = MCs->RPs->nameRepro( name() );
    if ( repro == 0 ) {
      warnings += "Removed unknown RePro \"<b>";
      warnings += name();
      warnings += "</b>\" in Macro \"<b>";
      warnings += MC->name();
      warnings += "</b>\".\n";
      return false;
    }
    else {
      RP = repro;
      if ( repro != 0 )
	Name = repro->uniqueName();
    }

  }

  else if ( Command == ControlCom ) {
    // find Control:
    Control *control = MCs->CTs->control( name() );
    if ( control == 0 ) {
      warnings += "Removed unknown Control \"<b>";
      warnings += name();
      warnings += "</b>\" in Macro \"<b>";
      warnings += MC->name();
      warnings += "</b>\".\n";
      return false;
    }
    else {
      CT = control;
      if ( control != 0 )
	Name = control->uniqueName();
    }

  }

  else {
    warnings += "Removed command \"<b>";
    warnings += name();
    warnings += "</b>\" of unknown type in Macro \"<b>";
    warnings += MC->name();
    warnings += "</b>\".\n";
    return false;
  }

  return true;
}


void MacroCommand::checkOptions( string &warnings )
{
  if ( Command == ReProCom ) {
    // check options:
    string error = RP->checkOptions( MC->expandParameter( parameter() ) );
    if ( error.size() > 0 ) {
      warnings += "Invalid options for RePro \"<b>";
      warnings += RP->name();
      warnings += "</b>\" from Macro \"<b>";
      warnings += Name;
      warnings += "</b>\":<br>";
      warnings += error;
      warnings += ".\n";
    }
  }

  /*
  else if ( Command == ControlCom ) {
  }
  */
}


bool MacroCommand::execute( bool saving )
{
  setRunIcon();

  // execute macro:
  if ( Command == MacroCom ) {
    MCs->executeMacro( MacroIndex, Params );
  }
  // execute shell command:
  else if ( Command == ShellCom ) {
    string com = "nice " + Name + " " + Params;
    MCs->RW->printlog( "execute \"" + com + "\"" );
    int r = system( com.c_str() );
    MCs->RW->printlog( "execute returned " + Str( r ) );
  }
  // filter:
  else if ( Command == FilterCom ) {
    if ( FilterCommand == 2 && Name.empty() ) {
      MCs->RW->printlog( "filter \"ALL\": auto-configure " +
			 Str( AutoConfigureTime ) + "s" );
      MCs->RW->filterDetectors()->autoConfigure( AutoConfigureTime );
    }
    else {
      Filter *filter = MCs->RW->filterDetectors()->filter( Name );
      if ( filter != 0 ) {
	if ( FilterCommand == 1 ) {
	  MCs->RW->printlog( "filter \"" + filter->ident() + "\": save \"" +
			     Params + "\"" );
	  filter->save( Params );
	}
	else {
	  MCs->RW->printlog( "filter \"" + filter->ident() + "\": auto-configure " +
			     Str( AutoConfigureTime ) + "s" );
	  MCs->RW->filterDetectors()->autoConfigure( filter, AutoConfigureTime );
	}
      }
    }
  }
  // detector:
  else if ( Command == DetectorCom ) {
    if ( DetectorCommand == 2 && Name.empty() ) {
      MCs->RW->printlog( "detector \"ALL\": auto-configure " + 
			 Str( AutoConfigureTime ) + "s" );
      MCs->RW->filterDetectors()->autoConfigure( AutoConfigureTime );
    }
    else {
      Filter *filter = MCs->RW->filterDetectors()->detector( Name );
      if ( filter != 0 ) {
	if ( DetectorCommand == 1 ) {
	  MCs->RW->printlog( "detector \"" + filter->ident() + "\" save: \"" + 
			     Params + "\"" );
	  filter->save( Params );
	}
	else {
	  MCs->RW->printlog( "detector \"" + filter->ident() + "\": auto-configure " + 
			     Str( AutoConfigureTime ) + "s" );
	  MCs->RW->filterDetectors()->autoConfigure( filter, AutoConfigureTime );
	}
      }
    }
  }
  // switch macros:
  else if ( Command == SwitchCom ) {
    MCs->RW->printlog( "switch to macro file \"" + Name + "\"" );
    MCs->loadMacros( Name );
    if ( MCs->boolean( "fallbackonreload" ) )
      return false;
  }
  // start session:
  else if ( Command == StartSessionCom ) {
    MCs->RW->session()->startTheSession( false );
  }
  // stop session:
  else if ( Command == StopSessionCom ) {
    MCs->RW->session()->doStopTheSession( false, false );
  }
  // shut down:
  else if ( Command == ShutdownCom ) {
    MCs->RW->shutdown();
  }
  // message:
  else if ( Command == MessageCom ) {
    Str msg = Params;
    int i = msg.find( "$(" );
    while ( i >= 0 ) {
      int c = msg.findBracket( i+1, "(", "" );
      if ( c < 1 )
	c = msg.size();
      string cs( msg.substr( i+2, c-i-2 ) );
      FILE *p = popen( cs.c_str(), "r" );
      Str ns = "";
      char ls[1024];
      while ( fgets( ls, 1024, p ) != 0 )
	ns += ls;
      pclose( p );
      ns.strip();
      msg.replace( i, c-i+1, ns );
      i = msg.find( "$(", i+3 );
    }
    if ( !msg.empty() ) {
      MessageBox::information( Name, msg, TimeOut, MCs );
      msg.eraseMarkup();
      MCs->RW->printlog( "message " + Name + ": " + msg );
    }
  }
  else if ( Command == BrowseCom ) {
    Str file = Params;
    int i = file.find( "$(" );
    while ( i >= 0 ) {
      int c = file.findBracket( i+1, "(", "" );
      if ( c < 1 )
	c = file.size();
      string cs( file.substr( i+2, c-i-2 ) );
      FILE *p = popen( cs.c_str(), "r" );
      Str ns = "";
      char ls[1024];
      while ( fgets( ls, 1024, p ) != 0 )
	ns += ls;
      pclose( p );
      ns.strip();
      file.replace( i, c-i+1, ns );
      i = file.find( "$(", i+3 );
    }
    file.expandPath();
    if ( !file.empty() ) {
      MCs->RW->printlog( "browse " + Name + ": " + file );
      // create and exec dialog:
      OptDialog *od = new OptDialog( false, MCs );
      od->setCaption( Name );
      QTextBrowser *hb = new QTextBrowser( MCs );
      QStringList fpl;
      fpl.push_back( file.dir().c_str() );
      hb->setSearchPaths( fpl );
      hb->setSource( QUrl::fromLocalFile( file.notdir().c_str() ) );
      if ( hb->toHtml().isEmpty() ) {
	hb->setText( string( "Sorry, can't find file <b>" + file + "</b>." ).c_str() );
      }
      hb->setMinimumSize( 600, 400 );
      od->addWidget( hb );
      od->addButton( "&Ok" );
      od->exec();
    }
  }
  else if ( CT != 0 ) {
    // control:
    MCs->RW->printlog( "control " + Name + ": " + Params );
    CT->Options::read( Params );
  }
  else if ( RP != 0 ) {
    // start RePro:
    RP->Options::setDefaults();
    RP->Options::delFlags( OutData::Mutable );
    RP->Options::read( MC->expandParameter( Params ), 0, OutData::Mutable );
    RP->Options::read( RP->overwriteOptions() );
    RP->Options::read( CO );
    MCs->RW->startRePro( RP, MC->action(), saving );
    return true;
  }
  return false;
}


void MacroCommand::reloadRePro( RePro *repro )
{
  if ( Command == ReProCom &&
       Name == repro->name() )
    RP = repro;
}


void MacroCommand::start( void )
{
  MC->variables().setDefaults();
  MCs->startMacro( MacroNum, CommandNum, true, true );
  // Note: in case of a switch command, *this does not exist anymore?
}


void MacroCommand::run( void )
{
  MC->variables().setDefaults();
  MCs->startMacro( MacroNum, CommandNum, true, true );
  // XXX Note: in case of a switch command, *this does not exist anymore? XXX
  MCs->setThisOnly( ( Command == MacroCom ) );
}


void MacroCommand::view( void )
{
  MCs->RPs->raise( RP );
}


void MacroCommand::reload( void )
{
  MCs->RPs->reload( RP );
}


void MacroCommand::help( void )
{
  MCs->RPs->help( RP );
}


void MacroCommand::enable( void )
{
  Enabled = ! Enabled;
  EnabledAction->setText( Enabled ? "&Disable" : "&Enable" );
  SubMenu->menuAction()->setIcon( Enabled ? *EnabledIcon : *DisabledIcon );
}


void MacroCommand::dialog( void )
{
  if ( DialogOpen || ( RP != 0 && RP->dialogOpen() ) )
    return;

  DialogOpen = true;
  DO = &MCs->RPs->dialogOptions();

  if ( Command == MacroCom ) {
    // Macro dialog:
    MacroVars = MC->variables();
    MacroVars.setDefaults();
    MacroVars.read( MC->expandParameter( Params ) );
    // create and exec dialog:
    OptDialog *od = new OptDialog( false, MCs );
    od->setCaption( "Macro " + Name + " Variables" );
    string tabhotkeys = "oarc";
    od->addOptions( MacroVars, 0, 0, 0, 0, &tabhotkeys );
    od->setVerticalSpacing( int(9.0*exp(-double(MacroVars.size())/14.0))+1 );
    od->addSeparator();
    od->setRejectCode( 0 );
    od->addButton( "&Ok", OptDialog::Accept, 1 );
    od->addButton( "&Apply", OptDialog::Accept, 1, false );
    od->addButton( "&Run", OptDialog::Accept, 2, false );
    //    od->addButton( "&Defaults", OptDialog::Defaults );
    od->addButton( "&Cancel" );
    connect( od, SIGNAL( dialogClosed( int ) ),
	     this, SLOT( dialogClosed( int ) ) );
    connect( od, SIGNAL( buttonClicked( int ) ),
	     this, SLOT( dialogAction( int ) ) );
    connect( od, SIGNAL( valuesChanged( void ) ),
	     this, SLOT( acceptDialog( void ) ) );
    od->exec();
  }
  else if ( Command == ReProCom ) {
    // RePro dialog:
    RP->Options::setDefaults();
    RP->Options::read( MC->expandParameter( Params ), RePro::MacroFlag );
    RP->Options::read( RP->overwriteOptions(), 0, RePro::OverwriteFlag );
    RP->Options::read( CO, 0, RePro::CurrentFlag );

    RP->dialog();

    connect( (ConfigDialog*)RP, SIGNAL( dialogAccepted( void ) ),
	     this, SLOT( acceptDialog( void ) ) );
    connect( (ConfigDialog*)RP, SIGNAL( dialogAction( int ) ),
	     this, SLOT( dialogAction( int ) ) );
    connect( (ConfigDialog*)RP, SIGNAL( dialogClosed( int ) ),
	     this, SLOT( dialogClosed( int ) ) );
  }
}


void MacroCommand::acceptDialog( void )
{
  if ( Command == MacroCom ) {
    Options po( MC->expandParameter( Params ) );
    po.readAppend( MacroVars, OptDialog::changedFlag() );
    Params = po.save();
    // update menu:
    Str s = MenuShortcut;
    s += "Macro " + Name;
    if ( !Params.empty() ) {
      s += ": ";
      int index = 0;
      int nc = 10 + Name.size();
      for ( int i=0; nc+index < Macros::MenuWidth && index >= 0; i++ ) {
	index = Params.find( ';', index+1 );
      }
      if ( index < 0 )
	s += ": " + Params;
      else
	s += ": " + Params.substr( 0, index ) + " ...";
    }
    SubMenu->menuAction()->setText( s.c_str() );
  }
  else {
    Options newopt( *((Options*)RP), OptDialog::changedFlag() );
    if ( DO->boolean( "overwrite" ) ) {
      RP->overwriteOptions().readAppend( newopt );
    }
    if ( DO->boolean( "default" ) ) {
      RP->Options::setToDefaults();
      RP->Options::read( MC->expandParameter( Params ) );
      RP->Options::read( RP->overwriteOptions() );
      CO.assign( *((Options*)RP), Options::NonDefault );
      RP->Options::setDefaults();
      CO.read( *((Options*)RP) );
    }
    else {
      CO.readAppend( newopt );
    }
  }
}


void MacroCommand::dialogAction( int r )
{
  // defaults:
  if ( r == 3 )
    CO.clear();

  // run:
  if ( r == 2 )
    MCs->startMacro( MacroNum, CommandNum );
  // Note: in case of a switch command, *this does not exist anymore?
}


void MacroCommand::dialogClosed( int r )
{
  DialogOpen = false;
  if ( Command == ReProCom && RP != 0 ) {
    disconnect( (ConfigDialog*)RP, SIGNAL( dialogAccepted( void ) ),
		this, SLOT( acceptDialog( void ) ) );
    disconnect( (ConfigDialog*)RP, SIGNAL( dialogAction( int ) ),
		this, SLOT( dialogAction( int ) ) );
    disconnect( (ConfigDialog*)RP, SIGNAL( dialogClosed( int ) ),
		this, SLOT( dialogClosed( int ) ) );
  }
}


void MacroCommand::createIcons( int size )
{
  int my = size - 2;
  int mx = my;

  EnabledIcon = new QPixmap( mx, mx );
  QPainter p;
  p.begin( EnabledIcon );
  p.eraseRect( EnabledIcon->rect() );
  p.setPen( QPen( Qt::black, 1 ) );
  p.setBrush( Qt::green );
  p.drawEllipse( 0, 0, mx-1, mx-1 );
  p.end();
  EnabledIcon->setMask( EnabledIcon->createHeuristicMask() );

  DisabledIcon = new QPixmap( mx, mx );
  p.begin( DisabledIcon );
  p.eraseRect( DisabledIcon->rect() );
  p.setPen( QPen( Qt::black, 1 ) );
  p.setBrush( Qt::red );
  p.drawEllipse( 0, 0, mx-1, mx-1 );
  p.end();
  DisabledIcon->setMask( DisabledIcon->createHeuristicMask() );

  RunningIcon = new QPixmap( mx, mx );
  p.begin( RunningIcon );
  p.eraseRect( RunningIcon->rect() );
  p.setPen( QPen( Qt::black, 1 ) );
  p.setBrush( Qt::green );
  QPolygon pa( 7 );
  pa.setPoint( 0, 0, 2*my/3 );
  pa.setPoint( 1, mx/2, 2*my/3 );
  pa.setPoint( 2, mx/2, my );
  pa.setPoint( 3, mx-1, my/2 );
  pa.setPoint( 4, mx/2, 0 );
  pa.setPoint( 5, mx/2, my/3 );
  pa.setPoint( 6, 0, my/3 );
  p.drawPolygon( pa );
  p.end();
  RunningIcon->setMask( RunningIcon->createHeuristicMask() );
}


void MacroCommand::destroyIcons( void )
{
  delete EnabledIcon;
  delete DisabledIcon;
  delete RunningIcon;
}


ostream &operator<< ( ostream &str, const MacroCommand &command )
{
  str << "  " << command.CommandNum+1 << " ";
  if ( command.Command == MacroCommand::ReProCom )
    str << "RePro";
  else if ( command.Command == MacroCommand::MacroCom )
    str << "Macro";
  else if ( command.Command == MacroCommand::ShellCom )
    str << "Shell";
  else if ( command.Command == MacroCommand::FilterCom )
    str << "Filter " << ( command.FilterCom == 1 ? "save" : "auto-configure" );
  else if ( command.Command == MacroCommand::DetectorCom )
    str << "Detector " << ( command.DetectorCom == 1 ? "save" : "auto-configure" );
  else if ( command.Command == MacroCommand::ControlCom )
    str << "Control";
  else if ( command.Command == MacroCommand::SwitchCom )
    str << "Switch";
  else if ( command.Command == MacroCommand::StartSessionCom )
    str << "StartSession";
  else if ( command.Command == MacroCommand::StopSessionCom )
    str << "StopSession";
  else if ( command.Command == MacroCommand::ShutdownCom )
    str << "Shutdown";
  else if ( command.Command == MacroCommand::MessageCom ) {
    str << "Message";
    if ( command.TimeOut > 0.0 )
      str << " (timeout " << command.TimeOut << ")";
  }
  else if ( command.Command == MacroCommand::BrowseCom )
    str << "Browse";
  else
    str << "Unknown command";
  str << ": " << command.Name 
      << " -> " << command.Params << '\n';
  return str;
}


MacroButton::MacroButton( const string &title, QWidget *parent )
  : QPushButton( title.c_str(), parent )
{
}


void MacroButton::mouseReleaseEvent( QMouseEvent *qme )
{
  QPushButton::mouseReleaseEvent( qme );
  if ( qme->button() == Qt::RightButton ) {
    emit rightClicked();
  }
}


}; /* namespace relacs */

#include "moc_macros.cc"

