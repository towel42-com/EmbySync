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

#ifndef _TABUIINFO_H
#define _TABUIINFO_H

#include <memory>
#include <map>
#include <QList>
#include <QString>
#include <QPointer>
#include <QToolBar>
class QMenu;
class QAction;
class QToolBar;
class QMainWindow;

class CTabUIInfo
{
public:
    void clear();
    void cleanupUI( QMainWindow * mainWindow );
    void setupUI( QMainWindow * mainWindow, QMenu * inserBeforeMenu );

    QAction * findInsertBefore( QWidget * container, bool insertBefore );

    QMenu * findMenu( QMainWindow * mainWindow, const QString & text );
    QToolBar * findToolBar( QMainWindow * mainWindow, const QString & text );

    void removeFromContainer( QWidget * container, const std::pair< bool, QList< QPointer< QAction > > > & ii );
    void addToContainer( QWidget * container, const std::pair< bool, QList< QPointer< QAction > > > & ii );

    QList< QPointer< QMenu > > fMenus;
    QList< QPointer< QToolBar > > fToolBars; // do not include Reload actions on the tool bar, they are added to the main windows own toolbar
    std::map< QString, std::pair< bool, QList< QPointer< QAction > > > > fActions;  // if there is a toolbar that matches the menu name, add the actiosn there as well
};

#endif
