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
#include <QDebug>

void CTabUIInfo::clear()
{
    fMenus.clear();
    fActions.clear();
    fToolBars.clear();
    fCreatedMenus.clear();
}

void CTabUIInfo::cleanupUI(QMainWindow* mainWindow)
{
    for (auto&& ii : fMenus)
    {
        if (ii)
            mainWindow->menuBar()->removeAction(ii->menuAction());
    }
    for (auto&& ii : fActions)
    {
        auto menu = findMenu(mainWindow, ii.first);
        if (menu)
            removeFromContainer(menu, ii.second);

        auto toolBar = findToolBar(mainWindow, ii.first);
        if (toolBar)
            removeFromContainer(toolBar, ii.second);
    }
    for (auto&& ii : fToolBars)
    {
        if (ii)
            mainWindow->removeToolBar(ii);
    }

    for (auto&& ii : fCreatedMenus)
        delete ii.data();

    clear();
}


void CTabUIInfo::removeFromContainer(QWidget* container, const std::pair< bool, QList< QPointer< QAction > > >& ii)
{
    for (auto&& jj : ii.second)
    {
        if (jj)
            container->removeAction(jj);
    }
}

QAction* CTabUIInfo::findInsertBefore(QWidget* container, bool insertBefore)
{
    if (!insertBefore)
        return nullptr;

    auto actions = container->actions();
    QAction* retVal = nullptr;
    for (int ii = 1; ii < actions.count(); ++ii)
    {
        auto prev = actions.at(ii - 1);
        auto curr = actions.at(ii);

        if (prev->isSeparator() && curr->isSeparator())
        {
            retVal = curr;
            break;
        }
    }

    //Q_ASSERT( retVal );
    return retVal;
}

QMenu* CTabUIInfo::findMenu(QMainWindow* mainWindow, const QString& text)
{
    if (!mainWindow || !mainWindow->menuBar())
        return nullptr;

    auto menuBarActions = mainWindow->menuBar()->actions();

    QMenu* menu = nullptr;
    for (auto&& jj : menuBarActions)
    {
        if (jj->text() == text)
        {
            menu = jj->menu();
            break;
        }
    }
    return menu;
}

QToolBar* CTabUIInfo::findToolBar(QMainWindow* mainWindow, const QString& text)
{
    if (!mainWindow || !mainWindow->menuBar())
        return nullptr;

    auto toolBars = mainWindow->findChildren< QToolBar* >();

    QToolBar* toolBar = nullptr;
    for (auto&& jj : toolBars)
    {
        if (jj->windowTitle() == text)
        {
            toolBar = jj;
            break;
        }
    }
    return toolBar;
}

void CTabUIInfo::setupUI(QMainWindow* mainWindow, QMenu* insertBeforeMenu)
{
    for (auto&& ii : fMenus)
    {
        if (ii)
            mainWindow->menuBar()->insertAction(insertBeforeMenu->menuAction(), ii->menuAction());
    }
    for (auto&& ii : fActions)
    {
        if (!findMenu(mainWindow, ii.first))
        {
            auto menu = new QMenu(mainWindow);
            menu->setObjectName(ii.first);
            menu->setTitle(ii.first);
            fCreatedMenus << menu;
            mainWindow->menuBar()->insertAction(insertBeforeMenu->menuAction(), menu->menuAction());
        }
    }
    for (auto&& ii : fToolBars)
    {
        if (ii)
        {
            mainWindow->addToolBar(Qt::TopToolBarArea, ii);
            ii->show();
        }
    }
    for (auto&& ii : fActions)
    {
        auto menu = findMenu(mainWindow, ii.first);
        if (menu)
            addToContainer(menu, ii.second);

        auto toolBar = findToolBar(mainWindow, ii.first);
        if (toolBar)
            addToContainer(toolBar, ii.second);
    }
}

void CTabUIInfo::addToContainer(QWidget* container, const std::pair< bool, QList< QPointer< QAction > > >& ii)
{
    auto insertBefore = findInsertBefore(container, ii.first);
    for (auto&& jj : ii.second)
    {
        if (jj)
            container->insertAction(insertBefore, jj);
    }
}
