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

#include <QObject>
#include <chrono>

QJsonObject SMediaUserData::toJSON() const
{
    QJsonObject obj;
    obj[ "IsFavorite" ] = fIsFavorite;
    obj[ "Played" ] = fPlayed;
    obj[ "PlayCount" ] = static_cast<int64_t>( fPlayCount );
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

CMediaData::CMediaData( const QString & name, const QString & type ) :
    fName( name ),
    fType( type ),
    fLHSUserData( std::make_shared< SMediaUserData >() ),
    fRHSUserData( std::make_shared< SMediaUserData >() )
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
    return isLHS ? fLHSUserData->fMediaID.isEmpty() : fRHSUserData->fMediaID.isEmpty();
}

bool CMediaData::hasMissingInfo() const
{
    return isMissing( true ) || isMissing( false );
}

void CMediaData::loadUserDataFromJSON( const QJsonObject & media, bool isLHSServer  )
{
    auto userDataObj = media[ "UserData" ].toObject();

    //auto tmp = QJsonDocument( userDataObj );
    //qDebug() << tmp.toJson();

    if ( isLHSServer )
    {
        fLHSUserData->loadUserDataFromJSON( userDataObj );
    }
    else
    {
        fRHSUserData->loadUserDataFromJSON( userDataObj );
    }
}

bool CMediaData::userDataEqual( bool includeTimestamp ) const
{
    return fLHSUserData->userDataEqual( *fRHSUserData, includeTimestamp );
}

bool operator==( const SMediaUserData & lhs, const SMediaUserData & rhs )
{
    return lhs.userDataEqual( rhs, false );
}

bool SMediaUserData::userDataEqual( const SMediaUserData & rhs, bool includeTimeStamp ) const
{
    auto equal = true;
    equal = equal && fIsFavorite == rhs.fIsFavorite;
    equal = equal && fPlayed == rhs.fPlayed;
    if ( includeTimeStamp )
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
    return lhs ? fLHSUserData->fPlayed : fRHSUserData->fPlayed;
}

uint64_t CMediaData::playCount( bool lhs ) const
{
    return lhs ? fLHSUserData->fPlayCount : fRHSUserData->fPlayCount;
}

bool CMediaData::isFavorite( bool lhs ) const
{
    return lhs ? fLHSUserData->fIsFavorite : fRHSUserData->fIsFavorite;
}

QDateTime CMediaData::lastPlayed( bool lhs ) const
{
    return lhs ? fLHSUserData->fLastPlayedDate : fRHSUserData->fLastPlayedDate;
}

// 1 tick = 10000 ms
uint64_t CMediaData::playbackPositionTicks( bool lhs ) const
{
    return lhs ? fLHSUserData->fPlaybackPositionTicks : fRHSUserData->fPlaybackPositionTicks;
}

QString CMediaData::playbackPosition( bool lhs ) const
{
    auto playbackMS = playbackPositionMSecs( lhs );
    if ( playbackMS == 0 )
        return {};

    if ( sMSecsToStringFunc )
        return sMSecsToStringFunc( playbackMS );
    return QString::number( playbackMS );
}

QTime CMediaData::playbackPositionTime( bool lhs ) const
{
    auto playbackMS = playbackPositionMSecs( lhs );
    if ( playbackMS == 0 )
        return {};

    return QTime::fromMSecsSinceStartOfDay( playbackMS );
}

// stored in ticks
// 1 tick = 10000 ms
uint64_t CMediaData::playbackPositionMSecs( bool lhs ) const
{
    return playbackPositionTicks( lhs ) / 10000;
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
    return fLHSUserData->fLastPlayedDate == fRHSUserData->fLastPlayedDate;
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

void CMediaData::addProvider( const QString & providerName, const QString & providerID )
{
    fProviders[ providerName ] = providerID;
}

void CMediaData::setMediaID( const QString & id, bool isLHS )
{
    if ( isLHS )
        fLHSUserData->fMediaID = id;
    else
        fRHSUserData->fMediaID = id;
}

QString CMediaData::getMediaID( bool isLHS ) const
{
    if ( isLHS )
        return fLHSUserData->fMediaID;
    else
        return fRHSUserData->fMediaID;
}

bool CMediaData::beenLoaded( bool isLHS ) const
{
    return isLHS ? fLHSUserData->fBeenLoaded : fRHSUserData->fBeenLoaded;
}

bool CMediaData::rhsLastPlayedOlder() const
{
    //qDebug() << fLHSServer->fLastPlayedDate;
    //qDebug() << fRHSServer->fLastPlayedDate;
    return fLHSUserData->fLastPlayedDate > fRHSUserData->fLastPlayedDate;
}

bool CMediaData::lhsLastPlayedOlder() const
{
    //qDebug() << fLHSServer->fLastPlayedDate;
    //qDebug() << fRHSServer->fLastPlayedDate;
    return fRHSUserData->fLastPlayedDate > fLHSUserData->fLastPlayedDate;
}

void CMediaData::setItem( QTreeWidgetItem * item, bool lhs )
{
    if ( lhs )
        fLHSUserData->fItem = item;
    else if ( !lhs )
        fRHSUserData->fItem = item;
}

QTreeWidgetItem * CMediaData::getItem( bool lhs )
{
    return lhs ? fLHSUserData->fItem : fRHSUserData->fItem;
}

QTreeWidgetItem * CMediaData::getItem( bool forLHS ) const
{
    return forLHS ? fLHSUserData->fItem : fRHSUserData->fItem;
}

QString CMediaData::getDirectionLabel() const
{
    if ( userDataEqual( true ) )
        return "   =   ";
    if ( rhsLastPlayedOlder() )
        return "<<<";
    else if ( lhsLastPlayedOlder() )
        return ">>>";
    else
        return "===";
}

void CMediaData::updateFromOther( std::shared_ptr< CMediaData > other, bool toLHS )
{
    if ( toLHS )
        fLHSUserData = other->fLHSUserData;
    else
        fRHSUserData = other->fRHSUserData;
}

QStringList CMediaData::getColumns( bool forLHS )
{
    auto retVal = QStringList()
        << fName
        << getMediaID( forLHS )
        << ( isFavorite( forLHS ) ? "Yes" : "No" )
        << ( isPlayed( forLHS ) ? "Yes" : "No" )
        << lastPlayed( forLHS ).toString()
        << QString::number( playCount( forLHS ) )
        << playbackPosition( forLHS )
        ;
    return retVal;
}

std::pair< QStringList, QStringList > CMediaData::getColumns( const std::map< int, QString > & providersByColumn )
{
    QStringList providerColumns;
    for ( auto && ii : providersByColumn )
    {
        auto pos = fProviders.find( ii.second );
        if ( pos == fProviders.end() )
            providerColumns << QString();
        else
            providerColumns << ( *pos ).second;
    }

    auto lhsColumns = getColumns( true ) << providerColumns;
    auto rhsColumns = getColumns( false ) << providerColumns;

    return { lhsColumns, rhsColumns };
}
