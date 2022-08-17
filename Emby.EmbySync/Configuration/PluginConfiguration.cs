using System.Collections.Generic;
using System.Net;
using MediaBrowser.Model.Plugins;

namespace Emby.ServerSync.Configuration
{
    public class PluginConfiguration : BasePluginConfiguration
    {
        //User Configuration Files
        public bool EnableServerSync { get; set; }
        public List<Server> Servers { get; set; }
        public bool SyncMovies { get; set; } = true;
        public bool SyncShows { get; set; } = true;
        public bool SyncMusic { get; set; } = true;

        public PluginConfiguration()
        {
            //add default values here to use
           EnableServerSync = true;
           Servers = new List<Server>();

        }
    }
    public class Server
    {
        public string Name { get; set; }
        public string Address { get; set; }
    }
}
