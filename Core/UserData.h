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
#include <memory>

class CMediaData;
class QTreeWidgetItem;

class CUserData
{
public:
    CUserData( const QString & name, const QString & connectedID, bool isLHSName );

    QString name( bool forLHS ) const;
    void setName( const QString & name, bool isLHS );
    QString displayName() const;
    QString connectedID() const { return fConnectedID; }
    
    bool isUser( const QRegularExpression & regEx ) const;

    QTreeWidgetItem * getItem() const { return fItem; }
    void setItem( QTreeWidgetItem * item ) { fItem = item; }

    QString getUserID( bool isLHS ) const;
    void setUserID( const QString & id, bool isLHS );

    void clearMedia();

    bool onLHSServer() const;
    bool onRHSServer() const;
    bool canBeSynced() const;

    void addMedia( std::shared_ptr< CMediaData > mediaData );
    const std::list< std::shared_ptr< CMediaData > > & playedMedia() const;
    bool hasMedia() const;
    int numPlayedMedia() const;
private:
    bool isMatch( const QRegularExpression & regEx, const QString & value ) const;

    std::pair< QString, QString > fName;
    QString fConnectedID;
    std::pair< QString, QString > fUserID;
    QTreeWidgetItem * fItem{ nullptr };
    std::list< std::shared_ptr< CMediaData > > fMedia;
};
#endif 
