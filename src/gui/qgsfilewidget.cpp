/***************************************************************************
  qgsfilewidget.cpp

 ---------------------
 begin                : 17.12.2015
 copyright            : (C) 2015 by Denis Rouzaud
 email                : denis.rouzaud@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsfilewidget.h"

#include <QLineEdit>
#include <QToolButton>
#include <QLabel>
#include <QFileDialog>
#include <QGridLayout>
#include <QUrl>
#include <QDropEvent>

#include "qgssettings.h"
#include "qgsfilterlineedit.h"
#include "qgslogger.h"
#include "qgsproject.h"
#include "qgsapplication.h"

QgsFileWidget::QgsFileWidget( QWidget *parent )
  : QWidget( parent )
  , mFilePath( QString() )
  , mButtonVisible( true )
  , mUseLink( false )
  , mFullUrl( false )
  , mDialogTitle( QString() )
  , mFilter( QString() )
  , mDefaultRoot( QString() )
  , mStorageMode( GetFile )
  , mRelativeStorage( Absolute )
{
  setBackgroundRole( QPalette::Window );
  setAutoFillBackground( true );

  mLayout = new QHBoxLayout();
  mLayout->setMargin( 0 );

  // If displaying a hyperlink, use a QLabel
  mLinkLabel = new QLabel( this );
  // Make Qt opens the link with the OS defined viewer
  mLinkLabel->setOpenExternalLinks( true );
  // Label should always be enabled to be able to open
  // the link on read only mode.
  mLinkLabel->setEnabled( true );
  mLinkLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
  mLinkLabel->setTextFormat( Qt::RichText );
  mLinkLabel->hide(); // do not show by default

  // otherwise, use the traditional QLineEdit subclass
  mLineEdit = new QgsFileDropEdit( this );
  connect( mLineEdit, &QLineEdit::textChanged, this, &QgsFileWidget::textEdited );
  mLayout->addWidget( mLineEdit );

  mFileWidgetButton = new QToolButton( this );
  mFileWidgetButton->setText( QStringLiteral( "…" ) );
  connect( mFileWidgetButton, &QAbstractButton::clicked, this, &QgsFileWidget::openFileDialog );
  mLayout->addWidget( mFileWidgetButton );

  setLayout( mLayout );
}

QString QgsFileWidget::filePath()
{
  return mFilePath;
}

void QgsFileWidget::setFilePath( QString path )
{
  if ( path == QgsApplication::nullRepresentation() )
  {
    path = QLatin1String( "" );
  }

  //will trigger textEdited slot
  mLineEdit->setValue( path );
}

void QgsFileWidget::setReadOnly( bool readOnly )
{
  mFileWidgetButton->setEnabled( !readOnly );
  mLineEdit->setEnabled( !readOnly );
}

QString QgsFileWidget::dialogTitle() const
{
  return mDialogTitle;
}

void QgsFileWidget::setDialogTitle( const QString &title )
{
  mDialogTitle = title;
}

QString QgsFileWidget::filter() const
{
  return mFilter;
}

void QgsFileWidget::setFilter( const QString &filters )
{
  mFilter = filters;
  mLineEdit->setFilters( filters );
}

bool QgsFileWidget::fileWidgetButtonVisible() const
{
  return mButtonVisible;
}

void QgsFileWidget::setFileWidgetButtonVisible( bool visible )
{
  mButtonVisible = visible;
  mFileWidgetButton->setVisible( visible );
}

void QgsFileWidget::textEdited( const QString &path )
{
  mFilePath = path;
  mLinkLabel->setText( toUrl( path ) );
  emit fileChanged( mFilePath );
}

bool QgsFileWidget::useLink() const
{
  return mUseLink;
}

void QgsFileWidget::setUseLink( bool useLink )
{
  mUseLink = useLink;
  mLinkLabel->setVisible( mUseLink );
  mLineEdit->setVisible( !mUseLink );
  if ( mUseLink )
  {
    mLayout->removeWidget( mLineEdit );
    mLayout->insertWidget( 0, mLinkLabel );
  }
  else
  {
    mLayout->removeWidget( mLinkLabel );
    mLayout->insertWidget( 0, mLineEdit );
  }
}

bool QgsFileWidget::fullUrl() const
{
  return mFullUrl;
}

void QgsFileWidget::setFullUrl( bool fullUrl )
{
  mFullUrl = fullUrl;
}

QString QgsFileWidget::defaultRoot() const
{
  return mDefaultRoot;
}

void QgsFileWidget::setDefaultRoot( const QString &defaultRoot )
{
  mDefaultRoot = defaultRoot;
}

QgsFileWidget::StorageMode QgsFileWidget::storageMode() const
{
  return mStorageMode;
}

void QgsFileWidget::setStorageMode( QgsFileWidget::StorageMode storageMode )
{
  mStorageMode = storageMode;
  mLineEdit->setStorageMode( storageMode );
}

QgsFileWidget::RelativeStorage QgsFileWidget::relativeStorage() const
{
  return mRelativeStorage;
}

void QgsFileWidget::setRelativeStorage( QgsFileWidget::RelativeStorage relativeStorage )
{
  mRelativeStorage = relativeStorage;
}

QLineEdit *QgsFileWidget::lineEdit()
{
  return mLineEdit;
}

void QgsFileWidget::openFileDialog()
{
  QgsSettings settings;
  QString oldPath;

  // If we use fixed default path
  if ( !mDefaultRoot.isEmpty() )
  {
    oldPath = QDir::cleanPath( mDefaultRoot );
  }
  // if we use a relative path option, we need to obtain the full path
  else if ( !mFilePath.isEmpty() )
  {
    oldPath = relativePath( mFilePath, false );
  }

  // If there is no valid value, find a default path to use
  QUrl url = QUrl::fromUserInput( oldPath );
  if ( !url.isValid() )
  {
    QString defPath = QDir::cleanPath( QgsProject::instance()->fileInfo().absolutePath() );
    if ( defPath.isEmpty() )
    {
      defPath = QDir::homePath();
    }
    oldPath = settings.value( QStringLiteral( "UI/lastFileNameWidgetDir" ), defPath ).toString();
  }

  // Handle Storage
  QString fileName;
  QString title;
  if ( mStorageMode == GetFile )
  {
    title = !mDialogTitle.isEmpty() ? mDialogTitle : tr( "Select a file" );
    fileName = QFileDialog::getOpenFileName( this, title, QFileInfo( oldPath ).absoluteFilePath(), mFilter );
  }
  else if ( mStorageMode == GetDirectory )
  {
    title = !mDialogTitle.isEmpty() ? mDialogTitle : tr( "Select a directory" );
    fileName = QFileDialog::getExistingDirectory( this, title, QFileInfo( oldPath ).absoluteFilePath(),  QFileDialog::ShowDirsOnly );
  }

  if ( fileName.isEmpty() )
    return;


  fileName = QDir::toNativeSeparators( QDir::cleanPath( QFileInfo( fileName ).absoluteFilePath() ) );
  // Store the last used path:

  if ( mStorageMode == GetFile )
  {
    settings.setValue( QStringLiteral( "UI/lastFileNameWidgetDir" ), QFileInfo( fileName ).absolutePath() );
  }
  else if ( mStorageMode == GetDirectory )
  {
    settings.setValue( QStringLiteral( "UI/lastFileNameWidgetDir" ), fileName );
  }

  // Handle relative Path storage
  fileName = relativePath( fileName, true );

  // Keep the new value
  setFilePath( fileName );
}


QString QgsFileWidget::relativePath( const QString &filePath, bool removeRelative ) const
{
  QString RelativePath;
  if ( mRelativeStorage == RelativeProject )
  {
    RelativePath = QDir::toNativeSeparators( QDir::cleanPath( QgsProject::instance()->fileInfo().absolutePath() ) );
  }
  else if ( mRelativeStorage == RelativeDefaultPath && !mDefaultRoot.isEmpty() )
  {
    RelativePath = QDir::toNativeSeparators( QDir::cleanPath( mDefaultRoot ) );
  }

  if ( !RelativePath.isEmpty() )
  {
    if ( removeRelative )
    {
      return QDir::cleanPath( QDir( RelativePath ).relativeFilePath( filePath ) );
    }
    else
    {
      return QDir::cleanPath( QDir( RelativePath ).filePath( filePath ) );
    }
  }

  return filePath;
}


QString QgsFileWidget::toUrl( const QString &path ) const
{
  QString rep;
  if ( path.isEmpty() )
  {
    return QgsApplication::nullRepresentation();
  }

  QString urlStr = relativePath( path, false );
  QUrl url = QUrl::fromUserInput( urlStr );
  if ( !url.isValid() || !url.isLocalFile() )
  {
    QgsDebugMsg( QString( "URL: %1 is not valid or not a local file!" ).arg( path ) );
    rep =  path;
  }

  QString pathStr = url.toString();
  if ( mFullUrl )
  {
    rep = QStringLiteral( "<a href=\"%1\">%2</a>" ).arg( pathStr, path );
  }
  else
  {
    QString fileName = QFileInfo( urlStr ).fileName();
    rep = QStringLiteral( "<a href=\"%1\">%2</a>" ).arg( pathStr, fileName );
  }

  return rep;
}




///@cond PRIVATE


QgsFileDropEdit::QgsFileDropEdit( QWidget *parent )
  : QgsFilterLineEdit( parent )
{
  mDragActive = false;
  setAcceptDrops( true );
}

void QgsFileDropEdit::setFilters( const QString &filters )
{
  mAcceptableExtensions.clear();

  if ( filters.contains( QStringLiteral( "*.*" ) ) )
    return; // everything is allowed!

  QRegularExpression rx( "\\*\\.(\\w+)" );
  QRegularExpressionMatchIterator i = rx.globalMatch( filters );
  while ( i.hasNext() )
  {
    QRegularExpressionMatch match = i.next();
    if ( match.hasMatch() )
    {
      mAcceptableExtensions << match.captured( 1 ).toLower();
    }
  }
}

QString QgsFileDropEdit::acceptableFilePath( QDropEvent *event ) const
{
  QString path;
  if ( event->mimeData()->hasUrls() )
  {
    QFileInfo file( event->mimeData()->urls().first().toLocalFile() );
    if ( ( mStorageMode == QgsFileWidget::GetFile && file.isFile() &&
           ( mAcceptableExtensions.isEmpty() || mAcceptableExtensions.contains( file.suffix(), Qt::CaseInsensitive ) ) )
         || ( mStorageMode == QgsFileWidget::GetDirectory && file.isDir() ) )
      path = file.filePath();
  }
  return path;
}

void QgsFileDropEdit::dragEnterEvent( QDragEnterEvent *event )
{
  QString filePath = acceptableFilePath( event );
  if ( !filePath.isEmpty() )
  {
    event->acceptProposedAction();
    mDragActive = true;
    update();
  }
  else
  {
    event->ignore();
  }
}

void QgsFileDropEdit::dragLeaveEvent( QDragLeaveEvent *event )
{
  QgsFilterLineEdit::dragLeaveEvent( event );
  event->accept();
  mDragActive = false;
  update();
}

void QgsFileDropEdit::dropEvent( QDropEvent *event )
{
  QString filePath = acceptableFilePath( event );
  if ( !filePath.isEmpty() )
  {
    setText( filePath );
    selectAll();
    setFocus( Qt::MouseFocusReason );
    event->acceptProposedAction();
    mDragActive = false;
    update();
  }
}

void QgsFileDropEdit::paintEvent( QPaintEvent *e )
{
  QgsFilterLineEdit::paintEvent( e );
  if ( mDragActive )
  {
    QPainter p( this );
    int width = 2;  // width of highlight rectangle inside frame
    p.setPen( QPen( palette().highlight(), width ) );
    QRect r = rect().adjusted( width, width, -width, -width );
    p.drawRect( r );
  }
}

///@endcond
