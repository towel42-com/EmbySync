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

#include "UserData.h"
#include <QRegularExpression>

CUserData::CUserData( const QString & name, const QString & connectedID, bool isLHS ) :
    fConnectedID( connectedID )
{
    setName( name, isLHS );
}

QString CUserData::name( bool isLHS ) const
{
    if ( isLHS )
        return fName.first;
    else
        return fName.second;
}

void CUserData::setName( const QString & name, bool isLHS )
{
    if ( isLHS )
        fName.first = name;
    else
        fName.second = name;
}

QString CUserData::displayName() const
{
    QString retVal = fName.first;
    bool needsParen = false;
    if ( fName.first != fName.second )
    {
        needsParen = true;
        retVal += "(";
        retVal += fName.second;
    }
    if ( !connectedID().isEmpty() )
    {
        if ( !needsParen )
        {
            needsParen = true;
            retVal += "(";
        }
        else
            retVal += "-";
        retVal += connectedID();
    }

    if ( needsParen )
        retVal += ")";
    return retVal;
}

bool CUserData::isUserNameMatch( const QString & regExStr ) const
{
    auto regEx = QRegularExpression( regExStr );

    auto match = regEx.match( fName.first );
    if ( match.hasMatch() && ( match.captured( 0 ).length() == fName.first.length() ) )
        return true;

    match = regEx.match( fName.second );
    if ( match.hasMatch() && ( match.captured( 0 ).length() == fName.second.length() ) )
        return true;
    return false;
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

void CUserData::clearMedia()
{
    fMedia.clear();
}

bool CUserData::onLHSServer() const
{
    return !fUserID.first.isEmpty();
}

bool CUserData::onRHSServer() const
{
    return !fUserID.second.isEmpty();
}

bool CUserData::canBeSynced() const
{
    return onLHSServer() && onRHSServer();
}

void CUserData::addMedia( std::shared_ptr< CMediaData > mediaData )
{
    fMedia.push_back( mediaData );
}

const std::list< std::shared_ptr< CMediaData > > & CUserData::playedMedia() const
{
    return fMedia;
}

bool CUserData::hasMedia() const
{
    return !fMedia.empty();
}

int CUserData::numPlayedMedia() const
{
    return static_cast<int>( fMedia.size() );
}
