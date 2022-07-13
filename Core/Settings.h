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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <QDialog>
#include <QString>
#include <memory>
#include <tuple>
#include <QUrl>
namespace Ui
{
    class CSettings;
}

class CSettings
{
public:
    CSettings();
    CSettings( const QString & fileName, QWidget * parentWidget );

    virtual ~CSettings();

    QString fileName() const { return fFileName; }
    bool load( QWidget * parent );
    bool load( const QString & fileName, QWidget * parent );

    bool save( QWidget * parent );
    bool maybeSave( QWidget * parent );

    QUrl getServerURL( bool lhs )
    {
        return lhs ? getLHSURL() : getRHSURL();
    }

    QUrl getLHSURL() const
    {
        return getUrl( true );
    }

    QUrl getRHSURL() const
    {
        return getUrl( false );
    }

    QString lhsURL() const { return fLHSServer.first; }
    void setLHSURL( const QString & url );

    QString lhsAPI() const { return fLHSServer.second; }
    void setLHSAPI( const QString & api );

    QString rhsURL() const { return fRHSServer.first; }
    void setRHSURL( const QString & url );

    QString rhsAPI() const { return fRHSServer.second; }
    void setRHSAPI( const QString & api );

    bool changed() const { return fChanged; }
    void reset()
    {
        fLHSServer = {};
        fRHSServer = {};
        fChanged = false;
        fFileName.clear();
    }

    QString getServerName( bool lhs );
    bool onlyShowSyncableUsers() { return fOnlyShowSyncableUsers; };
    void setOnlyShowSyncableUsers( bool value ) { fOnlyShowSyncableUsers = value; };

    bool onlyShowMediaWithDifferences() { return fOnlyShowMediaWithDifferences; };
    void setOnlyShowMediaWithDifferences( bool value ) { fOnlyShowMediaWithDifferences = value; };

    void addRecentProject( const QString & fileName );
    QStringList recentProjectList() const;
private:
    QUrl getUrl( bool lhs ) const;

    void updateValue( QString & lhs, const QString & rhs );

    QString fFileName;
    std::pair< QString, QString > fLHSServer;
    std::pair< QString, QString > fRHSServer;

    bool fOnlyShowSyncableUsers{ true };
    bool fOnlyShowMediaWithDifferences{ true };
    bool fChanged{ false };
};

#endif 
