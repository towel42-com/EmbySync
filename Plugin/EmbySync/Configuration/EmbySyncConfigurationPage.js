define([
        "loading", "dialogHelper", "mainTabsManager", "formDialogStyle", "emby-checkbox", "emby-select", "emby-toggle",
        "emby-collapse"
    ],
    function(loading, dialogHelper, mainTabsManager) {

        const pluginId = "9231CE3A-DD57-43F1-AC60-4550AE01BD89";

        function getTabs() {
            return [
                {
                    href: Dashboard.getConfigurationPageUrl('EmbySyncConfigurationPage'),
                    name: 'EmbySync'
                }
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

                view.querySelector(".chkEnableEmbySync").checked = config.EnableEmbySync;
                view.querySelector(".chkSyncAudio").checked = config.SyncAudio;
                view.querySelector(".chkSyncVideo").checked = config.SyncVideo;
                view.querySelector(".chkSyncEpisode").checked = config.SyncEpisode;
                view.querySelector(".chkSyncMovie").checked = config.SyncMovie;
                view.querySelector(".chkSyncTrailer").checked = config.SyncTrailer;
                view.querySelector(".chkSyncAdultVideo").checked = config.SyncAdultVideo;
                view.querySelector(".chkSyncMusicVideo").checked = config.SyncMusicVideo;
                view.querySelector(".chkSyncGame").checked = config.SyncGame;
                view.querySelector(".chkSyncBook").checked = config.SyncBook;
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
                
                document.querySelector('.pageTitle').innerHTML = "EmbySync" + '<a is="emby-linkbutton" class="raised raised-mini emby-button" target="_blank" href=""><i class="md-icon button-icon button-icon-left secondaryText headerHelpButtonIcon">help</i><span class="headerHelpButtonText">Help</span></a>';

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

                var enableEmbySync = view.querySelector(".chkEnableEmbySync");
                enableEmbySync.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.EnableEmbySync = enableEmbySync.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncAudio = view.querySelector(".chkSyncAudio");
                syncAudio.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncAudio = syncAudio.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncVideo = view.querySelector(".chkSyncVideo");
                syncVideo.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncVideo = syncVideo.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncEpisode = view.querySelector(".chkSyncEpisode");
                syncEpisode.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncEpisode = syncEpisode.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncMovie = view.querySelector(".chkSyncMovie");
                syncMovie.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncMovie = syncMovie.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncTrailer = view.querySelector(".chkSyncTrailer");
                syncTrailer.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncTrailer = syncTrailer.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });
                var syncAdultVideo = view.querySelector(".chkSyncAdultVideo");
                syncAdultVideo.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncAdultVideo = syncAdultVideo.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncMusicVideo = view.querySelector(".chkSyncMusicVideo");
                syncMusicVideo.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncMusicVideo = syncMusicVideo.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncGame = view.querySelector(".chkSyncGame");
                syncGame.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncGame = syncGame.checked;
                            ApiClient.updatePluginConfiguration(pluginId, config).then((r) => {
                                Dashboard.processPluginConfigurationUpdateResult(r);
                            });
                        });
                    });

                var syncBook = view.querySelector(".chkSyncBook");
                syncBook.addEventListener('change',
                    (e) => {
                        e.preventDefault();
                        ApiClient.getPluginConfiguration(pluginId).then((config) => {
                            config.SyncBook = syncBook.checked;
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