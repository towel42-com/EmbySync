using System;
using System.Collections.Generic;
using System.IO;
using Emby.EmbySync.Configuration;
using MediaBrowser.Common.Configuration;
using MediaBrowser.Common.Plugins;
using MediaBrowser.Model.Drawing;
using MediaBrowser.Model.Plugins;
using MediaBrowser.Model.Serialization;

namespace Emby.EmbySync
{
	public class Plugin : BasePlugin<PluginConfiguration>, IHasWebPages, IHasThumbImage
	{
        public static Plugin Instance { get; set; }

        //You will need to generate a new GUID and paste it here - Tools => Create GUID
        private Guid _id = new Guid("9231CE3A-DD57-43F1-AC60-4550AE01BD89");
       
        public override string Name => "EmbySync";

		public override string Description => "Sync User Data across servers";

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
                Name = "EmbySyncConfigurationPage",
                EmbeddedResourcePath = GetType().Namespace + ".Configuration.EmbySyncConfigurationPage.html",
                EnableInMainMenu = false,
                /*MenuSection = "server",*/
                //MenuIcon = "theaters"
            },
            new PluginPageInfo
            {
                //javascript file
                Name = "EmbySyncConfigurationPageJS",
                EmbeddedResourcePath = GetType().Namespace + ".Configuration.EmbySyncConfigurationPage.js"
            }
        };

        
        

        
    }
}
