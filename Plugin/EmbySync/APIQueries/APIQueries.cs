using System.IO.Pipes;
using System.Linq;
using System.Net;
using System.Threading;
using System.Threading.Tasks;
using MediaBrowser.Controller;
using MediaBrowser.Controller.Api;
using MediaBrowser.Controller.Configuration;
using MediaBrowser.Model.ApiClient;
using MediaBrowser.Model.Logging;
using MediaBrowser.Model.System;

namespace EmbySync.APIQueries
{
    public class ApiQueries
    {
        public static ApiQueries Instance { get; set; }
        private readonly ILogger _log;
        private readonly IServerApplicationHost _serverApplicationHost;
        

        public ApiQueries(ILogManager logManager, IServerApplicationHost serverApplicationHost)
        {
            _serverApplicationHost = serverApplicationHost;
            Instance = this;
            _log = logManager.GetLogger(Plugin.Instance.Name);
        }


        public ItemsResult GetRemoteServerItems(string url, string userId, string authToken)
        {
            string query = ""; //must include "&format=Json" in order to allow for the items to be read.
            string queryUrl = string.Format("{0}Shows/NextUp?UserId={1}{2}", url, userId, query, authToken); //Query Format taken from Swagger
            //return .GenericApiQuery(queryUrl);//Interrogate the API based on the query string.
            return null;
        }
    }

    public class ItemsResult
    {
    }
}
