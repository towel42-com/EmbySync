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

#ifndef _SERVERINFO_H
#define _SERVERINFO_H

#include <QString>
#include <QUrl>
#include <utility>
#include <QObject>
#include <QIcon>

class QJsonObject;

class CServerInfo : public QObject
{
    Q_OBJECT;
public:
    CServerInfo() = default;
    CServerInfo( const QString & name, const QString & url, const QString & apiKey, bool enabled );
    CServerInfo( const QString & name );

    bool operator==( const CServerInfo & rhs ) const;
    bool operator!=( const CServerInfo & rhs ) const
    {
        return !operator==( rhs );
    }

    bool setUrl( const QString & url );
    QString url( bool clean=false ) const;
    QUrl getUrl() const; // includes the query for apikey
    QUrl getUrl( const QString & extraPath, const std::list< std::pair< QString, QString > > & queryItems ) const;

    QString displayName( bool verbose=false ) const; // returns the name, if empty returns the fqdn, if the same fqdn is used more than once, it use fqdn:port, verbose includes the url
    bool setDisplayName( const QString & name, bool generated );
    void autoSetDisplayName( bool usePort );

    bool displayNameGenerated() const { return fName.second; }

    QString keyName() const;// getUrl().toString()

    bool canSync() const;

    QString apiKey() const { return fAPIKey; }
    bool setAPIKey( const QString & key );

    QJsonObject toJson() const;
    static std::shared_ptr< CServerInfo > fromJson( const QJsonObject & obj, QString & errorMsg );

    bool isEnabled() const { return canSync() && fIsEnabled; }
    bool setIsEnabled( bool isEnabled );

    void update( const QJsonObject & serverData );

    QIcon icon() const { return fIcon; }
    void setIcon( const QByteArray & data, const QString & type );

Q_SIGNALS:
    void sigServerInfoChanged();
private:
    std::pair< QString, bool > fName; // may be automatically generated or not
    mutable QString fKeyName;
    QString fURL;
    QString fAPIKey;
    bool fIsEnabled{ true };

    // from server info
    QIcon fIcon;
    QString fLocalAddress;
    std::list< QString > fLocalAddresses;
    QString fWANAddress;
    std::list< QString > fWANAddresses;

    QString fServerName;
    QString fVersion;
    QString fID;
};

#endif 
