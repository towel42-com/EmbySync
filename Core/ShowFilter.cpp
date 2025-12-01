#include "ShowFilter.h"
#include "MediaData.h"

#include <QVariant>
#include <QRegularExpression>

bool SShowFilter::operator==( const SShowFilter &rhs ) const
{
    if ( fSeriesID != rhs.fSeriesID )
        return false;

    if ( fSeriesName != rhs.fSeriesName )
        return false;

    if ( fPremierYear != rhs.fPremierYear )
        return false;

    if ( fTrackEpisodes != rhs.fTrackEpisodes )
        return false;

    if ( fMinSeason.has_value() != rhs.fMinSeason.has_value() )
        return false;
    if ( fMinSeason.has_value() && ( fMinSeason.value() != rhs.fMinSeason.value() ) )
        return false;

    if ( fMaxSeason.has_value() != rhs.fMaxSeason.has_value() )
        return false;
    if ( fMaxSeason.has_value() && ( fMaxSeason.value() != rhs.fMaxSeason.value() ) )
        return false;

    return true;
}

SShowFilter::SShowFilter( const QString &seriesID, const QString &name, int premierYear, const QString &min, const QString &max, bool trackEpisodes ) :
    fSeriesName( name ),
    fPremierYear( premierYear ),
    fSeriesID( seriesID ),
    fTrackEpisodes( trackEpisodes )
{
    if ( !min.isEmpty() )
    {
        bool aOK = false;
        auto tmp = min.toInt( &aOK );
        if ( aOK )
            fMinSeason = tmp;
    }
    if ( !max.isEmpty() )
    {
        bool aOK = false;
        auto tmp = max.toInt( &aOK );
        if ( aOK )
            fMaxSeason = tmp;
    }
}

SShowFilter::SShowFilter( const QString &seriesID, const QString &name, int premierYear, const QVariant &min, const QVariant &max, bool trackEpisodes ) :
    SShowFilter( seriesID, name, premierYear, ( min.isValid() && min.canConvert< QString >() && min.canConvert< int >() ) ? min.toString() : QString(), ( max.isValid() && max.canConvert< QString >() && max.canConvert< int >() ) ? max.toString() : QString(), trackEpisodes )
{
}

bool SShowFilter::showEpisode( const std::optional< TFilterMap > &filterMap, std::shared_ptr< CMediaData > mediaData )
{
    if ( !mediaData || !mediaData->season().has_value() )
        return false;

    return showEpisode( filterMap, mediaData->seriesID(), mediaData->seriesName(), mediaData->season().value(), {} );
}

bool SShowFilter::showEpisode( const std::optional< TFilterMap > &filterMap, const QString &seriesID, const QString &seriesName, int seasonNum, std::optional< QRegularExpression > regEx )
{
    if ( !filterMap.has_value() )
        return true;

    auto key = CMediaData::seriesSearchKey( seriesName, seriesID );
    auto pos = filterMap.value().find( key );
    auto hasShowFilter = pos != filterMap.value().end();
    if ( hasShowFilter )
    {
        auto &&filter = ( *pos ).second;
        if ( !filter->fTrackEpisodes )
            return false;

        std::optional< bool > seasonMatch;   // 3 states, not set by the filter, set and the season num matches, set and the season num doesnt
        if ( filter->fMinSeason.has_value() )
        {
            seasonMatch = ( seasonNum >= filter->fMinSeason.value() );
        }

        if ( filter->fMaxSeason.has_value() )
        {
            if ( seasonMatch.has_value() )
                seasonMatch = seasonMatch.value();
            else
                seasonMatch = ( seasonNum <= filter->fMaxSeason.value() );
        }

        if ( seasonMatch.has_value() && !seasonMatch.value() )   // if the season doesnt match either because there was a season filter and its outside the range, dont show
            return false;
    }

    // seasonal filter failed to remove the show

    if ( regEx.has_value() && regEx.value().isValid() )
    {
        auto match = regEx.value().match( seriesName );
        bool isMatch = ( match.hasMatch() && ( match.captured( 0 ).length() == seriesName.length() ) );
        if ( !isMatch )
            return false;
    }

    return true;
}
