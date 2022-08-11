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

#include "Version.h"
#include <iostream>

#include <QTimer>
#include <QDateTime>

CMainObj::CMainObj( const QString & settingsFile, QObject * parent /*= nullptr*/ ) :
    QObject( parent ),
    fSettingsFile( settingsFile )
{
    fSettings = std::make_shared< CSettings >();
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

    fSyncSystem = std::make_shared< CSyncSystem >( fSettings );

    connect( fSyncSystem.get(), &CSyncSystem::sigAddToLog, this, &CMainObj::slotAddToLog );
    connect( fSyncSystem.get(), &CSyncSystem::sigLoadingUsersFinished, this, &CMainObj::slotLoadingUsersFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMainObj::slotProcess );

    connect( fSyncSystem.get(), &CSyncSystem::sigProcessingFinished, this, &CMainObj::slotProcessingFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaCompletelyLoaded, this, &CMainObj::slotUserMediaCompletelyLoaded );
    connect( fSyncSystem.get(), &CSyncSystem::sigFinishedCheckingForMissingMedia, this, &CMainObj::slotFinishedCheckingForMissingMedia );

    SProgressFunctions progressFuncs;
    progressFuncs.fSetupFunc = [ this ]( const QString & title )
    {
        fCurrentProgress = { 0, title, QString() };
        std::cout << "\r" << std::get< 1 >( fCurrentProgress ).toStdString() << std::endl;
    };
    progressFuncs.fIncFunc = [this]()
    {
        std::get< 0 >( fCurrentProgress )++;
        static constexpr auto chars = R"(|||///---***---\\\)";
        static auto cnt = strlen( chars );
        auto value = std::get< 0 >( fCurrentProgress ) % cnt;
        std::cout << chars[ value ] << '\b';
    };
    progressFuncs.fResetFunc = [this]()
    {
        if ( std::get< 1 >( fCurrentProgress ) != std::get< 2 >( fCurrentProgress ) )
        {
            std::cout << '\r' << "Finished " << std::get< 1 >( fCurrentProgress ).toStdString() << std::endl;
            std::get< 2 >( fCurrentProgress ) = std::get< 1 >( fCurrentProgress );
        }
    };

    fSyncSystem->setProgressFunctions( progressFuncs );
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

    auto allUsers = fSyncSystem->getAllUsers();
    fUsersToSync.clear();
    for ( auto && ii : allUsers )
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
            unsyncable[ (*ii)->displayName() ] = *ii;
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
        unsyncableMsg += "\t" + ii.second->displayName() + " - Missing from '";
        if ( !ii.second->onLHSServer() )
            unsyncableMsg += fSettings->lhsURL();
        else //if ( ii->onRHSServer() )
            unsyncableMsg += fSettings->rhsURL();
        unsyncableMsg += "'\n";
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
    slotAddToLog( EMsgType::eInfo, "Processing user: " + currUser->displayName() );

    fSyncSystem->resetMedia();
    fSyncSystem->loadUsersMedia( currUser );

    //QTimer::singleShot( 0, this, &CMain::slotProcessNextUser );
}

void CMainObj::slotFinishedCheckingForMissingMedia()
{
    slotAddToLog( EMsgType::eInfo, "Finished processing missing media" );
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

void CMainObj::slotProcess()
{
    fSyncSystem->process( fForce.first, fForce.second );
}

