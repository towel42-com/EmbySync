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

#include "Core/MediaData.h"
#include "Core/Settings.h"
#include "SABUtils/utils.h"

#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QListWidget>

void setItemColor( QTreeWidgetItem * item, int column, const QColor & clr )
{
    if ( !item )
        return;
    item->setBackground( column, clr );
}

void setItemColor( QTreeWidgetItem * item, const QColor & clr )
{
    for ( int ii = 0; ii < item->columnCount(); ++ii )
        setItemColor( item, ii, clr );
}

template< typename T >
void setItemColor( QTreeWidgetItem * lhs, QTreeWidgetItem * rhs, int column, const T & lhsValue, const T & rhsValue, bool rhsOlder, std::shared_ptr< CSettings > settings )
{
    if ( lhsValue != rhsValue )
    {
        auto older = settings->mediaSourceColor();
        auto newer = settings->mediaDestColor();

        setItemColor( lhs, column, rhsOlder ? newer : older );
        setItemColor( rhs, column, rhsOlder ? older : newer );
    }
}

void CMediaData::setItemColors( std::shared_ptr< CSettings > settings )
{
    //if ( isMissing( true ) )
    //    setItemColor( fLHSServer->fItem, Qt::red );
    //if ( isMissing( false ) )
    //    setItemColor( fRHSServer->fItem, Qt::red );
    if ( !hasMissingInfo() )
    {
        if ( *lhsUserData() != *rhsUserData() )
        {
            bool rhsOlder = this->rhsLastPlayedOlder();
            ::setItemColor( lhsUserData()->fItem, rhsUserData()->fItem, EColumn::eName, false, true, rhsOlder, settings );
            ::setItemColor( lhsUserData()->fItem, rhsUserData()->fItem, EColumn::eFavorite, isFavorite( true ), isFavorite( false ), rhsOlder, settings );
            ::setItemColor( lhsUserData()->fItem, rhsUserData()->fItem, EColumn::ePlayed, isPlayed( true ), isPlayed( false ), rhsOlder, settings );
            ::setItemColor( lhsUserData()->fItem, rhsUserData()->fItem, EColumn::eLastPlayed, lastPlayed( true ), lastPlayed( false ), rhsOlder, settings );
            ::setItemColor( lhsUserData()->fItem, rhsUserData()->fItem, EColumn::ePlayCount, playCount( true ), playCount( false ), rhsOlder, settings );
            ::setItemColor( lhsUserData()->fItem, rhsUserData()->fItem, EColumn::ePlaybackPosition, playbackPositionTicks( true ), playbackPositionTicks( false ), rhsOlder, settings );
        }
    }
}

std::pair< QTreeWidgetItem *, QTreeWidgetItem * > CMediaData::createItems( QTreeWidget * lhsTree, QTreeWidget * rhsTree, QTreeWidget * dirTree, const std::map< int, QString > & providersByColumn, std::shared_ptr< CSettings > settings )
{
    auto columns = getColumns( providersByColumn );

    if ( lhsTree )
        lhsUserData()->fItem = new QTreeWidgetItem( lhsTree, columns.first );
    if ( dirTree )
    {
        fDirItem = new QTreeWidgetItem( dirTree, QStringList() << getDirectionLabel() );
        //fDirItem->setTextAlignment( 0, Qt::AlignHCenter );
        if ( lhsTree )
        {
            auto sizeHint = lhsUserData()->fItem->sizeHint( 0 );
            fDirItem->setSizeHint( 0, sizeHint );
        }
    }

    if ( rhsTree )
        rhsUserData()->fItem = new QTreeWidgetItem( rhsTree, columns.second );

    setItemColors( settings );

    return { lhsUserData()->fItem, rhsUserData()->fItem };
}

void CMediaData::updateItems( const std::map< int, QString > & providersByColumn, std::shared_ptr< CSettings > settings )
{
    auto bothColumns = getColumns( providersByColumn );
    updateItems( bothColumns.first, true, settings );
    updateItems( bothColumns.second, false, settings );
}

void CMediaData::updateItems( const QStringList & data, bool forLHS, std::shared_ptr< CSettings > settings )
{
    auto item = getItem( forLHS );
    Q_ASSERT( item && ( item->columnCount() == data.count() ) );
    for ( int ii = 0; ii < data.count(); ++ii )
    {
        item->setText( ii, data[ ii ] );
    }
    setItemColors( settings );
}
