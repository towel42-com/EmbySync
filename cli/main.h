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


#ifndef __MAIN_H
#define __MAIN_H

#include <QObject>
#include <list>
#include <memory>

class CSettings;
class CSyncSystem;
class CUserData;
class CMain : public QObject
{
    Q_OBJECT;
public:
    CMain( const QString & settingsFile, const QString & usersRegEx, QObject * parent = nullptr );

    void run();
Q_SIGNALS:
    void sigExit( int exitCode );
public Q_SLOTS:
    void slotAddToLog( const QString & msg );
    void slotLoadingUsersFinished();
    void slotProcessNextUser();
    void slotUserMediaCompletelyLoaded();
    void slotFinishedCheckingForMissingMedia();
    void slotProcessingFinished( const QString & userName );
private:
    std::shared_ptr< CSettings > fSettings;
    std::shared_ptr< CSyncSystem > fSyncSystem;

    QString fSettingsFile;
    QString fUserRegEx;

    std::list< std::shared_ptr< CUserData > > fUsersToSync;
};

#endif