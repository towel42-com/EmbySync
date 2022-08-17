using MediaBrowser.Controller.Configuration;
using MediaBrowser.Controller.Plugins;
using MediaBrowser.Model.Tasks;

namespace Emby.ServerSync.ServerEntryPoint
{
	public class PluginEntryPoint : IServerEntryPoint
	{
		private readonly IServerConfigurationManager _config;

		private readonly ITaskManager _taskManager;

		public PluginEntryPoint(IServerConfigurationManager config, ITaskManager taskManager)
		{
			_config = config;
			_taskManager = taskManager;
		}

		public void Run()
		{
			//TODO: default to local ip
		}

		

		public void Dispose()
		{
		}
	}
}
