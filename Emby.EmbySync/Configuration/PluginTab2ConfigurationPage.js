define([
        "loading", "dialogHelper", "mainTabsManager", "formDialogStyle", "emby-checkbox", "emby-select", "emby-toggle",
        "emby-collapse"
    ],
    function(loading, dialogHelper, mainTabsManager) {

        const pluginId = "9231CE3A-DD57-43F1-AC60-4550AE01BD89";

        function getTabs() {
            return [
                {
                    href: Dashboard.getConfigurationPageUrl('ServerSyncConfigurationPage'),
                    name: 'Server-Sync'
                }/*,
                {
                    href: Dashboard.getConfigurationPageUrl('PluginTab2ConfigurationPage'),
                    name: 'PluginTab 2'
                },
                {
                    href: Dashboard.getConfigurationPageUrl('PluginTab3ConfigurationPage'),
                    name: 'PluginTab 3'
                }*/
            ];
        }


        return function(view) {
            view.addEventListener('viewshow',
                async () => {

                    loading.show();
                    mainTabsManager.setTabs(this, 1, getTabs);

                    var config = await ApiClient.getPluginConfiguration(pluginId);

                    ApiClient.getPluginConfiguration(pluginId).then(function(config) {


                        
                    });
                    loading.hide();
                });
        }
    });
    