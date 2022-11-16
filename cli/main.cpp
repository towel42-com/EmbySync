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

#include "Version.h"
#include <iostream>
#include <QCoreApplication>
#include <QCommandLineParser>

void showVersion()
{
    std::cout << NVersion::APP_NAME.toStdString() << " - " << NVersion::getVersionString( true ).toStdString() << "\n";
}

int main( int argc, char ** argv )
{
    QCoreApplication appl( argc, argv );
    NVersion::setupApplication( appl, true );

    QCommandLineParser parser;
    parser.setApplicationDescription( NVersion::APP_NAME + " CLI - a tool to sync two emby servers" );
    auto helpOption = parser.addHelpOption();
    auto versionOption = parser.addVersionOption();

    auto settingsFileOption = QCommandLineOption( QStringList() << "settings" << "s", "The settings json file", "Settings file" );
    parser.addOption( settingsFileOption );

    auto modeOption = QCommandLineOption( QStringList() << "mode" << "m", "The particular mode of operation you wish to use valid values are check_missing|sync", "Mode" );
    parser.addOption( modeOption );

    auto selectedServerOption = QCommandLineOption( QStringList() << "selected_server", "The server name you wish to use as the primary server to use as the source server (required for check_missing)", "Selected Server" );
    parser.addOption( selectedServerOption );

    auto dateStr = QDate::currentDate().toString( "MM/dd/yyyy" );
    auto minDateOption = QCommandLineOption( QStringList() << "min_date", QString( "The oldest premier date to check if its missing (default %1)" ).arg( dateStr ), "min date", dateStr );
    parser.addOption( minDateOption );

    auto maxDateOption = QCommandLineOption( QStringList() << "max_date", QString( "The latest premier date to check if its missing (default %1)" ).arg( dateStr ), "max date", dateStr );
    parser.addOption( maxDateOption );

    auto quietOption = QCommandLineOption( QStringList() << "quiet" << "q", QString( "Minimize text output" ), "" );
    parser.addOption( quietOption );

    parser.process( appl );

    if ( !parser.unknownOptionNames().isEmpty() )
    {
        showVersion();
        std::cerr << "The following options were set and are unknown:\n";
        for ( auto && ii : parser.unknownOptionNames() )
            std::cerr << "    " << ii.toStdString() << "\n";
        parser.showHelp();
        return -1;
    }

    if ( parser.isSet( helpOption ) )
    {
        showVersion();
        parser.showHelp();
        return 0;
    }

    if ( !parser.isSet( modeOption ) )
    {
        showVersion();
        parser.showHelp();
        return -1;
    }

    if ( !parser.isSet( quietOption ) )
        showVersion();

    auto mode = parser.value( modeOption ).toLower();
    if ( parser.isSet( versionOption ) )
    {
        return 0;
    }

    if ( !parser.isSet( settingsFileOption ) )
    {
        std::cerr << "--settings must be set\n";
        std::cerr << parser.helpText().toStdString() << "\n";
        return -1;
    }

    auto settingsFile = parser.value( settingsFileOption );
    auto mainObj = std::make_shared< CMainObj >( settingsFile, mode );
    QObject::connect( mainObj.get(), &CMainObj::sigExit, &appl, &QCoreApplication::exit );

    if ( parser.isSet( selectedServerOption )  )
        mainObj->setSelectiveProcesssServer( parser.value( selectedServerOption ) );

    mainObj->setMinimumDate( parser.value( minDateOption ) );
    mainObj->setMaximumDate( parser.value( maxDateOption ) );
    mainObj->setQuiet( parser.isSet( quietOption ) );
    if ( !mainObj->aOK() )
    {
        std::cerr << mainObj->errorString().toStdString() << "\n";
        parser.showHelp();
        return -1;
    }

    mainObj->run();

    if ( !mainObj->aOK() )
    {
        std::cerr << mainObj->errorString().toStdString() << "\n";
        parser.showHelp();
        return -1;
    }

    int retVal = appl.exec();
    return retVal;
}
