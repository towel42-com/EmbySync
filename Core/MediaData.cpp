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

void CMediaDataBase::loadUserDataFromJSON( const QJsonObject & userDataObj )
{
    //qDebug() << userDataObj;

    fIsFavorite = userDataObj[ "IsFavorite" ].toBool();
    fLastPlayedDate = userDataObj[ "LastPlayedDate" ].toVariant().toDateTime();
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
    fLHSServer( std::make_shared< CMediaDataBase >() ),
    fRHSServer( std::make_shared< CMediaDataBase >() )
{
}

QString CMediaData::name() const
{
    return fName;
}

QString CMediaData::computeName( QJsonObject & mediaObj )
{
    QString retVal = mediaObj[ "Name" ].toString();

    if ( mediaObj[ "Type" ] == "Episode" )
    {
        //auto tmp = QJsonDocument( media );
        //qDebug() << tmp.toJson();

        auto series = mediaObj[ "SeriesName" ].toString();
        auto season = mediaObj[ "SeasonName" ].toString();
        auto pos = season.lastIndexOf( ' ' );
        if ( pos != -1 )
        {
            bool aOK;
            auto seasonNum = season.mid( pos + 1 ).toInt( &aOK );
            if ( aOK )
                season = QString( "S%1" ).arg( seasonNum, 2, 10, QChar( '0' ) );
        }

        auto episodeNum = mediaObj[ "IndexNumber" ].toInt();
        auto episodeName = mediaObj[ "EpisodeTitle" ].toString();

        retVal = QString( "%1 - %2E%3 - %4 - %5" ).arg( series ).arg( season ).arg( episodeNum, 2, 10, QChar( '0' ) ).arg( episodeName ).arg( retVal );
    }
    return retVal;
}

bool CMediaData::isMissing( bool isLHS ) const
{
    return isLHS ? fLHSServer->fMediaID.isEmpty() : fRHSServer->fMediaID.isEmpty();
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
        fLHSServer->loadUserDataFromJSON( userDataObj );
    }
    else
    {
        fRHSServer->loadUserDataFromJSON( userDataObj );
    }
}

bool CMediaData::serverDataEqual() const
{
    return *fLHSServer == *fRHSServer;
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
    return lhs ? fLHSServer->fPlayed : fRHSServer->fPlayed;
}

uint64_t CMediaData::playCount( bool lhs ) const
{
    return lhs ? fLHSServer->fPlayCount : fRHSServer->fPlayCount;
}

bool CMediaData::isFavorite( bool lhs ) const
{
    return lhs ? fLHSServer->fIsFavorite : fRHSServer->fIsFavorite;
}

QDateTime CMediaData::lastPlayed( bool lhs ) const
{
    return lhs ? fLHSServer->fLastPlayedDate : fRHSServer->fLastPlayedDate;
}

// 1 tick = 10000 ms
uint64_t CMediaData::playbackPositionTicks( bool lhs ) const
{
    return lhs ? fLHSServer->fPlaybackPositionTicks : fRHSServer->fPlaybackPositionTicks;
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
    return fLHSServer->fLastPlayedDate == fRHSServer->fLastPlayedDate;
}

bool operator==( const CMediaDataBase & lhs, const CMediaDataBase & rhs )
{
    auto equal = true;
    equal = equal && lhs.fIsFavorite == rhs.fIsFavorite;
    equal = equal && lhs.fPlayed == rhs.fPlayed;
    // equal = equal && lhs.fLastPlayedDate == rhs.fLastPlayedDate;
    equal = equal && lhs.fPlayCount == rhs.fPlayCount;
    equal = equal && lhs.fPlaybackPositionTicks == rhs.fPlaybackPositionTicks;
    return equal;
}

QUrlQuery CMediaData::getMissingItemQuery() const
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
        fLHSServer->fMediaID = id;
    else
        fRHSServer->fMediaID = id;
}

QString CMediaData::getMediaID( bool isLHS ) const
{
    if ( isLHS )
        return fLHSServer->fMediaID;
    else
        return fRHSServer->fMediaID;
}

bool CMediaData::beenLoaded( bool isLHS ) const
{
    return isLHS ? fLHSServer->fBeenLoaded : fRHSServer->fBeenLoaded;
}

bool CMediaData::lhsMoreRecent() const
{
    //qDebug() << fLHSServer->fLastPlayedDate;
    //qDebug() << fRHSServer->fLastPlayedDate;
    return fLHSServer->fLastPlayedDate > fRHSServer->fLastPlayedDate;
}

void CMediaData::setItem( QTreeWidgetItem * item, bool lhs )
{
    if ( lhs )
        fLHSServer->fItem = item;
    else if ( !lhs )
        fRHSServer->fItem = item;
}

QTreeWidgetItem * CMediaData::getItem( bool lhs )
{
    return lhs ? fLHSServer->fItem : fRHSServer->fItem;
}

QTreeWidgetItem * CMediaData::getItem( bool forLHS ) const
{
    return forLHS ? fLHSServer->fItem : fRHSServer->fItem;
}

void CMediaData::updateFromOther( std::shared_ptr< CMediaData > other, bool toLHS )
{
    if ( toLHS )
        fLHSServer = other->fLHSServer;
    else
        fRHSServer = other->fRHSServer;
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
