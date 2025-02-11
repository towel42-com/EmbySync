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
#include "EditServerDlg.h"
#include "Core/Settings.h"
#include "Core/ServerInfo.h"
#include "Core/UserData.h"
#include "Core/SyncSystem.h"
#include "Core/ServerModel.h"
#include "SABUtils/ButtonEnabler.h"
#include "SABUtils/WidgetChanged.h"

#include <QFileDialog>
#include <QDesktopServices>
#include <QMetaMethod>
#include <QMessageBox>
#include <QColorDialog>
#include <QInputDialog>
#include <QDebug>
#include <QPushButton>

#include "ui_SettingsDlg.h"

CSettingsDlg::CSettingsDlg( std::shared_ptr< CSettings > settings, std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CSyncSystem > syncSystem, QWidget *parentWidget ) :
    QDialog( parentWidget ),
    fImpl( new Ui::CSettingsDlg ),
    fSettings( settings ),
    fServerModel( serverModel ),
    fSyncSystem( syncSystem )
{
    fImpl->setupUi( this );
    new NSABUtils::CButtonEnabler( fImpl->usersList, fImpl->delUser );
    new NSABUtils::CButtonEnabler( fImpl->usersList, fImpl->editUser );
    new NSABUtils::CButtonEnabler( fImpl->servers, fImpl->delServer );
    new NSABUtils::CButtonEnabler( fImpl->servers, fImpl->editServer );
    new NSABUtils::CButtonEnabler( fImpl->searchServers, fImpl->delSearchServer );
    new NSABUtils::CButtonEnabler( fImpl->searchServers, fImpl->editSearchServer );
    new NSABUtils::CButtonEnabler( fImpl->showsList, fImpl->delShow );
    new NSABUtils::CButtonEnabler( fImpl->showsList, fImpl->editShow );

    fServerTestButton = fImpl->serverTestButtonBox->addButton( tr( "Test" ), QDialogButtonBox::ButtonRole::ActionRole );
    fServerTestButton->setObjectName( "Test Server Button" );
    connect( fServerTestButton, &QPushButton::clicked, this, &CSettingsDlg::slotTestServers );
    connect( fSyncSystem.get(), &CSyncSystem::sigTestServerResults, this, &CSettingsDlg::slotTestServerResults );

    fSearchServerTestButton = fImpl->searchServerTestButtonBox->addButton( tr( "Test Search" ), QDialogButtonBox::ButtonRole::ActionRole );
    fSearchServerTestButton->setObjectName( "Test Search Server Button" );
    connect( fSearchServerTestButton, &QPushButton::clicked, this, &CSettingsDlg::slotTestSearchServers );

    load();

    connect(
        fImpl->dataMissingColor, &QToolButton::clicked,
        [ this ]()
        {
            auto newColor = QColorDialog::getColor( fDataMissingColor, this, tr( "Select Color" ) );
            if ( newColor.isValid() )
            {
                fDataMissingColor = newColor;
                updateColors();
            }
        } );
    connect(
        fImpl->mediaSourceColor, &QToolButton::clicked,
        [ this ]()
        {
            auto newColor = QColorDialog::getColor( fMediaSourceColor, this, tr( "Select Color" ) );
            if ( newColor.isValid() )
            {
                fMediaSourceColor = newColor;
                updateColors();
            }
        } );
    connect(
        fImpl->mediaDestColor, &QToolButton::clicked,
        [ this ]()
        {
            auto newColor = QColorDialog::getColor( fMediaDestColor, this, tr( "Select Color" ) );
            if ( newColor.isValid() )
            {
                fMediaDestColor = newColor;
                updateColors();
            }
        } );
    connect( fImpl->addUser, &QToolButton::clicked, [ this ]() { editUser( nullptr ); } );
    connect(
        fImpl->delUser, &QToolButton::clicked,
        [ this ]()
        {
            auto curr = fImpl->usersList->currentItem();
            if ( !curr )
                return;
            delete curr;
            updateKnownUsers();
        } );
    connect(
        fImpl->editUser, &QToolButton::clicked,
        [ this ]()
        {
            auto curr = fImpl->usersList->currentItem();
            editUser( curr );
        } );

    connect( fImpl->usersList, &QListWidget::itemDoubleClicked, [ this ]( QListWidgetItem *item ) { return editUser( item ); } );

    connect( fImpl->addShow, &QToolButton::clicked, [ this ]() { editShow( nullptr ); } );
    connect(
        fImpl->delShow, &QToolButton::clicked,
        [ this ]()
        {
            auto curr = fImpl->showsList->currentItem();
            if ( !curr )
                return;
            delete curr;
            updateKnownShows();
        } );
    connect(
        fImpl->editShow, &QToolButton::clicked,
        [ this ]()
        {
            auto curr = fImpl->showsList->currentItem();
            editShow( curr );
        } );

    connect( fImpl->showsList, &QListWidget::itemDoubleClicked, [ this ]( QListWidgetItem *item ) { return editShow( item ); } );

    connect( fImpl->addServer, &QToolButton::clicked, [ this ]() { editServer( nullptr ); } );
    connect(
        fImpl->delServer, &QToolButton::clicked,
        [ this ]()
        {
            auto curr = fImpl->servers->currentItem();
            if ( !curr )
                return;
            delete curr;
        } );
    connect(
        fImpl->editServer, &QToolButton::clicked,
        [ this ]()
        {
            auto curr = fImpl->servers->currentItem();
            editServer( curr );
        } );
    connect( fImpl->servers, &QTreeWidget::itemDoubleClicked, [ this ]( QTreeWidgetItem *item ) { return editServer( item ); } );
    connect( fImpl->servers, &QTreeWidget::itemSelectionChanged, this, &CSettingsDlg::slotCurrServerChanged );
    NSABUtils::setupModelChanged( fImpl->servers->model(), this, SLOT( slotServerModelChanged() ) );
    connect( fImpl->servers, &QTreeWidget::itemSelectionChanged, this, &CSettingsDlg::slotCurrServerChanged );
    connect( fImpl->moveServerUp, &QToolButton::clicked, this, &CSettingsDlg::slotMoveServerUp );
    connect( fImpl->moveServerDown, &QToolButton::clicked, this, &CSettingsDlg::slotMoveServerDown );

    connect( fImpl->addSearchServer, &QToolButton::clicked, [ this ]() { editSearchServer( nullptr ); } );
    connect(
        fImpl->delSearchServer, &QToolButton::clicked,
        [ this ]()
        {
            auto curr = fImpl->searchServers->currentItem();
            if ( !curr )
                return;
            delete curr;
        } );
    connect(
        fImpl->editSearchServer, &QToolButton::clicked,
        [ this ]()
        {
            auto curr = fImpl->searchServers->currentItem();
            editServer( curr );
        } );
    connect( fImpl->searchServers, &QTreeWidget::itemDoubleClicked, [ this ]( QTreeWidgetItem *item ) { return editSearchServer( item ); } );
    connect( fImpl->searchServers, &QTreeWidget::itemSelectionChanged, this, &CSettingsDlg::slotCurrSearchServerChanged );
    connect( fImpl->searchServers, &QTreeWidget::itemSelectionChanged, this, &CSettingsDlg::slotCurrSearchServerChanged );
    connect( fImpl->moveSearchServerUp, &QToolButton::clicked, this, &CSettingsDlg::slotMoveSearchServerUp );
    connect( fImpl->moveSearchServerDown, &QToolButton::clicked, this, &CSettingsDlg::slotMoveSearchServerDown );

    fImpl->tabWidget->setCurrentIndex( 0 );
}

void CSettingsDlg::slotMoveServerUp()
{
    moveCurrServer( true );
}

void CSettingsDlg::slotMoveServerDown()
{
    moveCurrServer( false );
}

void CSettingsDlg::slotCurrServerChanged()
{
    currServerChanged( fImpl->servers, fImpl->moveServerUp, fImpl->moveServerDown );
}

void CSettingsDlg::slotCurrSearchServerChanged()
{
    currServerChanged( fImpl->searchServers, fImpl->moveSearchServerUp, fImpl->moveSearchServerDown );
}

void CSettingsDlg::currServerChanged( QTreeWidget *serverTree, QToolButton *up, QToolButton *down )
{
    if ( !serverTree || !up || !down )
        return;

    auto curr = serverTree->currentItem();
    if ( !curr )
        return;

    auto itemAbove = serverTree->itemAbove( curr );
    up->setEnabled( itemAbove != nullptr );

    auto itemBelow = serverTree->itemBelow( curr );
    down->setEnabled( itemBelow != nullptr );
}

void CSettingsDlg::moveCurrSearchServer( bool up )
{
    if ( moveCurrServer( fImpl->searchServers, up ) )
        slotCurrSearchServerChanged();
}

void CSettingsDlg::moveCurrServer( bool up )
{
    if ( moveCurrServer( fImpl->servers, up ) )
        slotCurrServerChanged();
}

bool CSettingsDlg::moveCurrServer( QTreeWidget *serverTree, bool up )
{
    if ( !serverTree )
        return false;

    auto curr = serverTree->currentItem();
    if ( !curr )
        return false;

    auto parentItem = curr->parent();
    if ( parentItem )
    {
        auto currIdx = parentItem->indexOfChild( curr );
        parentItem->takeChild( currIdx );
        currIdx += up ? -1 : +1;
        parentItem->insertChild( currIdx, curr );
    }
    else
    {
        auto currIdx = serverTree->indexOfTopLevelItem( curr );
        serverTree->takeTopLevelItem( currIdx );
        currIdx += up ? -1 : +1;
        serverTree->insertTopLevelItem( currIdx, curr );
    }
    serverTree->selectionModel()->clearSelection();
    curr->setSelected( true );
    serverTree->setCurrentItem( curr );
    return true;
}

void CSettingsDlg::slotMoveSearchServerUp()
{
    moveCurrSearchServer( true );
}

void CSettingsDlg::slotMoveSearchServerDown()
{
    moveCurrSearchServer( false );
}

void CSettingsDlg::loadKnownUsers( const std::vector< std::shared_ptr< CUserData > > &knownUsers )
{
    auto headerLabels = QStringList() << tr( "Connected ID" );
    for ( auto &&serverInfo : *fServerModel )
        headerLabels << serverInfo->displayName();
    fImpl->knownUsers->setColumnCount( headerLabels.count() );
    fImpl->knownUsers->setHeaderLabels( headerLabels );

    for ( auto &&ii : knownUsers )
    {
        auto data = QStringList() << ii->connectedID();
        for ( auto &&serverInfo : *fServerModel )
            data << ii->name( serverInfo->keyName() );

        auto item = new QTreeWidgetItem( fImpl->knownUsers, data );
        item->setIcon( 0, QIcon( QPixmap::fromImage( ii->anyAvatar() ) ) );

        fKnownUsers.push_back( std::make_pair( ii, item ) );
    }
    updateKnownUsers();
}

void CSettingsDlg::loadKnownShows( const std::set< QString > &knownShows )
{
    auto headerLabels = QStringList() << tr( "Show Name" );
    fImpl->knownShows->setColumnCount( headerLabels.count() );
    fImpl->knownShows->setHeaderLabels( headerLabels );

    for ( auto &&ii : knownShows )
    {
        auto item = new QTreeWidgetItem( fImpl->knownShows, QStringList() << ii );
        fKnownShows.push_back( std::make_pair( ii, item ) );
    }
    updateKnownShows();
}

void CSettingsDlg::editUser( QListWidgetItem *item )
{
    auto curr = item ? item->text() : QString();

    auto newUserName = QInputDialog::getText( this, tr( "Regular Expression" ), tr( "Regular Expression matching User names to sync:" ), QLineEdit::EchoMode::Normal, curr );
    if ( newUserName.isEmpty() || ( item && ( newUserName == item->text() ) ) )
        return;

    if ( !item )
        item = new QListWidgetItem( newUserName, fImpl->usersList );
    else
        item->setText( newUserName );

    updateKnownUsers();
}

void CSettingsDlg::editShow( QListWidgetItem *item )
{
    auto curr = item ? item->text() : QString();

    auto newShowName = QInputDialog::getText( this, tr( "Regular Expression" ), tr( "Regular Expression matching Show Series name to ignore:" ), QLineEdit::EchoMode::Normal, curr );
    if ( newShowName.isEmpty() || ( item && ( newShowName == item->text() ) ) )
        return;

    if ( !item )
        item = new QListWidgetItem( newShowName, fImpl->showsList );
    else
        item->setText( newShowName );

    updateKnownShows();
}

void CSettingsDlg::editServer( QTreeWidgetItem *item )
{
    editServer( fImpl->servers, item );
}

void CSettingsDlg::editSearchServer( QTreeWidgetItem *item )
{
    editServer( fImpl->searchServers, item );
}

void CSettingsDlg::editServer( QTreeWidget *serverTree, QTreeWidgetItem *item )
{
    if ( !serverTree )
        return;

    auto displayName = item ? item->text( 0 ) : QString();
    auto url = item ? item->text( 1 ) : QString();
    auto apiKey = item ? item->text( 2 ) : QString();
    bool enabled = item ? ( item->checkState( 0 ) == Qt::CheckState::Checked ) : true;

    CEditServerDlg dlg( displayName, url, apiKey, enabled, serverTree == fImpl->searchServers, this );
    if ( dlg.exec() == QDialog::Accepted )
    {
        if ( !item )
            item = new QTreeWidgetItem( serverTree );
        item->setText( 0, dlg.name() );
        item->setCheckState( 0, dlg.enabled() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
        item->setIcon( 0, QIcon( QString::fromUtf8( ":/SABUtilsResources/unknownStatus.png" ) ) );
        item->setText( 1, dlg.url() );
        item->setText( 2, dlg.apiKey() );
    }
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
    fImpl->servers->setColumnCount( 3 );
    for ( auto &&ii : *fServerModel )
    {
        loadServer( fImpl->servers, ii );
    }

    loadPrimaryServers();

    auto searchServers = fSettings->searchServers();
    for ( auto &&ii : searchServers )
    {
        loadServer( fImpl->searchServers, ii );
    }

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
    for ( auto &&ii : syncUsers )
    {
        if ( ii.isEmpty() )
            continue;
        new QListWidgetItem( ii, fImpl->usersList );
    }

    fImpl->checkForLatest->setChecked( CSettings::checkForLatest() );
    fImpl->loadLastProject->setChecked( CSettings::loadLastProject() );
}

void CSettingsDlg::loadServer( QTreeWidget *serverTree, const std::shared_ptr< CServerInfo > &serverInfo )
{
    if ( !serverInfo || !serverTree )
        return;
    auto name = serverInfo->displayName();
    auto url = serverInfo->url();
    auto apiKey = serverInfo->apiKey();

    auto item = new QTreeWidgetItem( serverTree, QStringList() << name << url << apiKey );
    item->setCheckState( 0, serverInfo->isEnabled() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
    item->setIcon( 0, QIcon( QString::fromUtf8( ":/SABUtilsResources/unknownStatus.png" ) ) );
}

void CSettingsDlg::loadPrimaryServers()
{
    auto primServerUrl = fSettings->primaryServer();
    QString primServerName;
    if ( fImpl->primaryServer->count() )
    {
        primServerUrl = fImpl->primaryServer->currentData().toString();
        primServerName = fImpl->primaryServer->currentText();
    }

    fImpl->primaryServer->clear();

    auto servers = getServerInfos( fImpl->servers, true );
    for ( auto &&serverInfo : servers )
    {
        if ( !serverInfo->isEnabled() )
            continue;

        auto name = serverInfo->displayName();
        auto url = serverInfo->url();

        fImpl->primaryServer->addItem( tr( "%1 (%2)" ).arg( name ).arg( url ), url );
    }

    auto ii = fImpl->primaryServer->findData( primServerUrl );
    if ( ( ii == -1 ) && !primServerName.isEmpty() )
    {
        ii = fImpl->primaryServer->findText( primServerName );
    }
    fImpl->primaryServer->setCurrentIndex( ii );
}

void CSettingsDlg::save()
{
    auto servers = getServerInfos( fImpl->servers, false );
    fServerModel->setServers( servers );

    auto primServer = fImpl->primaryServer->currentData().toString();
    fSettings->setPrimaryServer( primServer );

    auto searchServers = getServerInfos( fImpl->searchServers, false );
    fSettings->setSearchServers( { searchServers.begin(), searchServers.end() } );

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
    fSettings->setIgnoreShowList( ignoreShowStrings() );

    CSettings::setCheckForLatest( fImpl->checkForLatest->isChecked() );
    CSettings::setLoadLastProject( fImpl->loadLastProject->isChecked() );
}

std::vector< std::shared_ptr< CServerInfo > > CSettingsDlg::getServerInfos( QTreeWidget *serverTree, bool enabledOnly ) const
{
    if ( !serverTree )
        return {};

    std::vector< std::shared_ptr< CServerInfo > > retVal;
    for ( int ii = 0; ii < serverTree->topLevelItemCount(); ++ii )
    {
        auto curr = getServerInfo( serverTree, ii );
        if ( !curr )
            continue;

        if ( !enabledOnly || curr->isEnabled() )
            retVal.push_back( curr );
    }
    return retVal;
}

std::shared_ptr< CServerInfo > CSettingsDlg::getServerInfo( QTreeWidget *serverTree, int ii ) const
{
    if ( !serverTree )
        return {};

    auto item = serverTree->topLevelItem( ii );

    auto name = item->text( 0 );
    auto url = item->text( 1 );
    auto apiKey = item->text( 2 );
    auto enabled = item->checkState( 0 ) == Qt::CheckState::Checked;

    return std::make_shared< CServerInfo >( name, url, apiKey, enabled );
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

QStringList CSettingsDlg::ignoreShowStrings() const
{
    QStringList retVal;
    for ( int ii = 0; ii < fImpl->showsList->count(); ++ii )
    {
        auto curr = fImpl->showsList->item( ii );
        if ( !curr )
            continue;
        retVal << curr->text();
    }
    return retVal;
}

QRegularExpression CSettingsDlg::validateRegExes( QListWidget *list ) const
{
    if ( !list )
        return {};

    QStringList regExList;
    for ( int ii = 0; ii < list->count(); ++ii )
    {
        auto curr = list->item( ii );
        if ( !curr )
            continue;
        if ( !QRegularExpression( curr->text() ).isValid() )
        {
            curr->setBackground( Qt::red );
            continue;
        }
        else
            curr->setBackground( QBrush() );

        regExList << "(" + curr->text() + ")";
    }
    auto regExpStr = regExList.join( "|" );

    QRegularExpression regExp;
    if ( !regExpStr.isEmpty() )
        regExp = QRegularExpression( regExpStr );
    //qDebug() << regExp << regExp.isValid() << regExp.pattern();
    return regExp;
}

void CSettingsDlg::updateKnownUsers()
{
    auto regExp = validateRegExes( fImpl->usersList );
    for ( auto &&ii : fKnownUsers )
    {
        bool isMatch = ii.first->isUser( regExp );
        setIsMatch( ii.second, isMatch, false );
    }
}

void CSettingsDlg::updateKnownShows()
{
    auto regExp = validateRegExes( fImpl->showsList );

    for ( auto &&ii : fKnownShows )
    {
        auto match = regExp.match( ii.first );
        bool isMatch = ( match.hasMatch() && ( match.captured( 0 ).length() == ii.first.length() ) );
        setIsMatch( ii.second, isMatch, true );
    }
}

void CSettingsDlg::setIsMatch( QTreeWidgetItem *item, bool isMatch, bool negMatch )
{
    if ( !item )
        return;
    for ( int column = 0; column < item->columnCount(); ++column )
    {
        if ( negMatch )
        {
            if ( isMatch )
                item->setBackground( column, Qt::red );
            else
                item->setBackground( column, QBrush() );
        }
        else
        {
            if ( isMatch )
                item->setBackground( column, Qt::green );
            else
                item->setBackground( column, QBrush() );
        }
    }
}

void CSettingsDlg::updateColors()
{
    updateColor( fImpl->mediaSource, fMediaSourceColor );
    updateColor( fImpl->mediaDest, fMediaDestColor );
    updateColor( fImpl->dataMissing, fDataMissingColor );
}

void CSettingsDlg::updateColor( QLabel *label, const QColor &color )
{
    QString styleSheet;
    if ( color == Qt::black )
        styleSheet = QString( "QLabel { background-color: %1; color: #ffffff }" ).arg( color.name() );
    else
        styleSheet = QString( "QLabel { background-color: %1 }" ).arg( color.name() );
    label->setStyleSheet( styleSheet );
}

bool CSettings::load( const QString &fileName, bool addToRecentFileList, QWidget *parentWidget )
{
    fFileName = fileName;
    return load( addToRecentFileList, parentWidget );
}

bool CSettings::load( bool addToRecentFileList, QWidget *parentWidget )
{
    if ( fFileName.isEmpty() )
    {
        auto fileName = QFileDialog::getOpenFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
        if ( fileName.isEmpty() )
            return false;
        fFileName = QFileInfo( fileName ).absoluteFilePath();
    }

    return load(
        fFileName, [ parentWidget ]( const QString &title, const QString &msg ) { QMessageBox::critical( parentWidget, title, msg ); }, addToRecentFileList );
}

// std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc
bool CSettings::save( QWidget *parentWidget )
{
    return save(
        parentWidget,
        [ parentWidget ]() -> QString
        {
            auto fileName = QFileDialog::getSaveFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
            if ( fileName.isEmpty() )
                return {};

            return fileName;
        },
        [ parentWidget ]( const QString &title, const QString &msg ) { QMessageBox::critical( parentWidget, title, msg ); } );
}

bool CSettings::saveAs( QWidget *parentWidget )
{
    auto fileName = QFileDialog::getSaveFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
    if ( fileName.isEmpty() )
        return false;
    fFileName = fileName;
    return save( parentWidget );
}

bool CSettings::maybeSave( QWidget *parentWidget )
{
    return maybeSave(
        parentWidget,
        [ parentWidget ]() -> QString
        {
            auto fileName = QFileDialog::getSaveFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
            if ( fileName.isEmpty() )
                return {};

            return fileName;
        },
        [ parentWidget ]( const QString &title, const QString &msg ) { QMessageBox::critical( parentWidget, title, msg ); } );
}

bool CSettings::save( QWidget *parentWidget, std::function< QString() > selectFileFunc, std::function< void( const QString &title, const QString &msg ) > errorFunc )
{
    if ( fFileName.isEmpty() )
        return maybeSave( parentWidget, selectFileFunc, errorFunc );

    return save( errorFunc );
}

bool CSettings::maybeSave( QWidget *parentWidget, std::function< QString() > selectFileFunc, std::function< void( const QString &title, const QString &msg ) > errorFunc )
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

void CSettingsDlg::slotTestServers()
{
    for ( int ii = 0; ii < fImpl->servers->topLevelItemCount(); ++ii )
    {
        fImpl->servers->topLevelItem( ii )->setIcon( 0, QIcon( QString::fromUtf8( ":/SABUtilsResources/unknownStatus.png" ) ) );
    }

    auto tmp = getServerInfos( fImpl->servers, true );
    std::vector< std::shared_ptr< const CServerInfo > > servers;
    for ( auto &&ii : tmp )
        servers.push_back( std::const_pointer_cast< const CServerInfo >( ii ) );

    fSyncSystem->testServers( servers );
}

void CSettingsDlg::slotTestServerResults( const QString &serverName, bool results, const QString &msg )
{
    for ( int ii = 0; ii < fImpl->servers->topLevelItemCount(); ++ii )
    {
        auto serverInfo = getServerInfo( fImpl->servers, ii );
        if ( serverInfo->keyName() == serverName )
        {
            auto item = fImpl->servers->topLevelItem( ii );
            if ( results )
                item->setIcon( 0, QIcon( QString::fromUtf8( ":/SABUtilsResources/ok.png" ) ) );
            else
                item->setIcon( 0, QIcon( QString::fromUtf8( ":/SABUtilsResources/error.png" ) ) );
        }
    }
    if ( !results )
    {
        QMessageBox::critical( this, tr( "Error" ), tr( "Error in Testing: '%1' - %1" ).arg( serverName ).arg( msg ), QMessageBox::StandardButton::Ok );
    }
}

void CSettingsDlg::slotTestSearchServers()
{
    for ( int ii = 0; ii < fImpl->searchServers->topLevelItemCount(); ++ii )
    {
        auto item = fImpl->searchServers->topLevelItem( ii );

        auto serverInfo = getServerInfo( fImpl->searchServers, ii );
        auto url = serverInfo->searchUrl( "the empire strikes back" );
        if ( url.isValid() )
        {
            item->setIcon( 0, QIcon( QString::fromUtf8( ":/SABUtilsResources/ok.png" ) ) );
            QDesktopServices::openUrl( url );
        }
        else
            item->setIcon( 0, QIcon( QString::fromUtf8( ":/SABUtilsResources/error.png" ) ) );
    }
}

void CSettingsDlg::slotServerModelChanged()
{
    loadPrimaryServers();
}