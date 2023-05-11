#ifndef __MOVIESTUB_H
#define __MOVIESTUB_H

#include <QString>
#include <utility>
#include <memory>
#include <optional>
class QPoint;
class QJsonObject;
class CMediaData;
struct SMovieStub
{
    QString fName;
    int fYear{ 0 };
    std::optional< std::pair< int, int > > fResolution;

    SMovieStub( const QString &name );
    SMovieStub( const QString &name, int year );
    SMovieStub( const QString &name, int year, const std::optional< std::pair< int, int > > &resolution );
    SMovieStub( const QString &name, int year, const QPoint &resolution );
    SMovieStub( std::shared_ptr< CMediaData > data );
    bool isMovie( const QString &movieName ) const { return nameKey() == nameKey( movieName ); }

    QString nameKey() const { return nameKey( fName ); }
    bool hasResolution() const
    {
        if ( !fResolution.has_value() )
            return false;
        return ( fResolution.value() != std::make_pair( 0, 0 ) ) && ( fResolution.value() != std::make_pair( -1, -1 ) );
    }

    QString resolution() const
    {
        if ( hasResolution() )
            return QString( "%1x%2" ).arg( fResolution.value().first ).arg( fResolution.value().second );
        else
            return {};
    }

    static QString nameKey( const QString &name );

    bool operator==( const SMovieStub &r ) const { return nameKey() == r.nameKey(); }
    QJsonObject toJSON() const;
    std::size_t hash( bool useName, bool useYear, bool useResolution ) const;
    bool equal( const SMovieStub &rhs, bool useName, bool useYear, bool useResolution ) const;
    bool equal( std::shared_ptr< CMediaData > mediaData, bool useName, bool useYear, bool useResolution ) const;

    QJsonObject toJson() const;
};

struct SNameHash
{
    size_t operator()( const SMovieStub &movie ) const { return movie.hash( true, false, false ); }
};
struct SNameCompare
{
    bool operator()( const SMovieStub &lhs, const SMovieStub &rhs ) const { return lhs.equal( rhs, true, false, false ); };
};
struct SNameYearHash
{
    size_t operator()( const SMovieStub &movie ) const { return movie.hash( true, true, false ); }
};
struct SNameYearCompare
{
    size_t operator()( const SMovieStub &lhs, const SMovieStub &rhs ) const { return lhs.equal( rhs, true, true, false ); }
};

struct SFullHash
{
    size_t operator()( const SMovieStub &movie ) const { return movie.hash( true, true, true ); }
};
struct SFullCompare
{
    size_t operator()( const SMovieStub &lhs, const SMovieStub &rhs ) const { return lhs.equal( rhs, true, true, true ); }
};

#endif
