define(["dom", "loading", "globalize", "emby-linkbutton", "emby-checkbox", "emby-input", "emby-button"], function (dom, loading, globalize) {
    "use strict";

    function loadPage(page, config) {

        page.querySelector(".chkBigScreen").checked = config.EnableVP;
        //page.querySelector('.chkEpisodes').checked = config.EnableIntrosForEpisodes;
        page.querySelector("#txtTimeLimit", page).value = config.TimeLimit;

        page.querySelector(".chkEnableWatchedBumpers").checked = config.EnableWatchedBumpers;
        page.querySelector(".chkDisableWatchedMovies").checked = config.DisableWatchedMovies;
        page.querySelector("#txtWatchedBumperPath", page).value = config.WatchedBumperPath || "";

        page.querySelector(".chkEnablePersonalBumpers").checked = config.EnablePersonalBumper;
        page.querySelector("#txtPersonalBumperPath", page).value = config.PersonalBumperPath || "";

        page.querySelector(".chkUpcomingTheaterTrailers").checked = config.EnableIntrosFromUpcomingTrailers;
        page.querySelector(".chkUpcomingDvdTrailers").checked = config.EnableIntrosFromUpcomingDvdMovies;
        page.querySelector(".chkUpcomingStreamingTrailers").checked = config.EnableIntrosFromUpcomingStreamingMovies;
        page.querySelector(".chkOtherTrailers").checked = config.EnableIntrosFromSimilarMovies;

        page.querySelector(".chkMyMovieTrailers").checked = config.EnableIntrosFromMoviesInLibrary;

        page.querySelector("#txtNumTrailers", page).value = config.TrailerLimit;
        page.querySelector(".chkEnableParentalControl").checked = config.EnableIntrosParentalControl;
        page.querySelector(".chkUnwatchedOnly").checked = !config.EnableIntrosForWatchedContent;
        
        page.querySelector("#txtTriviaIntrosPath", page).value = config.TriviaIntroPath || "";
        page.querySelector("#txtCustomIntrosPath", page).value = config.CustomIntroPath || "";
        page.querySelector("#txtNumAds", page).value = config.AdsLimit;
        page.querySelector("#txtNumTrivia", page).value = config.TriviaLimit;

        page.querySelector("#txtCourtesyPath", page).value = config.CourtesyPath || "";

        page.querySelector(".chkBBFCTitleCert").checked = config.EnableBBFCTitleCert;
        page.querySelector("#txtAgeRatingPath", page).value = config.RatingPath || "";
        page.querySelector("#txtCountdownPath", page).value = config.CountdownPath || "";
        page.querySelector("#txtComingSoonPath", page).value = config.ComingSoonPath || "";
        page.querySelector("#txtFeaturePath", page).value = config.FeatureBumperPath || "";
        page.querySelector("#txtCodecIntrosPath", page).value = config.MediaInfoIntroPath || "";

        page.querySelector("#txtCurrentDevice", page).value = config.DeviceName + " - " + config.ClientName || "";

        page.querySelector("#txtAllOffActionUrl",page).value = config.AllOffActionUrl || "";
        page.querySelector("#txtTriviaIntrosActionUrl", page).value = config.TriviaIntroActionUrl || "";
        page.querySelector("#txtComingSoonActionUrl", page).value = config.AttractionActionUrl || "";
        page.querySelector("#txtCodecActionUrl", page).value = config.CodecActionUrl || "";
        page.querySelector("#txtFeatureActionUrl", page).value = config.FeatureActionUrl || "";
        
        

        loading.hide();
    }

    function onSubmit(e) {
        loading.show();

        var form = this;

        var page = dom.parentWithClass(form, "page");

        ApiClient.getNamedConfiguration("PluginName").then(function (config) {

	        config.EnableVP = page.querySelector(".chkBigScreen").checked;
	        //config.EnableIntrosForEpisodes = page.querySelector('.chkEpisodes').checked;
	        config.TimeLimit = page.querySelector("#txtTimeLimit").value;

            config.DisableWatchedMovies = page.querySelector(".chkDisableWatchedMovies").checked;
            config.EnableWatchedBumpers = page.querySelector(".chkEnableWatchedBumpers").checked;
            config.WatchedBumperPath = page.querySelector("#txtWatchedBumperPath").value;

            config.EnablePersonalBumper = page.querySelector(".chkEnablePersonalBumpers").checked;
            config.PersonalBumperPath = page.querySelector("#txtPersonalBumperPath").value;

	        config.EnableIntrosFromUpcomingTrailers = page.querySelector(".chkUpcomingTheaterTrailers").checked;
	        config.EnableIntrosFromUpcomingDvdMovies = page.querySelector(".chkUpcomingDvdTrailers").checked;
	        config.EnableIntrosFromUpcomingStreamingMovies = page.querySelector(".chkUpcomingStreamingTrailers").checked;
	        config.EnableIntrosFromSimilarMovies = page.querySelector(".chkOtherTrailers").checked;

	        config.EnableIntrosFromMoviesInLibrary = page.querySelector(".chkMyMovieTrailers").checked;
            
            config.TrailerLimit = page.querySelector("#txtNumTrailers").value;

            config.EnableIntrosParentalControl = page.querySelector(".chkEnableParentalControl").checked;
            config.EnableIntrosForWatchedContent = !page.querySelector(".chkUnwatchedOnly").checked;
            
            config.TriviaIntroPath = page.querySelector("#txtTriviaIntrosPath").value;
            config.TriviaLimit = page.querySelector("#txtNumTrivia").value;

            config.CustomIntroPath = page.querySelector("#txtCustomIntrosPath").value;
            config.AdsLimit = page.querySelector("#txtNumAds").value;
            config.CourtesyPath = page.querySelector("#txtCourtesyPath").value;
            config.EnableBBFCTitleCert = page.querySelector(".chkBBFCTitleCert").checked;
            config.RatingPath = page.querySelector("#txtAgeRatingPath").value;
            config.CountdownPath = page.querySelector("#txtCountdownPath").value;
            config.ComingSoonPath = page.querySelector("#txtComingSoonPath").value;
            config.FeatureBumperPath = page.querySelector("#txtFeaturePath").value;
            config.MediaInfoIntroPath = page.querySelector("#txtCodecIntrosPath").value;
          
            var x = page.querySelector('#deviceName');
            config.DeviceName = x.options[x.selectedIndex >= 0 ? x.selectedIndex : 0].dataset.name;
            config.ClientName = x.options[x.selectedIndex >= 0 ? x.selectedIndex : 0].dataset.app;            
            
            config.AllOffActionUrl = page.querySelector("#txtAllOffActionUrl").value;
            config.TriviaIntroActionUrl = page.querySelector("#txtTriviaIntrosActionUrl").value;
            config.AttractionActionUrl = page.querySelector("#txtComingSoonActionUrl").value;
            config.CodecActionUrl = page.querySelector("#txtCodecActionUrl").value;
            config.FeatureActionUrl = page.querySelector("#txtFeatureActionUrl").value;
            
            ApiClient.updateNamedConfiguration("PluginName", config).then(Dashboard.processServerConfigurationUpdateResult);
            loadPage(page, config);
        });

        // Disable default form submission
        e.preventDefault();
        e.stopPropagation();
        return false;
    }

    return function (view, params) {

        var page = view;

        //Fill the Device List - This is a sorted list without duplications
            var embyDeviceList = page.querySelector('#deviceName');
            embyDeviceList.innerHTML = "";

            //Get the Available Devices from the Server
            ApiClient.getJSON(ApiClient.getUrl("EmbyDeviceList")).then(
                (devices) => {
                    devices.forEach(
                        (device) => {
                            embyDeviceList.innerHTML +=
                                ('<option value="' + device.Name + '" data-app="' + device.AppName + '" data-name="' + device.Name + '">' + device.Name + ' - ' + device.AppName + '</option>');
                                
                        });
                });

            //var defaultString = 'http://192.168.0.106:3480/data_request?id=action&serviceId=urn:micasaverde-com:serviceId:HomeAutomationGateway1&action=RunScene&SceneNum=13%20HTTP/1.1';
        
        

        page.querySelector("#btnTestTriviaUrl").addEventListener("click", function() {

	        var url = page.querySelector("#txtTriviaIntrosActionUrl").value;
	        
	        var Http = new XMLHttpRequest();
	        Http.open('POST', url);
	        Http.send();
        });
        page.querySelector("#btnTestComingSoonUrl").addEventListener("click", function() {

	        var url = page.querySelector("#txtComingSoonActionUrl").value;
	        var Http = new XMLHttpRequest();
	        Http.open('POST', url);
	        Http.send();
        });
        page.querySelector("#btnTestCodecUrl").addEventListener("click", function() {

	        var url = page.querySelector("#txtCodecActionUrl").value;
	        var Http = new XMLHttpRequest();
	        Http.open('POST', url);
	        Http.send();
        });
        page.querySelector("#btnTestFeatureUrl").addEventListener("click", function() {

	        var url = page.querySelector("#txtFeatureActionUrl").value;
	        var Http = new XMLHttpRequest();
	        Http.open('POST', url);
	        Http.send();
        });

        page.querySelector("#btnTestOffUrl").addEventListener("click", function() {

	        var url = page.querySelector("#txtAllOffActionUrl").value;
	        var Http = new XMLHttpRequest();
	        Http.open('POST', url);
	        Http.send();
        });

        page.querySelector("#btnSelectWatchedBumperPath").addEventListener("click", function () {

	        require(["directorybrowser"], function (directoryBrowser) {

		        var picker = new directoryBrowser();

		        picker.show({

			        callback: function (path) {

				        if (path) {
					        page.querySelector("#txtWatchedBumperPath").value = path;
				        }
				        picker.close();
			        },

			        header: "Select Your Watched Movie Intro/Bumper Path"
		        });
	        });
        });

        page.querySelector("#btnSelectPersonalBumperPath").addEventListener("click", function () {

	        require(["directorybrowser"], function (directoryBrowser) {

		        var picker = new directoryBrowser();

		        picker.show({

			        callback: function (path) {

				        if (path) {
					        page.querySelector("#txtPersonalBumperPath").value = path;
				        }
				        picker.close();
			        },

			        header: "Select Your Personal Bumper Path"
		        });
	        });
        });

        page.querySelector("#btnSelectTriviaIntrosPath").addEventListener("click", function () {

	        require(["directorybrowser"], function (directoryBrowser) {

		        var picker = new directoryBrowser();

		        picker.show({

			        callback: function (path) {

				        if (path) {
					        page.querySelector("#txtTriviaIntrosPath").value = path;
				        }
				        picker.close();
			        },

			        header: "Select Trivia & Quiz Video's Path"
		        });
	        });
        });

        page.querySelector("#btnSelectCustomIntrosPath").addEventListener("click", function () {

            require(["directorybrowser"], function (directoryBrowser) {

                var picker = new directoryBrowser();

                picker.show({

                    callback: function (path) {

                        if (path) {
                            page.querySelector("#txtCustomIntrosPath").value = path;
                        }
                        picker.close();
                    },

                    header: "Select Ads/Funnies Video Path"
                });
            });
        });


        page.querySelector("#btnSelectCodecIntrosPath").addEventListener("click", function () {

            require(["directorybrowser"], function (directoryBrowser) {

                var picker = new directoryBrowser();

                picker.show({

                    callback: function (path) {

                        if (path) {
                            page.querySelector("#txtCodecIntrosPath").value = path;
                        }
                        picker.close();
                    },

                    header: globalize.translate("HeaderSelectCodecIntrosPath")
                });
            });
        });

        page.querySelector("#btnSelecCourtesysPath").addEventListener("click", function () {

	        require(["directorybrowser"], function (directoryBrowser) {

		        var picker = new directoryBrowser();

		        picker.show({

			        callback: function (path) {

				        if (path) {
					        page.querySelector("#txtCourtesyPath").value = path;
				        }
				        picker.close();
			        },

			        header: "Select Courtesy Video Path"
		        });
	        });
        });

        page.querySelector("#btnSelecRatingPath").addEventListener("click", function () {

	        require(["directorybrowser"], function (directoryBrowser) {

		        var picker = new directoryBrowser();

		        picker.show({

			        callback: function (path) {

				        if (path) {
					        page.querySelector("#txtAgeRatingPath").value = path;
				        }
				        picker.close();
			        },

			        header: "Select Age Certification Path"
		        });
	        });
        });

        page.querySelector("#btnSelecCountdownPath").addEventListener("click", function () {

	        require(["directorybrowser"], function (directoryBrowser) {

		        var picker = new directoryBrowser();

		        picker.show({

			        callback: function (path) {

				        if (path) {
					        page.querySelector("#txtCountdownPath").value = path;
				        }
				        picker.close();
			        },

			        header: "Select Countdown Path"
		        });
	        });
        });

        page.querySelector("#btnSelecComingSoonPath").addEventListener("click", function () {

	        require(["directorybrowser"], function (directoryBrowser) {

		        var picker = new directoryBrowser();

		        picker.show({

			        callback: function (path) {

				        if (path) {
					        page.querySelector("#txtComingSoonPath").value = path;
				        }
				        picker.close();
			        },

			        header: "Select Coming Soon Bumper Path"
		        });
	        });
        });

        page.querySelector("#btnSelecFeaturePath").addEventListener("click", function () {

	        require(["directorybrowser"], function (directoryBrowser) {

		        var picker = new directoryBrowser();

		        picker.show({

			        callback: function (path) {

				        if (path) {
					        page.querySelector("#txtFeaturePath").value = path;
				        }
				        picker.close();
			        },

			        header: "Select Feature Presentation Bumper Path"
		        });
	        });
        });

        page.querySelector("form").addEventListener("submit", onSubmit);

        view.addEventListener("viewshow", function () {

            loading.show();

            var page = this;

            ApiClient.getNamedConfiguration("PluginName").then(function (config) {

                loadPage(page, config);

            });
        });
    };
});;
