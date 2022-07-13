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
#include <QTreeWidgetItem>
#include <QTreeWidget>

void CMediaData::setItemColor( QTreeWidgetItem * item, const QColor & clr )
{
    for ( int ii = 0; ii < item->columnCount(); ++ii )
        setItemColor( item, ii, clr );
}

void CMediaData::setItemColor( QTreeWidgetItem * item, int column, const QColor & clr )
{
    item->setBackground( column, clr );
}

void CMediaData::setItemColors()
{
    //if ( isMissing( true ) )
    //    setItemColor( fLHSServer->fItem, Qt::red );
    //if ( isMissing( false ) )
    //    setItemColor( fRHSServer->fItem, Qt::red );
    if ( !hasMissingInfo() )
    {
        if ( *fLHSServer != *fRHSServer )
        {
            setItemColor( fRHSServer->fItem, EColumn::eName, Qt::yellow );
            setItemColor( fLHSServer->fItem, EColumn::eName, Qt::yellow );
            if ( played( true ) != played( false ) )
            {
                setItemColor( fRHSServer->fItem, EColumn::ePlayed, Qt::yellow );
                setItemColor( fLHSServer->fItem, EColumn::ePlayed, Qt::yellow );
            }
            if ( isFavorite( true ) != isFavorite( false ) )
            {
                setItemColor( fRHSServer->fItem, EColumn::eFavorite, Qt::yellow );
                setItemColor( fLHSServer->fItem, EColumn::eFavorite, Qt::yellow );
            }
            if ( lastPlayedPos( true ) != lastPlayedPos( false ) )
            {
                setItemColor( fRHSServer->fItem, EColumn::eLastPlayedPos, Qt::yellow );
                setItemColor( fLHSServer->fItem, EColumn::eLastPlayedPos, Qt::yellow );
            }
        }
    }
}

void CMediaData::createItems( QTreeWidget * lhsTree, QTreeWidget * rhsTree, const std::map< int, QString > & providersByColumn )
{
    auto columns = getColumns( providersByColumn );

    if ( lhsTree )
        fLHSServer->fItem = new QTreeWidgetItem( lhsTree, columns.first );
    if ( rhsTree )
        fRHSServer->fItem = new QTreeWidgetItem( rhsTree, columns.second );

    setItemColors();
}

void CMediaData::updateItems( const std::map< int, QString > & providersByColumn )
{
    auto bothColumns = getColumns( providersByColumn );
    updateItems( bothColumns.first, true );
    updateItems( bothColumns.second, false );
}

void CMediaData::updateItems( const QStringList & data, bool forLHS )
{
    auto item = getItem( forLHS );
    Q_ASSERT( item && ( item->columnCount() == data.count() ) );
    for ( int ii = 0; ii < data.count(); ++ii )
    {
        item->setText( ii, data[ ii ] );
    }
    setItemColors();
}
