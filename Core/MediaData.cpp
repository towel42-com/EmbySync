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

QDateTime CMediaDataBase::getLastModifiedTime() const
{
    if ( !fUserData.contains( "LastPlayedDate" ) )
        return {};

    auto value = fUserData[ "LastPlayedDate" ].toDateTime();
    return value;
}

void CMediaDataBase::loadUserDataFromJSON( const QJsonObject & userDataObj, const QMap<QString, QVariant> & userDataVariant )
{
    fIsFavorite = userDataObj[ "IsFavorite" ].toBool();
    fLastPlayedPos = userDataObj[ "PlaybackPositionTicks" ].toVariant().toLongLong();
    fPlayed = userDataObj[ "Played" ].toVariant().toBool();
    fUserData = userDataVariant;
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

    auto userDataVariant = media[ "UserData" ].toVariant().toMap();
    if ( isLHSServer )
    {
        fLHSServer->loadUserDataFromJSON( userDataObj, userDataVariant );
    }
    else
    {
        fRHSServer->loadUserDataFromJSON( userDataObj, userDataVariant );
    }
}

bool CMediaData::serverDataEqual() const
{
    return *fLHSServer == *fRHSServer;
}

bool CMediaData::mediaWatchedOnServer( bool isLHS )
{
    return isLHS ? !fLHSServer->fMediaID.isEmpty() : !fRHSServer->fMediaID.isEmpty();
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

bool CMediaData::isFavorite( bool lhs ) const
{
    return lhs ? fLHSServer->fIsFavorite : fRHSServer->fIsFavorite;
}

QString CMediaData::lastModified( bool lhs ) const
{
    return lhs ? fLHSServer->getLastModifiedTime().toString() : fRHSServer->getLastModifiedTime().toString();
}

QString CMediaData::lastPlayed( bool lhs ) const
{
    return ( lastPlayedPos( lhs ) ? QString::number( lastPlayedPos( lhs ) ) : QString() );
}

bool CMediaData::bothPlayed() const
{
    return ( played( true ) != 0 ) && ( played( false ) != 0 );
}

bool CMediaData::playPositionTheSame() const
{
    return lastPlayedPos( true ) == lastPlayedPos( false );
}

bool CMediaData::bothFavorites() const
{
    return isFavorite( true ) && isFavorite( false );
}

bool CMediaData::lastPlayedTheSame() const
{
    return fLHSServer->getLastModifiedTime() == fRHSServer->getLastModifiedTime();
}

uint64_t CMediaData::lastPlayedPos( bool lhs ) const
{
    return lhs ? fLHSServer->fLastPlayedPos : fRHSServer->fLastPlayedPos;
}

bool CMediaData::played( bool lhs ) const
{
    return lhs ? fLHSServer->fPlayed : fRHSServer->fPlayed;
}

bool operator==( const CMediaDataBase & lhs, const CMediaDataBase & rhs )
{
    bool equal = lhs.fLastPlayedPos == rhs.fLastPlayedPos;
    equal = equal && lhs.fIsFavorite == rhs.fIsFavorite;
    equal = equal && lhs.fPlayed == rhs.fPlayed;
    //equal = equal && lhs.getLastModifiedTime() == rhs.getLastModifiedTime();
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
    //qDebug() << fLHSServer->getLastModifiedTime();
    //qDebug() << fRHSServer->getLastModifiedTime();
    return fLHSServer->getLastModifiedTime() > fRHSServer->getLastModifiedTime();
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
    QStringList retVal = QStringList() << fName << getMediaID( forLHS ) << ( isPlayed( forLHS ) ? "Yes" : "No" ) << QString( isFavorite( forLHS ) ? "Yes" : "No" ) << lastPlayed( forLHS ) << lastModified( forLHS );
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
