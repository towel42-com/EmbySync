// The MIT License( MIT )
//
// Copyright( c ) 2022 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software iRHS
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

#include "MainWindow.h"
#include "SettingsDlg.h"

#include "Core/SyncSystem.h"
#include "Core/UserData.h"
#include "Core/MediaData.h"
#include "Core/Settings.h"
#include "SABUtils/QtUtils.h"

#include "ui_MainWindow.h"

#include <unordered_set>

#include <QTimer>
#include <QScrollBar>
#include <QFileInfo>
#include <QProgressDialog>
#include <QMessageBox>

CMainWindow::CMainWindow( QWidget * parent )
    : QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    fImpl->users->sortByColumn( 0, Qt::SortOrder::AscendingOrder );
    fImpl->lhsMedia->sortByColumn( 0, Qt::SortOrder::AscendingOrder );
    fImpl->rhsMedia->sortByColumn( 0, Qt::SortOrder::AscendingOrder );

    fSettings = std::make_shared< CSettings >();

    fSyncSystem = std::make_shared< CSyncSystem >( fSettings, this );
    connect( fSyncSystem.get(), &CSyncSystem::sigAddToLog, this, &CMainWindow::slotAddToLog );
    connect( fSyncSystem.get(), &CSyncSystem::sigLoadingUsersFinished, this, &CMainWindow::slotLoadingUsersFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMainWindow::slotUserMediaLoaded );
    connect( fSyncSystem.get(), &CSyncSystem::sigMissingMediaLoaded, this, &CMainWindow::slotMissingMediaLoaded );

    fSyncSystem->setUserItemFunc(
        [this]( std::shared_ptr< CUserData > userData )
        {
            if ( !userData )
                return;

            if ( !userData->getItem() )
            {
                auto columns = QStringList() << userData->name() << QString() << QString();
                userData->setItem( new QTreeWidgetItem( columns ) );
                fImpl->users->addTopLevelItem( userData->getItem() );
            }
            userData->getItem()->setText( 1, userData->onLHSServer() ? "Yes" : "No" );
            userData->getItem()->setText( 2, userData->onRHSServer() ? "Yes" : "No" );
        } );

    fSyncSystem->setMediaItemFunc(
        [this]( std::shared_ptr< CMediaData > mediaData )
        {
            if ( !mediaData )
                return;

            mediaData->updateItems( fProviderColumnsByColumn );
        } );
    fSyncSystem->setProcessNewMediaFunc(
        [this]( std::shared_ptr<CMediaData > mediaData )
        {
            updateProviderColumns( mediaData );
        } );

    fSyncSystem->setUserMsgFunc(
        [this]( const QString & title, const QString & msg, bool isCritical )
        {
            if ( isCritical )
                QMessageBox::critical( this, title, msg );
            else
                QMessageBox::information( this, title, msg );
        } );
    SProgressFunctions progressFuncs;
    progressFuncs.fSetupFunc = [this]( const QString & title )
    {
        return setupProgressDlg( title );
    };
    progressFuncs.fSetMaximumFunc = [this]( int count )
    {
        return setProgressMaximum( count );
    };
    progressFuncs.fIncFunc = [this]()
    {
        return incProgressDlg();
    };
    progressFuncs.fResetFunc = [this]()
    {
        return resetProgressDlg();
    };
    progressFuncs.fWasCanceledFunc = [this]()
    {
        if ( fProgressDlg )
            return fProgressDlg->wasCanceled();
        return false;
    };

    fSyncSystem->setProgressFunctions( progressFuncs );

    connect( fImpl->actionLoadProject, &QAction::triggered, this, &CMainWindow::slotLoadProject );
    connect( fImpl->menuLoadRecent, &QMenu::aboutToShow, this, &CMainWindow::slotRecentMenuAboutToShow );
    connect( fImpl->actionReloadServers, &QAction::triggered, this, &CMainWindow::slotReloadServers );
    connect( fImpl->actionReloadCurrentUser, &QAction::triggered, this, &CMainWindow::slotReloadCurrentUser );
    connect( fImpl->actionProcess, &QAction::triggered, this, &CMainWindow::slotProcess );
    connect( fImpl->actionOnlyShowSyncableUsers, &QAction::triggered, this, &CMainWindow::slotToggleOnlyShowSyncableUsers );
    connect( fImpl->actionOnlyShowMediaWithDifferences, &QAction::triggered, this, &CMainWindow::slotToggleOnlyShowMediaWithDifferences );
    connect( fImpl->actionSave, &QAction::triggered, this, &CMainWindow::slotSave );
    connect( fImpl->actionSettings, &QAction::triggered, this, &CMainWindow::slotSettings );

    connect( fImpl->users, &QTreeWidget::currentItemChanged, this, &CMainWindow::slotCurrentItemChanged );

    connect( fImpl->lhsMedia, &QTreeWidget::currentItemChanged,
             [this]( QTreeWidgetItem * current, QTreeWidgetItem * /*prev*/ )
             {
                 if ( !current )
                     return;

                 auto itemText = current->text( 0 );
                 auto items = fImpl->rhsMedia->findItems( itemText, Qt::MatchExactly, 0 );
                 if ( items.empty() )
                     return;
                 fImpl->rhsMedia->setCurrentItem( items.front() );
             }
    );

    connect( fImpl->rhsMedia, &QTreeWidget::currentItemChanged,
             [this]( QTreeWidgetItem * current, QTreeWidgetItem * /*prev*/ )
             {
                 if ( !current )
                     return;
                 auto itemText = current->text( 0 );
                 auto items = fImpl->lhsMedia->findItems( itemText, Qt::MatchExactly, 0 );
                 if ( items.empty() )
                     return;
                 fImpl->lhsMedia->setCurrentItem( items.front() );
             }
    );
    connect( fImpl->lhsMedia->verticalScrollBar(), &QScrollBar::sliderMoved, fImpl->rhsMedia->verticalScrollBar(), &QScrollBar::setValue );
    connect( fImpl->lhsMedia->verticalScrollBar(), &QScrollBar::valueChanged, fImpl->rhsMedia->verticalScrollBar(), &QScrollBar::setValue );

    connect( fImpl->rhsMedia->verticalScrollBar(), &QScrollBar::sliderMoved, fImpl->lhsMedia->verticalScrollBar(), &QScrollBar::setValue );
    connect( fImpl->rhsMedia->verticalScrollBar(), &QScrollBar::valueChanged, fImpl->lhsMedia->verticalScrollBar(), &QScrollBar::setValue );

    connect( fImpl->lhsMedia->horizontalScrollBar(), &QScrollBar::sliderMoved, fImpl->rhsMedia->horizontalScrollBar(), &QScrollBar::setValue );
    connect( fImpl->lhsMedia->horizontalScrollBar(), &QScrollBar::valueChanged, fImpl->rhsMedia->horizontalScrollBar(), &QScrollBar::setValue );

    connect( fImpl->rhsMedia->horizontalScrollBar(), &QScrollBar::sliderMoved, fImpl->lhsMedia->horizontalScrollBar(), &QScrollBar::setValue );
    connect( fImpl->rhsMedia->horizontalScrollBar(), &QScrollBar::valueChanged, fImpl->lhsMedia->horizontalScrollBar(), &QScrollBar::setValue );

    auto recentProjects = fSettings->recentProjectList();
    if ( !recentProjects.isEmpty() )
    {
        auto project = recentProjects[ 0 ];
        QTimer::singleShot( 0, [this, project]()
                            {
                                loadFile( project );
                            } );
    }
}

CMainWindow::~CMainWindow()
{
}

void CMainWindow::closeEvent( QCloseEvent * event )
{
    if ( !fSettings->maybeSave( this ) )
        event->ignore();
    else
        event->accept();
}

void CMainWindow::slotSettings()
{
    CSettingsDlg settings( fSettings, this );
    settings.exec();
    loadSettings();
}

void CMainWindow::reset()
{
    resetServers();

    fSettings->reset();
    fImpl->log->clear();
}

void CMainWindow::resetServers()
{
    fImpl->users->clear();
    fImpl->lhsMedia->clear();
    fImpl->rhsMedia->clear();
    fSyncSystem->reset();
}

void CMainWindow::slotRecentMenuAboutToShow()
{
    fImpl->menuLoadRecent->clear();

    auto recentProjects = fSettings->recentProjectList();
    int num = 0;
    for ( auto && ii : recentProjects )
    {
        if ( !QFileInfo( ii ).exists() )
            continue;
        num++;
        auto action = new QAction( QString( "%1%2 - %3" ).arg( ( num < 10 ) ? "&" : "" ).arg( num ).arg( ii ) );
        action->setData( ii );
        action->setStatusTip( tr( "Open %1" ).arg( ii ) );
        connect( action, &QAction::triggered,
                 [=]()
                 {
                     auto fileName = action->data().toString();
                     loadFile( fileName );
                 } );


        fImpl->menuLoadRecent->addAction( action );
    }

    if ( num == 0 )
    {
        fImpl->menuLoadRecent->addAction( fImpl->actionNoRecentFiles );
    }

}

void CMainWindow::loadFile( const QString & fileName )
{
    if ( fileName == fSettings->fileName() )
        return;

    reset();
    if ( fSettings->load( fileName, this ) )
        loadSettings();
}

void CMainWindow::slotLoadProject()
{
    reset();

    if ( fSettings->load( this ) )
        loadSettings();
}

void CMainWindow::slotSave()
{
    fSettings->save( this );
}

void CMainWindow::loadSettings()
{
    slotAddToLog( "Loading Settings" );
    fImpl->lhsServerLabel->setText( tr( "Server: %1" ).arg( fSettings->lhsURL() ) );
    fImpl->rhsServerLabel->setText( tr( "Server: %1" ).arg( fSettings->rhsURL() ) );
    fImpl->actionOnlyShowSyncableUsers->setChecked( fSettings->onlyShowSyncableUsers() );
    fImpl->actionOnlyShowMediaWithDifferences->setChecked( fSettings->onlyShowMediaWithDifferences() );

    fSyncSystem->loadUsers();
}

void CMainWindow::slotLoadingUsersFinished()
{
    onlyShowSyncableUsers();
}

void CMainWindow::slotUserMediaLoaded()
{
    auto currUser = getCurrUserData();
    auto lhsTree = currUser->onLHSServer() ? fImpl->lhsMedia : nullptr;
    auto rhsTree = currUser->onRHSServer() ? fImpl->rhsMedia : nullptr;

    fSyncSystem->forEachMedia(
        [this, lhsTree, rhsTree]( std::shared_ptr< CMediaData > media )
        {
            media->createItems( lhsTree, rhsTree, fProviderColumnsByColumn );
        } );
    QTimer::singleShot( 0, fSyncSystem.get(), &CSyncSystem::slotFindMissingMedia );
}

void CMainWindow::slotReloadServers()
{
    resetServers();
    fSyncSystem->loadUsers();
}

void CMainWindow::slotReloadCurrentUser()
{
    fSyncSystem->clearCurrUser();
    auto currItem = fImpl->users->currentItem();
    slotCurrentItemChanged( currItem, currItem );
}

void CMainWindow::slotCurrentItemChanged( QTreeWidgetItem * curr, QTreeWidgetItem * prev )
{
    fImpl->actionReloadCurrentUser->setEnabled( curr != nullptr );

    if ( fSyncSystem->isRunning() )
        return;

    auto prevUserData = userDataForItem( prev );
    if ( !curr )
        curr = fImpl->users->currentItem();

    if ( !curr )
        return;

    auto userData = userDataForItem( curr );
    if ( !userData )
        return;

    if ( fSyncSystem->currUser() == userData )
        return;

    if ( prevUserData )
        prevUserData->clearWatchedMedia();
    userData->clearWatchedMedia();
    fSyncSystem->resetMedia();
    fImpl->lhsMedia->clear();
    fImpl->rhsMedia->clear();
    fImpl->lhsMedia->setHeaderLabels( CMediaData::getHeaderLabels() );
    fImpl->rhsMedia->setHeaderLabels( CMediaData::getHeaderLabels() );

    fSyncSystem->loadUsersMedia( userData );
}

void CMainWindow::slotMissingMediaLoaded()
{
    NSABUtils::autoSize( fImpl->lhsMedia );
    NSABUtils::autoSize( fImpl->rhsMedia );
    onlyShowMediaWithDifferences();
}

std::shared_ptr< CUserData > CMainWindow::getCurrUserData() const
{
    auto curr = fImpl->users->currentItem();
    if ( !curr || curr->isHidden() )
        return {};

    return userDataForItem( curr );
}

std::shared_ptr< CUserData > CMainWindow::userDataForItem( QTreeWidgetItem * item ) const
{
    if ( !item )
        return {};

    auto name = item->text( 0 );
    auto userData = fSyncSystem->getUserData( name );
    return userData;
}

void CMainWindow::updateProviderColumns( std::shared_ptr< CMediaData > mediaData )
{
    for ( auto && ii : mediaData->getProviders() )
    {
        auto pos = fProviderColumnsByName.find( ii.first );
        if ( pos == fProviderColumnsByName.end() )
        {
            fProviderColumnsByName.insert( ii.first );
            fProviderColumnsByColumn[ fImpl->lhsMedia->columnCount() ] = ii.first;

            fImpl->lhsMedia->setColumnCount( fImpl->lhsMedia->columnCount() );
            auto headerItem = fImpl->lhsMedia->headerItem();
            headerItem->setText( fImpl->lhsMedia->columnCount(), ii.first );

            fImpl->rhsMedia->setColumnCount( fImpl->rhsMedia->columnCount() );
            headerItem = fImpl->rhsMedia->headerItem();
            headerItem->setText( fImpl->rhsMedia->columnCount(), ii.first );
        }
    }
}

void CMainWindow::slotToggleOnlyShowSyncableUsers()
{
    fSettings->setOnlyShowSyncableUsers( fImpl->actionOnlyShowSyncableUsers->isChecked() );
    onlyShowSyncableUsers();
}

void CMainWindow::onlyShowSyncableUsers()
{
    bool onlyShowSync = fSettings->onlyShowSyncableUsers();

    auto currItem = fImpl->users->currentItem();
    fSyncSystem->forEachUser(
        [this, onlyShowSync]( std::shared_ptr< CUserData > userData )
        {
            if ( !userData )
                return;

            auto item = userData->getItem();
            if ( !item )
                return;

            item->setHidden( onlyShowSync && ( !userData->onLHSServer() || !userData->onRHSServer() ) );
        }
    );

    auto newItem = NSABUtils::nextVisibleItem( currItem );
    fImpl->users->setCurrentItem( newItem );
}

void CMainWindow::slotToggleOnlyShowMediaWithDifferences()
{
    fSettings->setOnlyShowMediaWithDifferences( fImpl->actionOnlyShowMediaWithDifferences->isChecked() );
    onlyShowMediaWithDifferences();
}

void CMainWindow::onlyShowMediaWithDifferences()
{
    bool onlyShowMediaWithDiff = fSettings->onlyShowMediaWithDifferences();

    fSyncSystem->forEachMedia(
        [onlyShowMediaWithDiff]( std::shared_ptr< CMediaData > mediaData )
        {
            if ( !mediaData )
                return;

            auto hide = onlyShowMediaWithDiff && mediaData->serverDataEqual();
            if ( mediaData->getItem( true ) && mediaData->getItem( true )->treeWidget() )
                mediaData->getItem( true )->setHidden( hide );
            if ( mediaData->getItem( false ) && mediaData->getItem( false )->treeWidget() )
                mediaData->getItem( false )->setHidden( hide );
        }
    );
    resetProgressDlg();
}

void CMainWindow::slotProcess()
{
    fSyncSystem->process();
}

void CMainWindow::slotAddToLog( const QString & msg )
{
    qDebug() << msg;
    fImpl->log->appendPlainText( QString( "%1" ).arg( msg.endsWith( '\n' ) ? msg.left( msg.length() - 1 ) : msg ) );
    fImpl->statusbar->showMessage( msg, 500 );
}

void CMainWindow::resetProgressDlg()
{
    if ( fProgressDlg )
        fProgressDlg.data()->deleteLater();
}

void CMainWindow::setupProgressDlg( const QString & title )
{
    if ( !fProgressDlg )
    {
        fProgressDlg = new QProgressDialog( title, tr( "Cancel" ), 0, 0, this );
        fProgressDlg->setAutoClose( true );
        fProgressDlg->setMinimumDuration( 0 );
        fProgressDlg->setValue( 0 );
        fProgressDlg->open();
    }
}

void CMainWindow::setProgressMaximum( int count )
{
    if ( !fProgressDlg )
        return;
    fProgressDlg->setMaximum( count );
}

void CMainWindow::incProgressDlg()
{
    if ( !fProgressDlg )
        return;

    fProgressDlg->setValue( fProgressDlg->value() + 1 );
}

