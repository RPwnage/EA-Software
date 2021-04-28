/* PreloadFactory
 *
 * manages preloading of must-haves for offline mode to work
 * @file bootstrap/factories/preload.js
 */
(function() {
    'use strict';

    function PreloadFactory($http, LogFactory) {
        var myEvents = new Origin.utils.Communicator(),
            criticalOfflineReady = false,
            criticalOfflineSuccess = false,
            documentReady = false;

        //those under 'component' will use ComponentsConfigFactory.getTemplatePath to retrieve the full path
        var mustLoad = {
            app: [
                'views/notfound.html'
            ],
            component: [
                'gamelibrary/views/loggedin.html',
                'game/views/contextmenu.html',
                'game/views/tile.html',
                'game/views/progress.html',
                'panelmessage/views/panelmessage.html',
                'dialog/content/views/prompt.html'
            ]
        };

        function getPathPriv() {
            //IE10 doesn't support window.location.origin
            if (typeof window.location.origin === 'undefined') {
                window.location.origin = window.location.protocol + '//' + window.location.host;
            }
            var loc = window.location.origin + window.location.pathname;
            //need to strip out any html refeence in the location
            var dir = loc.substring(0, loc.lastIndexOf('/'));
            return dir;
        }

        function absolutize(endPoint) {
            if (endPoint.indexOf('http') === -1) {
                endPoint = getPathPriv() + '/' + endPoint;
            }
            return endPoint;
        }

        function preloadForOffline(appUrls) {
            return new Promise(function(resolve, reject) {

                var templateRequestPromises = [],
                    preloadTemplatesReady = false,
                    lastError = '',
                    appRouteUrl = appUrls.routes;

                function resolveIfCompleted() {
                    //criticalOfflineReady would indicate that all the .js files have been loaded & nooffline.html has been loaded
                    if (criticalOfflineReady) {
                        if (preloadTemplatesReady) {
                            //only consider criticalOfflineReady when determing failure for offline
                            //if one of the routeUrl or mustload didn't exist, let the app display an error html
                            if (criticalOfflineSuccess) {
                                resolve();
                            } else {
                                reject(new Error(lastError));
                            }
                        }
                    }
                }

                function handlePreloadSuccess() {
                    /*jshint unused: false */
                    preloadTemplatesReady = true;
                    resolveIfCompleted();
                }

                function handlePreloadFailure(response) {
                    lastError = '[PRELOAD:preloadTemplates FAILED ' + response.data + '(' + response.status + ')';
                    LogFactory.error(lastError);
                    preloadTemplatesReady = true;
                    resolveIfCompleted();
                }


                //if document isn't finished loading yet then listen for it
                if (!documentReady) {
                    myEvents.on('preload:documentReady', resolveIfCompleted);
                }

                //for each route, preload the offline one so it can get cached
                //if offline one isn't specified, fall back on loading the default
                for (var route in appRouteUrl) {
                    if (appRouteUrl.hasOwnProperty(route)) {
                        var offlineEndPoint = appRouteUrl[route].offlineUrl;
                        if (!offlineEndPoint || offlineEndPoint === '') {
                            offlineEndPoint = appRouteUrl[route].url;
                        }

                        //check for relative path
                        offlineEndPoint = absolutize(offlineEndPoint);
                        templateRequestPromises.push($http.get(offlineEndPoint));
                    }
                }

                Promise.all(templateRequestPromises)
                    .then(handlePreloadSuccess, handlePreloadFailure)
                    .catch(handleExceptions);
            });
        }

        function handleExceptions(error) {
            LogFactory.error(error.stack);
        }

        function preloadCriticalComplete(success) {
            criticalOfflineSuccess = success;
            criticalOfflineReady = true;
            //this will trigger a check for overall completion
            myEvents.fire('preload:documentReady');
        }

        function handlePreloadCriticalSuccess() {
            preloadCriticalComplete(true);
        }

        function handlePreloadCriticalFailure(response) {
            //this will return the first error it encountered, if there is more than one it will only show the first
            LogFactory.error('[PRELOAD:preloadCritical FAILED', response.data + '(' + response.status + ')');
            preloadCriticalComplete(false);
        }

        function preloadCritical() {
            //initialize
            //now go thru and load the app htmls
            var i,
                endPoint,
                criticalRequestPromises = [];

            for (i = 0; i < mustLoad.app.length; i++) {
                endPoint = absolutize(mustLoad.app[i]);
                criticalRequestPromises.push($http.get(endPoint));
            }

            //now go thru and load the component htmls
            for (i = 0; i < mustLoad.component.length; i++) {

                //will move this path to the angular.constants in the next pass. We don't want a dependency on ComponentsConfig here as it will require us to pull in the whole origin-components package
                //during the bootstrap phase
                endPoint = absolutize('bower_components/origin-components/dist/directives/' + mustLoad.component[i]);
                criticalRequestPromises.push($http.get(endPoint));
            }

            Promise.all(criticalRequestPromises)
                .then(handlePreloadCriticalSuccess, handlePreloadCriticalFailure)
                .catch(handleExceptions);
        }

        angular.element(document).ready(function() {
            documentReady = true;

            //we need to make sure everything in mustLoad gets loaded before we set criticalOfflineReady
            preloadCritical();
        });

        return {

            /**
             * initiates the preload of must-haves for offline mode
             * @memberof PreloadFactory
             * @method PreloadFactory.preload
             * @param {Object} appRouteUrl contains the endpoints for the app routes
             */
            preload: function(appUrls) {
                return preloadForOffline(appUrls);
            },

            //should really be in an util factory....
            getPath: getPathPriv
        };
    }
    /**
     * @ngdoc service
     * @name originApp.factories.PreloadFactory
     * @requires $rootScope
     * @requires origin-components.factories.ComponentsConfigFactory
     * @description
     *
     * PreloadFactory
     */
    angular.module('bootstrap')
        .factory('PreloadFactory', PreloadFactory);
}());