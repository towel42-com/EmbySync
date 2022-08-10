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

    auto usersOption = QCommandLineOption( QStringList() << "users" << "u", "A regex for users to sync (Default .* for all)", "Users to sync" );
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

    auto users = parser.isSet( usersOption ) ? parser.value( usersOption ) : ".*";
    auto settingsFile = parser.value( settingsFileOption );
    auto mainObj = std::make_shared< CMainObj >( settingsFile, users );
    if ( !mainObj->aOK() )
    {
        return -1;
    }
    QObject::connect( mainObj.get(), &CMainObj::sigExit, &appl, &QCoreApplication::exit );

    if ( parser.isSet( forceLeftOption ) || parser.isSet( forceRightOption ) )
        mainObj->setForceProcessing( parser.isSet( forceLeftOption ), parser.isSet( forceRightOption ) );

    mainObj->run();

    int retVal = appl.exec();
    return retVal;
}
