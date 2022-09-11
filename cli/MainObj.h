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


#ifndef __MAINOBJ_H
#define __MAINOBJ_H

#include <QObject>
#include <QRegularExpression>
#include <list>
#include <memory>
#include <tuple>

class CSettings;
class CSyncSystem;
class CUserData;
class CMediaModel;
class CUsersModel;
class CMainObj : public QObject
{
    Q_OBJECT;
public:
    CMainObj( const QString & settingsFile, QObject * parent = nullptr );

    void run();
    void setSelectiveProcesssServer( const QString & selectedServer )
    {
        this->fSelectedServerToProcess = selectedServer;
    }
    bool aOK() const
    {
        return fAOK;
    }
Q_SIGNALS:
    void sigExit( int exitCode );
public Q_SLOTS:
    void slotAddToLog( int msgType, const QString & msg );
    void slotLoadingUsersFinished();
    void slotProcessNextUser();
    void slotUserMediaCompletelyLoaded();
    void slotProcessingFinished( const QString & userName );
    void slotProcessMedia();
private:
    std::shared_ptr< CSettings > fSettings;
    std::shared_ptr< CSyncSystem > fSyncSystem;

    std::shared_ptr< CMediaModel > fMediaModel;
    std::shared_ptr< CUsersModel > fUsersModel;

    QString fSettingsFile;
    QRegularExpression fUserRegExp;
    bool fAOK{ false };

    std::tuple< int, QString, QString > fCurrentProgress{ 0, QString(), QString() };

    std::list< std::shared_ptr< CUserData > > fUsersToSync;
    QString fSelectedServerToProcess;
};

#endif