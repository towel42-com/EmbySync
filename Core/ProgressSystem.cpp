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

#include "ProgressSystem.h"

void CProgressSystem::setTitle( const QString & title )
{
    if ( fSetTitleFunc )
        fSetTitleFunc( title );
}

void CProgressSystem::pushState()
{
    fStateStack.push_back( std::make_tuple( title(), value(), maximum() ) );
}

void CProgressSystem::popState()
{
    if ( fStateStack.empty() )
        return;
    auto newState = fStateStack.back();
    fStateStack.pop_back();
    setTitle( std::get< 0 >( newState ) );
    setValue( std::get< 1 >( newState ) );
    setMaximum( std::get< 2 >( newState ) );
}

QString CProgressSystem::title() const
{
    if ( fTitleFunc )
        return fTitleFunc();
    return {};
}

void CProgressSystem::setMaximum( int count )
{
    if ( fSetMaximumFunc )
        fSetMaximumFunc( count );
}

int CProgressSystem::maximum() const
{
    if ( fMaximumFunc )
        return fMaximumFunc();
    return 0;
}

int CProgressSystem::value() const
{
    if ( fValueFunc )
        return fValueFunc();
    return 0;
}

void CProgressSystem::setValue( int value ) const
{
    if ( fSetValueFunc )
        fSetValueFunc( value );
}

void CProgressSystem::incProgress()
{
    if ( fIncFunc )
        fIncFunc();
}

void CProgressSystem::resetProgress() const
{
    if ( fResetFunc )
        fResetFunc();
}

bool CProgressSystem::wasCanceled() const
{
    if ( fWasCanceledFunc )
        return fWasCanceledFunc();
    return false;
}

void CProgressSystem::setSetTitleFunc( std::function< void( const QString & title ) > setTitleFunc )
{
    fSetTitleFunc = setTitleFunc;
}

void CProgressSystem::setTitleFunc( std::function< QString() > titleFunc )
{
    fTitleFunc = titleFunc;
}

void CProgressSystem::setMaximumFunc( std::function< int() > maximumFunc )
{
    fMaximumFunc = maximumFunc;
}

void CProgressSystem::setSetMaximumFunc( std::function< void( int ) > setMaximumFunc )
{
    fSetMaximumFunc = setMaximumFunc;
}

void CProgressSystem::setValueFunc( std::function< int() > valueFunc )
{
    fValueFunc = valueFunc;
}

void CProgressSystem::setSetValueFunc( std::function< void( int ) > setValueFunc )
{
    fSetValueFunc = setValueFunc;
}

void CProgressSystem::setIncFunc( std::function< void() > incFunc )
{
    fIncFunc = incFunc;
}

void CProgressSystem::setResetFunc( std::function< void() > resetFunc )
{
    fResetFunc = resetFunc;
}

void CProgressSystem::setWasCanceledFunc( std::function< bool() > wasCanceledFunc )
{
    fWasCanceledFunc = wasCanceledFunc;
}

