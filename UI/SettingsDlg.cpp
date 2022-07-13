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

#include "ui_SettingsDlg.h"

CSettingsDlg::CSettingsDlg( std::shared_ptr< CSettings > settings, QWidget * parent )
    : QDialog( parent ),
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
