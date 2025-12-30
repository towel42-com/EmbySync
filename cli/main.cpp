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

#include "Version.h"
#include <iostream>
#include <QCoreApplication>
#include <QCommandLineParser>

#include <conio.h>

int main( int argc, char **argv )
{
    QCoreApplication appl( argc, argv );
    NVersion::setupApplication( appl, true );

    int retVal = -1;
    do
    {
        auto mainObj = std::make_shared< CMainObj >( appl );
        QObject::connect( mainObj.get(), &CMainObj::sigExit, &appl, &QCoreApplication::exit );

        auto status = mainObj->status();
        if ( status.has_value() )
        {
            ( status.value().first ? std::cout : std::cerr ) << mainObj->statusText().toStdString() << "\n";
            return status.value().second;
        }

        mainObj->run();

        retVal = appl.exec();
        QString msg = "Press 'R' to re-run, otherwise press any key to close this window...";
        std::cout << msg.toStdString();
        auto ch = _getche();
        if ( ( ch == 'Y' ) || ( ch == 'y' ) || ( ch == 'R' ) || ( ch == 'r' ) )
        {
            std::cout << '\r' << QString( msg.size() + 1, '=' ).toStdString() << "\n";
            continue;
        }
        break;
    }
    while ( true );

    return retVal;
}
