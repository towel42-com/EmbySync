#ifndef __SHOWFILTER_H
#define __SHOWFILTER_H
#include <QString>
#include <map>
#include <optional>
#include <memory>
class QVariant;

class CMediaData;
struct SShowFilter;
using TFilterMap = std::map< QString, std::shared_ptr< SShowFilter > >;

struct SShowFilter
{
    SShowFilter() = default;
    SShowFilter( const QString &seriesID, const QString &name, int premierYear, const QString &min, const QString &max, bool trackEpisodes );
    SShowFilter( const QString &seriesID, const QString &name, int premierYear, const QVariant &min, const QVariant &max, bool trackEpisodes );

    static bool showEpisode( const std::optional< TFilterMap > &filterMap, const QString &seriesID, const QString &seriesName, int seasonNum, std::optional< QRegularExpression > regEx );
    static bool showEpisode( const std::optional< TFilterMap > &filterMap, std::shared_ptr< CMediaData > mediaData );

    bool operator==( const SShowFilter &rhs ) const;
    bool operator!=( const SShowFilter &rhs ) const { return !operator==( rhs ); }

    QString fSeriesID;
    QString fSeriesName;
    int fPremierYear{ 0 };
    std::optional< int > fMinSeason;
    std::optional< int > fMaxSeason;

    bool fTrackEpisodes{ true };
};

using TFilterMap = std::map< QString, std::shared_ptr< SShowFilter > >;

#endif
