﻿// The MIT License( MIT )
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

#include "UserData.h"

CUserData::CUserData( const QString & name ) :
    fName( name )
{

}

QString CUserData::getUserID( bool isLHS ) const
{
    if ( isLHS )
        return fUserID.first;
    else
        return fUserID.second;
}

void CUserData::setUserID( const QString & id, bool isLHS )
{
    if ( isLHS )
        fUserID.first = id;
    else
        fUserID.second = id;
}

void CUserData::clearWatchedMedia()
{
    fPlayedMedia.clear();
}

bool CUserData::onLHSServer() const
{
    return !fUserID.first.isEmpty();
}

bool CUserData::onRHSServer() const
{
    return !fUserID.second.isEmpty();
}

void CUserData::addPlayedMedia( std::shared_ptr< CMediaData > mediaData )
{
    fPlayedMedia.push_back( mediaData );
}

const std::list< std::shared_ptr< CMediaData > > & CUserData::playedMedia() const
{
    return fPlayedMedia;
}

bool CUserData::hasMedia() const
{
    return !fPlayedMedia.empty();
}

int CUserData::numPlayedMedia() const
{
    return static_cast<int>( fPlayedMedia.size() );
}