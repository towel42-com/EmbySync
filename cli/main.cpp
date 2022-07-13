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

#include "Core/Settings.h"
#include "Core/SyncSystem.h"

#include "Version.h"

#include <QCoreApplication>

int main( int argc, char ** argv )
{
    QCoreApplication appl( argc, argv );
    appl.setApplicationName( QString::fromStdString( NVersion::APP_NAME ) );
    appl.setApplicationVersion( QString::fromStdString( NVersion::getVersionString( true ) ) );
    appl.setOrganizationName( QString::fromStdString( NVersion::VENDOR ) );
    appl.setOrganizationDomain( QString::fromStdString( NVersion::HOMEPAGE ) );
    appl.setOrganizationDomain( "github.com/towel42-com/EmbySync" ); // QString::fromStdString( NVersion::HOMEPAGE ) );

    auto settings = std::make_shared< CSettings >();
    auto dataFile = std::make_shared< CSyncSystem >( settings );

    return appl.exec();
}
