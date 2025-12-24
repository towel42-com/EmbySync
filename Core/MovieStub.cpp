#include "MovieStub.h"
#include "MediaData.h"
#include "SABUtils/HashUtils.h"
#include <QJsonObject>
#include <QJsonArray>

#include <QRegularExpression>
#include <unordered_map>
#include "SABUtils/StringUtils.h"

SMovieStub::SMovieStub( const QString &name ) :
    SMovieStub( name, {} )
{
}

SMovieStub::SMovieStub( const QString &name, const std::optional< int > &year ) :
    SMovieStub( name, year, std::optional< std::pair< int, int > >() )
{
}

SMovieStub::SMovieStub( const QString &name, const std::optional< int > &year, const std::optional< QPoint > &resolution ) :
    SMovieStub( name, year, resolution.has_value() ? std::make_pair( resolution.value().x(), resolution.value().y() ) : std::optional< std::pair< int, int > >() )
{
}

SMovieStub::SMovieStub( const QString &name, const std::optional< int > &year, const std::optional< std::pair< int, int > > &resolution ) :
    fName( name ),
    fYear( year ),
    fResolution( resolution )
{
}

SMovieStub::SMovieStub( std::shared_ptr< CMediaData > data )
{
    if ( !data )
        return;
    fName = data->name();
    fYear = data->premiereDate().year();
    fResolution = data->resolutionValue();
}

bool SMovieStub::compareYear( int64_t lhs, int64_t rhs )
{
    return ::abs( lhs - rhs ) <= 3;
}

QString SMovieStub::nameKey( const QString &name )
{
    static std::unordered_map< QString, QString > sCache;
    auto pos = sCache.find( name );
    if ( pos != sCache.end() )
        return ( *pos ).second;

    auto retVal = name.toLower();
    retVal = retVal.replace( QRegularExpression( "[^a-zA-Z0-9 ]" ), " " );
    retVal = retVal.replace( QRegularExpression( R"(\b(chapter|part)\b)" ), " " );

    auto startsWith = QStringList() << "the"
                                    << "national lampoons"
                                    << "monty pythons";
    auto endsWith = QStringList() << ": based on the novel push by sapphire";

    for ( auto &&ii : startsWith )
    {
        if ( retVal.startsWith( ii ) )
            retVal = retVal.mid( ii.length() );
    }

    for ( auto &&ii : endsWith )
    {
        if ( retVal.endsWith( ii ) )
            retVal = retVal.mid( 0, ii.length() - ii.length() );
    }


    auto words = NTowel42Utils::NStringUtils::getImportantWordsInOrder( retVal, true );
    retVal.clear();
    for ( auto &&ii : words )
    {
        if ( !retVal.isEmpty() )
            retVal += " ";
        retVal += ii;
    }

    sCache[ name ] = retVal;
    return retVal;
}

QJsonObject SMovieStub::toJSON() const
{
    QJsonObject retVal;
    retVal[ "name" ] = fName;
    if ( hasYear() )
        retVal[ "year" ] = fYear.value();
    if ( hasResolution() )
        retVal[ "resolution" ] = QString( "%1x%2" ).arg( fResolution.value().first ).arg( fResolution.value().second );
    return retVal;
}

std::size_t SMovieStub::hash( bool useName, bool useYear, bool useResolution ) const
{
    Q_ASSERT( useName || useYear || useResolution );

    std::size_t retVal = 0;

    if ( useName )
        retVal = NTowel42Utils::HashCombine( retVal, nameKey() );
    if ( useYear && hasYear() )
        retVal = NTowel42Utils::HashCombine( retVal, fYear );
    if ( useResolution && hasResolution() )
        retVal = NTowel42Utils::HashCombine( retVal, fResolution.value().first );

    return retVal;
}

bool SMovieStub::hasYear() const
{
    return fYear.has_value();
}

bool resolutionMatches( const std::optional< std::pair< int, int > > &lhs, const std::optional< std::pair< int, int > > &rhs )
{
    if ( lhs.has_value() != rhs.has_value() )
        return false;

    if ( lhs.has_value() && rhs.has_value() )
        return ( ( std::abs( lhs.value().first - rhs.value().first ) < 5 ) || ( std::abs( lhs.value().second - rhs.value().second ) < 5 ) );
    else
        return true;
}

bool SMovieStub::equal( const SMovieStub &rhs, bool useName, bool useYear, bool useResolution ) const
{
    bool retVal = true;
    if ( useName )
        retVal = retVal && nameKey() == rhs.nameKey();
    if ( useYear && hasYear() && rhs.hasYear() )
        retVal = retVal && compareYear( fYear.value(), rhs.fYear.value() );
    if ( useResolution && hasResolution() && rhs.hasResolution() )
        retVal = retVal && resolutionMatches( fResolution, rhs.fResolution );
    return retVal;
}

bool SMovieStub::equal( std::shared_ptr< CMediaData > mediaData, bool useName, bool useYear, bool useResolution ) const
{
    bool retVal = true;
    if ( useName )
    {
        retVal = nameKey() == SMovieStub::nameKey( mediaData->name() );
        if ( !retVal )
            retVal = nameKey() == SMovieStub::nameKey( mediaData->originalTitle() );
    }
    if ( useYear && hasYear() )
        retVal = retVal && SMovieStub::compareYear( fYear.value(), mediaData->premiereDate().year() );

    if ( useResolution )
        retVal = retVal && resolutionMatches( fResolution, mediaData->resolutionValue() );

    return retVal;
}
