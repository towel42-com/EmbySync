// The MIT License( MIT )
//
// Copyright( c ) 2022 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software iRHS
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

#include "MediaData.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QObject>
#include <chrono>

QJsonObject SMediaUserData::userDataJSON() const
{
    QJsonObject obj;
    obj[ "IsFavorite" ] = fIsFavorite;
    obj[ "Played" ] = fPlayed;
    obj[ "PlayCount" ] = static_cast<int64_t>( fPlayCount );
    if ( fLastPlayedDate.isNull() )
        obj[ "LastPlayedDate" ] = QJsonValue::Null;
    else
        obj[ "LastPlayedDate" ] = fLastPlayedDate.toUTC().toString( Qt::ISODateWithMs );

    auto ticks = static_cast<int64_t>( fPlaybackPositionTicks );
    if ( fPlaybackPositionTicks >= static_cast<uint64_t>( std::numeric_limits< int64_t >::max() ) )
        ticks = std::numeric_limits< int64_t >::max() - 1;

    obj[ "PlaybackPositionTicks" ] = static_cast<int64_t>( ticks );

    return obj;
}

void SMediaUserData::loadUserDataFromJSON( const QJsonObject & userDataObj )
{
    //qDebug() << QJsonDocument( userDataObj ).toJson();

    fIsFavorite = userDataObj[ "IsFavorite" ].toBool();
    fLastPlayedDate = userDataObj[ "LastPlayedDate" ].toVariant().toDateTime();
    //qDebug() << fLastPlayedDate.toString();
    //qDebug() << fLastPlayedDate;
    fPlayCount = userDataObj[ "PlayCount" ].toVariant().toLongLong();
    fPlaybackPositionTicks = userDataObj[ "PlaybackPositionTicks" ].toVariant().toLongLong();
    fPlayed = userDataObj[ "Played" ].toVariant().toBool();
}

QStringList CMediaData::getHeaderLabels()
{
    return QStringList()
        << QObject::tr( "Name" )
        << QObject::tr( "ID" )
        << QObject::tr( "Is Favorite?" )
        << QObject::tr( "Played?" )
        << QObject::tr( "Last Played" )
        << QObject::tr( "Play Count" )
        << QObject::tr( "Play Position" )
        ;
}

std::function< QString( uint64_t ) > CMediaData::sMSecsToStringFunc;
void CMediaData::setMSecsToStringFunc( std::function< QString( uint64_t ) > func )
{
    sMSecsToStringFunc = func;
}

std::function< QString( uint64_t ) > CMediaData::mecsToStringFunc()
{
    return sMSecsToStringFunc;
}

CMediaData::CMediaData( const QString & name, const QString & type ) :
    fName( name ),
    fType( type ),
    fLHSUserMediaData( std::make_shared< SMediaUserData >() ),
    fRHSUserMediaData( std::make_shared< SMediaUserData >() )
{
}

QString CMediaData::name() const
{
    return fName;
}

QString CMediaData::computeName( QJsonObject & media )
{
    auto name = media[ "Name" ].toString();
    QString retVal = name;
    if ( media[ "Type" ] == "Episode" )
    {
        //auto tmp = QJsonDocument( media );
        //qDebug() << tmp.toJson();

        auto series = media[ "SeriesName" ].toString();
        auto season = media[ "SeasonName" ].toString();
        auto pos = season.lastIndexOf( ' ' );
        if ( pos != -1 )
        {
            bool aOK;
            auto seasonNum = season.mid( pos + 1 ).toInt( &aOK );
            if ( aOK )
                season = QString( "S%1" ).arg( seasonNum, 2, 10, QChar( '0' ) );
        }

        auto episodeNum = media[ "IndexNumber" ].toInt();
        retVal = QString( "%1 - %2E%3" ).arg( series ).arg( season ).arg( episodeNum, 2, 10, QChar( '0' ) );
        auto episodeName = media[ "EpisodeTitle" ].toString();
        if ( !episodeName.isEmpty() )
            retVal += QString( " - %1" ).arg( episodeName );
        if ( !name.isEmpty() )
            retVal += QString( " - %1" ).arg( name );
    }
    return retVal;
}

bool CMediaData::isMissing( bool isLHS ) const
{
    return isLHS ? fLHSUserMediaData->fMediaID.isEmpty() : fRHSUserMediaData->fMediaID.isEmpty();
}

bool CMediaData::hasMissingInfo( bool checkForProviders ) const
{
    if ( isMissing( true ) || isMissing( false ) )
        return true;
    if ( checkForProviders && getProviders().empty() )
        return true;
    return false;
}

void CMediaData::loadUserDataFromJSON( const QJsonObject & media, bool isLHSServer  )
{
    //auto tmp = QJsonDocument( media );
    //qDebug() << tmp.toJson();

    auto externalUrls = media[ "ExternalUrls" ].toArray();
    for ( auto && ii : externalUrls )
    {
        auto urlObj = ii.toObject();
        auto name = urlObj[ "Name" ].toString();
        auto url = urlObj[ "Url" ].toString();
        fExternalUrls[ name ] = url;
    }


    auto userDataObj = media[ "UserData" ].toObject();

    //auto tmp = QJsonDocument( userDataObj );
    //qDebug() << tmp.toJson();

    if ( isLHSServer )
    {
        fLHSUserMediaData->loadUserDataFromJSON( userDataObj );
    }
    else
    {
        fRHSUserMediaData->loadUserDataFromJSON( userDataObj );
    }


    auto providerIDsObj = media[ "ProviderIds" ].toObject();
    for ( auto && ii = providerIDsObj.begin(); ii != providerIDsObj.end(); ++ii )
    {
        auto providerName = ii.key();
        auto providerID = ii.value().toString();
        addProvider( providerName, providerID );
    }

}

QString CMediaData::externalUrlsText() const
{
    auto retVal = QString( "External Urls:</br>\n<ul>\n%1\n</ul>" );

    QStringList externalUrls;
    for ( auto && ii : fExternalUrls )
    {
        auto curr = QString( R"(<li>%1 - <a href="%2">%2</a></li>)" ).arg( ii.first ).arg( ii.second );
        externalUrls << curr;
    }
    retVal = retVal.arg( externalUrls.join( "\n" ) );
    return retVal;
}

bool CMediaData::userDataEqual() const
{
    return fLHSUserMediaData->userDataEqual( *fRHSUserMediaData );
}

bool operator==( const SMediaUserData & lhs, const SMediaUserData & rhs )
{
    return lhs.userDataEqual( rhs );
}

uint64_t SMediaUserData::playbackPositionMSecs() const
{
    return fPlaybackPositionTicks / 10000;
}

QString SMediaUserData::playbackPosition() const
{
    auto playbackMS = playbackPositionMSecs();
    if ( playbackMS == 0 )
        return {};

    if ( CMediaData::mecsToStringFunc() )
        return CMediaData::mecsToStringFunc()( playbackMS );
    return QString::number( playbackMS );
}

QTime SMediaUserData::playbackPositionTime() const
{
    auto playbackMS = playbackPositionMSecs();
    return QTime::fromMSecsSinceStartOfDay( playbackMS );
}

void SMediaUserData::setPlaybackPosition( const QTime & time )
{
    setPlaybackPositionMSecs( 10000ULL * time.msecsSinceStartOfDay() );
}

void SMediaUserData::setPlaybackPositionMSecs( uint64_t msecs )
{
    fPlaybackPositionTicks = ( 10000ULL * msecs );
}

bool SMediaUserData::userDataEqual( const SMediaUserData & rhs ) const
{
    auto equal = true;
    equal = equal && fIsFavorite == rhs.fIsFavorite;
    equal = equal && fPlayed == rhs.fPlayed;
    if ( !fLastPlayedDate.isNull() && !rhs.fLastPlayedDate.isNull() )
        equal = equal && fLastPlayedDate == rhs.fLastPlayedDate;
    equal = equal && fPlayCount == rhs.fPlayCount;
    equal = equal && fPlaybackPositionTicks == rhs.fPlaybackPositionTicks;
    return equal;
}



bool CMediaData::hasProviderIDs() const
{
    return !fProviders.empty();
}

QString CMediaData::getProviderList() const
{
    QStringList retVal;
    for ( auto && ii : fProviders )
    {
        retVal << ii.first.toLower() + "." + ii.second.toLower();
    }
    return retVal.join( "," );
}

bool CMediaData::isPlayed( bool lhs ) const
{
    return lhs ? fLHSUserMediaData->fPlayed : fRHSUserMediaData->fPlayed;
}

uint64_t CMediaData::playCount( bool lhs ) const
{
    return lhs ? fLHSUserMediaData->fPlayCount : fRHSUserMediaData->fPlayCount;
}

bool CMediaData::isFavorite( bool lhs ) const
{
    return lhs ? fLHSUserMediaData->fIsFavorite : fRHSUserMediaData->fIsFavorite;
}

QDateTime CMediaData::lastPlayed( bool lhs ) const
{
    return lhs ? fLHSUserMediaData->fLastPlayedDate : fRHSUserMediaData->fLastPlayedDate;
}

// 1 tick = 10000 ms
uint64_t CMediaData::playbackPositionTicks( bool lhs ) const
{
    return lhs ? fLHSUserMediaData->fPlaybackPositionTicks : fRHSUserMediaData->fPlaybackPositionTicks;
}

// stored in ticks
// 1 tick = 10000 ms
uint64_t CMediaData::playbackPositionMSecs( bool lhs ) const
{
    return lhs ? fLHSUserMediaData->playbackPositionMSecs() : fRHSUserMediaData->playbackPositionMSecs();
}

QString CMediaData::playbackPosition( bool lhs ) const
{
    return lhs ? fLHSUserMediaData->playbackPosition() : fRHSUserMediaData->playbackPosition();
}

QTime CMediaData::playbackPositionTime( bool lhs ) const
{
    return lhs ? fLHSUserMediaData->playbackPositionTime() : fRHSUserMediaData->playbackPositionTime();
}


bool CMediaData::bothPlayed() const
{
    return ( isPlayed( true ) != 0 ) && ( isPlayed( false ) != 0 );
}

bool CMediaData::playbackPositionTheSame() const
{
    return playbackPositionTicks( true ) == playbackPositionTicks( false );
}

bool CMediaData::bothFavorites() const
{
    return isFavorite( true ) && isFavorite( false );
}

bool CMediaData::lastPlayedTheSame() const
{
    if ( fLHSUserMediaData->fLastPlayedDate.isNull() || fRHSUserMediaData->fLastPlayedDate.isNull() )
        return true;
    return fLHSUserMediaData->fLastPlayedDate == fRHSUserMediaData->fLastPlayedDate;
}

QUrlQuery CMediaData::getSearchForMediaQuery() const
{
    QUrlQuery query;

    query.addQueryItem( "IncludeItemTypes", "Movie,Episode,Video" );
    query.addQueryItem( "AnyProviderIdEquals", getProviderList() );
    query.addQueryItem( "SortBy", "SortName" );
    query.addQueryItem( "SortOrder", "Ascending" );
    query.addQueryItem( "Recursive", "True" );

    return query;
}

QString CMediaData::getProviderID( const QString & provider )
{
    auto pos = fProviders.find( provider );
    if ( pos == fProviders.end() )
        return {};
    return ( *pos ).second;
}

std::map< QString, QString > CMediaData::getProviders( bool addKeyIfEmpty /*= false */ ) const
{
    auto retVal = fProviders;
    if ( addKeyIfEmpty && retVal.empty() )
    {
        retVal[ fType ] = fName;
    }

    return std::move( retVal );
}

void CMediaData::addProvider( const QString & providerName, const QString & providerID )
{
    fProviders[ providerName ] = providerID;
}

void CMediaData::setMediaID( const QString & id, bool isLHS )
{
    if ( isLHS )
        fLHSUserMediaData->fMediaID = id;
    else
        fRHSUserMediaData->fMediaID = id;
}

QString CMediaData::getMediaID( bool isLHS ) const
{
    if ( isLHS )
        return fLHSUserMediaData->fMediaID;
    else
        return fRHSUserMediaData->fMediaID;
}

bool CMediaData::beenLoaded( bool isLHS ) const
{
    return isLHS ? fLHSUserMediaData->fBeenLoaded : fRHSUserMediaData->fBeenLoaded;
}

bool CMediaData::rhsNeedsUpdating() const
{
    //qDebug() << fLHSServer->fLastPlayedDate;
    //qDebug() << fRHSServer->fLastPlayedDate;
    return fLHSUserMediaData->fLastPlayedDate > fRHSUserMediaData->fLastPlayedDate;
}

bool CMediaData::lhsNeedsUpdating() const
{
    //qDebug() << fLHSServer->fLastPlayedDate;
    //qDebug() << fRHSServer->fLastPlayedDate;
    return fRHSUserMediaData->fLastPlayedDate > fLHSUserMediaData->fLastPlayedDate;
}

QString CMediaData::getDirectionLabel() const
{
    QString retVal;
    if ( hasMissingInfo() )
        retVal = "<NA>";
    else if ( userDataEqual() )
        retVal = "   =   ";
    else if ( rhsNeedsUpdating() )
        retVal = ">>>";
    else if ( lhsNeedsUpdating() )
        retVal = "<<<";
    else
        retVal = QString();

    return retVal;
}

int CMediaData::getDirectionValue() const
{
    int retVal{ 0 };
    if ( hasMissingInfo() )
        retVal = 0;
    else if ( userDataEqual() )
        retVal = 2;
    else if ( rhsNeedsUpdating() )
        retVal = 1;
    else if ( lhsNeedsUpdating() )
        retVal = 3;
    else
        retVal = 0;
    return retVal;
}

void CMediaData::updateFromOther( std::shared_ptr< CMediaData > other, bool toLHS )
{
    if ( toLHS )
        fLHSUserMediaData = other->fLHSUserMediaData;
    else
        fRHSUserMediaData = other->fRHSUserMediaData;
}
