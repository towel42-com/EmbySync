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

#ifndef _USERDATADLG_H
#define _USERDATADLG_H

#include <QDialog>
#include <QColor>
#include <memory>
namespace Ui
{
    class CUserDataDlg;
}

class QLabel;
class CUserData;
class CMediaData;
class CSyncSystem;
class CSettings;
struct SMediaUserData;

class CUserDataDlg : public QDialog
{
    Q_OBJECT
public:
    CUserDataDlg( bool origFromLHS, std::shared_ptr< CUserData > userData, std::shared_ptr< CMediaData > mediaData, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CSettings > settings, QWidget * parentWidget=nullptr );

    virtual ~CUserDataDlg() override;
    virtual void accept() override;

    void load();
    void save();

    std::shared_ptr< SMediaUserData > getMediaData() const;

public Q_SLOTS:
private:

    std::unique_ptr< Ui::CUserDataDlg > fImpl;

    std::shared_ptr< CUserData > fUserData;
    bool fFromLHS{ false };
    std::shared_ptr< CMediaData > fMediaData;
    std::shared_ptr< CSyncSystem > fSyncSystem;
    std::shared_ptr< CSettings > fSettings;

};
#endif 
