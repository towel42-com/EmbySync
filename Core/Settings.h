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

#include <QString>
#include <QColor>
#include <memory>
#include <tuple>
#include <QUrl>

class QWidget;
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
    bool load( const QString & fileName, std::function<void( const QString & title, const QString & msg )> errorFunc );

    bool save();

    bool save( QWidget * parent );
    bool maybeSave( QWidget * parent );
    bool save( QWidget * parent, std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc );
    bool save( std::function<void( const QString & title, const QString & msg )> errorFunc );

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

    QUrl getUrl( bool lhs ) const;

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

    QColor mediaSourceColor( bool forBackground = true ) const;
    void setMediaSourceColor( const QColor & color );

    QColor mediaDestColor( bool forBackground = true ) const;
    void setMediaDestColor( const QColor & color );

    QColor dataMissingColor( bool forBackground = true ) const;
    void setDataMissingColor( const QColor & color );

    int maxItems() const { return fMaxItems; }
    void setMaxItems( int maxItems );

    QString getServerName( bool lhs );
    bool onlyShowSyncableUsers() { return fOnlyShowSyncableUsers; };
    void setOnlyShowSyncableUsers( bool value ) { fOnlyShowSyncableUsers = value; };

    bool onlyShowMediaWithDifferences() { return fOnlyShowMediaWithDifferences; };
    void setOnlyShowMediaWithDifferences( bool value ) { fOnlyShowMediaWithDifferences = value; };

    bool showMediaWithIssues() { return fShowMediaWithIssues; };
    void setShowMediaWithIssues( bool value ) { fShowMediaWithIssues = value; };

    
    void addRecentProject( const QString & fileName );
    QStringList recentProjectList() const;
private:
    QColor getColor( const QColor & clr, bool forBackground /*= true */ ) const;
    bool maybeSave( QWidget * parent, std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc );

    template< typename T >
    void updateValue( T & lhs, const T & rhs )
    {
        if ( lhs != rhs )
        {
            lhs = rhs;
            fChanged = true;
        }
    }


    QString fFileName;
    std::pair< QString, QString > fLHSServer;
    std::pair< QString, QString > fRHSServer;

    QColor fMediaSourceColor{ "yellow" };
    QColor fMediaDestColor{ "yellow" };
    QColor fMediaDataMissingColor{ "red" };
    int fMaxItems{ 0 };

    bool fOnlyShowSyncableUsers{ true };
    bool fOnlyShowMediaWithDifferences{ true };
    bool fShowMediaWithIssues{ false };
    bool fChanged{ false };
};

#endif 
