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

#include "SettingsDlg.h"
#include "Core/Settings.h"

#include <QFileDialog>
#include <QMessageBox>

#include "ui_SettingsDlg.h"

CSettingsDlg::CSettingsDlg( std::shared_ptr< CSettings > settings, QWidget * parentWidget )
    : QDialog( parentWidget ),
    fImpl( new Ui::CSettingsDlg ),
    fSettings( settings )
{
    fImpl->setupUi( this );
    fImpl->embyURL1->setText( fSettings->lhsURL() );
    fImpl->embyAPI1->setText( fSettings->lhsAPI() );
    fImpl->embyURL2->setText( fSettings->rhsURL() );
    fImpl->embyAPI2->setText( fSettings->rhsAPI() );
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::accept()
{
    fSettings->setLHSURL( fImpl->embyURL1->text() );
    fSettings->setLHSAPI( fImpl->embyAPI1->text() );
    fSettings->setRHSURL( fImpl->embyURL2->text() );
    fSettings->setRHSAPI( fImpl->embyAPI2->text() );

    QDialog::accept();
}

CSettings::CSettings( const QString & fileName, QWidget * parentWidget ) :
    CSettings()
{
    fFileName = fileName;
    load( parentWidget );
}

bool CSettings::load( const QString & fileName, QWidget * parentWidget )
{
    fFileName = fileName;
    return load( parentWidget );
}

bool CSettings::load( QWidget * parentWidget )
{
    if ( fFileName.isEmpty() )
    {
        auto fileName = QFileDialog::getOpenFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
        if ( fileName.isEmpty() )
            return false;
        fFileName = QFileInfo( fileName ).absoluteFilePath();
    }

    return load( fFileName,
                 [ parentWidget ]( const QString & title, const QString & msg )
    {
        QMessageBox::critical( parentWidget, title, msg );
    } );
}

//std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc
bool CSettings::save( QWidget * parentWidget )
{
    return save( parentWidget,
                 [parentWidget]() -> QString
                 {
                     auto fileName = QFileDialog::getSaveFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
                     if ( fileName.isEmpty() )
                         return {};

                     return fileName;
                 },
                 [parentWidget]( const QString & title, const QString & msg )
                 {
                     QMessageBox::critical( parentWidget, title, msg );
                 } );
}
  
bool CSettings::maybeSave( QWidget * parentWidget )
{
    return maybeSave( parentWidget,
                      [parentWidget]() -> QString
                      {
                          auto fileName = QFileDialog::getSaveFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
                          if ( fileName.isEmpty() )
                              return {};

                          return fileName;
                      },
                      [parentWidget]( const QString & title, const QString & msg )
                      {
                          QMessageBox::critical( parentWidget, title, msg );
                      } );
}

bool CSettings::save( QWidget * parentWidget, std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc )
{
    if ( fFileName.isEmpty() )
        return maybeSave( parentWidget, selectFileFunc, errorFunc );

    return save( errorFunc );
}

bool CSettings::maybeSave( QWidget * parentWidget, std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc )
{
    if ( !fChanged )
        return true;

    if ( fFileName.isEmpty() )
    {
        if ( selectFileFunc )
        {
            auto fileName = selectFileFunc();
            if ( fileName.isEmpty() )
                return true;
            fFileName = fileName;
        }
    }

    if ( fFileName.isEmpty() )
        return false;
    return save( parentWidget, selectFileFunc, errorFunc );
}
