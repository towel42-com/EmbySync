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

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <unordered_set>
#include <tuple>
#include <optional>
namespace Ui
{
    class CMainWindow;
}
class QTreeWidgetItem;
class CMediaData;
class CUserData;
class CSettings;
class CSyncSystem;

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    CMainWindow( QWidget * parent = nullptr );
    virtual ~CMainWindow() override;

    virtual void closeEvent( QCloseEvent * closeEvent ) override;
public Q_SLOTS:
    void slotSettings();

    void loadSettings();

    void slotLoadProject();
    void slotSave();
    void slotRecentMenuAboutToShow();
    void slotMissingMediaLoaded();

private Q_SLOTS:
    void slotCurrentItemChanged( QTreeWidgetItem *, QTreeWidgetItem * );

    void slotReloadUsers();
    void slotReloadCurrentUser();
    void slotProcess();
    void slotToggleOnlyShowSyncableUsers();

    void slotToggleOnlyShowMediaWithDifferences();
    void slotAddToLog( const QString & msg );

    void slotLoadingUsersFinished();
    void slotUserMediaLoaded();
private:
    void onlyShowSyncableUsers();
    void onlyShowMediaWithDifferences();
    std::shared_ptr< CUserData > getCurrUserData() const;
    std::shared_ptr< CUserData > userDataForItem( QTreeWidgetItem * item ) const;

    void loadFile( const QString & fileName );

    void reset();

    void loadAllMedia();
    void loadAllMedia( bool isLHS );


    void updateProviderColumns( std::shared_ptr< CMediaData > ii );

    std::unique_ptr< Ui::CMainWindow > fImpl;
    std::shared_ptr< CSettings > fSettings;
    std::shared_ptr< CSyncSystem > fSyncSystem;

    std::unordered_set< QString > fProviderColumnsByName;
    std::map< int, QString > fProviderColumnsByColumn;
};
#endif 
