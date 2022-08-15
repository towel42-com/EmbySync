// The MIT License( MIT )
//
// Copyright( c ) 2022 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "SettingsDlg.h"
#include "Core/Settings.h"
#include "Core/UserData.h"
#include "SABUtils/ButtonEnabler.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QInputDialog>
#include <QDebug>

#include "ui_SettingsDlg.h"

CSettingsDlg::CSettingsDlg( std::shared_ptr< CSettings > settings, const std::vector< std::shared_ptr< CUserData > > & knownUsers, QWidget * parentWidget )
    : QDialog( parentWidget ),
    fImpl( new Ui::CSettingsDlg ),
    fSettings( settings )
{
    fImpl->setupUi( this );
    new NSABUtils::CButtonEnabler( fImpl->usersList, fImpl->delUser );

    for ( auto && ii : knownUsers )
    {
        fKnownUsers.push_back( std::make_pair( ii, new QTreeWidgetItem( fImpl->knownUsers, QStringList() << ii->name( true ) << ii->name( false ) << ii->connectedID() ) ) );
    }
    load();

    connect( fImpl->dataMissingColor, &QToolButton::clicked,
             [this]()
             {
                 auto newColor = QColorDialog::getColor( fDataMissingColor, this, tr( "Select Color" ) );
                 if ( newColor.isValid() )
                 {
                     fDataMissingColor = newColor;
                     updateColors();
                 }
             } );
    connect( fImpl->mediaSourceColor, &QToolButton::clicked,
             [this]()
             {
                 auto newColor = QColorDialog::getColor( fMediaSourceColor, this, tr( "Select Color" ) );
                 if ( newColor.isValid() )
                 {
                     fMediaSourceColor = newColor;
                     updateColors();
                 }
             } );
    connect( fImpl->mediaDestColor, &QToolButton::clicked,
             [this]()
             {
                 auto newColor = QColorDialog::getColor( fMediaDestColor, this, tr( "Select Color" ) );
                 if ( newColor.isValid() )
                 {
                     fMediaDestColor = newColor;
                     updateColors();
                 }
             } );
    connect( fImpl->addUser, &QToolButton::clicked,
        [ this ]()
        {
            auto newUserName = QInputDialog::getText( this, tr( "Regular Expression" ), tr( "Regular Expression matching User names to sync:" ) );
            if ( newUserName.isEmpty() )
                return;
            new QListWidgetItem( newUserName, fImpl->usersList );
            updateKnownUsers();
        } );
    connect( fImpl->delUser, &QToolButton::clicked,
        [ this ]()
        {
            auto curr = fImpl->usersList->currentItem();
            if ( !curr )
                return;
            delete curr;
            updateKnownUsers();
        } );
    connect( fImpl->usersList, &QListWidget::itemDoubleClicked, 
        [ this ]( QListWidgetItem *item )
        {
            if ( !item )
                return;

            auto newUserName = QInputDialog::getText( this, tr( "Regular Expression" ), tr( "Regular Expression matching User names to sync:" ), QLineEdit::EchoMode::Normal, item->text() );
            if ( newUserName.isEmpty() || ( newUserName == item->text() ) )
                return;
            item->setText( newUserName );
            updateKnownUsers();
        } );
    fImpl->tabWidget->setCurrentIndex( 0 );
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::accept()
{
    save();
    QDialog::accept();
}


void CSettingsDlg::load()
{
    fImpl->embyURL1->setText( fSettings->url( true ) );
    fImpl->embyAPI1->setText( fSettings->apiKey( true ) );
    fImpl->embyURL2->setText( fSettings->url( false ) );
    fImpl->embyAPI2->setText( fSettings->apiKey( false ) );

    fMediaSourceColor = fSettings->mediaSourceColor();
    fMediaDestColor = fSettings->mediaDestColor();
    fDataMissingColor = fSettings->dataMissingColor();
    updateColors();
    auto maxItems = fSettings->maxItems();
    if ( maxItems < fImpl->maxItems->minimum() )
        maxItems = fImpl->maxItems->minimum();
    fImpl->maxItems->setValue( maxItems );

    fImpl->syncAudio->setChecked( fSettings->syncAudio() );
    fImpl->syncVideo->setChecked( fSettings->syncVideo() );
    fImpl->syncEpisode->setChecked( fSettings->syncEpisode() );
    fImpl->syncMovie->setChecked( fSettings->syncMovie() );
    fImpl->syncTrailer->setChecked( fSettings->syncTrailer() );
    fImpl->syncAdultVideo->setChecked( fSettings->syncAdultVideo() );
    fImpl->syncMusicVideo->setChecked( fSettings->syncMusicVideo() );
    fImpl->syncGame->setChecked( fSettings->syncGame() );
    fImpl->syncBook->setChecked( fSettings->syncBook() );

    auto syncUsers = fSettings->syncUserList();
    for ( auto && ii : syncUsers )
    {
        if ( ii.isEmpty() )
            continue;
        new QListWidgetItem( ii, fImpl->usersList );
    }

    updateKnownUsers();
}

void CSettingsDlg::save()
{
    fSettings->setURL( fImpl->embyURL1->text(), true );
    fSettings->setAPIKey( fImpl->embyAPI1->text(), true );
    fSettings->setURL( fImpl->embyURL2->text(), false );
    fSettings->setAPIKey( fImpl->embyAPI2->text(), false );

    fSettings->setMediaSourceColor( fMediaSourceColor );
    fSettings->setMediaDestColor( fMediaDestColor );
    fSettings->setDataMissingColor( fDataMissingColor );
    fSettings->setMaxItems( ( fImpl->maxItems->value() == fImpl->maxItems->minimum() ) ? -1 : fImpl->maxItems->value() );

    fSettings->setSyncAudio( fImpl->syncAudio->isChecked() );
    fSettings->setSyncVideo( fImpl->syncVideo->isChecked() );
    fSettings->setSyncEpisode( fImpl->syncEpisode->isChecked() );
    fSettings->setSyncMovie( fImpl->syncMovie->isChecked() );
    fSettings->setSyncTrailer( fImpl->syncTrailer->isChecked() );
    fSettings->setSyncAdultVideo( fImpl->syncAdultVideo->isChecked() );
    fSettings->setSyncMusicVideo( fImpl->syncMusicVideo->isChecked() );
    fSettings->setSyncGame( fImpl->syncGame->isChecked() );
    fSettings->setSyncBook( fImpl->syncBook->isChecked() );
    fSettings->setSyncUserList( syncUserStrings() );
}

QStringList CSettingsDlg::syncUserStrings() const
{
    QStringList syncUsers;
    for ( int ii = 0; ii < fImpl->usersList->count(); ++ii )
    {
        auto curr = fImpl->usersList->item( ii );
        if ( !curr )
            continue;
        syncUsers << curr->text();
    }
    return syncUsers;
}

void CSettingsDlg::updateKnownUsers()
{
    QStringList syncUsers;
    for ( int ii = 0; ii < fImpl->usersList->count(); ++ii )
    {
        auto curr = fImpl->usersList->item( ii );
        if ( !curr )
            continue;
        if ( !QRegularExpression( curr->text() ).isValid() )
        {
            curr->setBackground( Qt::red );
            continue;
        }
        else
            curr->setBackground( QBrush() );

        syncUsers << "(" + curr->text() + ")";
    }

    auto regExpStr = syncUsers.join( "|" );
    QRegularExpression regExp;
    if ( !regExpStr.isEmpty() )
        regExp = QRegularExpression( regExpStr );
    qDebug() << regExp << regExp.isValid() << regExp.pattern();
    for ( auto && ii : fKnownUsers )
    {
        bool isMatch = ii.first->isUser( regExp );
        for ( int column = 0; column < ii.second->columnCount(); ++column )
        {
            if ( isMatch )
                ii.second->setBackground( column, Qt::green );
            else
                ii.second->setBackground( column, QBrush() );
        }
    }
}

void CSettingsDlg::updateColors()
{
    updateColor( fImpl->mediaSource, fMediaSourceColor);
    updateColor( fImpl->mediaDest, fMediaDestColor );
    updateColor( fImpl->dataMissing, fDataMissingColor );
}

void CSettingsDlg::updateColor( QLabel * label, const QColor & color )
{
    QString styleSheet;
    if ( color == Qt::black )
        styleSheet = QString( "QLabel { background-color: %1; color: #ffffff }" ).arg( color.name() );
    else
        styleSheet = QString( "QLabel { background-color: %1 }" ).arg( color.name() );
    label->setStyleSheet( styleSheet );
}

bool CSettings::load( const QString & fileName, bool addToRecentFileList, QWidget * parentWidget )
{
    fFileName = fileName;
    return load( addToRecentFileList, parentWidget );
}

bool CSettings::load( bool addToRecentFileList, QWidget * parentWidget )
{
    if ( fFileName.isEmpty() )
    {
        auto fileName = QFileDialog::getOpenFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
        if ( fileName.isEmpty() )
            return false;
        fFileName = QFileInfo( fileName ).absoluteFilePath();
    }

    return load( fFileName,
                 [ parentWidget ]( const QString & title, const QString & msg )
    {
        QMessageBox::critical( parentWidget, title, msg );
    }, addToRecentFileList );
}

//std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc
bool CSettings::save( QWidget * parentWidget )
{
    return save( parentWidget,
                 [parentWidget]() -> QString
                 {
                     auto fileName = QFileDialog::getSaveFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
                     if ( fileName.isEmpty() )
                         return {};

                     return fileName;
                 },
                 [parentWidget]( const QString & title, const QString & msg )
                 {
                     QMessageBox::critical( parentWidget, title, msg );
                 } );
}
  
bool CSettings::maybeSave( QWidget * parentWidget )
{
    return maybeSave( parentWidget,
                      [parentWidget]() -> QString
                      {
                          auto fileName = QFileDialog::getSaveFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
                          if ( fileName.isEmpty() )
                              return {};

                          return fileName;
                      },
                      [parentWidget]( const QString & title, const QString & msg )
                      {
                          QMessageBox::critical( parentWidget, title, msg );
                      } );
}

bool CSettings::save( QWidget * parentWidget, std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc )
{
    if ( fFileName.isEmpty() )
        return maybeSave( parentWidget, selectFileFunc, errorFunc );

    return save( errorFunc );
}

bool CSettings::maybeSave( QWidget * parentWidget, std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc )
{
    if ( !fChanged )
        return true;

    if ( fFileName.isEmpty() )
    {
        if ( selectFileFunc )
        {
            auto fileName = selectFileFunc();
            if ( fileName.isEmpty() )
                return true;
            fFileName = fileName;
        }
    }

    if ( fFileName.isEmpty() )
        return false;
    return save( parentWidget, selectFileFunc, errorFunc );
}
