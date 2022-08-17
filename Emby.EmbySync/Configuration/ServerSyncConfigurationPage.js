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

        const serverListContainer = document.getElementById('serverListContainer');
        let serverItemElements = [];
        let ServerList = [];

        function RenderIntroItems() {
            return ServerList.map(function (serverItem) {

                let listItem = `
                <li class="introItemContainer listItem listItem-border metadataFetcherItem sortableOption" draggable="true" style="display: flex; flex-direction: row; align-items: center; justify-content: flex-start; width: 95%; height: 100%; gap: 10px;">

                    <div id="introNameId" class="introName" style="display: flex;font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; font-size: 1.5em; font-weight: 400;width: 200px;
                        color: #fff;transform: rotateZ(0deg); align-self: center;margin-right: 3em; margin-left: 3rem;"
                        data-item-name=${serverItem.Name}                        
                        data-item-address=${serverItem.Address}>
                        ${serverItem.Name}
                    </div>

                    <div id="introAddressId" class="introName" style="display: flex;font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; font-size: 1.5em; font-weight: 400;width: 200px;
                        color: #fff;transform: rotateZ(0deg); align-self: center;margin-right: 3em; margin-left: 3rem;">                        
                        ${serverItem.Address}
                    </div>               
                    <div style="margin-bottom: 3rem;"><div>
                </li>
                `;
                serverItemElements.push(listItem);
                return listItem;
            }).join('');
        }

        function LoadConfig(view, config) {

            ServerList = [];
            serverItemElements = [];
            serverListContainer.innerHTML = "";
            var serverList = document.getElementById('serverListContainer');

            ApiClient.getPluginConfiguration(pluginId).then(function (config) {

                view.querySelector(".chkEnableServerSync").checked = config.EnableServerSync;
                view.querySelector(".chkSyncMovies").checked = config.SyncMovies;
                view.querySelector(".chkSyncShows").checked = config.SyncShows;
                view.querySelector(".chkSyncMusic").checked = config.SyncMusic;
                var storedServers = config.Servers;

                console.log("storedItems: ", storedServers);
                storedServers.map(function(server) {
                    ServerList.push(server);
                });
                var itemElements = RenderIntroItems();
                serverList.innerHTML = (itemElements);
            });
        }

        return function (view) {
            view.addEventListener('viewshow', async () => {

                loading.show();

                mainTabsManager.setTabs(this, 0, getTabs);

                var config = await ApiClient.getPluginConfiguration(pluginId);
                LoadConfig(view, config);

                loading.hide();
                
                document.querySelector('.pageTitle').innerHTML = "Server Sync" + '<a is="emby-linkbutton" class="raised raised-mini emby-button" target="_blank" href=""><i class="md-icon button-icon button-icon-left secondaryText headerHelpButtonIcon">help</i><span class="headerHelpButtonText">Help</span></a>';

                var addServerOptions = view.querySelector("#AddServerOptions");
                var addServerBtn = view.querySelector("#btnAddNewServer");
                var saveServerBtn = view.querySelector("#btnSaveServer");
                addServerOptions.style.display = "none";
                
                addServerBtn.addEventListener('click', (e) => {
                    e.preventDefault();
                    addServerOptions.style.display = "flex";
                });

                saveServerBtn.addEventListener('click', (e) => {
                    e.preventDefault();

                    var server = new Server();
                    server.Name = view.querySelector("#txtServerName", view).value || "";
                    server.Address = view.querySelector("#txtServerAddress", view).value || "";

                    ServerList.push(server);
                    //console.log("VP Intro Items Array:", VPIntroArray);
                    config.Servers = ServerList;

                    ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                        Dashboard.processPluginConfigurationUpdateResult(r);
                        LoadConfig(view, config);
                    });

                    addServerOptions.style.display = "none";
                });

                var enableServerSync = view.querySelector(".chkEnableServerSync");
                enableServerSync.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.EnableServerSync = enableServerSync.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncMovies = view.querySelector(".chkSyncMovies");
                syncMovies.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncMovies = syncMovies.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncShows = view.querySelector(".chkSyncShows");
                syncShows.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncShows = syncShows.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncMusic = view.querySelector(".chkSyncMusic");
                syncMusic.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncMusic = syncMusic.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });
            });
        }
    });

class Server {
    constructor() {
        this.Name = "";
        this.Address = "";
    }
}