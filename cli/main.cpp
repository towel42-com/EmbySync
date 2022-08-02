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

#include "main.h"

#include "Core/Settings.h"
#include "Core/SyncSystem.h"
#include "Core/UserData.h"

#include "Version.h"
#include <iostream>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QTimer>
#include <memory>

int main( int argc, char ** argv )
{
    QCoreApplication appl( argc, argv );
    appl.setApplicationName( QString::fromStdString( NVersion::APP_NAME ) );
    appl.setApplicationVersion( QString::fromStdString( NVersion::getVersionString( true ) ) );
    appl.setOrganizationName( QString::fromStdString( NVersion::VENDOR ) );
    appl.setOrganizationDomain( QString::fromStdString( NVersion::HOMEPAGE ) );
    appl.setOrganizationDomain( "github.com/towel42-com/EmbySync" ); // QString::fromStdString( NVersion::HOMEPAGE ) );

    QCommandLineParser parser;
    parser.setApplicationDescription( QString::fromStdString( NVersion::APP_NAME + " CLI - a tool to sync two emby servers" ) );
    auto helpOption = parser.addHelpOption();
    auto versionOption = parser.addVersionOption();

    auto settingsFileOption = QCommandLineOption( QStringList() << "settings" << "s", "The settings json file", "Settings file" );
    parser.addOption( settingsFileOption );

    auto usersOption = QCommandLineOption( QStringList() << "users" << "u", "A regex for users to sync (.* for all)", "Users to sync" );
    parser.addOption( usersOption );

    auto forceLeftOption = QCommandLineOption( QStringList() << "force_left" << "l", "Force process from lhs server to rhs" );
    parser.addOption( forceLeftOption );

    auto forceRightOption = QCommandLineOption( QStringList() << "force_right" << "r", "Force process from rhs server to lhs" );
    parser.addOption( forceRightOption );

    parser.process( appl );
    
    if ( !parser.unknownOptionNames().isEmpty() )
    {
        std::cerr << "The following options were set and are unknown:\n";
        for ( auto && ii : parser.unknownOptionNames() )
            std::cerr << "    " << ii.toStdString() << "\n";
        parser.showHelp();
        return -1;
    }

    if ( parser.isSet( helpOption ) )
    {
        parser.showHelp();
        return 0;
    }

    if ( parser.isSet( versionOption ) )
    {
        std::cout << NVersion::APP_NAME << " - " <<  NVersion::getVersionString( true ) << "\n";
        return 0;
    }

    if ( !parser.isSet( settingsFileOption ) )
    {
        std::cerr << "--settings must be set\n";
        std::cerr << parser.helpText().toStdString() << "\n";
        return -1;
    }


    if ( !parser.isSet( usersOption ) )
    {
        std::cerr << "--users must be set\n";
        std::cerr << parser.helpText().toStdString() << "\n";
        return -1;
    }

    auto settingsFile = parser.value( settingsFileOption );
    auto mainObj = std::make_shared< CMain >( settingsFile, parser.value( usersOption ) );
    QObject::connect( mainObj.get(), &CMain::sigExit, &appl, &QCoreApplication::exit );

    mainObj->setForceProcessing( parser.isSet( forceLeftOption ), parser.isSet( forceRightOption ) );

    mainObj->run();

    int retVal = appl.exec();
    return retVal;
}


CMain::CMain( const QString & settingsFile, const QString & usersRegEx, QObject * parent /*= nullptr*/ ) :
    QObject( parent ),
    fSettingsFile( settingsFile ),
    fUserRegEx( usersRegEx )
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

    fSyncSystem = std::make_shared< CSyncSystem >( fSettings );

    connect( fSyncSystem.get(), &CSyncSystem::sigAddToLog, this, &CMain::slotAddToLog );
    connect( fSyncSystem.get(), &CSyncSystem::sigLoadingUsersFinished, this, &CMain::slotLoadingUsersFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMain::slotProcess );

    connect( fSyncSystem.get(), &CSyncSystem::sigProcessingFinished, this, &CMain::slotProcessingFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaCompletelyLoaded, this, &CMain::slotUserMediaCompletelyLoaded );
    connect( fSyncSystem.get(), &CSyncSystem::sigFinishedCheckingForMissingMedia, this, &CMain::slotFinishedCheckingForMissingMedia );



    SProgressFunctions progressFuncs;
    progressFuncs.fSetupFunc = []( const QString & title )
    {
        std::cout << title.toStdString() << std::endl;
    };

    fSyncSystem->setProgressFunctions( progressFuncs );
}


void CMain::run()
{
    if ( !fSettings || !fSyncSystem )
        return;

    fSyncSystem->loadUsers();
}

void CMain::slotAddToLog( const QString & msg )
{
    std::cout << msg.toStdString() << "\n";
}

void CMain::slotLoadingUsersFinished()
{
    if ( !fSyncSystem )
        return;

    auto allUsers = fSyncSystem->getAllUsers();
    fUsersToSync.clear();
    for ( auto && ii : allUsers )
    {
        if ( ii->isUserNameMatch( fUserRegEx ) )
            fUsersToSync.push_back( ii );
    }
    if ( fUsersToSync.empty() )
    {
        std::cerr << "No users matched '" << fUserRegEx.toStdString() << "'" << std::endl;

        emit sigExit( -1 );
        return;
    }

    std::list< std::shared_ptr< CUserData > > unsyncable;
    for ( auto && ii = fUsersToSync.begin(); ii != fUsersToSync.end(); )
    {
        if ( !( *ii )->canBeSynced() )
        {
            unsyncable.push_back( *ii );
            ii = fUsersToSync.erase( ii );
        }
        else
            ++ii;
    }

    if ( !unsyncable.empty() )
        std::cerr << "Warning: The following users matched but can not be synced\n";
    for ( auto && ii : unsyncable )
    {
        std::cerr << "\t" << ii->name().toStdString();
        if ( ii->onLHSServer() )
            std::cerr << " - Missing from '" << fSettings->lhsURL().toStdString();
        else //if ( ii->onRHSServer() )
            std::cerr << " - Missing from '" << fSettings->rhsURL().toStdString();
        std::cerr << "\n";
    }

    if ( fUsersToSync.empty() )
    {
        emit sigExit( -1 );
        return;
    }

    QTimer::singleShot( 0, this, &CMain::slotProcessNextUser );
}

void CMain::slotProcessNextUser()
{
    if ( fUsersToSync.empty() )
    {
        emit sigExit( 0 );
        return;
    }

    auto currUser = fUsersToSync.front();
    fUsersToSync.pop_front();
    slotAddToLog( "Processing user: " + currUser->name() );

    fSyncSystem->resetMedia();
    fSyncSystem->loadUsersMedia( currUser );

    //QTimer::singleShot( 0, this, &CMain::slotProcessNextUser );
}

void CMain::slotFinishedCheckingForMissingMedia()
{
    slotAddToLog( "Finished processing missing media" );
}

void CMain::slotUserMediaCompletelyLoaded()
{
    slotAddToLog( "Finished loading media information" );
}

void CMain::slotProcessingFinished( const QString & userName )
{
    slotAddToLog( QString( "Finished processing user '%1'" ).arg( userName ) );
    QTimer::singleShot( 0, this, &CMain::slotProcessNextUser );
}

void CMain::slotProcess()
{
    fSyncSystem->process( fForce.first, fForce.second );
}

