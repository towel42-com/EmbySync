using System.Collections.Generic;
using System.Net;
using MediaBrowser.Model.Plugins;

namespace Emby.EmbySync.Configuration
{
    public class PluginConfiguration : BasePluginConfiguration
    {
        //User Configuration Files
        public bool EnableEmbySync { get; set; }
        public List<Server> Servers { get; set; }

        public bool SyncAudio { get; set; } = true;
        public bool SyncVideo { get; set; } = true;
        public bool SyncEpisode { get; set; } = true;
        public bool SyncMovie { get; set; } = true;
        public bool SyncTrailer { get; set; } = true;
        public bool SyncAdultVideo { get; set; } = true;
        public bool SyncMusicVideo { get; set; } = true;
        public bool SyncGame { get; set; } = true;
        public bool SyncBook { get; set; } = true;

        public PluginConfiguration()
        {
            //add default values here to use
            EnableEmbySync = true;
           Servers = new List<Server>();

        }
    }
    public class Server
    {
        public string Name { get; set; }
        public string Address { get; set; }
    }
}
