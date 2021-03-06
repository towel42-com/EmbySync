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

#ifndef _SETTINGSDLG_H
#define _SETTINGSDLG_H

#include <QDialog>
#include <QColor>
#include <memory>
namespace Ui
{
    class CSettingsDlg;
}

class QLabel;
class CSettings;
class CSettingsDlg : public QDialog
{
    Q_OBJECT
public:
    CSettingsDlg( std::shared_ptr< CSettings > settings, QWidget * parent = nullptr );

    void load();

    virtual ~CSettingsDlg() override;

    virtual void accept() override;

    void save();

public Q_SLOTS:
private:
    void updateColors();
    void updateColor( QLabel * label, const QColor & color );

    std::unique_ptr< Ui::CSettingsDlg > fImpl;
    std::shared_ptr< CSettings > fSettings;
    QColor fMediaSourceColor;
    QColor fMediaDestColor;
    QColor fDataMissingColor;

};
#endif 
