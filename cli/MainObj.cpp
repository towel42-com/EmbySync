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
#include "Core/MediaModel.h"
#include "Core/ServerModel.h"

#include "Version.h"
#include <iostream>

#include <QTimer>
#include <QDateTime>
#include <QJsonObject>

CMainObj::CMainObj( const QString & settingsFile, QObject * parent /*= nullptr*/ ) :
    QObject( parent ),
    fSettingsFile( settingsFile )
{
    fServerModel = std::make_shared< CServerModel >();
    fSettings = std::make_shared< CSettings >( fServerModel );
    if ( !fSettings->load( settingsFile,
         [settingsFile]( const QString & /*title*/, const QString & msg )
         {
             std::cerr << "--settings file '" << settingsFile.toStdString() << "' could not be loaded: " << msg.toStdString() << "\n";
         }, false ) )
    {
        fSettings.reset();
        return;
    }

    auto userRegExList = fSettings->syncUserList();
    QStringList syncUsers;
    for ( auto && ii : userRegExList )
    {
        if ( !QRegularExpression( ii ).isValid() )
        {
            std::cerr << "SyncUserList contains invalid regular expression: '" << ii.toStdString() << "'.\n";
            return;
        }
        syncUsers << "(" + ii + ")";
    }
    auto regExStr = syncUsers.join( "|" );
    fUserRegExp = QRegularExpression( regExStr );
    if ( !fUserRegExp.isValid() )
    {
        std::cerr << "SyncUserList creates an invalid regular expression: '" << regExStr.toStdString() << "'.\n";
        return;
    }

    if ( regExStr.isEmpty() )
    {
        std::cerr << "SyncUserList is not set in the settings file.\n";
        return;
    }

    fUsersModel = std::make_shared< CUsersModel >( fSettings, fServerModel );
    fMediaModel = std::make_shared< CMediaModel >( fSettings, fServerModel );

    fSyncSystem = std::make_shared< CSyncSystem >( fSettings, fUsersModel, fMediaModel, fServerModel );

    connect( fSyncSystem.get(), &CSyncSystem::sigAddToLog, this, &CMainObj::slotAddToLog );
    connect( fSyncSystem.get(), &CSyncSystem::sigLoadingUsersFinished, this, &CMainObj::slotLoadingUsersFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMainObj::slotProcessMedia );

    connect( fSyncSystem.get(), &CSyncSystem::sigProcessingFinished, this, &CMainObj::slotProcessingFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMainObj::slotUserMediaCompletelyLoaded );
    
    auto progressSystem = std::make_shared< CProgressSystem >();
    progressSystem->setSetTitleFunc( [ this ]( const QString & title )
    {
        fCurrentProgress = { 0, title, QString() };
        std::cout << "\r" << std::get< 1 >( fCurrentProgress ).toStdString() << std::endl;
    } );
    progressSystem->setIncFunc( [this]()
    {
        std::get< 0 >( fCurrentProgress )++;
        static constexpr auto chars = R"(|||///---***---\\\)";
        static auto cnt = strlen( chars );
        auto value = std::get< 0 >( fCurrentProgress ) % cnt;
        std::cout << chars[ value ] << '\b';
    } );
    progressSystem->setResetFunc( [this]()
    {
        if ( std::get< 1 >( fCurrentProgress ) != std::get< 2 >( fCurrentProgress ) )
        {
            std::cout << '\r' << "Finished " << std::get< 1 >( fCurrentProgress ).toStdString() << std::endl;
            std::get< 2 >( fCurrentProgress ) = std::get< 1 >( fCurrentProgress );
        }
    } );


    fSyncSystem->setProgressSystem( progressSystem );
    fSyncSystem->setUserMsgFunc(
        []( const QString & /*title*/, const QString & msg, bool isCritical )
        {
            if ( isCritical )
                std::cerr << "\r" << "ERROR: " << msg.toStdString() << std::endl;
            else 
                std::cout << "\r" << "INFO: " << msg.toStdString() << std::endl;
        } );

    fAOK = true;
}

void CMainObj::slotAddToLog( int msgType, const QString & msg )
{
    auto stream = ( msgType != EMsgType::eInfo ) ? &std::cerr : &std::cout;
    ( *stream ) << "\r" << createMessage( static_cast< EMsgType >( msgType ), msg ).toStdString() << "\n";
}

void CMainObj::run()
{
    if ( !fSettings || !fSyncSystem )
        return;

    fSyncSystem->loadUsers();
}

void CMainObj::slotLoadingUsersFinished()
{
    if ( !fSyncSystem )
        return;

    fUsersToSync.clear();
    auto users = fUsersModel->getAllUsers( false );
    for ( auto && ii : users )
    {
        if ( ii->isUser( fUserRegExp ) )
            fUsersToSync.push_back( ii );
    }
    if ( fUsersToSync.empty() )
    {
        std::cerr << "No users matched '" << fUserRegExp.pattern().toStdString() << "'" << std::endl;

        emit sigExit( -1 );
        return;
    }

    std::map< QString, std::shared_ptr< CUserData > > unsyncable;
    for ( auto && ii = fUsersToSync.begin(); ii != fUsersToSync.end(); )
    {
        if ( !( *ii )->canBeSynced() )
        {
            unsyncable[ (*ii)->allNames() ] = *ii;
            ii = fUsersToSync.erase( ii );
        }
        else
            ++ii;
    }


    QString unsyncableMsg;
    if ( !unsyncable.empty() )
        unsyncableMsg = "The following users matched but can not be synced\n";
    for ( auto && ii : unsyncable )
    {
        auto missingServerList = ii.second->missingServers();
        for ( auto && jj : missingServerList )
            unsyncableMsg += "\t" + ii.second->allNames() + " - Missing from '" + jj + "\n";
    }
    if ( !unsyncableMsg.isEmpty() )
        slotAddToLog( EMsgType::eWarning, unsyncableMsg );

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
    slotAddToLog( EMsgType::eInfo, "Processing user: " + currUser->allNames() );

    fSyncSystem->loadUsersMedia( currUser );
}

void CMainObj::slotUserMediaCompletelyLoaded()
{
    slotAddToLog( EMsgType::eInfo, "Finished loading media information" );
}

void CMainObj::slotProcessingFinished( const QString & userName )
{
    slotAddToLog( EMsgType::eInfo, QString( "Finished processing user '%1'" ).arg( userName ) );
    QTimer::singleShot( 0, this, &CMainObj::slotProcessNextUser );
}

void CMainObj::slotProcessMedia()
{
    fSyncSystem->selectiveProcessMedia( fSelectedServerToProcess );
}
