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

#ifndef __MEDIAUSERDATA_H
#define __MEDIAUSERDATA_H
//
#include <QString>
#include <QDateTime>
#include <cstdint>
#include <QJsonObject>

struct SMediaServerData
{
    QString fMediaID;
    bool fIsFavorite{ false };
    bool fPlayed{ false };
    QDateTime fLastPlayedDate;
    uint64_t fPlayCount;
    uint64_t fPlaybackPositionTicks; // 1 tick = 10000 ms

    uint64_t playbackPositionMSecs() const;
    void setPlaybackPositionMSecs( uint64_t msecs );

    QTime playbackPositionTime() const;
    void setPlaybackPosition( const QTime & time );

    QString playbackPosition() const;

    bool userDataEqual( const SMediaServerData & rhs ) const;

    QJsonObject userDataJSON() const;
    void loadUserDataFromJSON( const QJsonObject & userDataObj );

    bool isValid() const;
    bool fBeenLoaded{ false };
};

bool operator==( const SMediaServerData & lhs, const SMediaServerData & rhs );
inline bool operator!=( const SMediaServerData & lhs, const SMediaServerData & rhs )
{
    return !operator==( lhs, rhs );
}
#endif 
