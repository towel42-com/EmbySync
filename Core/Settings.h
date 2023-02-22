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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <QString>
#include <QColor>
#include <QUrl>
#include <QRegularExpression>

#include <memory>
#include <tuple>
#include <functional>
#include <vector>
#include <map>
#include <optional>
#include <set>

class QWidget;
class QJsonObject;
class CServerModel;
namespace Ui
{
    class CSettings;
}

class CSettings
{
public:
    CSettings(std::shared_ptr< CServerModel > serverModel);
    CSettings(bool saveOnDelete, std::shared_ptr< CServerModel > serverModel);
    virtual ~CSettings();

    // global settings store in the registry
    static bool checkForLatest();
    static void setCheckForLatest(bool value);

    static bool loadLastProject();
    static void setLoadLastProject(bool value);

    // settings stored in the json settings file
    QString fileName() const
    {
        return fFileName;
    }

    bool load(bool addToRecentFileList, QWidget* parent);
    bool load(const QString& fileName, bool addToRecentFileList, QWidget* parent);
    bool load(const QString& fileName, std::function<void(const QString& title, const QString& msg)> errorFunc, bool addToRecentFileList);

    bool save();

    bool save(QWidget* parent);
    bool saveAs(QWidget* parent);
    bool maybeSave(QWidget* parent);
    bool save(QWidget* parent, std::function<QString()> selectFileFunc, std::function<void(const QString& title, const QString& msg)> errorFunc);
    bool save(std::function<void(const QString& title, const QString& msg)> errorFunc);

    bool changed() const
    {
        return fChanged;
    }
    void reset();

    // other settings
    QColor mediaSourceColor(bool forBackground = true) const;
    void setMediaSourceColor(const QColor& color);

    QColor mediaDestColor(bool forBackground = true) const;
    void setMediaDestColor(const QColor& color);

    QColor dataMissingColor(bool forBackground = true) const;
    void setDataMissingColor(const QColor& color);

    int maxItems() const
    {
        return fMaxItems;
    }
    void setMaxItems(int maxItems);

    bool syncAudio() const
    {
        return fSyncAudio;
    }
    void setSyncAudio(bool value);

    bool syncVideo() const
    {
        return fSyncVideo;
    }
    void setSyncVideo(bool value);

    bool syncEpisode() const
    {
        return fSyncEpisode;
    }
    void setSyncEpisode(bool value);

    bool syncMovie() const
    {
        return fSyncMovie;
    }
    void setSyncMovie(bool value);

    bool syncTrailer() const
    {
        return fSyncTrailer;
    }
    void setSyncTrailer(bool value);

    bool syncAdultVideo() const
    {
        return fSyncAdultVideo;
    }
    void setSyncAdultVideo(bool value);

    bool syncMusicVideo() const
    {
        return fSyncMusicVideo;
    }
    void setSyncMusicVideo(bool value);

    bool syncGame() const
    {
        return fSyncGame;
    }
    void setSyncGame(bool value);

    bool syncBook() const
    {
        return fSyncBook;
    }
    void setSyncBook(bool value);

    QString getSyncItemTypes() const;
    bool onlyShowSyncableUsers()
    {
        return fOnlyShowSyncableUsers;
    };
    void setOnlyShowSyncableUsers(bool value);

    bool onlyShowMediaWithDifferences()
    {
        return fOnlyShowMediaWithDifferences;
    };
    void setOnlyShowMediaWithDifferences(bool value);

    bool showMediaWithIssues()
    {
        return fShowMediaWithIssues;
    };
    void setShowMediaWithIssues(bool value);

    bool onlyShowUsersWithDifferences()
    {
        return fOnlyShowUsersWithDifferences;
    };
    void setOnlyShowUsersWithDifferences(bool value);

    bool showUsersWithIssues()
    {
        return fShowUsersWithIssues;
    };
    void setShowUsersWithIssues(bool value);

    bool onlyShowEnabledServers()
    {
        return fOnlyShowEnabledServers;
    };
    void setOnlyShowEnabledServers(bool value);

    QStringList syncUserList() const
    {
        return fSyncUserList;
    }
    void setSyncUserList(const QStringList& value);

    std::set< QString > ignoreShowList() const
    {
        return fIgnoreShowList;
    }
    QRegularExpression ignoreShowRegEx() const;

    void setIgnoreShowList(const QStringList& value);

    void addRecentProject(const QString& fileName);
    QStringList recentProjectList() const;

    void setPrimaryServer(const QString& serverName);
    QString primaryServer() const;
private:
    QVariant getValue(const QJsonObject& data, const QString& fieldName, const QVariant& defaultValue) const;

    QColor getColor(const QColor& clr, bool forBackground /*= true */) const;
    bool maybeSave(QWidget* parent, std::function<QString()> selectFileFunc, std::function<void(const QString& title, const QString& msg)> errorFunc);

    template< typename T >
    void updateValue(T& lhs, const T& rhs)
    {
        if (lhs != rhs)
        {
            lhs = rhs;
            fChanged = true;
        }
    }

    QString fFileName;

    QColor fMediaSourceColor{ "yellow" };
    QColor fMediaDestColor{ "yellow" };
    QColor fMediaDataMissingColor{ "red" };
    int fMaxItems{ -1 };

    bool fOnlyShowSyncableUsers{ true };

    bool fOnlyShowMediaWithDifferences{ true };
    bool fShowMediaWithIssues{ false };

    bool fOnlyShowUsersWithDifferences{ true };
    bool fShowUsersWithIssues{ false };
    bool fOnlyShowEnabledServers{ true };

    bool fSyncAudio{ true };
    bool fSyncVideo{ true };
    bool fSyncEpisode{ true };
    bool fSyncMovie{ true };
    bool fSyncTrailer{ true };
    bool fSyncAdultVideo{ true };
    bool fSyncMusicVideo{ true };
    bool fSyncGame{ true };
    bool fSyncBook{ true };

    QStringList fSyncUserList;
    std::set< QString > fIgnoreShowList;

    QString fPrimaryServer;

    std::shared_ptr< CServerModel > fServerModel;
    bool fChanged{ false };

    bool fSaveOnDelete{ true };
};

#endif 
