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

#include "MediaServerData.h"
#include "MediaData.h"
#include <QVariant>

QJsonObject SMediaServerData::userDataJSON() const
{
    QJsonObject obj;
    obj[ "IsFavorite" ] = fIsFavorite;
    obj[ "Played" ] = fPlayed;
    obj[ "PlayCount" ] = static_cast<qlonglong>( fPlayCount );
    if ( fLastPlayedDate.isNull() )
        obj[ "LastPlayedDate" ] = QJsonValue::Null;
    else
        obj[ "LastPlayedDate" ] = fLastPlayedDate.toUTC().toString( Qt::ISODateWithMs );

    auto ticks = static_cast<int64_t>( fPlaybackPositionTicks );
    if ( fPlaybackPositionTicks >= static_cast<uint64_t>( std::numeric_limits< qlonglong >::max() ) )
        ticks = std::numeric_limits< qlonglong >::max() - 1;

    obj[ "PlaybackPositionTicks" ] = static_cast<qlonglong>( ticks );

    return obj;
}

void SMediaServerData::loadUserDataFromJSON( const QJsonObject & userDataObj )
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

bool SMediaServerData::isValid() const
{
    return !fMediaID.isEmpty();
}

bool operator==( const SMediaServerData & lhs, const SMediaServerData & rhs )
{
    return lhs.userDataEqual( rhs );
}

uint64_t SMediaServerData::playbackPositionMSecs() const
{
    return fPlaybackPositionTicks / 10000;
}

QString SMediaServerData::playbackPosition() const
{
    auto playbackMS = playbackPositionMSecs();
    if ( playbackMS == 0 )
        return {};

    if ( CMediaData::mecsToStringFunc() )
        return CMediaData::mecsToStringFunc()( playbackMS );
    return QString::number( playbackMS );
}

QTime SMediaServerData::playbackPositionTime() const
{
    auto playbackMS = playbackPositionMSecs();
    return QTime::fromMSecsSinceStartOfDay( playbackMS );
}

void SMediaServerData::setPlaybackPosition( const QTime & time )
{
    setPlaybackPositionMSecs( 10000ULL * time.msecsSinceStartOfDay() );
}

void SMediaServerData::setPlaybackPositionMSecs( uint64_t msecs )
{
    fPlaybackPositionTicks = ( 10000ULL * msecs );
}

bool SMediaServerData::userDataEqual( const SMediaServerData & rhs ) const
{
    if ( isValid() != rhs.isValid() )
        return false;

    auto equal = true;
    equal = equal && fIsFavorite == rhs.fIsFavorite;
    equal = equal && fPlayed == rhs.fPlayed;
    if ( !fLastPlayedDate.isNull() && !rhs.fLastPlayedDate.isNull() )
        equal = equal && fLastPlayedDate == rhs.fLastPlayedDate;
    equal = equal && fPlayCount == rhs.fPlayCount;
    equal = equal && fPlaybackPositionTicks == rhs.fPlaybackPositionTicks;
    return equal;
}

