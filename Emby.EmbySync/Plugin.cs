using System;
using System.Collections.Generic;
using System.IO;
using Emby.ServerSync.Configuration;
using MediaBrowser.Common.Configuration;
using MediaBrowser.Common.Plugins;
using MediaBrowser.Model.Drawing;
using MediaBrowser.Model.Plugins;
using MediaBrowser.Model.Serialization;

namespace Emby.ServerSync
{
	public class Plugin : BasePlugin<PluginConfiguration>, IHasWebPages, IHasThumbImage
	{
        public static Plugin Instance { get; set; }

        //You will need to generate a new GUID and paste it here - Tools => Create GUID
        private Guid _id = new Guid("9231CE3A-DD57-43F1-AC60-4550AE01BD89");
       
        public override string Name => "Server Sync";

		public override string Description => "Sync Playstate data across multiple servers";

		public override Guid Id => _id;
        
        public Plugin(IApplicationPaths applicationPaths, IXmlSerializer xmlSerializer) : base(applicationPaths,
            xmlSerializer)
        {
            Instance = this;
        }
        public ImageFormat ThumbImageFormat => ImageFormat.Jpg;

        //Display Thumbnail image for Plugin Catalogue  - you will need to change build action for thumb.jpg to embedded Resource
        public Stream GetThumbImage()
        {
            Type type = GetType();
            return type.Assembly.GetManifestResourceStream(type.Namespace + ".thumb.jpg");
        }

        //Web pages for Server UI configuration
        public IEnumerable<PluginPageInfo> GetPages() => new[]
        {

            new PluginPageInfo
            {
                //html File
                Name = "ServerSyncConfigurationPage",
                EmbeddedResourcePath = GetType().Namespace + ".Configuration.ServerSyncConfigurationPage.html",
                EnableInMainMenu = false,
                /*MenuSection = "server",*/
                //MenuIcon = "theaters"
            },
            new PluginPageInfo
            {
                //javascript file
                Name = "ServerSyncConfigurationPageJS",
                EmbeddedResourcePath = GetType().Namespace + ".Configuration.ServerSyncConfigurationPage.js"
            },
            new PluginPageInfo
            {
                //html File
                Name = "PluginTab2ConfigurationPage",
                EmbeddedResourcePath = GetType().Namespace + ".Configuration.PluginTab2ConfigurationPage.html",
            },
            new PluginPageInfo
            {
                //javascript file
                Name = "PluginTab2ConfigurationPageJS",
                EmbeddedResourcePath = GetType().Namespace + ".Configuration.PluginTab2ConfigurationPage.js"
            },
            new PluginPageInfo
            {
                //html File
                Name = "PluginTab3ConfigurationPage",
                EmbeddedResourcePath = GetType().Namespace + ".Configuration.PluginTab3ConfigurationPage.html",
            },
            new PluginPageInfo
            {
                //javascript file
                Name = "PluginTab3ConfigurationPageJS",
                EmbeddedResourcePath = GetType().Namespace + ".Configuration.PluginTab3ConfigurationPage.js"
            },

        };

        
        

        
    }
}
