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

#include "T42-Utils/QtUtils.h"
#include "T42-Utils/uiUtils.h"

#include "Version.h"
#include <iostream>

#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QDate>

CCommandLineParser::CCommandLineParser( const QCoreApplication &appl ) :
    QCommandLineParser()
{
    setApplicationDescription( NVersion::APP_NAME + " CLI - a tool to sync two emby servers" );
    auto helpOption = addHelpOption();
    auto versionOption = addVersionOption();

    auto settingsFileOption = QCommandLineOption(
        QStringList() << "settings"
                      << "s",
        "The settings json file", "Settings file", CSettings::latestProjectSettingsFile() );
    addOption( settingsFileOption );

    auto modeOption = QCommandLineOption(
        QStringList() << "mode"
                      << "m",
        "The particular mode of operation you wish to use valid values are check_missing|sync", "Mode" );
    addOption( modeOption );

    auto selectedServerOption = QCommandLineOption( QStringList() << "selected_server", "The server name you wish to use as the primary server to use as the source server (required for check_missing)", "Selected Server" );
    addOption( selectedServerOption );

    auto minDateStr = QDate::currentDate().addDays( -60 ).toString( "MM/dd/yyyy" );
    auto minDateOption = QCommandLineOption( QStringList() << "min_date", QString( "The oldest premiere date to check if its missing (default %1)" ).arg( minDateStr ), "min date", minDateStr );
    addOption( minDateOption );

    auto maxDateStr = QDate::currentDate().toString( "MM/dd/yyyy" );
    auto maxDateOption = QCommandLineOption( QStringList() << "max_date", QString( "The latest premiere date to check if its missing (default %1)" ).arg( maxDateStr ), "max date", maxDateStr );
    addOption( maxDateOption );

#ifdef Q_OS_WIN
    auto launchMissing = QCommandLineOption( QStringList() << "launch", QString( "Launch search on missing episodes" ) );
    addOption( launchMissing );
#endif

    auto quietOption = QCommandLineOption( QStringList() << "quiet" << "q", QString( "Minimize text output" ) );
    addOption( quietOption );

    if ( !parse( appl.arguments() ) )
    {
        fStatus = { false, this->errorText(), -1 };
        return;
    }

    if ( !unknownOptionNames().isEmpty() )
    {
        auto msg = versionText() + "\n";
        msg += "The following options were set and are unknown:";
        for ( auto &&ii : unknownOptionNames() )
            msg += "\n    " + ii.toStdString();
        fStatus = { false, msg, -1 };
        return;
    }

    if ( isSet( helpOption ) )
    {
        fStatus = { true, versionText() + "\n" + helpText().trimmed(), 0 };
        return;
    }

    if ( isSet( "help-all" ) )
    {
        fStatus = { true, versionText() + "\n" + helpText( /*true*/ ).trimmed(), 0 };
        return;
    }

    if ( isSet( versionOption ) )
    {
        fStatus = { true, versionText(), 0 };
        return;
    }

    if ( !isSet( modeOption ) )
    {
        fStatus = { false, helpText().trimmed(), -1 };
        return;
    }

    fStatus = { true, "", std::optional< int >() };
    fSettingsFile = value( settingsFileOption );
    fMode = value( modeOption ).toLower();

    if ( isSet( selectedServerOption ) )
        fSelectedServer = value( selectedServerOption );

    fMinDate = value( minDateOption );
    fMaxDate = value( maxDateOption );
    fQuiet = isSet( quietOption );
    fLaunchMissingEpisodes = isSet( launchMissing );
}

QString CCommandLineParser::versionText() const
{
    return NVersion::APP_NAME + " - " + NVersion::getVersionText( true, false );
}

CMainObj::CMainObj( const QCoreApplication &appl, QObject *parent /*= nullptr*/ ) :
    QObject( parent )
{
    init( appl );
}

void CMainObj::init( const QCoreApplication &appl )
{
    fCLIParser = std::make_shared< CCommandLineParser >( appl );

    if ( std::get< 2 >( fCLIParser->status() ).has_value() )
        return;

    if ( !setMode( fCLIParser->mode() ) )
        return;

    fServerModel = std::make_shared< CServerModel >();
    fSettings = std::make_shared< CSettings >( false, fServerModel );
    if ( !fSettings->load( fCLIParser->settingsFile(), [ this ]( const QString & /*title*/, const QString &msg ) { fStatusText = QString( "--settings file '%1' could not be loaded: %2" ).arg( fCLIParser->settingsFile() ).arg( msg ); }, false ) )
    {
        return;
    }

    if ( fSettings->enabledServerCount() == 1 )
    {
        fSelectedServerToProcess = fSettings->firstEnabledServer()->displayName();
    }
    auto userRegExList = fSettings->syncUserList();
    QStringList syncUsers;
    for ( auto &&ii : userRegExList )
    {
        if ( !QRegularExpression( ii ).isValid() )
        {
            fStatusText = QString( "SyncUserList contains invalid regular expression: '%1'." ).arg( ii );
            return;
        }
        syncUsers << "(" + ii + ")";
    }
    auto regExStr = syncUsers.join( "|" );
    fUserRegExp = QRegularExpression( regExStr );
    if ( !fUserRegExp.isValid() )
    {
        fStatusText = QString( "SyncUserList creates an invalid regular expression: '%1'." ).arg( regExStr );
        return;
    }

    if ( regExStr.isEmpty() )
    {
        fStatusText = QString( "SyncUserList is not set in the settings file." );
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
    connect( fSyncSystem.get(), &CSyncSystem::sigAllShowsLoaded, this, &CMainObj::slotAllShowsLoaded );

    connect( fSyncSystem.get(), &CSyncSystem::sigProcessingFinished, this, &CMainObj::slotProcessingFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMainObj::slotUserMediaCompletelyLoaded );

    auto progressSystem = std::make_shared< CProgressSystem >();
    progressSystem->setSetTitleFunc(
        [ this ]( const QString &title )
        {
            if ( fQuiet )
                return;

            fCurrentProgress = { 0, title, QString() };
            addToLog( EMsgType::eInfo, std::get< 1 >( fCurrentProgress ) );
        } );
    progressSystem->setIncFunc(
        [ this ]()
        {
            if ( fQuiet )
                return;

            std::get< 0 >( fCurrentProgress )++;
            static constexpr auto chars = R"(|||///---***---\\\)";
            static auto cnt = strlen( chars );
            auto value = std::get< 0 >( fCurrentProgress ) % cnt;
            std::cout << chars[ value ] << '\b';
        } );
    progressSystem->setResetFunc(
        [ this ]()
        {
            if ( fQuiet )
                return;

            if ( std::get< 1 >( fCurrentProgress ) != std::get< 2 >( fCurrentProgress ) )
            {
                addToLog( EMsgType::eInfo, QString( "Finished '%1'" ).arg( std::get< 1 >( fCurrentProgress ) ) );
                std::get< 2 >( fCurrentProgress ) = std::get< 1 >( fCurrentProgress );
            }
        } );

    fSyncSystem->setProgressSystem( progressSystem );
    fSyncSystem->setUserMsgFunc( [ this ]( EMsgType msgType, const QString &title, QString msg ) { addToLog( msgType, title, msg ); } );

    if ( fCLIParser->selectedServer().has_value() )
        setSelectedServer( fCLIParser->selectedServer().value() );

    setMinimumDate( fCLIParser->minDate() );
    setMaximumDate( fCLIParser->maxDate() );
    setQuiet( fCLIParser->quiet() );
    setLaunchMissing( fCLIParser->launchMissingEpisodes() );

    fAOK = true;
}

std::optional< std::pair< bool, int > > CMainObj::status() const
{
    if ( std::get< 2 >( fCLIParser->status() ).has_value() )
    {
        fStatusText = std::get< 1 >( fCLIParser->status() );
        return std::make_pair( std::get< 0 >( fCLIParser->status() ), std::get< 2 >( fCLIParser->status() ).value() );
    }

    if ( ( fMode == EMode::eCheckMissing ) && fSelectedServerToProcess.isEmpty() )
    {
        fStatusText = "Selected server must be set to check for missing.";
        fAOK = false;
    }

    if ( fAOK == false )
        return std::make_pair( false, -1 );
    return {};
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
    if ( fQuiet && ( msgType != EMsgType::eStatus ) )
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
        fSelectedServer = fServerModel->enableServer( fSelectedServerToProcess, true, fStatusText );
        if ( !fSelectedServer )
        {
            fAOK = false;
            return;
        }
    }

    addToLog( EMsgType::eStatus, "Validating Users" );
    fSyncSystem->loadUsers();
}

void CMainObj::setMinimumDate( const QString &minDate )
{
    fMinDate = NTowel42Utils::getDate( minDate );
    if ( !fMinDate.isValid() )
    {
        fAOK = false;
        fStatusText = tr( "Invalid Minimum date '%1'." ).arg( minDate );
    }
}

void CMainObj::setMaximumDate( const QString &maxDate )
{
    fMaxDate = NTowel42Utils::getDate( maxDate );
    if ( !fMaxDate.isValid() )
    {
        fAOK = false;
        fStatusText = tr( "Invalid Maximum date '%1'." ).arg( maxDate );
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
        slotAddToLog( EMsgType::eStatus, "Processing user: " + currUser->allNames() );
        fSyncSystem->loadUsersMedia( ETool::ePlayState, currUser );
    }
    else if ( fMode == EMode::eCheckMissing )
    {
        fUsersToSync.push_back( currUser );
        slotAddToLog( EMsgType::eStatus, "Loading all shows" );
        if ( !fSyncSystem->loadAllShows( currUser, fSelectedServer ) )
        {
            fStatusText = tr( "No user found with Administrator Privileges on server '%1'" ).arg( fSelectedServer->displayName() );
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

void CMainObj::slotAllShowsLoaded()
{
    if ( fUsersToSync.empty() )
        return;

    auto currUser = fUsersToSync.front();
    fUsersToSync.pop_front();

    slotAddToLog( EMsgType::eStatus, "Loading missing episodes" );

    if ( !fSyncSystem->loadMissingEpisodes( currUser, fSelectedServer ) )
    {
        fStatusText = tr( "No user found with Administrator Privileges on server '%1'" ).arg( fSelectedServer->displayName() );
    }
}

void CMainObj::slotMissingEpisodesLoaded()
{
    slotAddToLog( EMsgType::eStatus, "Finished loading missing episodes" );

    auto filterMap = fSettings->missingShowFilterMap();

    std::list< std::shared_ptr< CMediaData > > episodes;

    for ( auto &&mediaInfo : *fMediaModel )
    {
        if ( !SShowFilter::showEpisode( filterMap, mediaInfo ) )
            continue;

        auto premiereDate = mediaInfo->premiereDate();
        if ( premiereDate > QDate::currentDate() )
            continue;

        episodes.push_back( mediaInfo );
    }

    episodes.sort(   //
        []( const std::shared_ptr< CMediaData > &lhs, const std::shared_ptr< CMediaData > &rhs )   //
        {
            return lhs->name() < rhs->name();   //
        } );
    if ( episodes.empty() )
    {
        slotAddToLog( EMsgType::eStatus, "No missing episodes found" );
    }
    else
    {
        slotAddToLog( EMsgType::eStatus, QString( "There were %1 missing episodes found." ).arg( episodes.size() ) );
        QLocale locale;
        bool launchedOK = true;
        for ( auto &&ii : episodes )
        {
            auto url = ii->getDefaultSearchURL( fSettings );
            auto msg = QString( R"__(    %1 - %2 - %3)__" ).arg( ii->name() ).arg( locale.toString( ii->premiereDate() ) ).arg( url.toString( QUrl::FullyEncoded ) );

            slotAddToLog( EMsgType::eStatus, msg );

            if ( launchedOK && fLaunchMissing )
            {
                auto msg = NTowel42Utils::openUrl( url );
                if ( msg.has_value() )
                {
                    addToLog( EMsgType::eError, msg.value() );
                    launchedOK = false;
                }
            }
        }
    }
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
        fStatusText = QString( "Invalid mode '%1'" ).arg( mode );
        fAOK = false;
        return false;
    }
    fAOK = true;
    return true;
}
