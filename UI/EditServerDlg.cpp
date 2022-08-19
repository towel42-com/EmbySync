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

#include "EditServerDlg.h"
#include "ui_EditServerDlg.h"
#include "SABUtils/WidgetChanged.h"
#include "Core/SyncSystem.h"
#include "Core/Settings.h"

#include <QPushButton>
#include <QMetaMethod>
#include <QMessageBox>

CEditServerDlg::CEditServerDlg( const QString & name, const QString & url, const QString & apiKey, bool enabled, QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CEditServerDlg )
{
    fImpl->setupUi( this );
    fImpl->name->setText( name );
    fImpl->url->setText( url );
    fImpl->apiKey->setText( apiKey );
    fImpl->enabled->setChecked( enabled );

    QObject::connect( fImpl->name, &QLineEdit::textChanged, this, &CEditServerDlg::slotChanged );
    QObject::connect( fImpl->apiKey, &QLineEdit::textChanged, this, &CEditServerDlg::slotChanged );
    QObject::connect( fImpl->url, &CHyperLinkLineEdit::textChanged, this, &CEditServerDlg::slotChanged );
    updateButtons();
}

CEditServerDlg::~CEditServerDlg()
{
}

bool CEditServerDlg::enabled() const
{
    return fImpl->enabled->isChecked();
}

QString CEditServerDlg::name() const
{
    return fImpl->name->text();
}

QString CEditServerDlg::url() const
{
    return fImpl->url->text();
}

QString CEditServerDlg::apiKey() const
{
    return fImpl->apiKey->text();
}

bool CEditServerDlg::okToTest()
{
    bool aOK = !fImpl->url->text().isEmpty();
    aOK = aOK && !fImpl->apiKey->text().isEmpty();
    return aOK;
}

void CEditServerDlg::slotChanged()
{
    updateButtons();
}

void CEditServerDlg::updateButtons()
{
    fImpl->buttonBox->button( QDialogButtonBox::StandardButton::Ok )->setEnabled( okToTest() );
}

