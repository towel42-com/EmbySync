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
#include "Core/MediaModel.h"
#include "Core/UsersModel.h"
#include "Core/Settings.h"
#include "Core/ProgressSystem.h"
#include "SABUtils/QtUtils.h"

#include "ui_MainWindow.h"

#include <QTimer>
#include <QScrollBar>
#include <QFileInfo>
#include <QProgressDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QDateTime>

CMainWindow::CMainWindow( QWidget * parent )
    : QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    fSettings = std::make_shared< CSettings >();

    fMediaModel = new CMediaModel( fSettings, this );
    fMediaFilterModel = new CMediaFilterModel( this );
    fMediaFilterModel->setSourceModel( fMediaModel );
    connect( fMediaModel, &CMediaModel::sigPendingMediaUpdate, this, &CMainWindow::slotPendingMediaUpdate );

    fUsersModel = new CUsersModel( fSettings, this );
    fUsersFilterModel = new CUsersFilterModel( this );
    fUsersFilterModel->setSourceModel( fUsersModel );

    fImpl->lhsMedia->setModel( fMediaFilterModel );
    fImpl->rhsMedia->setModel( fMediaFilterModel );

    fImpl->users->setModel( fUsersFilterModel );

    fMediaFilterModel->sort( -1, Qt::SortOrder::AscendingOrder );
    fUsersFilterModel->sort( -1, Qt::SortOrder::AscendingOrder );

    hideColumns( fImpl->lhsMedia, EWhichTree::eLHS );
    hideColumns( fImpl->rhsMedia, EWhichTree::eRHS );

    fSyncSystem = std::make_shared< CSyncSystem >( fSettings, this );
    connect( fSyncSystem.get(), &CSyncSystem::sigAddToLog, this, &CMainWindow::slotAddToLog );
    connect( fSyncSystem.get(), &CSyncSystem::sigLoadingUsersFinished, this, &CMainWindow::slotLoadingUsersFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMainWindow::slotUserMediaLoaded );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaCompletelyLoaded, this, &CMainWindow::slotUserMediaCompletelyLoaded );

    fSyncSystem->setLoadUserFunc(
        [ this ]( const QJsonObject & userData, bool isLHSServer )
        {
            return fUsersModel->loadUser( userData, isLHSServer );
        } );
    fSyncSystem->setLoadMediaFunc(
        [ this ]( const QJsonObject & mediaData, bool isLHSServer )
        {
            return fMediaModel->loadMedia( mediaData, isLHSServer );
        } );
    fSyncSystem->setReloadMediaFunc(
        [ this ]( const QJsonObject & mediaData, const QString & mediaID, bool isLHSServer )
        {
            return fMediaModel->reloadMedia( mediaData, mediaID, isLHSServer );
        } );
    fSyncSystem->setGetMediaDataForIDFunc(
        [ this ]( const QString & mediaID, bool isLHSServer )
        {
            return fMediaModel->getMediaDataForID( mediaID, isLHSServer );
        } );
    fSyncSystem->setMergeMediaFunc(
        [ this ]( std::shared_ptr< CProgressSystem > progressSystem )
        {
            return fMediaModel->mergeMedia( progressSystem );
        } );
    fSyncSystem->setGetAllMediaFunc(
        [ this ]()
        {
            return fMediaModel->getAllMedia();
        } );
    fSyncSystem->setUserMsgFunc(
        [ this ]( const QString & title, const QString & msg, bool isCritical )
        {
            if ( isCritical )
                QMessageBox::critical( this, title, msg );
            else
                QMessageBox::information( this, title, msg );
        } );
    auto progressSystem = std::make_shared< CProgressSystem >();
    progressSystem->setSetTitleFunc( [ this ]( const QString & title )
        {
            return progressSetup( title );
        } );
    progressSystem->setTitleFunc( [ this ]()
        {
            if ( fProgressDlg )
                return fProgressDlg->labelText();
            return QString();
        } );
    progressSystem->setMaximumFunc( [ this ]()
        {
            if ( fProgressDlg )
                return fProgressDlg->maximum();
            return 0;
        } );
    progressSystem->setSetMaximumFunc( [ this ]( int count )
        {
            progressSetMaximum( count );
        } );
    progressSystem->setValueFunc( [ this ]()
        {
            return progressValue();
        } );
    progressSystem->setSetValueFunc( [ this ]( int value )
        {
            return progressSetValue( value );
        } );
    progressSystem->setIncFunc( [ this ]()
        {
            return progressIncValue();
        } );
    progressSystem->setResetFunc( [ this ]()
        {
            return progressReset();
        } );
    progressSystem->setWasCanceledFunc( [ this ]()
        {
            if ( fProgressDlg )
                return fProgressDlg->wasCanceled();
            return false;
        } );

    fSyncSystem->setProgressSystem( progressSystem );

    connect( fImpl->actionLoadProject, &QAction::triggered, this, &CMainWindow::slotLoadProject );
    connect( fImpl->menuLoadRecent, &QMenu::aboutToShow, this, &CMainWindow::slotRecentMenuAboutToShow );
    connect( fImpl->actionReloadServers, &QAction::triggered, this, &CMainWindow::slotReloadServers );
    connect( fImpl->actionReloadCurrentUser, &QAction::triggered, this, &CMainWindow::slotReloadCurrentUser );

    connect( fImpl->actionOnlyShowSyncableUsers, &QAction::triggered, this, &CMainWindow::slotToggleOnlyShowSyncableUsers );
    connect( fImpl->actionOnlyShowMediaWithDifferences, &QAction::triggered, this, &CMainWindow::slotToggleOnlyShowMediaWithDifferences );
    connect( fImpl->actionShowMediaWithIssues, &QAction::triggered, this, &CMainWindow::slotToggleShowMediaWithIssues );

    connect( fImpl->actionSave, &QAction::triggered, this, &CMainWindow::slotSave );
    connect( fImpl->actionSettings, &QAction::triggered, this, &CMainWindow::slotSettings );

    connect( fImpl->users, &QTreeView::clicked, this, &CMainWindow::slotCurrentUserChanged );

    connect( fImpl->lhsMedia->selectionModel(), &QItemSelectionModel::currentChanged, this, &CMainWindow::slotSetCurrentMediaItem );
    connect( fImpl->rhsMedia->selectionModel(), &QItemSelectionModel::currentChanged, this, &CMainWindow::slotSetCurrentMediaItem );

    connect( fImpl->lhsMedia->verticalScrollBar(), &QScrollBar::sliderMoved, fImpl->rhsMedia->verticalScrollBar(), &QScrollBar::setValue );
    connect( fImpl->lhsMedia->verticalScrollBar(), &QScrollBar::valueChanged, fImpl->rhsMedia->verticalScrollBar(), &QScrollBar::setValue );

    connect( fImpl->rhsMedia->verticalScrollBar(), &QScrollBar::sliderMoved, fImpl->lhsMedia->verticalScrollBar(), &QScrollBar::setValue );
    connect( fImpl->rhsMedia->verticalScrollBar(), &QScrollBar::valueChanged, fImpl->lhsMedia->verticalScrollBar(), &QScrollBar::setValue );

    connect( fImpl->lhsMedia->horizontalScrollBar(), &QScrollBar::sliderMoved, fImpl->rhsMedia->horizontalScrollBar(), &QScrollBar::setValue );
    connect( fImpl->lhsMedia->horizontalScrollBar(), &QScrollBar::valueChanged, fImpl->rhsMedia->horizontalScrollBar(), &QScrollBar::setValue );

    connect( fImpl->rhsMedia->horizontalScrollBar(), &QScrollBar::sliderMoved, fImpl->lhsMedia->horizontalScrollBar(), &QScrollBar::setValue );
    connect( fImpl->rhsMedia->horizontalScrollBar(), &QScrollBar::valueChanged, fImpl->lhsMedia->horizontalScrollBar(), &QScrollBar::setValue );

    connect( fImpl->applyToLeft, &QToolButton::clicked, this, &CMainWindow::slotApplyToLeft );
    connect( fImpl->applyToRight, &QToolButton::clicked, this, &CMainWindow::slotApplyToRight );
    connect( fImpl->uploadUserMediaData, &QToolButton::clicked, this, &CMainWindow::slotUploadUserMediaData );

    connect( fImpl->actionProcess, &QAction::triggered, fSyncSystem.get(), &CSyncSystem::slotProcess );
    connect( fImpl->actionProcessToLeft, &QAction::triggered, fSyncSystem.get(), &CSyncSystem::slotProcessToLeft );
    connect( fImpl->actionProcessToRight, &QAction::triggered, fSyncSystem.get(), &CSyncSystem::slotProcessToRight );

    fImpl->lhsMedia->horizontalScrollBar()->installEventFilter( this );
    fImpl->rhsMedia->horizontalScrollBar()->installEventFilter( this );

    auto recentProjects = fSettings->recentProjectList();
    if ( !recentProjects.isEmpty() )
    {
        auto project = recentProjects[ 0 ];
        QTimer::singleShot( 0, [ this, project ]()
            {
                loadFile( project );
            } );
    }
    slotSetCurrentMediaItem( QModelIndex(), QModelIndex() );
}

void CMainWindow::hideColumns( QTreeView * treeView, EWhichTree whichTree )
{
    for ( int ii = 0; ii <= fMediaModel->columnCount(); ++ii )
    {
        switch ( whichTree )
        {
        case EWhichTree::eLHS:
            treeView->setColumnHidden( ii, !fMediaModel->isLHSColumn( ii ) );
            break;
        case EWhichTree::eRHS:
            treeView->setColumnHidden( ii, !fMediaModel->isRHSColumn( ii ) );
            break;
        }
    }
}

CMainWindow::~CMainWindow()
{
}

void CMainWindow::slotSetCurrentMediaItem( const QModelIndex & current, const QModelIndex & /*previous*/ )
{
    fImpl->lhsMedia->setCurrentIndex( current );
    fImpl->rhsMedia->setCurrentIndex( current );

    auto mediaInfo = getMediaData( current );

    fImpl->currMediaBox->setEnabled( mediaInfo.get() != nullptr );
    fImpl->currMediaName->setText( mediaInfo ? mediaInfo->name() : QString() );
    fImpl->currMediaType->setText( mediaInfo ? mediaInfo->mediaType() : QString() );
    fImpl->externalUrls->setText( mediaInfo ? mediaInfo->externalUrlsText() : tr( "External Urls:" ) );
    fImpl->externalUrls->setTextFormat( Qt::RichText );
    fImpl->lhsUserMediaData->setMediaUserData( mediaInfo ? mediaInfo->userMediaData( true ) : std::shared_ptr< SMediaUserData >() );
    fImpl->rhsUserMediaData->setMediaUserData( mediaInfo ? mediaInfo->userMediaData( false ) : std::shared_ptr< SMediaUserData >() );
}


void CMainWindow::showEvent( QShowEvent * /*event*/ )
{
}

void CMainWindow::closeEvent( QCloseEvent * event )
{
    if ( !fSettings->maybeSave( this ) )
        event->ignore();
    else
        event->accept();
}

bool CMainWindow::eventFilter( QObject * obj, QEvent * event )
{
    if ( obj == fImpl->lhsMedia->horizontalScrollBar() )
    {
        if ( event->type() == QEvent::Show )
        {
            fImpl->rhsMedia->horizontalScrollBar()->setVisible( true );
        }
        else if ( event->type() == QEvent::Hide )
        {
            fImpl->rhsMedia->horizontalScrollBar()->setHidden( true );
        }
    }
    else if ( obj == fImpl->rhsMedia->horizontalScrollBar() )
    {
        if ( event->type() == QEvent::Show )
        {
            fImpl->lhsMedia->horizontalScrollBar()->setVisible( true );
        }
        else if ( event->type() == QEvent::Hide )
        {
            fImpl->lhsMedia->horizontalScrollBar()->setHidden( true );
        }
    }

    return QMainWindow::eventFilter( obj, event );
}

void CMainWindow::slotSettings()
{
    CSettingsDlg settings( fSettings, fUsersModel->getAllUsers( true ), this );
    settings.exec();
    if ( fSettings->changed() )
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
    fUsersModel->clear();
    fMediaModel->clear();
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
            [ = ]()
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
    if ( fSettings->load( fileName, true, this ) )
        loadSettings();
}

void CMainWindow::slotLoadProject()
{
    reset();

    if ( fSettings->load( true, this ) )
        loadSettings();
}

void CMainWindow::slotSave()
{
    fSettings->save( this );
}

void CMainWindow::loadSettings()
{
    slotAddToLog( EMsgType::eInfo, "Loading Settings" );

    fImpl->lhsServerLabel->setText( tr( "Server: <a href=\"%1\">%1</a>" ).arg( fSettings->lhsURL() ) );
    fImpl->rhsServerLabel->setText( tr( "Server: <a href=\"%1\">%1</a>" ).arg( fSettings->rhsURL() ) );
    fImpl->actionOnlyShowSyncableUsers->setChecked( fSettings->onlyShowSyncableUsers() );
    fImpl->actionOnlyShowMediaWithDifferences->setChecked( fSettings->onlyShowMediaWithDifferences() );
    fImpl->actionShowMediaWithIssues->setChecked( fSettings->showMediaWithIssues() );

    fSyncSystem->loadUsers();
}

void CMainWindow::slotLoadingUsersFinished()
{
    onlyShowSyncableUsers();
    fUsersFilterModel->sort( 0, Qt::SortOrder::AscendingOrder );
}

void CMainWindow::slotUserMediaLoaded()
{
    auto currUser = getCurrUserData();
    if ( !currUser )
        return;

    hideColumns( fImpl->lhsMedia, EWhichTree::eLHS );
    hideColumns( fImpl->rhsMedia, EWhichTree::eRHS );
}

void CMainWindow::slotReloadServers()
{
    resetServers();
    fSyncSystem->loadUsers();
}

void CMainWindow::slotReloadCurrentUser()
{
    fSyncSystem->clearCurrUser();
    auto currIdx = fImpl->users->selectionModel()->currentIndex();
    slotCurrentUserChanged( currIdx );
}

void CMainWindow::slotCurrentUserChanged( const QModelIndex & index )
{
    fImpl->actionReloadCurrentUser->setEnabled( index.isValid() );

    if ( fSyncSystem->isRunning() )
        return;

    auto prevUserData = fSyncSystem->currUser();

    auto idx = index;
    if ( !idx.isValid() )
        idx = fImpl->users->selectionModel()->currentIndex();

    if ( !index.isValid() )
        return;

    auto userData = getUserData( idx );
    if ( !userData )
        return;

    if ( fSyncSystem->currUser() == userData )
        return;

    fSyncSystem->resetMedia();
    fMediaModel->clear();

    fSyncSystem->loadUsersMedia( userData );
}

void CMainWindow::slotUserMediaCompletelyLoaded()
{
    if ( !fMediaLoadedTimer )
    {
        fMediaLoadedTimer = new QTimer( this );
        fMediaLoadedTimer->setSingleShot( true );
        fMediaLoadedTimer->setInterval( 500 );
        connect( fMediaLoadedTimer, &QTimer::timeout,
            [ this ]()
            {
                NSABUtils::autoSize( fImpl->lhsMedia );
                NSABUtils::autoSize( fImpl->rhsMedia );
                onlyShowMediaWithDifferences();

                delete fMediaLoadedTimer;
                fMediaLoadedTimer = nullptr;
            } );
    }
    fMediaLoadedTimer->stop();
    fMediaLoadedTimer->start();
}

std::shared_ptr< CUserData > CMainWindow::getCurrUserData() const
{
    auto idx = fImpl->users->selectionModel()->currentIndex();
    if ( !idx.isValid() )
        return {};

    return getUserData( idx );
}

std::shared_ptr< CUserData > CMainWindow::getUserData( const QModelIndex & idx ) const
{
    auto retVal = fUsersModel->userDataForName( idx.data( CUsersModel::eConnectedIDRole ).toString() );
    if ( !retVal )
        retVal = fUsersModel->userDataForName( idx.data( CUsersModel::eLHSNameRole ).toString() );
    if ( !retVal )
        retVal = fUsersModel->userDataForName( idx.data( CUsersModel::eRHSNameRole ).toString() );

    return retVal;
}

void CMainWindow::slotToggleOnlyShowSyncableUsers()
{
    fSettings->setOnlyShowSyncableUsers( fImpl->actionOnlyShowSyncableUsers->isChecked() );
    onlyShowSyncableUsers();
}

void CMainWindow::onlyShowSyncableUsers()
{
    auto usersSummary = fUsersModel->settingsChanged();
    fImpl->usersLabel->setText( tr( "Users: %1 sync-able out of %2 total users" ).arg( usersSummary.fSyncable ).arg( usersSummary.fTotal ) );
}

void CMainWindow::slotToggleOnlyShowMediaWithDifferences()
{
    fSettings->setOnlyShowMediaWithDifferences( fImpl->actionOnlyShowMediaWithDifferences->isChecked() );
    onlyShowMediaWithDifferences();
}

void CMainWindow::slotToggleShowMediaWithIssues()
{
    fSettings->setShowMediaWithIssues( fImpl->actionShowMediaWithIssues->isChecked() );
    showMediaWithIssues();
}

void CMainWindow::onlyShowMediaWithDifferences()
{
    auto mediaSummary = fMediaModel->settingsChanged();

    progressReset();
    fImpl->mediaSummaryLabel->setText(
        tr( "Media Summary: %1 Items need Syncing, %2 on %3, %4 From %5, %6 can not be compared, %7 Total" )
        .arg( mediaSummary.fNeedsSyncing )
        .arg( mediaSummary.fLHSNeedsUpdating ).arg( fSettings->lhsURL() )
        .arg( mediaSummary.fRHSNeedsUpdating ).arg( fSettings->rhsURL() )
        .arg( mediaSummary.fMissingData )
        .arg( mediaSummary.fTotalMedia )
    );

    auto column = fMediaFilterModel->sortColumn();
    auto order = fMediaFilterModel->sortOrder();
    if ( column == -1 )
    {
        column = 0;
        order = Qt::AscendingOrder;
    }
    fMediaFilterModel->sort( column, order );
}

void CMainWindow::showMediaWithIssues()
{
    auto mediaSummary = fMediaModel->settingsChanged();

    progressReset();
    fImpl->mediaSummaryLabel->setText(
        tr( "Media Summary: %1 Items need Syncing, %2 on %3, %4 From %5, %6 can not be compared, %7 Total" )
        .arg( mediaSummary.fNeedsSyncing )
        .arg( mediaSummary.fLHSNeedsUpdating ).arg( fSettings->lhsURL() )
        .arg( mediaSummary.fRHSNeedsUpdating ).arg( fSettings->rhsURL() )
        .arg( mediaSummary.fMissingData )
        .arg( mediaSummary.fTotalMedia )
    );
}

void CMainWindow::slotAddToLog( int msgType, const QString & msg )
{
    auto fullMsg = createMessage( static_cast<EMsgType>( msgType ), msg );
    qDebug() << fullMsg;
    fImpl->log->appendPlainText( fullMsg );
    fImpl->statusbar->showMessage( fullMsg, 500 );
}

void CMainWindow::progressReset()
{
    if ( fProgressDlg )
        fProgressDlg->deleteLater();
    fProgressDlg = nullptr;
}

void CMainWindow::progressSetup( const QString & title )
{
    if ( !fProgressDlg )
    {
        fProgressDlg = new QProgressDialog( title, tr( "Cancel" ), 0, 0, this );
        connect( fProgressDlg, &QProgressDialog::canceled, fSyncSystem.get(), &CSyncSystem::slotCanceled );
    }
    fProgressDlg->setLabelText( title );
    fProgressDlg->setAutoClose( true );
    fProgressDlg->setMinimumDuration( 0 );
    fProgressDlg->setValue( 0 );
    fProgressDlg->open();
}

void CMainWindow::progressSetMaximum( int count )
{
    if ( !fProgressDlg )
        return;
    fProgressDlg->setMaximum( count );
}

int CMainWindow::progressValue() const
{
    if ( !fProgressDlg )
        return 0;
    return fProgressDlg->value();
}

void CMainWindow::progressSetValue( int value )
{
    if ( !fProgressDlg )
        return;
    fProgressDlg->setValue( value );
    fProgressDlg->open();
}

void CMainWindow::progressIncValue()
{
    if ( !fProgressDlg )
        return;

    fProgressDlg->setValue( progressValue() + 1 );
}

std::shared_ptr< CMediaData > CMainWindow::getMediaData( QModelIndex idx ) const
{
    if ( idx.model() != fMediaModel )
    {
        idx = fMediaFilterModel->mapToSource( idx );
    }

    auto mediaData = fMediaModel->getMediaData( idx );
    return mediaData;
}

void CMainWindow::slotPendingMediaUpdate()
{
    if ( !fPendingMediaUpdateTimer )
    {
        fPendingMediaUpdateTimer = new QTimer( this );
        fPendingMediaUpdateTimer->setSingleShot( true );
        fPendingMediaUpdateTimer->setInterval( 2500 );
        connect( fPendingMediaUpdateTimer, &QTimer::timeout,
            [ this ]()
            {
                onlyShowMediaWithDifferences();
                delete fPendingMediaUpdateTimer;
                fPendingMediaUpdateTimer = nullptr;
            } );
    }
    fPendingMediaUpdateTimer->stop();
    fPendingMediaUpdateTimer->start();
}

void CMainWindow::slotApplyToLeft()
{
    fImpl->lhsUserMediaData->applyMediaUserData( fImpl->rhsUserMediaData->createMediaUserData() );
}

void CMainWindow::slotApplyToRight()
{
    fImpl->rhsUserMediaData->applyMediaUserData( fImpl->lhsUserMediaData->createMediaUserData() );
}

void CMainWindow::slotUploadUserMediaData()
{
    auto currIdx = fImpl->lhsMedia->selectionModel()->currentIndex();
    auto mediaInfo = getMediaData( currIdx );
    if ( !mediaInfo )
        return;

    fSyncSystem->updateUserDataForMedia( mediaInfo, fImpl->lhsUserMediaData->createMediaUserData(), true );
    fSyncSystem->updateUserDataForMedia( mediaInfo, fImpl->rhsUserMediaData->createMediaUserData(), false );
}

