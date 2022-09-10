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

#ifndef _TABPAGEBASE_H
#define _TABPAGEBASE_H

#include <QWidget>
#include <tuple>
#include <memory>
class CSettings;
class CUsersModel;
class CMediaModel;
class CProgressSystem;
class CSyncSystem;
class CTabUIInfo;
class CDataTree;
class QAbstractItemModel;
class QSplitter;

class CTabPageBase : public QWidget
{
    Q_OBJECT
public:
    CTabPageBase( QWidget * parent = nullptr );
    virtual ~CTabPageBase() override;

    virtual void setupPage( std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CProgressSystem > progressSystem )
    {
        fSettings = settings;
        fSyncSystem = syncSystem;
        fMediaModel = mediaModel;
        fUsersModel = userModel;
        fProgressSystem = progressSystem;
    }


    virtual std::shared_ptr< CTabUIInfo > getUIInfo() const = 0;

    virtual void loadSettings() = 0;
    virtual void resetPage() = 0;
    virtual bool okToClose() = 0;
    virtual void loadingUsersFinished() = 0;

    QModelIndex currentDataIndex() const;
    std::pair< QModelIndex, QWidget * > dataIndexAt( const QPoint & pos ) const;

    virtual QSplitter * getDataSplitter() const = 0;

    void autoSizeDataTrees();
    void hideDataTreeColumns();
    virtual bool prepForClose() = 0;
Q_SIGNALS:
    void sigAddToLog( int msgType, const QString & msg );
    void sigAddInfoToLog( const QString & msg );
    void sigSettingsLoaded();
    void sigSetCurrentDataItem( const QModelIndex & current );
    void sigViewData( const QModelIndex & idx );
    void sigDataContextMenuRequested( CDataTree * tree, const QPoint & pos );

public Q_SLOTS:
    virtual void slotCanceled() {};
    virtual void slotModelDataChanged() {};
    virtual void slotSettingsChanged() {};

protected:
    void loadServers( QAbstractItemModel * model );
    std::shared_ptr< CSettings > fSettings;
    std::shared_ptr< CUsersModel > fUsersModel;
    std::shared_ptr< CMediaModel > fMediaModel;
    std::shared_ptr< CProgressSystem > fProgressSystem;
    std::shared_ptr< CSyncSystem > fSyncSystem;

    std::vector< CDataTree * > fDataTrees;
};
#endif
