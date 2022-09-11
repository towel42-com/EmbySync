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

struct SUserServerData;

class CUserData
{
public:
    CUserData( const QString & serverName, const QJsonObject & userObj );
    void loadFromJSON( const QString & serverName, const QJsonObject & userObj );

    QString connectedID() const { return fConnectedID.second; }
    QString connectedIDType() const { return fConnectedID.first; }

    QString connectedID( const QString & serverName ) const;
    QString connectedIDType( const QString & serverName ) const;

    void setConnectedID( const QString & serverName, const QString & idType, const QString & connectedID );

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

    bool hasAvatarInfo( const QString & serverName ) const;

    bool allUserNamesTheSame() const;
    bool allConnectIDTheSame() const;
    bool allConnectIDTypeTheSame() const;
    bool allIconInfoTheSame() const;
    bool allDateCreatedSame() const;
    bool allLastActivityDateSame() const;
    bool allLastLoginDateSame() const;
    bool allEnableAutoLoginTheSame() const;
    bool allPrefixTheSame() const;
    bool allAudioLanguagePreferenceTheSame() const;
    bool allPlayDefaultAudioTrackTheSame() const;
    bool allSubtitleLanguagePreferenceTheSame() const;
    bool allDisplayMissingEpisodesTheSame() const;
    bool allSubtitleModeTheSame() const;
    bool allEnableLocalPasswordTheSame() const;
    bool allOrderedViewsTheSame() const;
    bool allLatestItemsExcludesTheSame() const;
    bool allMyMediaExcludesTheSame() const;
    bool allHidePlayedInLatestTheSame() const;
    bool allRememberAudioSelectionsTheSame() const;
    bool allRememberSubtitleSelectionsTheSame() const;
    bool allEnableNextEpisodeAutoPlayTheSame() const;
    bool allResumeRewindSecondsTheSame() const;
    bool allIntroSkipModeTheSame() const;


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

    bool needsUpdating( const QString & serverName ) const;
    std::shared_ptr<SUserServerData> newestServerInfo() const;

    QImage globalAvatar() const; // when all servers use the same image

    std::tuple< QString, double, QImage > getAvatarInfo( const QString & serverName ) const;
    void setAvatarInfo( const QString & serverName, const QString & tag, double ratio );

    QImage getAvatar( const QString & serverName, bool useUnsetIcon=false ) const;
    void setAvatar( const QString & serverName, int serverCnt, const QImage & image );
    QImage anyAvatar() const; // first avatar non-null

    QDateTime getDateCreated( const QString & serverName ) const;
    QDateTime getLastActivityDate( const QString & serverName ) const;
    QDateTime getLastLoginDate( const QString & serverName ) const;
    
    QString prefix( const QString & serverName ) const;
    bool enableAutoLogin( const QString & serverName ) const;
    QString audioLanguagePreference( const QString & serverName ) const;
    bool playDefaultAudioTrack( const QString & serverName ) const;
    QString subtitleLanguagePreference( const QString & serverName ) const;
    bool displayMissingEpisodes( const QString & serverName ) const;
    QString subtitleMode( const QString & serverName ) const;
    bool enableLocalPassword( const QString & serverName ) const;
    QStringList orderedViews( const QString & serverName ) const;
    QStringList latestItemsExcludes( const QString & serverName ) const;
    QStringList myMediaExcludes( const QString & serverName ) const;
    bool hidePlayedInLatest( const QString & serverName ) const;
    bool rememberAudioSelections( const QString & serverName ) const;
    bool rememberSubtitleSelections( const QString & serverName ) const;
    bool enableNextEpisodeAutoPlay( const QString & serverName ) const;
    int resumeRewindSeconds( const QString & serverName ) const;
    QString introSkipMode( const QString & serverName ) const;

    bool canBeSynced() const;
    bool onServer( const QString & serverName ) const;

    QIcon getDirectionIcon( const QString & serverName ) const;
    bool isValidForServer( const QString & serverName ) const;
    bool validUserDataEqual() const;

    std::shared_ptr< SUserServerData > getServerInfo( const QString & serverName ) const;
private:
    void updateCanBeSynced();
    void updateConnectedID();
    void checkAllAvatarsTheSame( int serverNum );

    std::shared_ptr< SUserServerData > getServerInfo( const QString & serverName, bool addIfMissing );
    bool isMatch( const QRegularExpression & regEx, const QString & value ) const;

    mutable QString fSortKey;
    std::pair< QString, QString > fConnectedID; // type, ID
    bool fCanBeSynced{ false };

    mutable std::optional< QImage > fGlobalImage; // when all avatars are the same on the server
    std::map< QString, std::shared_ptr< SUserServerData > > fInfoForServer; // serverName to Info
};
#endif
