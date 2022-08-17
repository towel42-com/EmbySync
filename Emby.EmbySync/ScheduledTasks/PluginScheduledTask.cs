using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Emby.EmbySync.Configuration;
using Emby.EmbySync.Models;
using MediaBrowser.Common.Net;
using MediaBrowser.Controller;
using MediaBrowser.Controller.Entities;
using MediaBrowser.Controller.Library;
using MediaBrowser.Model.Logging;
using MediaBrowser.Model.System;
using MediaBrowser.Model.Tasks;

namespace Emby.EmbySync.ScheduledTasks
{
	//Use this section if you need to have Scheduled tasks run
    public class PluginScheduledTask : IScheduledTask, IConfigurableScheduledTask
    {
        private readonly ILibraryManager LibraryManager;

        private readonly ILogger _log;
        private readonly IServerApplicationHost _serverApplicationHost;
        // private readonly IUserDataManager _userDataManager;
        private IHttpClient _httpClient;


        public string Name => "EmbySync";

        public string Key => "EmbySync";

        public string Description => "Run this task to sync User Data across servers";

        public string Category => "EmbySync";

        public bool IsHidden => false;

        public bool IsEnabled => true;

        public bool IsLogged => true;

        //Constructor
        public PluginScheduledTask(ILibraryManager libraryManager, ILogManager logManager, IServerApplicationHost serverApplicationHost, IHttpClient httpClient)
        {
            LibraryManager = libraryManager;
            _serverApplicationHost = serverApplicationHost;
            _httpClient = httpClient;
            _log = logManager.GetLogger(Plugin.Instance.Name);
            
        }

        //progressBar fields
        //private double _totalProgress;
        //private int _totalItems;

        //Get Library Item fields
        private BaseItem[] _itemsInLibraries;
        private int _numberOfItemsInLibraries;


        //Task that will execute from the SheduleTask Menu
        public Task Execute(CancellationToken cancellationToken, IProgress<double> progress)
        {
            //Do work here for your Scheduled Task
            GetItemsInEmbyLibraries().ConfigureAwait(false);
            ConnectToServers();
            GetAuthInfo();

            return Task.CompletedTask;

        }

        //Task Triggers - Currently unset, user can set these themselves in the menu.
        public IEnumerable<TaskTriggerInfo> GetDefaultTriggers()
        {
            return new List<TaskTriggerInfo>();
        }

        private async Task GetItemsInEmbyLibraries()
        {
            try
            {
                _log.Info("Getting Library Items");

                var config = Plugin.Instance.Configuration;
                List<string> mediaTypeList = new List<string>();
                if (config.SyncAudio )
                    mediaTypeList.Add("Audio");
                if (config.SyncVideo )
                    mediaTypeList.Add("Video");
                if (config.SyncMovie)
                    mediaTypeList.Add("Movie");
                if (config.SyncTrailer)
                    mediaTypeList.Add("Trailer");
                if (config.SyncAdultVideo)
                    mediaTypeList.Add("AdultVideo");
                if (config.SyncMusicVideo)
                    mediaTypeList.Add("MusicVideo");
                if (config.SyncGame)
                    mediaTypeList.Add("Game");
                if (config.SyncBook)
                    mediaTypeList.Add("Book");
                string[] mediaTypeArray = mediaTypeList.ToArray();
                InternalItemsQuery queryList = new InternalItemsQuery
                {
                    Recursive = true,
                    IncludeItemTypes = mediaTypeArray,
                    IsVirtualItem = false,
                };

                _itemsInLibraries = LibraryManager.GetItemList(queryList);
                _numberOfItemsInLibraries = _itemsInLibraries.Length;
                _log.Info("Total No. of items in Library {0}", _numberOfItemsInLibraries.ToString());
            }
            catch (Exception ex)
            {
                _log.Error("No Lib Items Found in Library");
                _log.Error(ex.ToString());
            }
        }

        public Task<PublicSystemInfo> ConnectToServers()
        {
            _log.Debug("Getting Server Info");
            IPAddress ip = IPAddress.Parse("127.0.0.1");
            var serverInfo = _serverApplicationHost.GetPublicSystemInfo(CancellationToken.None);
            var systemInfo = _serverApplicationHost.GetSystemInfo(ip, CancellationToken.None);
            if (serverInfo != null)
            {
                 _log.Debug("Server Info: {0}", serverInfo.Result.ServerName);
                 _log.Debug("System Info: {0}", systemInfo.Result.InternalMetadataPath);
            }
            return serverInfo;
        }

        public void GetAuthInfo()
        {
            HttpRequestOptions request = new HttpRequestOptions();
            
        }

        /*
        private async Task ConnectToOtherServers()
        {
            var apiClient = BaseApiService.Request.Authorization;
            var config = Plugin.Instance.Configuration;
            HttpClient client = new HttpClient();
            var servers = config.Servers;
            foreach (var server in servers)
            {
                
                var response = client.GetAsync(server.Address).Result;
                var result = response.Content.ReadAsStreamAsync().Result;
                Log.Info("Connected to {0}", server.Address);
                Log.Info("Response {0}", response.StatusCode.ToString());
            }
        }
        */

        private async Task SendUdpDatagram()
        {
            //var serverIp = "255.255.255.255";
            var serverPort = 7359;
            var message = "who is EmbyServer?";
            try
            {
                _log.Info("Sending UDP Datagram to {0}:{1}", IPAddress.Broadcast, serverPort);
                var udpClient = new UdpClient();
                var remoteIpEndPoint = new IPEndPoint(IPAddress.Broadcast, serverPort);
                
                var sendBytes = Encoding.ASCII.GetBytes(message);
                udpClient.SendAsync(sendBytes, sendBytes.Length, remoteIpEndPoint);


                UdpReceiveResult receiveBytes = await udpClient.ReceiveAsync();
                var returnMessage = Encoding.ASCII.GetString(receiveBytes.Buffer);
                
                    _log.Info("UDP Datagram Received from {0}:{1}", IPAddress.Any, serverPort);
                    _log.Info(returnMessage);
                    await DeserializeJson(returnMessage).ConfigureAwait(false);

                
            }
            catch (Exception ex)
            {
                _log.Error("UDP Datagram Failed to Send");
                _log.Error(ex.ToString());
            }
        }

        private async Task DeserializeJson(string json)
        {
            var config = Plugin.Instance.Configuration;
            if (json == null)
            {
                _log.Error("MEDIAINFO JSON IS EMPTY - CONTACT THE BIG CHEESE!!");
                return;
            }

            var serverInfo = JsonSerializer.Deserialize<DatagramServerModel>(json);

            var servers = config.Servers.ToList();
            
            servers.Add(new Server
            {
                Name = serverInfo.Name,
                Address = serverInfo.Address,
            });

            //Becauset there is no physical indexes in the Server Class, this method prevents discovered servers duplicating in the list.
            List<Server> noDupeList = servers.GroupBy(s => s.Name).Select(grp => grp.FirstOrDefault()).OrderBy(s => s.Name).ToList();
            
            servers = noDupeList;
            Plugin.Instance.UpdateConfiguration(config);

            foreach (var server in servers)
            {
                _log.Info("Server Name: {0}", server.Name);
                _log.Info("Server Address: {0}", server.Address);
            }
        }

        private async Task SendUDPBroadcast()
        {
            var serverPort = 7359;
            var message = "who is EmbyServer?";
            
            UdpClient client = new UdpClient(new IPEndPoint(IPAddress.Any, 0));
            IPEndPoint endpoint = new IPEndPoint(IPAddress.Parse("255.255.255.255"), serverPort);
            byte[] buf = Encoding.Default.GetBytes(message);
            Thread t = new Thread(new ThreadStart(RecvThread));
            t.IsBackground = true;
            t.Start();
            while (true)
            {
                client.Send(buf, buf.Length, endpoint);
                Thread.Sleep(1000);
            }
        }

        private void RecvThread()
        {  
           UdpClient client = new UdpClient(new IPEndPoint(IPAddress.Any, 7359));  
           IPEndPoint endpoint = new IPEndPoint(IPAddress.Any, 0);  
           while (true)  
           {  
               byte[] buf = client.Receive(ref endpoint);  
               string msg = Encoding.Default.GetString(buf);  
               _log.Debug(msg);  
           }  
        }

        
    }
}
