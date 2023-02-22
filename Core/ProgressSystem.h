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

#ifndef __PROGRESSSYSTEM_H
#define __PROGRESSSYSTEM_H

#include <QString>
#include <functional>
class CProgressSystem
{
public:
    void pushState();
    void popState();

    QString title() const;
    void setTitle(const QString& title);

    int maximum() const;
    void setMaximum(int count);

    int value() const;
    void setValue(int value) const;

    void incProgress();
    void resetProgress() const;
    bool wasCanceled() const;

    void setSetTitleFunc(std::function< void(const QString& title) > setTitleFunc);
    void setTitleFunc(std::function< QString() > titleFunc);

    void setMaximumFunc(std::function< int() > maximumFunc);
    void setSetMaximumFunc(std::function< void(int) > setMaximumFunc);

    void setValueFunc(std::function< int() > valueFunc);
    void setSetValueFunc(std::function< void(int) > setValueFunc);

    void setIncFunc(std::function< void() > incFunc);
    void setResetFunc(std::function< void() > resetFunc);
    void setWasCanceledFunc(std::function< bool() > wasCanceledFunc);

private:
    std::function< void(const QString& title) > fSetTitleFunc;
    std::function< QString() > fTitleFunc;

    std::function< int() > fMaximumFunc;
    std::function< void(int) > fSetMaximumFunc;

    std::function< int() > fValueFunc;
    std::function< void(int) > fSetValueFunc;

    std::function< void() > fIncFunc;
    std::function< void() > fResetFunc;
    std::function< bool() > fWasCanceledFunc;

private:
    std::list< std::tuple< QString, int, int > > fStateStack;
};

#endif
