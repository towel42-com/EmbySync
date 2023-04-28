#ifndef __MOVIESTUB_H
#define __MOVIESTUB_H

#include <QString>
#include <utility>
#include <memory>
class QPoint;
class QJsonObject;
class CMediaData;
struct SMovieStub
{
    QString fName;
    int fYear{ 0 };
    std::pair< int, int > fResolution{ 0, 0 };

    SMovieStub( const QString &name );
    SMovieStub( const QString &name, int year );
    SMovieStub( const QString &name, int year, const std::pair< int, int > &resolution );
    SMovieStub( const QString &name, int year, const QPoint &resolution );
    SMovieStub( std::shared_ptr< CMediaData > data );
    bool isMovie( const QString &movieName ) const { return nameKey() == nameKey( movieName ); }

    QString nameKey() const { return nameKey( fName ); }
    QString resolution() const { return QString( "%1x%2" ).arg( fResolution.first ).arg( fResolution.second ); }

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
