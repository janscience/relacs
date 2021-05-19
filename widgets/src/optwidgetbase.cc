/*
 optwidgetbase.cc
 

 RELACS
 Relaxed ELectrophysiological data Acquisition, Control, and Stimulation
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

#include <cmath>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QDir>
#include <QFileDialog>
#include <QList>
#include <relacs/optwidgetbase.h>


namespace relacs {


OptWidgetBase::OptWidgetBase( Options::iterator param, QWidget *label,
			      Options *oo, OptWidget *ow, QMutex *mutex )
  : QObject(),
    Param( param ),
    OO( oo ),
    OW( ow ),
    LabelW( label ),
    W( 0 ),
    UnitLabel( 0 ),
    UnitBrowseW( 0 ),
    OMutex( mutex ),
    Editable( true ),
    ContUpdate( ow->continuousUpdate() ),
    InternChanged( false ),
    InternRead( false )
{
  if ( Param == OO->end() || OW->readOnlyMask() < 0 ||
       ( OW->readOnlyMask() > 0 && ( Param->flags() & OW->readOnlyMask() ) ) )
    Editable = false;
  else
    Param->delFlags( OW->changedFlag() );
  ow->addWidget( this );
}


OptWidgetBase::~OptWidgetBase( void )
{
}

void OptWidgetBase::get( void )
{
}


void OptWidgetBase::reset( void )
{
}


void OptWidgetBase::resetDefault( void )
{
}


void OptWidgetBase::update( void )
{
}


void OptWidgetBase::setMutex( QMutex *mutex )
{
  OMutex = mutex;
}


void OptWidgetBase::lockMutex( void )
{
  if ( OMutex != 0 )
    OMutex->lock();
}


bool OptWidgetBase::tryLockMutex( int timeout )
{
  if ( OMutex != 0 )
    return OMutex->tryLock( timeout );
  else
    return true;
}


void OptWidgetBase::unlockMutex( void )
{
  if ( OMutex != 0 )
    OMutex->unlock();
}


QWidget *OptWidgetBase::valueWidget( void )
{
  return W;
}


bool OptWidgetBase::editable( void ) const
{
  return Editable;
}


Options::const_iterator OptWidgetBase::param( void ) const
{
  return Param;
}


Options::iterator OptWidgetBase::param( void )
{
  return Param;
}


void OptWidgetBase::setUnitLabel( QLabel *l )
{
  if ( l != 0 ) {
    UnitLabel = l;
    UnitBrowseW = 0;
  }
}


void OptWidgetBase::initActivation( void )
{
}


void OptWidgetBase::addActivation( int index, OptWidgetBase *w )
{
  Index.push_back( index );
  Widgets.push_back( w );
  initActivation();
}


void OptWidgetBase::activateOption( bool eq )
{
  bool ac = Param != OO->end() ? eq : true;

  if ( OW->style() & OptWidget::HideStyle ) {
    if ( ac ) {
      if ( LabelW != 0 )
	LabelW->show();
      if ( W != 0 )
	W->show();
      if ( UnitLabel != 0 )
	UnitLabel->show();
      if ( UnitBrowseW != 0 )
	UnitBrowseW->show();
    }
    else {
      if ( LabelW != 0 )
	LabelW->hide();
      if ( W != 0 )
	W->hide();
      if ( UnitLabel != 0 )
	UnitLabel->hide();
      if ( UnitBrowseW != 0 )
	UnitBrowseW->hide();
    }
  }
  else {
    if ( LabelW != 0 )
      LabelW->setEnabled( ac );
    if ( W != 0 )
      W->setEnabled( ac );
    if ( UnitLabel != 0 )
      UnitLabel->setEnabled( ac );
    if ( UnitBrowseW != 0 )
      UnitBrowseW->setEnabled( ac );
  }
}


OptWidgetText::OptWidgetText( Options::iterator param, QWidget *label,
			      Options *oo, OptWidget *ow,
			      QMutex *mutex, QWidget *parent )
  : OptWidgetBase( param, label, oo, ow, mutex ),
    EW( 0 ),
    Value( "" ),
    LW( 0 ),
    BrowseButton( 0 )
{
  if ( (Param->style() & OptWidget::SelectText) > 0 )
    Editable = false;
  if ( Editable ) {
    W = EW = new QLineEdit( Param->text( "%s" ).c_str(), parent );
    OptWidget::setValueStyle( W, Param->style(), OptWidget::Text );
    Value = EW->text().toStdString();
    connect( EW, SIGNAL( textChanged( const QString& ) ),
	     this, SLOT( textChanged( const QString& ) ) );
    if ( Param->style() & OptWidget::Browse ) {
      BrowseButton = new QPushButton( "Browse...", parent );
      UnitBrowseW = BrowseButton;
      connect( BrowseButton, SIGNAL( clicked( void ) ),
	       this, SLOT( browse( void ) ) );
    }
  }
  else {
    LW = new QLabel( Param->text( "%s" ).c_str(), parent );
    OptWidget::setValueStyle( LW, Param->style(), OptWidget::Window );
    LW->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    LW->setLineWidth( 2 );
    W = LW;
  }
}


void OptWidgetText::get( void )
{
  if ( Editable ) {
    InternRead = true;
    bool cn = OO->notifying();
    OO->unsetNotify();
    Param->setText( EW->text().toStdString() );
    if ( Param->text( 0, "%s" ) != Value )
      Param->addFlags( OW->changedFlag() );
    Value = EW->text().toStdString();
    OO->setNotify( cn );
    InternRead = false;
  }
}


void OptWidgetText::reset( void )
{
  InternChanged = true;
  if ( Editable ) {
    EW->setText( Param->text( 0, "%s" ).c_str() );
  }
  else {
    LW->setText( Param->text( 0, "%s" ).c_str() );
  }
  InternChanged = false;
}


void OptWidgetText::resetDefault( void )
{
  if ( Editable ) {
    InternChanged = true;
    EW->setText( Param->defaultText( "%s" ).c_str() );
    InternChanged = false;
  }
}


void OptWidgetText::update( void )
{
  if ( UnitLabel != 0 ) {
    InternChanged = true;
    UnitLabel->setText( Param->outUnit().htmlUnit().c_str() );
    InternChanged = false;
  }
}


void OptWidgetText::textChanged( const QString &s )
{
  if ( InternRead || OW->updateDisabled() )
    return;

  Parameter p( *Param );
  p.setText( EW->text().toStdString() );
  emit valueChanged( p );

  if ( ContUpdate && Editable ) {
    if ( InternChanged ) {
      Value = EW->text().toStdString();
      bool cn = OO->notifying();
      OO->unsetNotify();
      Param->setText( Value );
      Param->delFlags( OW->changedFlag() );
      OO->setNotify( cn );
    }
    else
      doTextChanged( s );
  }
  for ( unsigned int k=0; k<Widgets.size(); k++ ) {
    if ( Widgets[k]->param() != OO->end() )
      Widgets[k]->activateOption( Widgets[k]->param()->testActivation( Index[k], s.toStdString() ) );
  }
}


class OptWidgetTextEvent : public QEvent
{
public:
  OptWidgetTextEvent( int type )
    : QEvent( QEvent::Type( QEvent::User+type ) ), Text( "" ), FileName( "" ) {};
  OptWidgetTextEvent( int type, const QString &s )
    : QEvent( QEvent::Type( QEvent::User+type ) ), Text( s ), FileName( "" ) {};
  OptWidgetTextEvent( int type, const Str &filename )
    : QEvent( QEvent::Type( QEvent::User+type ) ), Text( "" ), FileName( filename ) {};
  QString text( void ) const { return Text; };
  Str fileName( void ) const { return FileName; };
private:
  QString Text;
  Str FileName;
};


void OptWidgetText::doTextChanged( const QString &s )
{
  if ( ! tryLockMutex( 5 ) ) {
    // we do not get the lock for the data now,
    // so we repost the event to a later time.
    QCoreApplication::postEvent( this, new OptWidgetTextEvent( 1, s ) );
    return;
  }
  bool cn = OO->notifying();
  OO->unsetNotify();
  Param->setText( s.toStdString() );
  if ( Param->text( 0, "%s" ) != Value )
    Param->addFlags( OW->changedFlag() );
  Value = EW->text().toStdString();
  if ( cn )
    OO->notify();
  if ( ContUpdate )
    Param->delFlags( OW->changedFlag() );
  OO->setNotify( cn );
  unlockMutex();
}


void OptWidgetText::customEvent( QEvent *e )
{
  if ( e->type() == QEvent::User+1 ) {
    OptWidgetTextEvent *te = dynamic_cast<OptWidgetTextEvent*>( e );
    doTextChanged( te->text() );
  }
  else if ( e->type() == QEvent::User+2 ) {
    browse();
  }
  else if ( e->type() == QEvent::User+3 ) {
    OptWidgetTextEvent *te = dynamic_cast<OptWidgetTextEvent*>( e );
    doBrowse( te->fileName() );
  }
}


void OptWidgetText::initActivation( void )
{
  string s = "";
  if ( EW != 0 )
    s = EW->text().toStdString();
  else
    s = LW->text().toStdString();
  if ( Widgets.back()->param() != OO->end() )
    Widgets.back()->activateOption( Widgets.back()->param()->testActivation( Index.back(), s ) );
}


void OptWidgetText::browse( void )
{
  if ( OW->updateDisabled() )
    return;

  if ( ! tryLockMutex( 5 ) ) {
    // we do not get the lock for the data now,
    // so we repost the event to a later time.
    QCoreApplication::postEvent( this, new OptWidgetTextEvent( 2 ) );
    return;
  }
  Str file = Param->text( 0 );
  file.addWorking();
  int style = Param->style();
  unlockMutex();
  QFileDialog* fd = new QFileDialog( 0 );
  if ( style & OptWidget::BrowseExisting ) {
    fd->setFileMode( QFileDialog::ExistingFile );
    fd->setWindowTitle( "Open File" );
    fd->setDirectory( file.dir().c_str() );
  }
  else if ( style & OptWidget::BrowseAny ) {
    fd->setFileMode( QFileDialog::AnyFile );
    fd->setWindowTitle( "Save File" );
    fd->setDirectory( file.dir().c_str() );
  }
  else if ( style & OptWidget::BrowseDirectory ) {
    fd->setFileMode( QFileDialog::Directory );
    fd->setWindowTitle( "Choose directory" );
    fd->setDirectory( file.preventSlash().dir().c_str() );
  }
  fd->setNameFilter( tr("All (*)") );
  fd->setViewMode( QFileDialog::List );
  if ( fd->exec() == QDialog::Accepted ) {
    QStringList qsl = fd->selectedFiles();
    Str filename = "";
    if ( qsl.size() > 0 )
      filename = qsl[0].toStdString();
    doBrowse( filename );
  }
}


void OptWidgetText::doBrowse( Str filename )
{
  if ( ! tryLockMutex( 5 ) ) {
    // we do not get the lock for the data now,
    // so we repost the event to a later time.
    QCoreApplication::postEvent( this, new OptWidgetTextEvent( 3, filename ) );
    return;
  }
  if ( ( Param->style() & OptWidget::BrowseAbsolute ) == 0 )
    filename.stripWorkingPath( 5 );
  if ( ( Param->style() & OptWidget::BrowseDirectory ) )
    filename.provideSlash();
  bool cn = OO->notifying();
  OO->unsetNotify();
  Param->setText( filename );
  if ( Param->text( 0 ) != Value )
    Param->addFlags( OW->changedFlag() );
  EW->setText( Param->text( 0, "%s" ).c_str() );
  Value = EW->text().toStdString();
  if ( cn )
    OO->notify();
  if ( ContUpdate )
    Param->delFlags( OW->changedFlag() );
  OO->setNotify( cn );
  unlockMutex();
}


void OptWidgetText::setUnitLabel( QLabel *l )
{
  if ( l != 0 ) {
    UnitLabel = l;
    UnitBrowseW = l;
  }
}


QPushButton *OptWidgetText::browseButton( void )
{
  return BrowseButton;
}


OptWidgetMultiText::OptWidgetMultiText( Options::iterator param, QWidget *label,
					Options *oo, OptWidget *ow,
					QMutex *mutex, QWidget *parent )
  : OptWidgetBase( param, label, oo, ow, mutex ),
    EW( 0 ),
    CI( 0 ),
    Inserted( false ),
    Update( true ),
    Value( "" ),
    LW( 0 )
{
  if ( Editable ) {
    W = EW = new QComboBox( parent );
    EW->setEditable( (Param->style() & OptWidget::SelectText) == 0 );
    if ( (Param->style() & OptWidget::SelectText) > 0 )
      OptWidget::setValueStyle( W, Param->style(), OptWidget::Combo );
    else
      OptWidget::setValueStyle( W, Param->style(), OptWidget::Text );
    EW->setInsertPolicy( QComboBox::InsertAtTop );
    EW->setDuplicatesEnabled( false );
    if ( ( Param->style() & OptWidget::ComboAutoCompletion ) == 0 )
      EW->setCompleter( 0 );
    reset();
    if ( EW->isEditable() )
      connect( EW, SIGNAL( editTextChanged( const QString& ) ),
	       this, SLOT( insertText( const QString& ) ) );
    Value = EW->currentText().toStdString();
    PrevValue = Value;
    connect( EW, SIGNAL( currentIndexChanged( const QString& ) ),
	     this, SLOT( textChanged( const QString& ) ) );
    connect( EW, SIGNAL( activated( const QString& ) ),
	     this, SLOT( textChanged( const QString& ) ) );
  }
  else {
    LW = new QLabel( Param->text().c_str(), parent );
    OptWidget::setValueStyle( LW, Param->style(), OptWidget::Window );
    LW->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    LW->setLineWidth( 2 );
    W = LW;
  }
}


void OptWidgetMultiText::get( void )
{
  if ( Editable ) {
    InternRead = true;
    bool cn = OO->notifying();
    OO->unsetNotify();
    setParameter( *Param, EW->currentText().toStdString() );
    if ( Param->text( 0 ) != Value )
      Param->addFlags( OW->changedFlag() );
    Value = EW->currentText().toStdString();
    OO->setNotify( cn );
    InternRead = false;
  }
}


void OptWidgetMultiText::reset( void )
{
  InternChanged = true;
  if ( Editable ) {
    Update = false;
    EW->clear();
    if ( Param->size() > 0 ) {
      string first = Param->text( 0 );
      int firstindex = 0;
      for ( int k=0; k<Param->size(); k++ ) {
	string s = Param->text( k );
	bool newitem = true;
	for ( int j=1; j<k; j++ ) {
	  if ( Param->text( j ) == s ) {
	    newitem = false;
	    break;
	  }
	}
	if ( newitem ) {
	  EW->addItem( s.c_str() );
	  if ( s == first )
	    firstindex = k;
	}
      }
      if ( firstindex > 0 ) {
	EW->removeItem( 0 );
	EW->setCurrentIndex( firstindex-1 );
      }
      else
	EW->setCurrentIndex( 0 );
    }
    CI = EW->currentIndex();
    Inserted = false;
    Update = true;
  }
  else {
    LW->setText( Param->text( 0 ).c_str() );
  }
  InternChanged = false;
}


void OptWidgetMultiText::resetDefault( void )
{
  if ( Editable ) {
    InternChanged = true;
    Update = false;
    EW->setEditText( Param->defaultText().c_str() );
    Update = true;
    InternChanged = false;
  }
}


void OptWidgetMultiText::update( void )
{
  if ( UnitLabel != 0 ) {
    InternChanged = true;
    UnitLabel->setText( Param->outUnit().htmlUnit().c_str() );
    InternChanged = false;
  }
}


void OptWidgetMultiText::setParameter( Parameter &p, const string &s )
{
  p.setText( s );
  for ( int k=0; k<EW->count(); k++ ) {
    bool newitem = true;
    for ( int j=0; j<k; j++ ) {
      if ( EW->itemText( j ) == EW->itemText( k ) ) {
	newitem = false;
	break;
      }
    }
    if ( newitem )
      p.addText( EW->itemText( k ).toStdString() );
  }
}


void OptWidgetMultiText::textChanged( const QString &s )
{
  if ( InternRead || OW->updateDisabled() )
    return;

  Parameter p( *Param );
  setParameter( p, s.toStdString() );
  if ( p.text() != PrevValue ) {
    PrevValue = p.text();
    emit valueChanged( p );
  }

  if ( ContUpdate && Editable && Update) {
    if ( InternChanged ) {
      Value = EW->currentText().toStdString();
      bool cn = OO->notifying();
      OO->unsetNotify();
      setParameter( *Param, Value );
      Param->delFlags( OW->changedFlag() );
      OO->setNotify( cn );
    }
    else
      doTextChanged( s );
  }
  for ( unsigned int k=0; k<Widgets.size(); k++ ) {
    if ( Widgets[k]->param() != OO->end() )
      Widgets[k]->activateOption( Widgets[k]->param()->testActivation( Index[k], s.toStdString() ) );
  }
}


class OptWidgetMultiTextEvent : public QEvent
{
public:
  OptWidgetMultiTextEvent( int type, const QString &s )
    : QEvent( QEvent::Type( QEvent::User+type ) ), Text( s ) {};
  QString text( void ) const { return Text; };
private:
  QString Text;
};


void OptWidgetMultiText::doTextChanged( const QString &s )
{
  if ( ! tryLockMutex( 5 ) ) {
    // we do not get the lock for the data now,
    // so we repost the event to a later time.
    QCoreApplication::postEvent( this, new OptWidgetMultiTextEvent( 1, s ) );
    return;
  }
  bool cn = OO->notifying();
  OO->unsetNotify();
  setParameter( *Param, s.toStdString() );
  if ( Param->text( 0 ) != Value )
    Param->addFlags( OW->changedFlag() );
  Value = s.toStdString();
  if ( cn )
    OO->notify();
  if ( ContUpdate )
    Param->delFlags( OW->changedFlag() );
  OO->setNotify( cn );
  unlockMutex();
}


void OptWidgetMultiText::customEvent( QEvent *e )
{
  if ( e->type() == QEvent::User+1 ) {
    OptWidgetMultiTextEvent *te = dynamic_cast<OptWidgetMultiTextEvent*>( e );
    doTextChanged( te->text() );
  }
  else if ( e->type() == QEvent::User+2 ) {
    OptWidgetMultiTextEvent *te = dynamic_cast<OptWidgetMultiTextEvent*>( e );
    doInsertText( te->text() );
  }
}


void OptWidgetMultiText::initActivation( void )
{
  string s = "";
  if ( EW != 0 )
    s = EW->currentText().toStdString();
  else
    s = LW->text().toStdString();
  if ( Widgets.back()->param() != OO->end() )
    Widgets.back()->activateOption( Widgets.back()->param()->testActivation( Index.back(), s ) );
}


void OptWidgetMultiText::insertText( const QString &text )
{
  if ( ! Update || OW->updateDisabled() )
    return;

  doInsertText( text );
}


void OptWidgetMultiText::doInsertText( const QString &text )
{
  if ( ! tryLockMutex( 5 ) ) {
    // we do not get the lock for the data now,
    // so we repost the event to a later time.
    QCoreApplication::postEvent( this, new OptWidgetMultiTextEvent( 2, text ) );
    return;
  }
  if ( CI == EW->currentIndex() &&
       ( CI > 0 || EW->currentText() != EW->itemText( 0 ) ) ) {
    if ( ! Inserted ) {
      EW->insertItem( 0, EW->currentText() );
      EW->setCurrentIndex( 0 );
      CI = 0;
      Inserted = true;
    }
    else
      EW->setItemText( 0, EW->currentText() );
  }
  else {
    CI = EW->currentIndex();
    if ( Inserted ) {
      if ( EW->count() > 0 ) {
	EW->removeItem( 0 );
	EW->setCurrentIndex( CI-1 );
	CI = EW->currentIndex();
      }
      Inserted = false;
    }
  }
  unlockMutex();
}


void OptWidgetMultiText::setUnitLabel( QLabel *l )
{
  if ( l != 0 ) {
    UnitLabel = l;
    UnitBrowseW = l;
  }
}


OptWidgetNumberSpinBox::OptWidgetNumberSpinBox( QWidget *parent, int index )
  : DoubleSpinBox( parent ),
    Index( index )
{ 
  connect( this, SIGNAL( valueChanged( double ) ),
	   this, SLOT( valueChanged( double ) ) );
}


void OptWidgetNumberSpinBox::valueChanged( double v )
{ 
  emit valueChanged( v, Index );
}


OptWidgetNumber::OptWidgetNumber( Options::iterator param, QWidget *label,
				  Options *oo, OptWidget *ow,
				  QMutex *mutex, QWidget *parent )
  : OptWidgetBase( param, label, oo, ow, mutex )
{
  EW.clear();
  Value.clear();
  LW.clear();
  LCDW.clear();
  QHBoxLayout *hbox = 0;
  if ( Param->size() > 1 ) {
    W = new QWidget( parent );
    hbox = new QHBoxLayout;
    hbox->setContentsMargins( 0, 0, 0, 0 );
    W->setLayout( hbox );
    parent = W;
  }
  if ( Editable ) {
    double min = Param->minimum( Param->outUnit() );
    double max = Param->maximum( Param->outUnit() );
    double step = Param->step( Param->outUnit() );
    for ( int k=0; k<Param->size(); k++ ) {
      EW.push_back( new OptWidgetNumberSpinBox( parent, k ) );
      if ( hbox == 0 )
	W = EW[k];
      else
	hbox->addWidget( EW[k] );
      EW[k]->setRange( min, max );
      EW[k]->setSingleStep( step );
      if ( Param->isNumber() )
	EW[k]->setFormat( Param->format() );
      else
	EW[k]->setFormat( "%.0f" );
      if ( Param->style() & OptWidget::SpecialInfinite )
	EW[k]->setSpecialValueText( "infinite" );
      else if ( Param->style() & OptWidget::SpecialNone )
	EW[k]->setSpecialValueText( "none" );
      double val = Param->number( k, Param->outUnit() );
      EW[k]->setValue( val );
      OptWidget::setValueStyle( EW[k], Param->style(), OptWidget::Text );
      Value.push_back( EW[k]->value() );
      EW[k]->setKeyboardTracking( false );
      connect( EW[k], SIGNAL( valueChanged( double, int ) ),
	       this, SLOT( valueChanged( double, int ) ) );
    }
  }
  else {
    if ( Param->style() & OptWidget::ValueLCD ) {
      for ( int k=0; k<Param->size(); k++ ) {
	LCDW.push_back( new QLCDNumber( parent ) );
	LCDW[k]->setSegmentStyle( QLCDNumber::Filled );
	LCDW[k]->setSmallDecimalPoint( true );
	LCDW[k]->display( Param->text( k, "", Param->outUnit() ).c_str() );	
	OptWidget::setValueStyle( LCDW[k], Param->style(), OptWidget::TextShade );
	// size:
	if ( ( Param->style() & OptWidget::ValueHuge ) == OptWidget::ValueHuge )
	  LCDW[k]->setFixedHeight( 16 * LCDW[k]->sizeHint().height() / 10 );
	else if ( Param->style() & OptWidget::ValueLarge )
	  LCDW[k]->setFixedHeight( 13 * LCDW[k]->sizeHint().height() / 10 );
	else if ( Param->style() & OptWidget::ValueSmall )
	  LCDW[k]->setFixedHeight( 8 * LCDW[k]->sizeHint().height() / 10 );
	if ( hbox == 0 )
	  W = LCDW[k];
	else
	  hbox->addWidget( LCDW[k] );
      }
    }
    else {
      for ( int k=0; k<Param->size(); k++ ) {
	LW.push_back( new QLabel( Param->text( k ).c_str(), parent ) );
	LW[k]->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
	LW[k]->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	LW[k]->setLineWidth( 2 );
	OptWidget::setValueStyle( LW[k], Param->style(), OptWidget::Window );
	LW[k]->setFixedHeight( LW[k]->sizeHint().height() );
	if ( hbox == 0 )
	  W = LW[k];
	else
	  hbox->addWidget( LW[k] );
      }
    }
  }
  if ( W->minimumWidth() < W->minimumSizeHint().width() )
    W->setMinimumWidth( W->minimumSizeHint().width() );
}


void OptWidgetNumber::get( void )
{
  if ( Editable ) {
    InternRead = true;
    bool cn = OO->notifying();
    OO->unsetNotify();
    vector<double> oldvalue = Value;
    for ( unsigned int k=0; k<EW.size(); k++ )
      Value[k] = EW[k]->value();
    Param->setNumbers( Value, Param->outUnit() );
    for ( unsigned int k=0; k<oldvalue.size(); k++ ) {
      if ( fabs( Param->number( k, Param->outUnit() ) - oldvalue[k] ) > 0.0001*Param->step() ) {
	Param->addFlags( OW->changedFlag() );
	break;
      }
    }
    OO->setNotify( cn );
    InternRead = false;
  }
}


void OptWidgetNumber::reset( void )
{
  InternChanged = true;
  if ( Editable ) {
    for ( unsigned int k=0; k<EW.size(); k++ )
      EW[k]->setValue( Param->number( k, Param->outUnit() ) );
  }
  else {
    if ( Param->style() & OptWidget::ValueLCD ) {
      for ( unsigned int k=0; k<LW.size(); k++ )
	LCDW[k]->display( Param->text( k, "", Param->outUnit() ).c_str() );
    }
    else {
      for ( unsigned int k=0; k<LW.size(); k++ )
	LW[k]->setText( Param->text( k, "", Param->outUnit() ).c_str() );
    }
  }
  if ( W->minimumWidth() < W->minimumSizeHint().width() )
    W->setMinimumWidth( W->minimumSizeHint().width() );
  InternChanged = false;
}


void OptWidgetNumber::resetDefault( void )
{
  if ( Editable ) {
    InternChanged = true;
    for ( unsigned int k=0; k<EW.size(); k++ ) {
      EW[k]->setValue( Param->defaultNumber( k, Param->outUnit() ) );
      if ( EW[k]->minimumWidth() < EW[k]->minimumSizeHint().width() )
	EW[k]->setMinimumWidth( EW[k]->minimumSizeHint().width() );
    }
    InternChanged = false;
  }
}


void OptWidgetNumber::update( void )
{
  InternChanged = true;
  if ( UnitLabel != 0 )
    UnitLabel->setText( Param->outUnit().htmlUnit().c_str() );
  if ( Editable ) {
    double min = Param->minimum( Param->outUnit() );
    double max = Param->maximum( Param->outUnit() );
    double step = Param->step( Param->outUnit() );
    for ( unsigned int k=0; k<EW.size(); k++ ) {
      EW[k]->setRange( min, max );
      EW[k]->setSingleStep( step );
      if ( Param->isNumber() )
	EW[k]->setFormat( Param->format() );
      else
	EW[k]->setFormat( "%.0f" );
      InternRead = true;
      double val = Param->number( Param->outUnit() );
      EW[k]->setValue( val );
      if ( EW[k]->minimumWidth() < EW[k]->minimumSizeHint().width() )
	EW[k]->setMinimumWidth( EW[k]->minimumSizeHint().width() );
    }
    InternRead = false;
  }
  InternChanged = false;
}


void OptWidgetNumber::valueChanged( double v, int index )
{
  if ( InternRead || OW->updateDisabled() )
    return;

  Parameter p( *Param );
  p.setNumber( EW[0]->value(), p.outUnit() );
  for ( unsigned int k=1; k<EW.size(); k++ )
    p.addNumber( EW[k]->value(), p.outUnit() );
  emit valueChanged( p );

  if ( ContUpdate && Editable ) {
    if ( InternChanged ) {
      Value[index] = EW[index]->value();
      bool cn = OO->notifying();
      OO->unsetNotify();
      Param->setNumbers( Value, Param->outUnit() );
      Param->delFlags( OW->changedFlag() );
      OO->setNotify( cn );
    }
    else
      doValueChanged( v, index );
  }

  if ( W->minimumWidth() < W->minimumSizeHint().width() )
    W->setMinimumWidth( W->minimumSizeHint().width() );

  if ( index == 0 ) {
    double tol = 0.2;
    if ( Param->isNumber() )
      tol = 0.01*Param->step();
    for ( unsigned int k=0; k<Widgets.size(); k++ ) {
      if ( Widgets[k]->param() != OO->end() )
	Widgets[k]->activateOption( Widgets[k]->param()->testActivation( Index[k], v, tol ) );
    }
  }
}


class OptWidgetNumberEvent : public QEvent
{
public:
  OptWidgetNumberEvent( double value, int index )
    : QEvent( QEvent::Type( QEvent::User+1 ) ), Value( value ), Index( index ) {};
  double value( void ) const { return Value; };
  int index( void ) const { return Index; };
private:
  double Value;
  int Index;
};


void OptWidgetNumber::doValueChanged( double v, int index )
{
  if ( ! tryLockMutex( 5 ) ) {
    // we do not get the lock for the data now,
    // so we repost the event to a later time.
    QCoreApplication::postEvent( this, new OptWidgetNumberEvent( v, index ) );
    return;
  }
  bool cn = OO->notifying();
  OO->unsetNotify();
  bool changeflags = ( fabs( v - Value[index] ) > 0.0001*Param->step() );
  Value[index] = EW[index]->value();
  Param->setNumbers( Value, Param->outUnit() );
  if ( changeflags )
    Param->addFlags( OW->changedFlag() );
  if ( cn )
    OO->notify();
  if ( ContUpdate )
    Param->delFlags( OW->changedFlag() );
  OO->setNotify( cn );
  unlockMutex();
}


void OptWidgetNumber::customEvent( QEvent *e )
{
  if ( e->type() == QEvent::User+1 ) {
    OptWidgetNumberEvent *ne = dynamic_cast<OptWidgetNumberEvent*>( e );
    doValueChanged( ne->value(), ne->index() );
  }
}


void OptWidgetNumber::initActivation( void )
{
  double v = 0.0;
  if ( EW.empty() && LCDW.empty() && LW.empty() )
    v = Param->number( Param->outUnit() );
  else {
    if ( ! EW.empty() )
      v = EW[0]->value();
    else if ( ! LCDW.empty() )
      v = LCDW[0]->value();
    /*
    else if ( ! LW.empty() )
      v = LW[0]->value();
    */
  }
  double tol = 0.2;
  if ( Param->isNumber() )
    tol = 0.01*Param->step();
  if ( Widgets.back()->param() != OO->end() )
    Widgets.back()->activateOption( Widgets.back()->param()->testActivation( Index.back(), v, tol ) );
}


void OptWidgetNumber::setUnitLabel( QLabel *l )
{
  if ( l != 0 ) {
    UnitLabel = l;
    UnitBrowseW = l;
  }
}


OptWidgetBoolean::OptWidgetBoolean( Options::iterator param, Options *oo,
				    OptWidget *ow, const string &request,
				    QMutex *mutex, QWidget *parent )
  : OptWidgetBase( param, 0, oo, ow, mutex ),
    EW( 0 ),
    Value( false )
{
  W = new QWidget;
  QHBoxLayout *hb = new QHBoxLayout;
  hb->setContentsMargins( 0, 0, 0, 0 );
  W->setLayout( hb );
  EW = new QCheckBox;
  hb->addWidget( EW );
  OptWidget::setValueStyle( EW, Param->style(), OptWidget::Text );
  QLabel *label = new QLabel( request.c_str() );
  OptWidget::setLabelStyle( label, Param->style() );
  hb->addWidget( label );
  LabelW = label;
  reset();
  if ( Editable ) {
    EW->setEnabled( true );
    Value = EW->isChecked();
    connect( EW, SIGNAL( toggled( bool ) ),
	     this, SLOT( valueChanged( bool ) ) );
  }
  else {
    EW->setEnabled( false );
  }
}


void OptWidgetBoolean::get( void )
{
  if ( Editable ) {
    InternRead = true;
    bool cn = OO->notifying();
    OO->unsetNotify();
    Param->setBoolean( EW->isChecked() );
    if ( Param->boolean( 0 ) != Value )
      Param->addFlags( OW->changedFlag() );
    Value = EW->isChecked();
    OO->setNotify( cn );
    InternRead = false;
  }
}


void OptWidgetBoolean::reset( void )
{
  InternChanged = true;
  EW->setChecked( Param->boolean() );
  InternChanged = false;
}


void OptWidgetBoolean::resetDefault( void )
{
  if ( Editable ) {
    InternChanged = true;
    EW->setChecked( Param->defaultBoolean() );
    InternChanged = false;
  }
}


void OptWidgetBoolean::valueChanged( bool v )
{
  if ( InternRead || OW->updateDisabled() )
    return;

  Parameter p( *Param );
  p.setBoolean( EW->isChecked() );
  emit valueChanged( p );

  if ( ContUpdate && Editable ) {
    if ( InternChanged ) {
      Value = EW->isChecked();
      bool cn = OO->notifying();
      OO->unsetNotify();
      Param->setBoolean( Value );
      Param->delFlags( OW->changedFlag() );
      OO->setNotify( cn );
    }
    else
      doValueChanged( v );
  }
  string b( v ? "true" : "false" );
  for ( unsigned int k=0; k<Widgets.size(); k++ ) {
    if ( Widgets[k]->param() != OO->end() )
      Widgets[k]->activateOption( Widgets[k]->param()->testActivation( Index[k], b ) );
  }
}


class OptWidgetBooleanEvent : public QEvent
{
public:
  OptWidgetBooleanEvent( bool value )
    : QEvent( QEvent::Type( QEvent::User+1 ) ), Value( value ) {};
  bool value( void ) const { return Value; };
private:
  bool Value;
};


void OptWidgetBoolean::doValueChanged( bool v )
{
  if ( ! tryLockMutex( 5 ) ) {
    // we do not get the lock for the data now,
    // so we repost the event to a later time.
    QCoreApplication::postEvent( this, new OptWidgetBooleanEvent( v ) );
    return;
  }
  bool cn = OO->notifying();
  OO->unsetNotify();
  Param->setBoolean( v );
  if ( Param->boolean( 0 ) != Value )
    Param->addFlags( OW->changedFlag() );
  Value = EW->isChecked();
  if ( cn )
    OO->notify();
  if ( ContUpdate )
    Param->delFlags( OW->changedFlag() );
  OO->setNotify( cn );
  unlockMutex();
}


void OptWidgetBoolean::customEvent( QEvent *e )
{
  if ( e->type() == QEvent::User+1 ) {
    OptWidgetBooleanEvent *be = dynamic_cast<OptWidgetBooleanEvent*>( e );
    doValueChanged( be->value() );
  }
}


void OptWidgetBoolean::initActivation( void )
{
  string b( EW->isChecked() ? "true" : "false" );
  if ( Widgets.back()->param() != OO->end() )
    Widgets.back()->activateOption( Widgets.back()->param()->testActivation( Index.back(), b ) );
}


OptWidgetDate::OptWidgetDate( Options::iterator param, QWidget *label,
			      Options *oo, OptWidget *ow,
			      QMutex *mutex, QWidget *parent )
  : OptWidgetBase( param, label, oo, ow, mutex ),
    DE( 0 ),
    LW( 0 ),
    Year( 0 ),
    Month( 0 ),
    Day( 0 )
{
  if ( Editable ) {
    Year = Param->year( 0 );
    Month = Param->month( 0 );
    Day = Param->day( 0 );
    W = DE = new QDateEdit( QDate( Year, Month, Day ), parent );
    OptWidget::setValueStyle( W, Param->style(), OptWidget::Text );
    DE->setDisplayFormat( "yyyy-MM-dd" );
    Year = DE->date().year();
    Month = DE->date().month();
    Day = DE->date().day();
    connect( DE, SIGNAL( dateChanged( const QDate& ) ),
	     this, SLOT( valueChanged( const QDate& ) ) );
  }
  else {
    LW = new QLabel( Param->text().c_str(), parent );
    OptWidget::setValueStyle( LW, Param->style(), OptWidget::Window );
    LW->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    LW->setLineWidth( 2 );
    W = LW;
  }
}


void OptWidgetDate::get( void )
{
  if ( Editable ) {
    InternRead = true;
    bool cn = OO->notifying();
    OO->unsetNotify();
    Param->setDate( DE->date().year(), DE->date().month(), DE->date().day() );
    if ( Param->year( 0 ) != Year ||
	 Param->month( 0 ) != Month ||
	 Param->day( 0 ) != Day )
      Param->addFlags( OW->changedFlag() );
    Year = DE->date().year();
    Month = DE->date().month();
    Day = DE->date().day();
    OO->setNotify( cn );
    InternRead = false;
  }
}


void OptWidgetDate::reset( void )
{
  InternChanged = true;
  DE->setDate( QDate( Param->year(), Param->month(), Param->day() ) );
  InternChanged = false;
}


void OptWidgetDate::resetDefault( void )
{
  if ( Editable ) {
    InternChanged = true;
    DE->setDate( QDate( Param->defaultYear(),
			Param->defaultMonth(),
			Param->defaultDay() ) );
    InternChanged = false;
  }
}


void OptWidgetDate::valueChanged( const QDate &date )
{
  if ( InternRead || OW->updateDisabled() )
    return;

  Parameter p( *Param );
  p.setDate( DE->date().year(), DE->date().month(), DE->date().day() );
  emit valueChanged( p );

  if ( ContUpdate && Editable ) {
    if ( InternChanged ) {
      Year = DE->date().year();
      Month = DE->date().month();
      Day = DE->date().day();
      bool cn = OO->notifying();
      OO->unsetNotify();
      Param->setDate( Year, Month, Day );
      Param->delFlags( OW->changedFlag() );
      OO->setNotify( cn );
    }
    else
      doValueChanged( date );
  }
  string s = "";
  if ( DE != 0 )
    s = DE->date().toString( Qt::ISODate ).toStdString();
  else
    s = LW->text().toStdString();
  for ( unsigned int k=0; k<Widgets.size(); k++ ) {
    if ( Widgets[k]->param() != OO->end() )
      Widgets[k]->activateOption( Widgets[k]->param()->testActivation( Index[k], s ) );
  }
}


class OptWidgetDateEvent : public QEvent
{
public:
  OptWidgetDateEvent( const QDate &date )
    : QEvent( QEvent::Type( QEvent::User+1 ) ), Date( date ) {};
  QDate date( void ) const { return Date; };
private:
  QDate Date;
};


void OptWidgetDate::doValueChanged( const QDate &date )
{
  if ( ! tryLockMutex( 5 ) ) {
    // we do not get the lock for the data now,
    // so we repost the event to a later time.
    QCoreApplication::postEvent( this, new OptWidgetDateEvent( date ) );
    return;
  }
  bool cn = OO->notifying();
  OO->unsetNotify();
  Param->setDate( date.year(), date.month(), date.day() );
  if ( Param->year( 0 ) != Year ||
       Param->month( 0 ) != Month ||
       Param->day( 0 ) != Day )
    Param->addFlags( OW->changedFlag() );
  Year = Param->year( 0 );
  Month = Param->month( 0 );
  Day = Param->day( 0 );
  if ( cn )
    OO->notify();
  if ( ContUpdate )
    Param->delFlags( OW->changedFlag() );
  OO->setNotify( cn );
  unlockMutex();
}


void OptWidgetDate::customEvent( QEvent *e )
{
  if ( e->type() == QEvent::User+1 ) {
    OptWidgetDateEvent *de = dynamic_cast<OptWidgetDateEvent*>( e );
    doValueChanged( de->date() );
  }
}


void OptWidgetDate::initActivation( void )
{
  string s = "";
  if ( DE != 0 )
    s = DE->date().toString( Qt::ISODate ).toStdString();
  else
    s = LW->text().toStdString();
  if ( Widgets.back()->param() != OO->end() )
    Widgets.back()->activateOption( Widgets.back()->param()->testActivation( Index.back(), s ) );
}


OptWidgetTime::OptWidgetTime( Options::iterator param, QWidget *label,
			      Options *oo, OptWidget *ow,
			      QMutex *mutex, QWidget *parent )
  : OptWidgetBase( param, label, oo, ow, mutex ),
    TE( 0 ),
    LW( 0 ),
    Hour( 0 ),
    Minutes( 0 ),
    Seconds( 0 )
{
  if ( Editable ) {
    Hour = Param->hour( 0 );
    Minutes = Param->minutes( 0 );
    Seconds = Param->seconds( 0 );
    W = TE = new QTimeEdit( QTime( Hour, Minutes, Seconds ), parent );
    OptWidget::setValueStyle( W, Param->style(), OptWidget::Text );
    TE->setDisplayFormat( "hh:mm:ss" );
    Hour = TE->time().hour();
    Minutes = TE->time().minute();
    Seconds = TE->time().second();
    connect( TE, SIGNAL( timeChanged( const QTime& ) ),
	     this, SLOT( valueChanged( const QTime& ) ) );
  }
  else {
    LW = new QLabel( Param->text().c_str(), parent );
    OptWidget::setValueStyle( LW, Param->style(), OptWidget::Window );
    LW->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    LW->setLineWidth( 2 );
    W = LW;
  }
}


void OptWidgetTime::get( void )
{
  if ( Editable ) {
    InternRead = true;
    bool cn = OO->notifying();
    OO->unsetNotify();
    Param->setTime( TE->time().hour(), TE->time().minute(), TE->time().second() );
    if ( Param->hour( 0 ) != Hour ||
	 Param->minutes( 0 ) != Minutes ||
	 Param->seconds( 0 ) != Seconds )
      Param->addFlags( OW->changedFlag() );
    Hour = TE->time().hour();
    Minutes = TE->time().minute();
    Seconds = TE->time().second();
    OO->setNotify( cn );
    InternRead = false;
  }
}


void OptWidgetTime::reset( void )
{
  InternChanged = true;
  TE->setTime( QTime( Param->hour(), Param->minutes(), Param->seconds() ) );
  InternChanged = false;
}


void OptWidgetTime::resetDefault( void )
{
  if ( Editable ) {
    InternChanged = true;
    TE->setTime( QTime( Param->defaultHour(),
			Param->defaultMinutes(),
			Param->defaultSeconds() ) );
    InternChanged = false;
  }
}


void OptWidgetTime::valueChanged( const QTime &time )
{
  if ( InternRead || OW->updateDisabled() )
    return;

  Parameter p( *Param );
  p.setTime( TE->time().hour(), TE->time().minute(), TE->time().second() );
  emit valueChanged( p );

  if ( ContUpdate && Editable ) {
    if ( InternChanged ) {
      Hour = TE->time().hour();
      Minutes = TE->time().minute();
      Seconds = TE->time().second();
      bool cn = OO->notifying();
      OO->unsetNotify();
      Param->setTime( Hour, Minutes, Seconds );
      Param->delFlags( OW->changedFlag() );
      OO->setNotify( cn );
    }
    else
      doValueChanged( time );
  }
  string s = "";
  if ( TE != 0 )
    s = TE->time().toString( Qt::ISODate ).toStdString();
  else
    s = LW->text().toStdString();
  for ( unsigned int k=0; k<Widgets.size(); k++ ) {
    if ( Widgets[k]->param() != OO->end() )
      Widgets[k]->activateOption( Widgets[k]->param()->testActivation( Index[k], s ) );
  }
}


class OptWidgetTimeEvent : public QEvent
{
public:
  OptWidgetTimeEvent( const QTime &time )
    : QEvent( QEvent::Type( QEvent::User+1 ) ), Time( time ) {};
  QTime time( void ) const { return Time; };
private:
  QTime Time;
};


void OptWidgetTime::doValueChanged( const QTime &time )
{
  if ( ! tryLockMutex( 5 ) ) {
    // we do not get the lock for the data now,
    // so we repost the event to a later time.
    QCoreApplication::postEvent( this, new OptWidgetTimeEvent( time ) );
    return;
  }
  bool cn = OO->notifying();
  OO->unsetNotify();
  Param->setTime( time.hour(), time.minute(), time.second() );
  if ( Param->hour( 0 ) != Hour ||
       Param->minutes( 0 ) != Minutes ||
       Param->seconds( 0 ) != Seconds )
    Param->addFlags( OW->changedFlag() );
  Hour = Param->hour( 0 );
  Minutes = Param->minutes( 0 );
  Seconds = Param->seconds( 0 );
  if ( cn )
    OO->notify();
  if ( ContUpdate )
    Param->delFlags( OW->changedFlag() );
  OO->setNotify( cn );
  unlockMutex();
}


void OptWidgetTime::customEvent( QEvent *e )
{
  if ( e->type() == QEvent::User+1 ) {
    OptWidgetTimeEvent *te = dynamic_cast<OptWidgetTimeEvent*>( e );
    doValueChanged( te->time() );
  }
}


void OptWidgetTime::initActivation( void )
{
  string s = "";
  if ( TE != 0 )
    s = TE->time().toString( Qt::ISODate ).toStdString();
  else
    s = LW->text().toStdString();
  if ( Widgets.back()->param() != OO->end() )
    Widgets.back()->activateOption( Widgets.back()->param()->testActivation( Index.back(), s ) );
}


OptWidgetSection::OptWidgetSection( Options::section_iterator sec,
				    Options *oo, OptWidget *ow,
				    QMutex *mutex, QWidget *parent )
  : OptWidgetBase( oo->end(), 0, oo, ow, mutex ),
    Sec( sec )
{
  Str name = (*Sec)->name();
  string id = ( (*Sec)->style() & OptWidget::MathLabel ) ?
    name.htmlUnit() : name.html();
  QLabel *l = new QLabel( id.c_str(), parent );
  l->setTextFormat( Qt::RichText );
  l->setAlignment( Qt::AlignLeft );
  l->setWordWrap( false );
  W = l;
}


OptWidgetMultipleValues::OptWidgetMultipleValues(Options::iterator param, QWidget *label,
                                                 Options *oo, OptWidget *ow,
                                                 QMutex *mutex, QWidget *parent )
 : OptWidgetBase(param, label, oo, ow, mutex)
{
  W = Wrapper = new QWidget(parent);
  QHBoxLayout* layout = new QHBoxLayout(Wrapper);
  ListWidget = new QListWidget();
  layout->addWidget(ListWidget);

  connect(ListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(valueChanged(QListWidgetItem*)));

  if (Editable)
  {
    ListWidget->setDragDropMode(QAbstractItemView::DragDrop);
    ListWidget->setDefaultDropAction(Qt::MoveAction);

    QVBoxLayout* buttons = new QVBoxLayout();

    QPushButton* button = new QPushButton("+", parent);
    button->setFixedSize(25, 25);
    connect(button, SIGNAL(clicked()), this, SLOT(addItem()));
    buttons->addWidget(button);
    button = new QPushButton("-", parent);
    button->setFixedSize(25, 25);
    connect(button, SIGNAL(clicked()), this, SLOT(removeItem()));
    buttons->addWidget(button);

    layout->addLayout(buttons);

    if (Param->isAnyNumber())
      ListWidget->setItemDelegate(new NumberItemDelegate(*Param));
    else if ((Param->style() & Parameter::Select) == Parameter::Select)
      ListWidget->setItemDelegate(new ComboItemDelegate(*Param));
  }

  reset();
}

void OptWidgetMultipleValues::addItem(const string &text, int row)
{
  QListWidgetItem* item = new QListWidgetItem(text.c_str());
  if (Editable)
  {
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setSizeHint(QSize(item->sizeHint().width(), 20));
  }
  ListWidget->insertItem(row < 0 ? ListWidget->count() : row, item);
}

void OptWidgetMultipleValues::addItem(double value, int row)
{
  QListWidgetItem* item = new QListWidgetItem();
  item->setData(Qt::EditRole, QVariant(value));
  if (Editable)
  {
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setSizeHint(QSize(item->sizeHint().width(), 20));
  }
  ListWidget->insertItem(row < 0 ? ListWidget->count() : row, item);
}

void OptWidgetMultipleValues::get()
{
  if (!Editable)
    return;
  InternRead = true;
  bool notifing = OO->notifying();
  OO->unsetNotify();

  if (ListWidget->count() == 0)
  {
    Param->setText("");
  }
  else
  {
    if (Param->isText())
    {
      for (int i = 0; i < ListWidget->count(); ++i)
        Param->addText(ListWidget->item(i)->text().toStdString(), i == 0);
    }
    else if (Param->isInteger())
    {
      for (int i = 0; i < ListWidget->count(); ++i)
      {
        Param->addNumber(ListWidget->item(i)->data(Qt::EditRole).toInt(), -1, std::string(Param->unit().c_str()), i == 0);
      }
    }
    else if (Param->isNumber())
    {
      for (int i = 0; i < ListWidget->count(); ++i)
      {
        Param->addNumber(ListWidget->item(i)->data(Qt::EditRole).toDouble(), -1., std::string(Param->unit().c_str()), i == 0);
      }
    }
  }

  if (Changed)
  {
    Param->addFlags(OW->changedFlag());
    Changed = false;
  }

  OO->setNotify(notifing);
  InternRead = false;
}

void OptWidgetMultipleValues::reset()
{
  InternChanged = true;
  ListWidget->clear();

  if (Param->size() >= 1 && !Param->text(0).empty())
    for (int i = 0; i < Param->size(); ++i)
    {
      if (Param->isText())
        addItem(Param->text(i));
      else
        addItem(Param->number(i));
    }

  InternChanged = false;
}

void OptWidgetMultipleValues::resetDefault()
{
  if (!Editable)
    return;

  InternChanged = true;

  ListWidget->clear();
  if (Param->isText())
    addItem(Param->defaultText());
  else
    addItem(Param->defaultNumber());

  InternChanged = false;
}

void OptWidgetMultipleValues::valueChanged(QListWidgetItem *item)
{
  if ( InternRead || OW->updateDisabled() )
    return;

  /*
    XXX Needs to be implemented!
  Parameter p( *Param );
  p.setTime( DE->date().hour(), DE->date().minute(), DE->date().second() );
  emit valueChanged( p );
  */

  if ( ContUpdate && Editable ) {
    if ( InternChanged ) {
      get();
      Param->delFlags( OW->changedFlag() );
      Changed = false;
    }
    else
      doValueChanged();
  }
}

class OptWidgetMultipleValuesChangeEvent : public QEvent
{
public:
  OptWidgetMultipleValuesChangeEvent()
    : QEvent( QEvent::Type( QEvent::User+1 ) ){};
};

void OptWidgetMultipleValues::doValueChanged()
{
  if ( ! tryLockMutex( 5 ) ) {
    // we do not get the lock for the data now,
    // so we repost the event to a later time.
    QCoreApplication::postEvent( this, new OptWidgetMultipleValuesChangeEvent() );
    return;
  }
  bool cn = OO->notifying();
  get();
  Changed = false;
  if ( cn )
    OO->notify();
  if ( ContUpdate )
    Param->delFlags( OW->changedFlag() );

  unlockMutex();
}

void OptWidgetMultipleValues::customEvent( QEvent *e )
{
  if ( e->type() == QEvent::User+1 ) {
    doValueChanged();
  }
}

void OptWidgetMultipleValues::addItem()
{
  QList<QListWidgetItem*> selections = ListWidget->selectedItems();

  int row = ListWidget->count();
  if (!selections.empty())
    row = ListWidget->row(selections.front());

  QListWidgetItem* item = new QListWidgetItem();
  if (Param->isText())
    item->setText(Param->defaultText().c_str());
  else
    item->setData(Qt::EditRole, QVariant(Param->defaultNumber()));
  item->setFlags(item->flags() | Qt::ItemIsEditable);
  item->setSizeHint(QSize(item->sizeHint().width(), 20));
  ListWidget->insertItem(row + 1, item);
  ListWidget->setItemSelected(item, true);

  Changed = true;
  valueChanged(nullptr);
}

void OptWidgetMultipleValues::removeItem()
{
  QList<QListWidgetItem*> selections = ListWidget->selectedItems();
  if (selections.empty())
    return;

  for (QListWidgetItem* item : selections)
  {
    delete ListWidget->takeItem(ListWidget->row(item));
  }

  Changed = true;
  valueChanged(nullptr);
}

NumberItemDelegate::NumberItemDelegate(Parameter &parameter)
 : Param(parameter)
{

}

QWidget *NumberItemDelegate::createEditor(QWidget *parent,
                      const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
  double min = Param.minimum( Param.outUnit() );
  double max = Param.maximum( Param.outUnit() );
  double step = Param.step( Param.outUnit() );
  DoubleSpinBox* box = new DoubleSpinBox( parent );
  box->setRange( min, max );
  box->setSingleStep( step );
  if ( Param.isNumber() )
    box->setFormat( Param.format() );
  else
    box->setFormat( "%.0f" );
  if ( Param.style() & OptWidget::SpecialInfinite )
    box->setSpecialValueText( "infinite" );
  else if ( Param.style() & OptWidget::SpecialNone )
    box->setSpecialValueText( "none" );

  return box;
}

void NumberItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  QVariant value = index.model()->data(index, Qt::EditRole);
  static_cast<DoubleSpinBox*>(editor)->setValue(value.toDouble());
}

void NumberItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  editor->setGeometry(option.rect);
}


ComboItemDelegate::ComboItemDelegate(Parameter &parameter)
 : Param(parameter)
{

}

QWidget *ComboItemDelegate::createEditor(QWidget *parent,
                      const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
  QComboBox* combo = new QComboBox(parent);

  for (auto&& value : Param.selectOptions())
    combo->addItem(value.c_str());

  if ( (Param.style() & OptWidget::SelectText) > 0 )
    OptWidget::setValueStyle( combo, Param.style(), OptWidget::Combo );
  else
    OptWidget::setValueStyle( combo, Param.style(), OptWidget::Text );
  combo->setInsertPolicy( QComboBox::InsertAtTop );
  combo->setDuplicatesEnabled( false );
  if ( ( Param.style() & OptWidget::ComboAutoCompletion ) == 0 )
    combo->setCompleter( 0 );

  return combo;
}

void ComboItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  QVariant value = index.model()->data(index, Qt::EditRole);
  auto box = static_cast<QComboBox*>(editor);
  box->setCurrentIndex(box->findText(value.toString()));
}

void ComboItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  editor->setGeometry(option.rect);
}

}; /* namespace relacs */

#include "moc_optwidgetbase.cc"

