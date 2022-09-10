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

#include "TabPageBase.h"
#include "DataTree.h"
#include "Core/SyncSystem.h"
#include "Core/Settings.h"
#include "Core/ServerInfo.h"

#include <QSplitter>
#include <QModelIndex>

CTabPageBase::CTabPageBase( QWidget * parent )
    : QWidget( parent )
{
}

CTabPageBase::~CTabPageBase()
{
}

QModelIndex CTabPageBase::currentDataIndex() const
{
    if ( fDataTrees.empty() )
        return {};
    return fDataTrees.front()->currentIndex();
}

std::pair< QModelIndex, QWidget * > CTabPageBase::dataIndexAt( const QPoint & pos ) const
{
    for ( auto && ii : fDataTrees )
    {
        auto idx = ii->indexAt( pos );
        if ( idx.isValid() )
        {
            return { idx, ii };
        }
    }
    return {};
}

void CTabPageBase::autoSizeDataTrees()
{
    for ( auto && ii : fDataTrees )
        ii->autoSize();
}

void CTabPageBase::hideDataTreeColumns()
{
    for ( auto && ii : fDataTrees )
        ii->hideColumns();
}

void CTabPageBase::loadServers( QAbstractItemModel * model )
{
    for ( auto && ii : fDataTrees )
        delete ii;
    fDataTrees.clear();

    fSyncSystem->loadServers();

    for ( int ii = 0; ii < fSettings->serverCnt(); ++ii )
    {
        auto serverInfo = fSettings->serverInfo( ii );
        if ( !serverInfo->isEnabled() )
            continue;

        auto dataTree = new CDataTree( fSettings->serverInfo( ii ), getDataSplitter() );
        getDataSplitter()->addWidget( dataTree );
        dataTree->setModel( model );
        connect( dataTree, &CDataTree::sigCurrChanged, this, &CTabPageBase::sigSetCurrentDataItem );
        connect( dataTree, &CDataTree::sigViewData, this, &CTabPageBase::sigViewData );
        connect( dataTree, &CDataTree::sigDataContextMenuRequested, this, &CTabPageBase::sigDataContextMenuRequested );

        fDataTrees.push_back( dataTree );
    }

    for ( size_t ii = 0; ii < fDataTrees.size(); ++ii )
    {
        for ( size_t jj = 0; jj < fDataTrees.size(); ++jj )
        {
            if ( ii == jj )
                continue;
            fDataTrees[ ii ]->addPeerDataTree( fDataTrees[ jj ] );
        }
    }
}