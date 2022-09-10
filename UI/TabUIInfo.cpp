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

#include "TabUIInfo.h"
#include <QMainWindow>
#include <QMenuBar>

void CTabUIInfo::clear()
{
    fMenus.clear();
    fMenuActions.clear();
    fToolBars.clear();
}

void CTabUIInfo::cleanupUI( QMainWindow * mainWindow )
{
    for ( auto && ii : fMenus )
    {
        if ( ii )
            mainWindow->menuBar()->removeAction( ii->menuAction() );
    }
    for ( auto && ii : fMenuActions )
    {
        auto menu = findMenu( mainWindow, ii.first );
        if ( !menu )
            continue;
        for ( auto && jj : ii.second.second )
        {
            if ( jj )
                menu->removeAction( jj );
        }
    }
    for ( auto && ii : fToolBars )
    {
        if ( ii )
            mainWindow->removeToolBar( ii );
    }

    clear();
}


QAction * CTabUIInfo::findInsertBefore( QMenu * menu, bool insertBefore )
{
    if ( !insertBefore )
        return nullptr;

    auto actions = menu->actions();
    QAction * retVal = nullptr;
    for ( int ii = 1; ii < actions.count(); ++ii )
    {
        auto prev = actions.at( ii - 1 );
        auto curr = actions.at( ii );

        if ( prev->isSeparator() && curr->isSeparator() )
        {
            retVal = curr;
            break;
        }
    }
    Q_ASSERT( retVal );
    return retVal;
}

QMenu * CTabUIInfo::findMenu( QMainWindow * mainWindow, const QString & text )
{
    if ( !mainWindow || !mainWindow->menuBar() )
        return nullptr;

    auto menuBarActions = mainWindow->menuBar()->actions();

    QMenu * menu = nullptr;
    for ( auto && jj : menuBarActions )
    {
        if ( jj->text() == text )
        {
            menu = jj->menu();
            break;
        }
    }
    return menu;
}

void CTabUIInfo::setupUI( QMainWindow * mainWindow, QMenu * insertBeforeMenu )
{
    for ( auto && ii : fMenus )
    {
        if ( ii )
            mainWindow->menuBar()->insertAction( insertBeforeMenu->menuAction(), ii->menuAction() );
    }
    for ( auto && ii : fToolBars )
    {
        if ( ii )
            mainWindow->addToolBar( Qt::TopToolBarArea, ii );
    }
    for ( auto && ii : fMenuActions )
    {
        auto menu = findMenu( mainWindow, ii.first );
        if ( !menu )
            continue;
    
        auto insertBefore = findInsertBefore( menu, ii.second.first );
        for ( auto && jj : ii.second.second )
        {
            if ( jj )
                menu->insertAction( insertBefore, jj );
        }
    }
}