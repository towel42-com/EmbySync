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

#include "MainObj.h"

#include "Core/Settings.h"
#include "Core/SyncSystem.h"
#include "Core/UserData.h"
#include "Core/ProgressSystem.h"
#include "Core/UsersModel.h"
#include "Core/ServerInfo.h"
#include "Core/MediaModel.h"
#include "Core/ServerModel.h"
#include "Core/CollectionsModel.h"
#include "Core/MediaData.h"

#include "SABUtils/QtUtils.h"
#include "Version.h"
#include <iostream>

#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

CMainObj::CMainObj( const QString &settingsFile, const QString &mode, QObject *parent /*= nullptr*/ ) :
    QObject( parent ),
    fSettingsFile( settingsFile )
{
    if ( !setMode( mode ) )
        return;

    fServerModel = std::make_shared< CServerModel >();
    fSettings = std::make_shared< CSettings >( false, fServerModel );
    if ( !fSettings->load(
             settingsFile, [ this, settingsFile ]( const QString & /*title*/, const QString &msg ) { fErrorString = QString( "--settings file '%1' could not be loaded: %2" ).arg( settingsFile ).arg( msg ); }, false ) )
    {
        fSettings.reset();
        return;
    }

    auto userRegExList = fSettings->syncUserList();
    QStringList syncUsers;
    for ( auto &&ii : userRegExList )
    {
        if ( !QRegularExpression( ii ).isValid() )
        {
            fErrorString = QString( "SyncUserList contains invalid regular expression: '%1'." ).arg( ii );
            return;
        }
        syncUsers << "(" + ii + ")";
    }
    auto regExStr = syncUsers.join( "|" );
    fUserRegExp = QRegularExpression( regExStr );
    if ( !fUserRegExp.isValid() )
    {
        fErrorString = QString( "SyncUserList creates an invalid regular expression: '%1'." ).arg( regExStr );
        return;
    }

    if ( regExStr.isEmpty() )
    {
        fErrorString = QString( "SyncUserList is not set in the settings file." );
        return;
    }

    fUsersModel = std::make_shared< CUsersModel >( fSettings, fServerModel );
    fMediaModel = std::make_shared< CMediaModel >( fSettings, fServerModel );
    fCollectionsModel = std::make_shared< CCollectionsModel >( fMediaModel );

    fSyncSystem = std::make_shared< CSyncSystem >( fSettings, fUsersModel, fMediaModel, fCollectionsModel, fServerModel );

    connect( fSyncSystem.get(), &CSyncSystem::sigAddToLog, this, &CMainObj::slotAddToLog );
    connect( fSyncSystem.get(), &CSyncSystem::sigLoadingUsersFinished, this, &CMainObj::slotLoadingUsersFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMainObj::slotProcessMedia );
    connect( fSyncSystem.get(), &CSyncSystem::sigMissingEpisodesLoaded, this, &CMainObj::slotMissingEpisodesLoaded );

    connect( fSyncSystem.get(), &CSyncSystem::sigProcessingFinished, this, &CMainObj::slotProcessingFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMainObj::slotUserMediaCompletelyLoaded );

    auto progressSystem = std::make_shared< CProgressSystem >();
    progressSystem->setSetTitleFunc(
        [ this ]( const QString &title )
        {
            fCurrentProgress = { 0, title, QString() };
            addToLog( EMsgType::eInfo, std::get< 1 >( fCurrentProgress ) );
        } );
    progressSystem->setIncFunc(
        [ this ]()
        {
            std::get< 0 >( fCurrentProgress )++;
            static constexpr auto chars = R"(|||///---***---\\\)";
            static auto cnt = strlen( chars );
            auto value = std::get< 0 >( fCurrentProgress ) % cnt;
            std::cout << chars[ value ] << '\b';
        } );
    progressSystem->setResetFunc(
        [ this ]()
        {
            if ( std::get< 1 >( fCurrentProgress ) != std::get< 2 >( fCurrentProgress ) )
            {
                addToLog( EMsgType::eInfo, QString( "Finished '%1'" ).arg( std::get< 1 >( fCurrentProgress ) ) );
                std::get< 2 >( fCurrentProgress ) = std::get< 1 >( fCurrentProgress );
            }
        } );

    fSyncSystem->setProgressSystem( progressSystem );
    fSyncSystem->setUserMsgFunc( [ this ]( EMsgType msgType, const QString &title, QString msg ) { addToLog( msgType, title, msg ); } );

    fAOK = true;
}

bool CMainObj::aOK() const
{
    if ( ( fMode == EMode::eCheckMissing ) && fSelectedServerToProcess.isEmpty() )
    {
        fErrorString = "Selected server must be set to check for missing.";
        fAOK = false;
    }
    return fAOK;
}

void CMainObj::slotAddToLog( int msgType, const QString &msg )
{
    addToLog( msgType, QString(), msg );
}

void CMainObj::addToLog( int msgType, const QString &msg )
{
    addToLog( msgType, QString(), msg );
}

void CMainObj::addToLog( int msgType, const QString &title, const QString &msg )
{
    if ( fQuiet )
        return;

    auto tmp = QStringList() << title.trimmed() << msg.trimmed();
    tmp.removeAll( QString() );
    auto fullMsg = tmp.join( " - " ).trimmed();
    if ( msg.isEmpty() )
        return;

    auto stream = ( msgType != EMsgType::eInfo ) ? &std::cerr : &std::cout;

    ( *stream ) << "\r" << createMessage( static_cast< EMsgType >( msgType ), msg ).toStdString() << "\n";
}

void CMainObj::run()
{
    if ( !fSettings || !fSyncSystem )
        return;

    if ( fMode == EMode::eCheckMissing )
    {
        fSelectedServer = fServerModel->enableServer( fSelectedServerToProcess, true, fErrorString );
        if ( !fSelectedServer )
        {
            fAOK = false;
            return;
        }
    }

    fSyncSystem->loadUsers();
}

void CMainObj::setMinimumDate( const QString &minDate )
{
    fMinDate = NSABUtils::getDate( minDate );
    if ( !fMinDate.isValid() )
    {
        fAOK = false;
        fErrorString = tr( "Invalid Minimum date '%1'." ).arg( minDate );
    }
}

void CMainObj::setMaximumDate( const QString &maxDate )
{
    fMaxDate = NSABUtils::getDate( maxDate );
    if ( !fMaxDate.isValid() )
    {
        fAOK = false;
        fErrorString = tr( "Invalid Maximum date '%1'." ).arg( maxDate );
    }
}

void CMainObj::slotLoadingUsersFinished()
{
    if ( !fSyncSystem )
        return;

    fUsersToSync.clear();
    auto users = fUsersModel->getAllUsers( false );
    for ( auto &&ii : users )
    {
        if ( ii->isUser( fUserRegExp ) )
        {
            if ( fMode == EMode::eCheckMissing )
            {
                if ( !ii->isAdmin( fSelectedServer->keyName() ) )
                    continue;
            }
            fUsersToSync.push_back( ii );
            if ( fMode == EMode::eCheckMissing )
                break;
        }
    }
    if ( fUsersToSync.empty() )
    {
        std::cerr << "No users matched '" << fUserRegExp.pattern().toStdString() << "'";
        if ( fMode == EMode::eCheckMissing )
            std::cerr << " or were administrators.";
        std::cerr << std::endl;

        emit sigExit( -1 );
        return;
    }

    if ( fMode == EMode::eSync )
    {
        std::map< QString, std::shared_ptr< CUserData > > unsyncable;
        for ( auto &&ii = fUsersToSync.begin(); ii != fUsersToSync.end(); )
        {
            if ( !( *ii )->canBeSynced() )
            {
                unsyncable[ ( *ii )->allNames() ] = *ii;
                ii = fUsersToSync.erase( ii );
            }
            else
                ++ii;
        }

        QString unsyncableMsg;
        if ( !unsyncable.empty() )
            unsyncableMsg = "The following users matched but can not be synced\n";
        for ( auto &&ii : unsyncable )
        {
            auto missingServerList = ii.second->missingServers();
            for ( auto &&jj : missingServerList )
                unsyncableMsg += "\t" + ii.second->allNames() + " - Missing from '" + jj + "\n";
        }
        if ( !unsyncableMsg.isEmpty() )
            slotAddToLog( EMsgType::eWarning, unsyncableMsg );
    }
    if ( fUsersToSync.empty() )
    {
        emit sigExit( 0 );
        return;
    }

    QTimer::singleShot( 0, this, &CMainObj::slotProcessNextUser );
}

void CMainObj::slotProcessNextUser()
{
    if ( fUsersToSync.empty() )
    {
        emit sigExit( 0 );
        return;
    }

    auto currUser = fUsersToSync.front();
    fUsersToSync.pop_front();
    if ( fMode == EMode::eSync )
    {
        slotAddToLog( EMsgType::eInfo, "Processing user: " + currUser->allNames() );
        fSyncSystem->loadUsersMedia( ETool::ePlayState, currUser );
    }
    else if ( fMode == EMode::eCheckMissing )
    {
        if ( !fSyncSystem->loadMissingEpisodes( currUser, fSelectedServer, fMinDate, fMaxDate ) )
        {
            fErrorString = tr( "No user found with Administrator Privileges on server '%1'" ).arg( fSelectedServer->displayName() );
        }
    }
}

void CMainObj::slotUserMediaCompletelyLoaded()
{
    if ( fMode == EMode::eSync )
        slotAddToLog( EMsgType::eInfo, "Finished loading media information" );
}

void CMainObj::slotProcessingFinished( const QString &userName )
{
    slotAddToLog( EMsgType::eInfo, QString( "Finished processing user '%1'" ).arg( userName ) );
    QTimer::singleShot( 0, this, &CMainObj::slotProcessNextUser );
}

void CMainObj::slotProcessMedia()
{
    if ( fMode == EMode::eSync )
        fSyncSystem->selectiveProcessMedia( fSelectedServerToProcess );
}

void CMainObj::slotMissingEpisodesLoaded()
{
    slotAddToLog( EMsgType::eInfo, "Finished loading missing episodes" );
    QJsonArray shows;

    for ( auto &&mediaInfo : *fMediaModel )
    {
        shows.push_back( mediaInfo->toJson( true ) );
    }
    QJsonDocument doc( shows );
    std::cout << doc.toJson( QJsonDocument::JsonFormat::Indented ).toStdString() << "\n";
    QTimer::singleShot( 0, this, &CMainObj::slotProcessNextUser );
}

bool CMainObj::setMode( const QString &mode )
{
    if ( mode == "check_missing" )
        fMode = EMode::eCheckMissing;
    else if ( mode == "sync" )
        fMode = EMode::eSync;
    else
    {
        fErrorString = QString( "Invalid mode '%1'" ).arg( mode );
        fAOK = false;
        return false;
    }
    return true;
}
