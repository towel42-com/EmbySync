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

#ifndef __USERDATA_H
#define __USERDATA_H

#include <QString>
#include <list>
#include <optional>
#include <QImage>
#include <QDateTime>
#include <memory>
#include <map>

class CMediaData;
class CSettings;

struct SUserServerData
{
    bool isValid() const;
    bool userDataEqual( const SUserServerData & rhs ) const;

    QDateTime latestAccess() const;
    QJsonObject userDataJSON() const;


    QString fName;
    QString fUserID;
    QString fConnectedIDOnServer;

    QImage fImage;
    std::pair< QString, double > fImageTagInfo;
    QDateTime fDateCreated;
    QDateTime fLastLoginDate;
    QDateTime fLastActivityDate;
};
bool operator==( const SUserServerData & lhs, const SUserServerData & rhs );
inline bool operator!=( const SUserServerData & lhs, const SUserServerData & rhs )
{
    return !operator==( lhs, rhs );
}


class CUserData
{
public:
    CUserData( const QString & serverName, const QString & userID );

    QString connectedID() const { return fConnectedID; }
    QString connectedID( const QString & serverName ) const;
    void setConnectedID( const QString & serverName, const QString & connectedID );

    bool isValid() const;
    QString name( const QString & serverName ) const;
    void setName( const QString & serverName, const QString & name );
    
    QString userName( const QString & serverName ) const;
    QString allNames() const;
    QString sortName( std::shared_ptr< CSettings > settings ) const;

    QStringList missingServers() const;

    bool isUser( const QString & name ) const;
    bool isUser( const QRegularExpression & regEx ) const;
    bool isUser( const QString & serverName, const QString & userID ) const;

    bool connectedIDNeedsUpdate() const;

    QString getUserID( const QString & serverName ) const;
    void setUserID( const QString & serverName, const QString & id );

    void setDateCreated( const QString & serverName, const QDateTime & dateTS );
    void setLastActivityDate( const QString & serverName, const QDateTime & dateTS );
    void setLastLoginDate( const QString & serverName, const QDateTime & dateTS );

    bool hasImageTagInfo( const QString & serverName ) const;

    std::pair< QString, double > getImageTagInfo( const QString & serverName ) const;
    void setImageTagInfo( const QString & serverName, const QString & tag, double ratio );
    
    bool allUserNamesTheSame() const;
    bool allConnectIDTheSame() const;
    bool allIconsTheSame() const;
    bool allLastActivityDateSame() const;
    bool allLastLoginDateSame() const;

    bool needsUpdating( const QString & serverName ) const;
    std::shared_ptr<SUserServerData> newestServerInfo() const;

    QImage globalAvatar() const; // when all servers use the same image
    QImage getAvatar( const QString & serverName, bool useUnsetIcon=false ) const;
    void setAvatar( const QString & serverName, int serverCnt, const QImage & image );
    QImage anyAvatar() const; // first avatar non-null

    QDateTime getDateCreated( const QString & serverName ) const;
    QDateTime getLastActivityDate( const QString & serverName ) const;
    QDateTime getLastLoginDate( const QString & serverName ) const;

    bool canBeSynced() const;
    bool onServer( const QString & serverName ) const;

    QIcon getDirectionIcon( const QString & serverName ) const;
    bool isValidForServer( const QString & serverName ) const;
    bool validUserDataEqual() const;

    std::shared_ptr< SUserServerData > getServerInfo( const QString & serverName ) const;
private:

    template < typename T >
    std::optional< T > allSame( std::function < T( std::shared_ptr< SUserServerData > ) > getValue ) const
    {
        std::optional< T > prev;
        for ( auto && ii : fInfoForServer )
        {
            if ( prev.has_value() )
            {
                if ( prev.value() != getValue( ii.second ) )
                    return {};
            }
            else
                prev = getValue( ii.second );
        }
        return prev;
    }
    void updateCanBeSynced();
    void checkAllAvatarsTheSame( int serverNum );

    std::shared_ptr< SUserServerData > getServerInfo( const QString & serverName, bool addIfMissing );
    bool isMatch( const QRegularExpression & regEx, const QString & value ) const;

    QString fConnectedID;
    bool fCanBeSynced{ false };

    mutable std::optional< QImage > fGlobalImage; // when all avatars are the same on the server
    std::map< QString, std::shared_ptr< SUserServerData > > fInfoForServer; // serverName to Info
};
#endif 
