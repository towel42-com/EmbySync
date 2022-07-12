// The MIT License( MIT )
//
// Copyright( c ) 2020-2022 Scott Aron Bloom
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
#include "Settings.h"
#include "ui_MainWindow.h"

#include <unordered_set>

#include <QTimer>
#include <QMessageBox>
#include <QDebug>
#include <QCloseEvent>
#include <QProgressDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAuthenticator>

#include <QScrollBar>
#include <QSettings>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>

#include <QUrlQuery>


int autoSize( QAbstractItemView * view, QHeaderView * header, int minWidth = 150 )
{
    if ( !view || !view->model() || !header )
        return -1;

    auto model = view->model();

    auto numCols = model->columnCount();
    bool stretchLastColumn = header->stretchLastSection();
    header->setStretchLastSection( false );

    header->resizeSections( QHeaderView::ResizeToContents );
    int numVisibleColumns = 0;
    for ( int ii = 0; ii < numCols; ++ii )
    {
        if ( header->isSectionHidden( ii ) )
            continue;
        numVisibleColumns++;
        if ( numVisibleColumns > 1 )
            break;
    }
    bool dontResize = ( numVisibleColumns <= 1 ) && stretchLastColumn;

    int totalWidth = 0;
    for ( int ii = 0; ii < numCols; ++ii )
    {
        if ( header->isSectionHidden( ii ) )
            continue;

        int contentSz = view->sizeHintForColumn( ii );
        int headerSz = header->sectionSizeHint( ii );

        auto newWidth = std::max( { minWidth, contentSz, headerSz } );
        totalWidth += newWidth;

        if ( !dontResize )
            header->resizeSection( ii, newWidth );
    }
    if ( stretchLastColumn )
        header->setStretchLastSection( true );
    return totalWidth;
}

int autoSize( QTreeView * tree )
{
    return autoSize( tree, tree->header() );
}

std::unordered_set< QString > CMainWindow::fProviderColumnsByName;
std::map< int, QString > CMainWindow::fProviderColumnsByColumn;
CMainWindow::CMainWindow( QWidget * parent )
    : QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    fImpl->users->sortByColumn( 0, Qt::SortOrder::AscendingOrder );
    fImpl->lhsMedia->sortByColumn( 0, Qt::SortOrder::AscendingOrder );
    fImpl->rhsMedia->sortByColumn( 0, Qt::SortOrder::AscendingOrder );

    fSettings = std::make_shared< CSettings >();
    connect( fImpl->actionLoadProject, &QAction::triggered, this, &CMainWindow::slotLoadProject );
    connect( fImpl->menuLoadRecent, &QMenu::aboutToShow, this, &CMainWindow::slotRecentMenuAboutToShow );
    connect( fImpl->actionReloadUser, &QAction::triggered, this, &CMainWindow::slotReloadUser );
    connect( fImpl->actionProcess, &QAction::triggered, this, &CMainWindow::slotProcess );
    connect( fImpl->actionOnlyShowSyncableUsers, &QAction::triggered, this, &CMainWindow::slotToggleOnlyShowSyncableUsers );
    connect( fImpl->actionOnlyShowMediaWithDifferences, &QAction::triggered, this, &CMainWindow::slotToggleOnlyShowMediaWithDifferences );
    connect( fImpl->actionSave, &QAction::triggered, this, &CMainWindow::slotSave );
    connect( fImpl->actionSettings, &QAction::triggered, this, &CMainWindow::slotSettings );

    connect( fImpl->users, &QTreeWidget::currentItemChanged, this, &CMainWindow::slotCurrentItemChanged );

    fManager = new QNetworkAccessManager( this );
    fManager->setAutoDeleteReplies( true );

    connect( fManager, &QNetworkAccessManager::authenticationRequired, this, &CMainWindow::slotAuthenticationRequired );
    connect( fManager, &QNetworkAccessManager::encrypted, this, &CMainWindow::slotEncrypted );
    connect( fManager, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, &CMainWindow::slotPreSharedKeyAuthenticationRequired );
    connect( fManager, &QNetworkAccessManager::proxyAuthenticationRequired, this, &CMainWindow::slotProxyAuthenticationRequired );
    connect( fManager, &QNetworkAccessManager::sslErrors, this, &CMainWindow::slotSSlErrors );
    connect( fManager, &QNetworkAccessManager::finished, this, &CMainWindow::slotRequestFinished );

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
        QTimer::singleShot( 0, [this,project]()
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
    fUsers.clear();
    fAllMedia.clear();
    fMissingMedia.clear();
    fLHSMedia.clear();
    fRHSMedia.clear();
    fLHSProviderSearchMap.clear();
    fRHSProviderSearchMap.clear();
    fAttributes.clear();
    fImpl->users->clear();
    fImpl->lhsMedia->clear();
    fImpl->rhsMedia->clear();
    fSettings->reset();
    fImpl->log->clear();
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

void CMainWindow::setupProgressDlg( const QString & title, bool hasLHSServer, bool hasRHSServer )
{
    if ( !std::get< 0 >( fProgressDlg ) )
    {
        std::get< 0 >( fProgressDlg ) = new QProgressDialog( title, tr( "Cancel" ), 0, 0, this );
        std::get< 0 >( fProgressDlg )->setAutoClose( true );
        std::get< 0 >( fProgressDlg )->setMinimumDuration( 0 );
        std::get< 0 >( fProgressDlg )->setValue( 0 );
        std::get< 0 >( fProgressDlg )->show();
    }
    if ( hasLHSServer )
        std::get< 1 >( fProgressDlg ) = false;
    else
        std::get< 1 >( fProgressDlg ).reset();

    if ( hasRHSServer )
        std::get< 2 >( fProgressDlg ) = false;
    else
        std::get< 2 >( fProgressDlg ).reset();
}

void CMainWindow::loadSettings()
{
    addToLog( "Loading Settings" );
    fImpl->lhsServerLabel->setText( tr( "LHS Server: %1" ).arg( fSettings->lhsURL() ) );
    fImpl->rhsServerLabel->setText( tr( "RHS Server: %1" ).arg( fSettings->rhsURL() ) );
    fImpl->actionOnlyShowSyncableUsers->setChecked( fSettings->onlyShowSyncableUsers() );
    fImpl->actionOnlyShowMediaWithDifferences->setChecked( fSettings->onlyShowMediaWithDifferences() );

    setupProgressDlg( tr( "Loading Users" ), true, true );

    loadUsers( true );
    loadUsers( false );
}

void CMainWindow::updateProgressDlg( int count )
{
    if ( !progressDlg() )
        return;
    progressDlg()->setMaximum( progressDlg()->maximum() + count );
}

void CMainWindow::loadUsers( bool isLHSServer )
{
    addToLog( tr( "Loading users from %1 Server" ).arg( isLHSServer ? "LHS" : "RHS" ) );;
    auto && url = fSettings->getServerURL( isLHSServer );
    if ( !url.isValid() )
        return;

    addToLog( tr( "Server URL: %1" ).arg( url.toString() ) );

    auto path = url.path();
    path += "Users";
    url.setPath( path );
    //qDebug() << url;

    auto request = QNetworkRequest( url );

    auto reply = fManager->get( request );
    setIsLHS( reply, isLHSServer );
    setRequestType( reply, ERequestType::eUsers );
}

void CMainWindow::incProgressDlg()
{
    if ( !progressDlg() )
        return;

    progressDlg()->setValue( progressDlg()->value() + 1 );
}

void CMainWindow::setIsLHS( QNetworkReply * reply, bool isLHSServer )
{
    if ( !reply )
        return;
    fAttributes[ reply ][ kLHSServer ] = isLHSServer;
}

bool CMainWindow::isLHSServer( QNetworkReply * reply )
{
    if ( !reply )
        return false;
    return fAttributes[ reply ][ kLHSServer ].toBool();
}

void CMainWindow::setRequestType( QNetworkReply * reply, ERequestType requestType )
{
    if ( !reply )
        return;
    fAttributes[ reply ][ kRequestType ] = static_cast<int>( requestType );
}

ERequestType CMainWindow::requestType( QNetworkReply * reply )
{
    return static_cast<ERequestType>( fAttributes[ reply ][ kRequestType ].toInt() );
}

void CMainWindow::setExtraData( QNetworkReply * reply, QVariant extraData )
{
    if ( !reply )
        return;
    fAttributes[ reply ][ kExtraData ] = extraData;
}

QVariant CMainWindow::extraData( QNetworkReply * reply )
{
    if ( !reply )
        return false;
    return fAttributes[ reply ][ kExtraData ];
}

bool CMainWindow::handleError( QNetworkReply * reply )
{
    Q_ASSERT( reply );
    if ( !reply )
        return false;

    if ( reply && ( reply->error() != QNetworkReply::NoError ) ) // replys with an error do not get cached
    {
        auto data = reply->readAll();
        QMessageBox::critical( this, tr( "Error response from server" ), tr( "Error from Server: %1%2" ).arg( reply->errorString() ).arg( data.isEmpty() ? QString() : QString( " - %1" ).arg( QString( data ) ) ) );
        return false;
    }
    return true;
}

std::shared_ptr< SMediaData > CMainWindow::findMediaForProvider( const QString & provider, const QString & id, bool lhs )
{
    auto && map = lhs ? fLHSProviderSearchMap : fRHSProviderSearchMap;
    auto pos = map.find( provider );
    if ( pos == map.end() )
        return {};

    auto pos2 = ( *pos ).second.find( id );
    if ( pos2 == ( *pos ).second.end() )
        return {};
    return ( *pos2 ).second;
}

void CMainWindow::setMediaForProvider( const QString & providerName, const QString & providerID, std::shared_ptr< SMediaData > mediaData, bool isLHS )
{
    ( isLHS ? fLHSProviderSearchMap : fRHSProviderSearchMap )[ providerName ][ providerID ] = mediaData;
}

void CMainWindow::slotRequestFinished( QNetworkReply * reply )
{
    auto isLHSServer = this->isLHSServer( reply );
    auto requestType = this->requestType( reply );
    auto extraData = this->extraData( reply );

    //addToLog( QString( "Request Completed: %1" ).arg( reply->url().toString() ) );
    //addToLog( QString( "Is LHS? %1" ).arg( isLHSServer ? "Yes" : "No" ) );
    //addToLog( QString( "Request Type: %1" ).arg( toString( requestType ) ) );
    //addToLog( QString( "Extra Data: %1" ).arg( extraData.toString() ) );

    if ( !handleError( reply ) )
        return;

    auto pos = fAttributes.find( reply );
    if ( pos != fAttributes.end() )
        fAttributes.erase( pos );

    //qDebug() << "Requests Remaining" << fAttributes.size();
    auto data = reply->readAll();
    //qDebug() << data;

    switch ( requestType )
    {
        case ERequestType::eNone:
            break;
        case ERequestType::eUsers:
            if ( !progressDlg() || ( progressDlg() && !progressDlg()->wasCanceled() ) )
            {
                loadUsers( data, isLHSServer );
                setProgressDlgFinished( isLHSServer );

                if ( isProgressDlgFinished() )
                {
                    resetProgressDlg();
                    onlyShowSyncableUsers();
                }
            }
            break;
        case ERequestType::eMissingMedia:
            if ( !progressDlg() || ( progressDlg() && !progressDlg()->wasCanceled() ) )
            {
                loadMissingMediaItem( data, extraData.toString(), isLHSServer );
            }
            break;
        case ERequestType::eMediaList:
            if ( !progressDlg() || ( progressDlg() && !progressDlg()->wasCanceled() ) )
            {
                loadMediaList( data, isLHSServer, extraData.toString() );
                setProgressDlgFinished( isLHSServer );

                if ( isProgressDlgFinished() )
                {
                    resetProgressDlg();
                    loadMediaData();
                }
            }
            break;
        case ERequestType::eMediaData:
            if ( !progressDlg() || ( progressDlg() && !progressDlg()->wasCanceled() ) )
            {
                loadMediaData( data, isLHSServer, extraData.toString() );
                incProgressDlg();
                if ( fAttributes.empty() && !fShowMediaPending )
                {
                    fShowMediaPending = true;
                    QTimer::singleShot( 1000, this, &CMainWindow::slotShowMedia );
                }
            }
            break;
        case ERequestType::eUpdateData:
            {
            }
            break;
    }
}

void CMainWindow::resetProgressDlg()
{
    delete std::get< 0 >( fProgressDlg );
    fProgressDlg = { nullptr, {}, {} };
}

bool CMainWindow::isProgressDlgFinished()
{
    if ( !progressDlg() )
        return true;

    if ( progressDlg()->wasCanceled() )
    {
        resetProgressDlg();
        return true;
    }
    bool retVal = true;
    if ( std::get< 1 >( fProgressDlg ).has_value() )
        retVal = retVal && std::get< 1 >( fProgressDlg ).value();
    if ( std::get< 2 >( fProgressDlg ).has_value() )
        retVal = retVal && std::get< 2 >( fProgressDlg ).value();

    return retVal;
}

void CMainWindow::slotAuthenticationRequired( QNetworkReply * /*reply*/, QAuthenticator * /*authenticator*/ )
{
    //qDebug() << "slotAuthenticationRequired:" << reply << reply->url().toString() << authenticator;
}

void CMainWindow::slotEncrypted( QNetworkReply * /*reply*/ )
{
    //qDebug() << "slotEncrypted:" << reply << reply->url().toString();
}

void CMainWindow::slotPreSharedKeyAuthenticationRequired( QNetworkReply * /*reply*/, QSslPreSharedKeyAuthenticator * /*authenticator*/ )
{
    //qDebug() << "slotPreSharedKeyAuthenticationRequired: 0x" << Qt::hex << reply << reply->url().toString() << authenticator;
}

void CMainWindow::slotProxyAuthenticationRequired( const QNetworkProxy & /*proxy*/, QAuthenticator * /*authenticator*/ )
{
    //qDebug() << "slotProxyAuthenticationRequired: 0x" << Qt::hex << &proxy << authenticator;
}

void CMainWindow::slotSSlErrors( QNetworkReply * /*reply*/, const QList<QSslError> & /*errors*/ )
{
    //qDebug() << "slotSSlErrors: 0x" << Qt::hex << reply << errors;
}

void CMainWindow::loadUsers( const QByteArray & data, bool isLHSServer )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        QMessageBox::critical( this, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    //qDebug() << doc.toJson();
    auto users = doc.array();
    updateProgressDlg( users.count() );

    addToLog( QString( "%1 Server has %2 Users" ).arg( isLHSServer ? "LHS" : "RHS" ).arg( users.count() ) );

    for ( auto && ii : users )
    {
        if ( progressDlg()->wasCanceled() )
            break;
        incProgressDlg();


        auto user = ii.toObject();
        auto name = user[ "Name" ].toString();
        auto id = user[ "Id" ].toString();
        if ( name.isEmpty() || id.isEmpty() )
            continue;

        auto userData = getUserData( name );
        if ( !userData )
        {
            userData = std::make_shared< SUserData >();
            userData->fName = name;
            userData->fItem = new QTreeWidgetItem( getColumns( name, isLHSServer ) );

            fImpl->users->addTopLevelItem( userData->fItem );
            fUsers[ userData->fName ] = userData;
        }

        if ( isLHSServer )
            userData->fUserID.first = id;
        else
            userData->fUserID.second = id;
        userData->fItem->setText( isLHSServer ? 1 : 2, "Yes" );
    }
}

void CMainWindow::slotReloadUser()
{
    slotCurrentItemChanged( nullptr, nullptr );
}

void CMainWindow::slotCurrentItemChanged( QTreeWidgetItem * prev, QTreeWidgetItem * curr )
{
    if ( !isProgressDlgFinished() )
        return;

    if ( curr && prev == curr )
        return;

    std::shared_ptr< SUserData > userData;
    bool hasLHSServer = false;
    bool hasRHSServer = false;
    std::tie( userData, hasLHSServer, hasRHSServer ) = getCurrUserData();
    if ( !userData )
        return;

    addToLog( QString( "Loading watched media for '%1'" ).arg( userData->fName ) );

    fLHSMedia.clear();
    fRHSMedia.clear();
    fLHSProviderSearchMap.clear();
    fRHSProviderSearchMap.clear();
    userData->fWatchedMedia.clear();
    fImpl->lhsMedia->clear();
    fImpl->rhsMedia->clear();

    if ( !hasLHSServer && !hasRHSServer )
        return;

    setupProgressDlg( tr( "Loading Users Played Media" ), hasLHSServer, hasRHSServer );
    if ( hasLHSServer )
        loadUsersPlayedMedia( userData, true );
    if ( hasRHSServer )
        loadUsersPlayedMedia( userData, false );
}

void CMainWindow::loadUsersPlayedMedia( std::shared_ptr< SUserData > userData, bool isLHSServer )
{
    if ( !userData )
        return;

    auto && url = fSettings->getServerURL( isLHSServer );
    auto path = url.path();
    path += QString( "Users/%1/Items" ).arg( userData->getUserID( isLHSServer ) );
    url.setPath( path );

    QUrlQuery query;

    query.addQueryItem( "api_key", isLHSServer ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    query.addQueryItem( "Filters", "IsPlayed" );
    query.addQueryItem( "IncludeItemTypes", "Movie,Episode,Video" );
    query.addQueryItem( "SortBy", "SortName" );
    query.addQueryItem( "SortOrder", "Ascending" );
    query.addQueryItem( "Recursive", "True" );
    url.setQuery( query );

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    addToLog( QString( "Requesting Played media for '%1' from %2 server" ).arg( userData->fName ).arg( isLHSServer ? "LHS" : "RHS" ) );

    auto reply = fManager->get( request );
    setIsLHS( reply, isLHSServer );
    setRequestType( reply, ERequestType::eMediaList );
    setExtraData( reply, userData->fName );
}

//void CMainWindow::loadAllMedia()
//{
//    setupProgressDlg( tr( "Loading All Media" ), true, true );
//    loadAllMedia( true );
//    loadAllMedia( false );
//}

//void CMainWindow::loadAllMedia( bool isLHSServer )
//{
//    auto && url = fSettings->getServerURL( isLHSServer );
//    auto path = url.path();
//    path += QString( "Items" );
//    url.setPath( path );
//
//    QUrlQuery query;
//
//    query.addQueryItem( "api_key", isLHSServer ? fSettings->lhsAPI() : fSettings->rhsAPI() );
//    query.addQueryItem( "IncludeItemTypes", "Movie,Episode,Video" );
//    query.addQueryItem( "SortBy", "SortName" );
//    query.addQueryItem( "SortOrder", "Ascending" );
//    query.addQueryItem( "Recursive", "True" );
//    url.setQuery( query );
//
//    //qDebug() << url;
//    auto request = QNetworkRequest( url );
//
//    auto reply = fManager->get( request );
//    setIsLHSServer( reply, isLHSServer );
//}
//"{
//    "BackdropImageTags": [
//        "73047b0b31266acc5b8a36d3825d8800"
//    ],
//    "Id": "242837",
//    "ImageTags": {
//        "Logo": "dcc65df5e676c6bba96722bd59749033",
//        "Primary": "8dc166791fe3c68a853fd2d273a8cd7f"
//    },
//    "IsFolder": false,
//    "MediaType": "Video",
//    "Name": "Spider-Man: No Way Home",
//    "RunTimeTicks": 88899200000,
//    "ServerId": "be875b9574774ca58c487f1a290dff83",
//    "Type": "Movie",
//    "UserData": {
//        "IsFavorite": false,
//        "LastPlayedDate": "2022-06-22T15:57:25.0000000Z",
//        "PlayCount": 3,
//        "PlaybackPositionTicks": 61021779859,
//        "Played": true,
//        "PlayedPercentage": 68.64153992274396
//    }
//}


//{
//    "BackdropImageTags": [
//        "1724f1a1a100d2f79412808dc71d0e41"
//    ],
//    "Id": "209874",
//    "ImageTags": {
//        "Logo": "3bb4952cdd568f05efb52060136053bf",
//        "Primary": "155d235b93acd2892442782674436381"
//    },
//    "IsFolder": false,
//    "MediaType": "Video",
//    "Name": "The Bourne Identity",
//    "RunTimeTicks": 71170690000,
//    "ServerId": "be875b9574774ca58c487f1a290dff83",
//    "Type": "Movie",
//    "UserData": {
//        "IsFavorite": false,
//        "LastPlayedDate": "2022-01-15T20:28:39.0000000Z",
//        "PlayCount": 1,
//        "PlaybackPositionTicks": 0,
//        "Played": true
//    }
//}

void CMainWindow::loadMediaList( const QByteArray & data, bool isLHSServer, const QString & name )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        QMessageBox::critical( this, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    auto userData = getUserData( name );

    //qDebug() << doc.toJson();
    auto mediaList = doc[ "Items" ].toArray();
    updateProgressDlg( mediaList.count() );

    addToLog( QString( "%1 has %2 watched media items on %3 server" ).arg( name ).arg( mediaList.count() ).arg( isLHSServer ? "LHS" : "RHS" ) );

    int curr = 0;
    for ( auto && ii : mediaList )
    {
        //if ( curr >= 10 )
        //    break;
        curr++;

        if ( progressDlg() && progressDlg()->wasCanceled() )
            break;
        incProgressDlg();
            
        auto media = ii.toObject();

        auto id = media[ "Id" ].toString();
        std::shared_ptr< SMediaData > mediaData;
        auto pos = ( isLHSServer ? fLHSMedia.find( id ) : fRHSMedia.find( id ) );
        if ( pos == ( isLHSServer ? fLHSMedia.end() : fRHSMedia.end() ) )
        {
            mediaData = std::make_shared< SMediaData >();
            mediaData->fType = media[ "Type" ].toString();
            mediaData->fName = SMediaData::computeName( media );
        }
        else
            mediaData = ( *pos ).second;
        //qDebug() << isLHSServer << mediaData->fName;

        if ( isLHSServer && !mediaData->fLHSServer )
            mediaData->fLHSServer = std::make_shared< SMediaDataBase >();
        else if ( !isLHSServer && !mediaData->fRHSServer )
            mediaData->fRHSServer = std::make_shared< SMediaDataBase >();


        if ( isLHSServer )
            mediaData->fLHSServer->fMediaID = id;
        else
            mediaData->fRHSServer->fMediaID = id;
        
        /*
        "UserData": {
        "IsFavorite": false,
        "LastPlayedDate": "2022-01-15T20:28:39.0000000Z",
        "PlayCount": 1,
        "PlaybackPositionTicks": 0,
        "Played": true
        }
        */

        mediaData->loadUserDataFromJSON( media, isLHSServer );
        if ( isLHSServer )
        {
            if ( userData )
            {
                fLHSMedia[ mediaData->getMediaID( isLHSServer ) ] = mediaData;
                userData->fWatchedMediaServer.first.push_back( mediaData );
            }
        }
        else
        {
            if ( userData )
            {
                fRHSMedia[ mediaData->getMediaID( isLHSServer ) ] = mediaData;
                userData->fWatchedMediaServer.second.push_back( mediaData );
            }
        }
    }
    if ( userData )
    {
        userData->fWatchedMedia.insert( userData->fWatchedMedia.end(), isLHSServer ? userData->fWatchedMediaServer.first.begin() : userData->fWatchedMediaServer.second.begin(), isLHSServer ? userData->fWatchedMediaServer.first.end() : userData->fWatchedMediaServer.second.end() );
        isLHSServer ? userData->fWatchedMediaServer.first.clear() : userData->fWatchedMediaServer.second.clear();
    }
}

QString SMediaData::computeName( QJsonObject & mediaObj )
{
    QString retVal = mediaObj[ "Name" ].toString();

    if ( mediaObj[ "Type" ] == "Episode" )
    {
        //auto tmp = QJsonDocument( media );
        //qDebug() << tmp.toJson();

        auto series = mediaObj[ "SeriesName" ].toString();
        auto season = mediaObj[ "SeasonName" ].toString();
        auto episodeNum = mediaObj[ "IndexNumber" ].toInt();
        auto episodeName = mediaObj[ "EpisodeTitle" ].toString();

        retVal = series + " - " + season + " Episode " + QString::number( episodeNum ) + " " + episodeName + " - " + retVal;
    }
    return retVal;
}

void CMainWindow::loadMissingMediaItem( const QByteArray & data, const QString & mediaName, bool isLHSServer )
{
    incProgressDlg();
    //qDebug() << progressDlg()->value() << fMissingMedia.size();

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        QMessageBox::critical( this, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    auto userData = std::get< 0 >( getCurrUserData() );

    //qDebug() << doc.toJson();
    auto mediaList = doc[ "Items" ].toArray();
    updateProgressDlg( mediaList.count() );

    addToLog( QString( "%1 has %2 watched media items on %3 server that match missing data for '%4'- %5 remaining" ).arg( userData->fName ).arg( mediaList.count() ).arg( isLHSServer ? "LHS" : "RHS" ).arg( mediaName ).arg( fMissingMedia.size() ) );

    for ( auto && ii : mediaList )
    {
        auto media = ii.toObject();

        //auto tmp = QJsonDocument( media );
        //qDebug() << tmp.toJson();

        auto id = media[ "Id" ].toString();
        auto name = SMediaData::computeName( media );

        std::shared_ptr< SMediaData > mediaData;
        auto pos = fMissingMedia.find( name );
        //Q_ASSERT( pos != fMissingMedia.end() );
        if ( pos != fMissingMedia.end() )
            mediaData = ( *pos ).second;
        else
        {
            addToLog( QString( "Error:  COULD NOT FIND MEDIA '%1' on %2 server" ).arg( mediaName ).arg( isLHSServer ? "LHS" : "RHS" ) );
            continue;
        }
        fMissingMedia.erase( pos );

        //qDebug() << isLHSServer << mediaData->fName;

        Q_ASSERT( ( !isLHSServer && mediaData->fRHSServer ) || ( isLHSServer && mediaData->fLHSServer ) );

        if ( isLHSServer )
            mediaData->fLHSServer->fMediaID = id;
        else
            mediaData->fRHSServer->fMediaID = id;

        /*
        "UserData": {
        "IsFavorite": false,
        "LastPlayedDate": "2022-01-15T20:28:39.0000000Z",
        "PlayCount": 1,
        "PlaybackPositionTicks": 0,
        "Played": true
        }
        */

        mediaData->loadUserDataFromJSON( media, isLHSServer );
        mediaData->updateItems( true, isLHSServer );
        break;
    }
    if ( fMissingMedia.empty() )
        QTimer::singleShot( 0, this, &CMainWindow::slotMediaLoaded );
}

void CMainWindow::slotMediaLoaded()
{
    autoSize( fImpl->lhsMedia );
    autoSize( fImpl->rhsMedia );
    onlyShowMediaWithDifferences();
    resetProgressDlg();
}

void SMediaData::loadUserDataFromJSON( const QJsonObject & media, bool isLHSServer  )
{
    auto userDataObj = media[ "UserData" ].toObject();

    //auto tmp = QJsonDocument( userDataObj );
    //qDebug() << tmp.toJson();

    auto userDataVariant = media[ "UserData" ].toVariant().toMap();
    if ( isLHSServer )
    {
        fLHSServer->loadUserDataFromJSON( userDataObj, userDataVariant );
    }
    else
    {
        fRHSServer->loadUserDataFromJSON( userDataObj, userDataVariant );
    }
}

std::shared_ptr< SUserData > CMainWindow::getUserData( const QString & name ) const
{
    auto pos = fUsers.find( name );
    if ( pos == fUsers.end() )
        return {};

    auto userData = ( *pos ).second;
    if ( !userData )
        return {};
    return userData;
}

void CMainWindow::loadMediaData()
{
    std::shared_ptr< SUserData > userData;
    bool hasLHSServer = false;
    bool hasRHSServer = false;
    std::tie( userData, hasLHSServer, hasRHSServer ) = getCurrUserData();
    if ( !userData )
        return;

    if ( userData->fWatchedMedia.empty() )
        return;

    setupProgressDlg( tr( "Loading Media Provider Info" ), hasLHSServer, hasRHSServer );
    updateProgressDlg( static_cast<int>( userData->fWatchedMedia.size() ) );
    loadMediaData( userData );
    resetProgressDlg();
    setupProgressDlg( tr( "Loading Media Provider Info" ), hasLHSServer, hasRHSServer );
    updateProgressDlg( static_cast<int>( userData->fWatchedMedia.size() ) );
}

void CMainWindow::loadMediaData( std::shared_ptr< SUserData > userData )
{
    if ( !userData )
        return;
    for ( auto && ii : userData->fWatchedMedia )
    {
        incProgressDlg();

        loadMediaData( userData, ii, true );
        loadMediaData( userData, ii, false );
    }
}

std::tuple< std::shared_ptr< SUserData >, bool, bool > CMainWindow::getCurrUserData() const
{
    auto curr = fImpl->users->currentItem();
    if ( !curr )
        return { {}, false, false };

    auto name = curr->text( 0 );
    auto userData = getUserData( name );

    auto hasLHSServer = !curr->text( 1 ).isEmpty();
    auto hasRHSServer = !curr->text( 2 ).isEmpty();

    return { userData, hasLHSServer, hasRHSServer };
}

QStringList CMainWindow::getColumns( const QString & name, bool isLHSServer ) const
{
    return QStringList() << name << ( isLHSServer ? "Yes" : QString() ) << ( !isLHSServer ? "Yes" : QString() );
}

void CMainWindow::loadMediaData( std::shared_ptr< SUserData > userData, std::shared_ptr< SMediaData > mediaData, bool isLHSServer )
{
    if ( mediaData->beenLoaded( isLHSServer ) )
        return;

    if ( !mediaData->mediaWatchedOnServer( isLHSServer ) )
        return;

    auto && url = fSettings->getServerURL( isLHSServer );
    auto path = url.path();
    path += QString( "Users/%1/Items/%2" ).arg( userData->getUserID( isLHSServer ) ).arg( mediaData->getMediaID( isLHSServer ) );
    url.setPath( path );

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    auto reply = fManager->get( request );

    setIsLHS( reply, isLHSServer );
    setRequestType( reply, ERequestType::eMediaData );
    setExtraData( reply, mediaData->getMediaID( isLHSServer ) );
}

void CMainWindow::loadMediaData( const QByteArray & data, bool isLHSServer, const QString & itemID )
{
    auto pos = ( isLHSServer ? fLHSMedia.find( itemID ) : fRHSMedia.find( itemID ) );
    if ( pos == ( isLHSServer ? fLHSMedia.end() : fRHSMedia.end() ) )
        return;

    auto mediaData = ( *pos ).second;
    if ( !mediaData )
        return;

    auto treeItem = mediaData->getItem( isLHSServer );

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        QMessageBox::critical( this, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    //qDebug() << doc.toJson();
    auto mediaObject = doc.object();
    auto providerIDsObj = mediaObject[ "ProviderIds" ].toObject();
    QStringList providers;
    for ( auto && ii = providerIDsObj.begin(); ii != providerIDsObj.end(); ++ii )
    {
        auto providerName = ii.key();
        auto providerID = ii.value().toString();
        mediaData->fProviders[ providerName ] = providerID;
        providers << providerName + "=" + providerID;
        if ( isLHSServer )
        {
            fLHSProviderSearchMap[ providerName ][ providerID ] = mediaData;
        }
        else
        {
            fRHSProviderSearchMap[ providerName ][ providerID ] = mediaData;
        }
    }
    updateColumns( mediaData );
}

void CMainWindow::slotShowMedia()
{
    if ( !fAttributes.empty() )
    {
        QTimer::singleShot( 500, this, &CMainWindow::slotShowMedia );
        return;
    }

    resetProgressDlg();
    fShowMediaPending = false;

    mergeMediaData( fLHSMedia, fRHSMedia, true );
    mergeMediaData( fRHSMedia, fLHSMedia, false );

    for ( auto && ii : fLHSMedia )
        fAllMedia.insert( ii.second );

    for ( auto && ii : fRHSMedia )
        fAllMedia.insert( ii.second );

    fLHSProviderSearchMap.clear();
    fRHSProviderSearchMap.clear();

    for ( auto && ii : fAllMedia )
    {
        ii->createItems( fImpl->lhsMedia, fImpl->rhsMedia );
    }
    QTimer::singleShot( 0, this, &CMainWindow::slotFindMissingMedia );
}


void CMainWindow::slotFindMissingMedia()
{
    for ( auto && ii : fAllMedia )
    {
        if ( !ii->hasMissingInfo() || !ii->hasProviderIDs() )
            continue;

        fMissingMedia[ ii->fName ] = ii;
    }
    if ( fMissingMedia.empty() )
    {
        QTimer::singleShot( 0, this, &CMainWindow::slotMediaLoaded );
        return;
    }

    setupProgressDlg( "Finding Unplayed Info", true, true );
    updateProgressDlg( static_cast< int >( fMissingMedia.size() ) );

    for( auto && ii : fMissingMedia )
    {
        getMissingMedia( ii.second );
    }
}

void CMainWindow::updateColumns( std::shared_ptr< SMediaData > mediaData )
{
    for ( auto && ii : mediaData->fProviders )
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

void CMainWindow::mergeMediaData(
    TMediaIDToMediaData & lhs,
    TMediaIDToMediaData & rhs,
    bool lhsIsLHS
)
{
    mergeMediaData( lhs, lhsIsLHS );
    mergeMediaData( rhs, !lhsIsLHS );
}

void CMainWindow::mergeMediaData(
    TMediaIDToMediaData & lhs,
    bool lhsIsLHS
)
{
    std::unordered_map< std::shared_ptr< SMediaData >, std::shared_ptr< SMediaData > > replacementMap;
    for ( auto && ii : lhs )
    {
        auto mediaData = ii.second;
        if ( !mediaData )
            continue;
        QStringList dupeProviderForMedia;
        for ( auto && jj : mediaData->fProviders )
        {
            auto providerName = jj.first;
            auto providerID = jj.second;

            auto myMappedMedia = findMediaForProvider( providerName, providerID, lhsIsLHS );
            if ( myMappedMedia != mediaData )
            {
                replacementMap[ mediaData ] = myMappedMedia;
                continue;
            }

            auto otherData = findMediaForProvider( providerName, providerID, !lhsIsLHS );
            if ( otherData != mediaData )
                setMediaForProvider( providerName, providerID, mediaData, !lhsIsLHS );
        }
    }
    for ( auto && ii : replacementMap )
    {
        auto currMediaID = ii.first->getMediaID( lhsIsLHS );
        auto pos = lhs.find( currMediaID );
        lhs.erase( pos );
        ii.second->updateFromOther( ii.first, lhsIsLHS );

        lhs[ currMediaID ] = ii.second;
    }
}

void CMainWindow::slotToggleOnlyShowSyncableUsers()
{
    fSettings->setOnlyShowSyncableUsers( fImpl->actionOnlyShowSyncableUsers->isChecked() );
    onlyShowSyncableUsers();
}

void CMainWindow::slotToggleOnlyShowMediaWithDifferences()
{
    fSettings->setOnlyShowMediaWithDifferences( fImpl->actionOnlyShowMediaWithDifferences->isChecked() );
    onlyShowMediaWithDifferences();
}

void CMainWindow::onlyShowSyncableUsers()
{
    auto curr = fImpl->users->currentItem();
    bool onlyShowSync = fSettings->onlyShowSyncableUsers();
    int itemNum = -1;
    for ( int ii = 0; ii < fImpl->users->topLevelItemCount(); ++ii )
    {
        auto item = fImpl->users->topLevelItem( ii );
        if ( item == curr )
            itemNum = ii;

        auto hasLHSServer = !item->text( 1 ).isEmpty();
        auto hasRHSServer = !item->text( 2 ).isEmpty();

        item->setHidden( onlyShowSync && ( !hasLHSServer || !hasRHSServer ) );
    }
    if ( ( curr && curr->isHidden() ) || ( itemNum == -1 ) )
    {
        for ( int ii = ( itemNum + 1 ); ii < fImpl->users->topLevelItemCount(); ++ii )
        {
            auto item = fImpl->users->topLevelItem( ii );
            if ( !item->isHidden() )
            {
                fImpl->users->setCurrentItem( item );
                curr = item;
                break;
            }
        }
        slotCurrentItemChanged( nullptr, curr );
    }
}

void CMainWindow::onlyShowMediaWithDifferences()
{
    bool onlyShowMediaWithDiff = fSettings->onlyShowMediaWithDifferences();

    for ( auto && ii : fAllMedia )
    {
        if ( !ii )
            continue;

        bool hide = false;
        if ( !onlyShowMediaWithDiff )
            hide = false;
        else
            hide = ii->serverDataEqual() || !ii->hasProviderIDs();

        ii->fLHSServer->fItem->setHidden( hide );
        ii->fRHSServer->fItem->setHidden( hide );
    }
}
void CMainWindow::getMissingMedia( std::shared_ptr< SMediaData > mediaData )
{
    if ( !mediaData || !mediaData->hasMissingInfo() || !mediaData->hasProviderIDs() )
        return;

    QUrl url;
    QString apiKey;
    if ( mediaData->isMissing( true ) )
    {
        url = fSettings->getServerURL( true );
        apiKey = fSettings->lhsAPI();
    }
    else // if ( mediaData->isMissing( false ) ) // has to be true
    {
        url = fSettings->getServerURL( false );
        apiKey = fSettings->rhsAPI();
    }

    auto path = url.path();
    path += QString( "Items" );
    url.setPath( path );

    auto query = mediaData->getMissingItemQuery();
    query.addQueryItem( "api_key", mediaData->isMissing( true ) ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    url.setQuery( query );


    //qDebug() << mediaData->fName << url;
    auto request = QNetworkRequest( url );

    auto reply = fManager->get( request );
    setIsLHS( reply, mediaData->isMissing( true ) );
    setExtraData( reply, mediaData->fName );
    setRequestType( reply, ERequestType::eMissingMedia );
}

void CMainWindow::slotProcess()
{
    setupProgressDlg( "Processing Data", true, true );

    updateProgressDlg( static_cast<int>( fAllMedia.size() ) );
    for ( auto && ii : fAllMedia )
    {
        incProgressDlg();
        if ( processData( ii ) )
        {
            //break;
        }
    }
    resetProgressDlg();
}


bool SMediaData::serverDataEqual() const
{
    if ( fLHSServer && fRHSServer )
        return *fLHSServer == *fRHSServer;
    return false;
}

bool operator==( const SMediaDataBase & lhs, const SMediaDataBase & rhs )
{
    bool equal = lhs.fLastPlayedPos == rhs.fLastPlayedPos;
    equal = equal && lhs.fIsFavorite == rhs.fIsFavorite;
    equal = equal && lhs.fPlayed == rhs.fPlayed;
    //equal = equal && lhs.getLastModifiedTime() == rhs.getLastModifiedTime();
    return equal;
}

bool CMainWindow::processData( std::shared_ptr< SMediaData > mediaData )
{
    if ( !mediaData || mediaData->serverDataEqual() )
        return false;
    
    /*
        "UserData": {
        "IsFavorite": false,
        "LastPlayedDate": "2022-01-15T20:28:39.0000000Z",
        "PlayCount": 1,
        "PlaybackPositionTicks": 0,
        "Played": true
        }
    */

    //qDebug() << "processing " << mediaData->fName;
    setPlayed( mediaData );
    //setPlayPosition( mediaData );
    setFavorite( mediaData );
    //setLastPlayed( mediaData );
    return true;
}

void CMainWindow::setPlayPosition( std::shared_ptr< SMediaData > mediaData )
{
    if ( mediaData->playPositionTheSame() )
        return;

    bool lhsMoreRecent = mediaData->lhsMoreRecent();

    auto && userData = std::get< 0 >( getCurrUserData() );
    if ( !userData )
        return;

    auto && url = fSettings->getServerURL( !lhsMoreRecent );
    auto && mediaID = mediaData->getMediaID( !lhsMoreRecent );
    auto && userID = userData->getUserID( !lhsMoreRecent );

    auto playPosition = mediaData->lastPlayedPos( lhsMoreRecent );

    //qDebug() << "userID" << userID;
    //qDebug() << "mediaID" << mediaID;

    auto path = url.path();
    path += QString( "Users/%1/PlayingItems/%3/Progress" ).arg( userID ).arg( mediaID );
    url.setPath( path );
    QUrlQuery query;

    query.addQueryItem( "api_key", !lhsMoreRecent ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    query.addQueryItem( "PositionTicks", QString::number( playPosition ) );
    url.setQuery( query );

    //qDebug() << url;

    auto request = QNetworkRequest( url );

    QByteArray data;
    QNetworkReply * reply = nullptr;
    reply = fManager->post( request, data );

    setIsLHS( reply, lhsMoreRecent );
    setRequestType( reply, ERequestType::eUpdateData );
}

void CMainWindow::setPlayed( std::shared_ptr< SMediaData > mediaData )
{
    if ( mediaData->bothPlayed() )
        return;

    bool lhsMoreRecent = mediaData->lhsMoreRecent();
    bool deleteUpdate = false;
    if ( lhsMoreRecent )
        deleteUpdate = !mediaData->played( true );
    else
        deleteUpdate = !mediaData->played( false );

    QString updateType = "PlayedItems";
    updateMedia( mediaData, deleteUpdate, updateType );

}

void CMainWindow::setFavorite( std::shared_ptr< SMediaData > mediaData )
{
    if ( mediaData->bothFavorites() )
        return;

    bool lhsMoreRecent = mediaData->lhsMoreRecent();
    bool deleteUpdate = false;
    if ( lhsMoreRecent )
        deleteUpdate = !mediaData->isFavorite( true );
    else
        deleteUpdate = !mediaData->isFavorite( false );

    QString updateType = "FavoriteItems";
    updateMedia( mediaData, deleteUpdate, updateType );
}

void CMainWindow::updateMedia( std::shared_ptr<SMediaData> mediaData, bool deleteUpdate, const QString & updateType )
{
    auto && userData = std::get< 0 >( getCurrUserData() );
    if ( !userData )
        return;

    bool lhsMoreRecent = mediaData->lhsMoreRecent();
    auto && url = fSettings->getServerURL( !lhsMoreRecent );
    auto && mediaID = mediaData->getMediaID( !lhsMoreRecent );
    auto && userID = userData->getUserID( !lhsMoreRecent );

    //qDebug() << "userID" << userID;
    //qDebug() << "mediaID" << mediaID;
    //qDebug() << "updateType" << updateType;

    if ( mediaID.isEmpty() || userID.isEmpty() || updateType.isEmpty() )
        return;

    auto path = url.path();
    path += QString( "Users/%1/%2/%3" ).arg( userID ).arg( updateType ).arg( mediaID );
    url.setPath( path );
    //qDebug() << url;

    QUrlQuery query;
    query.addQueryItem( "api_key", !lhsMoreRecent ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    url.setQuery( query );

    auto request = QNetworkRequest( url );

    QByteArray data;
    QNetworkReply * reply = nullptr;
    if ( deleteUpdate )
        reply = fManager->deleteResource( request );
    else
        reply = fManager->post( request, data );

    setIsLHS( reply, !lhsMoreRecent );
    setRequestType( reply, ERequestType::eUpdateData );
}

QDateTime SMediaDataBase::getLastModifiedTime() const
{
    if ( !fUserData.contains( "LastPlayedDate" ) )
        return {};

    auto value = fUserData[ "LastPlayedDate" ].toDateTime();
    return value;
}

void SMediaDataBase::loadUserDataFromJSON( const QJsonObject & userDataObj, const QMap<QString, QVariant> & userDataVariant )
{
    fIsFavorite = userDataObj[ "IsFavorite" ].toBool();
    fLastPlayedPos = userDataObj[ "PlaybackPositionTicks" ].toVariant().toLongLong();
    fPlayed = userDataObj[ "Played" ].toVariant().toBool();
    fUserData = userDataVariant;
}

void CMainWindow::addToLog( const QString & msg )
{
    qDebug() << msg;
    fImpl->log->appendPlainText( QString( "%1" ).arg( msg.endsWith( '\n' ) ? msg.left( msg.length() - 1 ) : msg ) );
    fImpl->statusbar->showMessage( msg, 500 );
}

QString toString( ERequestType request )
{
    switch ( request )
    {
        case ERequestType::eNone: return "eNone";
        case ERequestType::eUsers: return "eUsers";
        case ERequestType::eMediaList: return "eMediaList";
        case ERequestType::eMissingMedia: return "eMissingMedia";
        case ERequestType::eMediaData: return "eMediaData";
        case ERequestType::eUpdateData: return "eUpdateData";
    }
    return QString();
}

void SMediaData::setItemColor( QTreeWidgetItem * item, const QColor & clr )
{
    for ( int ii = 0; ii < item->columnCount(); ++ii )
        setItemColor( item, ii, clr );
}

void SMediaData::setItemColor( QTreeWidgetItem * item, int column, const QColor & clr )
{
    item->setBackground( column, clr );
}


void SMediaData::setItemColors()
{
    //if ( isMissing( true ) )
    //    setItemColor( fLHSServer->fItem, Qt::red );
    //if ( isMissing( false ) )
    //    setItemColor( fRHSServer->fItem, Qt::red );
    if ( !hasMissingInfo() )
    {
        if ( *fLHSServer != *fRHSServer )
        {
            setItemColor( fRHSServer->fItem, EColumn::eName, Qt::yellow );
            setItemColor( fLHSServer->fItem, EColumn::eName, Qt::yellow );
            if ( played( true ) != played( false ) )
            {
                setItemColor( fRHSServer->fItem, EColumn::ePlayed, Qt::yellow );
                setItemColor( fLHSServer->fItem, EColumn::ePlayed, Qt::yellow );
            }
            if ( isFavorite( true ) != isFavorite( false ) )
            {
                setItemColor( fRHSServer->fItem, EColumn::eFavorite, Qt::yellow );
                setItemColor( fLHSServer->fItem, EColumn::eFavorite, Qt::yellow );
            }
            if ( lastPlayedPos( true ) != lastPlayedPos( false ) )
            {
                setItemColor( fRHSServer->fItem, EColumn::eLastPlayedPos, Qt::yellow );
                setItemColor( fLHSServer->fItem, EColumn::eLastPlayedPos, Qt::yellow );
            }
        }
    }
}

QUrlQuery SMediaData::getMissingItemQuery() const
{
    QUrlQuery query;

    query.addQueryItem( "IncludeItemTypes", "Movie,Episode,Video" );
    query.addQueryItem( "AnyProviderIdEquals", getProviderList() );
    query.addQueryItem( "SortBy", "SortName" );
    query.addQueryItem( "SortOrder", "Ascending" );
    query.addQueryItem( "Recursive", "True" );

    return query;
}

bool SMediaData::lhsMoreRecent() const
{
    if ( !fLHSServer || !fRHSServer )
        return false;

    //qDebug() << fLHSServer->getLastModifiedTime();
    //qDebug() << fRHSServer->getLastModifiedTime();
    return fLHSServer->getLastModifiedTime() > fRHSServer->getLastModifiedTime();
}

void SMediaData::createItems( QTreeWidget * lhsTree, QTreeWidget * rhsTree )
{
    auto columns = getColumns( false );


    fLHSServer->fItem = new QTreeWidgetItem( lhsTree, columns.first );
    fRHSServer->fItem = new QTreeWidgetItem( rhsTree, columns.second );

    setItemColors();
}

QStringList SMediaData::getColumns( bool /*unwatched*/, bool lhs )
{
    QStringList retVal;
    if ( ( lhs && !fLHSServer ) || ( !lhs && !fRHSServer ) )
    {
        retVal = QStringList() << fName << QString() << QString() << QString() << QString() << QString();
        if ( lhs )
            fLHSServer = std::make_shared< SMediaDataBase >();
        else 
            fRHSServer = std::make_shared< SMediaDataBase >();
    }
    else
    {
        retVal = QStringList() << fName << getMediaID( lhs ) << ( isPlayed( lhs ) ? "Yes" : "No" ) << QString( isFavorite( lhs ) ? "Yes" : "No" ) << lastPlayed( lhs ) << lastModified( lhs );
    }
    return retVal;
}

std::pair< QStringList, QStringList > SMediaData::getColumns( bool unwatched )
{
    QStringList providerColumns;
    for ( auto && ii : CMainWindow::fProviderColumnsByColumn )
    {
        auto pos = fProviders.find( ii.second );
        if ( pos == fProviders.end() )
            providerColumns << QString();
        else
            providerColumns << ( *pos ).second;
    }

    auto lhsColumns = getColumns( unwatched, true ) << providerColumns;
    auto rhsColumns = getColumns( unwatched, false ) << providerColumns;

    return { lhsColumns, rhsColumns };
}

void SMediaData::updateItems( bool unwatched, bool isLHS )
{
    auto lnrColumns = getColumns( unwatched );
    auto columns = isLHS ? lnrColumns.first : lnrColumns.second;
    auto item = getItem( isLHS );
    Q_ASSERT( item && ( item->columnCount() == columns.count() ) );
    for ( int ii = 0; ii < columns.count(); ++ii )
    {
        item->setText( ii, columns[ ii ] );
    }
    setItemColors();
}

