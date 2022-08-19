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
#include <map>

class CMediaData;
class QTreeWidgetItem;
class CSettings;

struct SUserServerData
{
    bool isValid() const;
    QString fName;
    QString fUserID;
};

class CUserData
{
public:
    CUserData( const QString & serverName, const QString & name, const QString & connectedID, const QString & userID );

    QString connectedID() const { return fConnectedID; }

    bool isValid() const;
    QString name( const QString & serverName ) const;
    void setName( const QString & serverName, const QString & name );
    
    QString userName( const QString & serverName ) const;
    QString allNames() const;
    QString sortName( std::shared_ptr< CSettings > settings ) const;

    QStringList missingServers() const;

    
    bool isUser( const QString & name ) const;
    bool isUser( const QRegularExpression & regEx ) const;

 
   QTreeWidgetItem * getItem() const { return fItem; }
    void setItem( QTreeWidgetItem * item ) { fItem = item; }

    QString getUserID( const QString & serverName ) const;
    void setUserID( const QString & serverName, const QString & id );

    bool canBeSynced() const;
    bool onServer( const QString & serverName ) const;
private:
    void updateCanBeSynced();

    std::shared_ptr< SUserServerData > getServerInfo( const QString & serverName, bool addIfMissing );
    std::shared_ptr< SUserServerData > getServerInfo( const QString & serverName ) const;
    bool isMatch( const QRegularExpression & regEx, const QString & value ) const;

    QString fConnectedID;
    bool fCanBeSynced{ false };

    std::map< QString, std::shared_ptr< SUserServerData > > fInfoForServer; // serverName to Info
    QTreeWidgetItem * fItem{ nullptr };
};
#endif 
