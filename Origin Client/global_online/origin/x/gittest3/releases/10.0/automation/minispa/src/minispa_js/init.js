( function() {
        'use strict';

        var dictionary;

        // getting URL parameters
        function getParameterByName(name) {
            name = name.replace(/[\[]/, '\\\[').replace(/[\]]/, '\\\]');
            var regex = new RegExp('[\\?&]' + name + '=([^&#]*)'),
                results = regex.exec(location.search);
            return results === null ? null : decodeURIComponent(results[1].replace(/\+/g, ' '));
        }

        function getLocationPage() {
            var path = window.location.pathname;
            return path.split('/').pop();
        }

        function init() {

            var usingMockup = false;
            var env = getParameterByName('env');
            var locale = getParameterByName('locale');
            var mockupServerUrl = getParameterByName('mockup');
            var account = getParameterByName('account');

            if (!env) {
                env = 'qa';
            }

            if (!locale) {
                locale = 'en_US';
            }

            dictionary = 'https://cms.x.origin.com/' + locale + '/content/components/translations/components.json';

            if (!mockupServerUrl) {
                usingMockup = false;
            } else {
                usingMockup = true;
            }

            if (!account) {
                // this account has all entitlments which cover 'Cross section of different types of packages'
                account = 'minispadefault';
            }

            var config = {
                common : {
                    env : env,
                    locale : locale,
                }
            };

            if (usingMockup) {
                var mockupServerURL = 'http://' + mockupServerUrl + ':1400';
                var accountPath = '/data/' + account;
                config.common.dynServicesOrigin = mockupServerURL;
                config.common.dynServicesPath = accountPath + '/jssdkconfig.json';
                dictionary = mockupServerURL + accountPath + '/translations/' + locale + '/components.json';
            } else {
                if (env) {
                    config.common.dynServicesOrigin = 'https://s3.amazonaws.com/otk.dm.origin.com';
                    config.common.dynServicesPath = '/config/{env}/{module}';
                }
            }

            Origin.init(config).then(function() {
                console.log('initialized jssdk successfully');

                angular.element(document).ready(function() {
                    angular.bootstrap(document, ['testApp']);

                    var $div = $('<div ng-controller="initController"><div id="minispa-directive"></div></div>');
                    $(document.body).append($div);

                    angular.element(document).injector().invoke(function($compile) {
                        var scope = angular.element($div).scope();
                        $compile($div)(scope);
                    });
                });

            }).catch(function(error) {
                console.error('init ' + error.stack);
            });

        }

        function setComponentsBasePath(ComponentsConfigFactory) {
            ComponentsConfigFactory.setBasePath('bower_components/origin-components/');
        }

        function initController($element, $compile, $scope, $http, LocFactory, UserDataFactory, AppCommFactory) {

            // load dictionary
            LocFactory.on('ready', function() {
                console.log('LocFactory ready');

                var directive = '<minispa-testdata></minispa-testdata>';

                var page = getLocationPage();
                if ('minispa-handcraft.html' === page) {
                    directive = '<minispa-handcraft></minispa-handcraft>';
                } else if ('minispa-automation.html' === page) {
                    var source = getParameterByName('source');
                    if (!source) {
                        // use deafult
                        source = 'minispa_data/default.xml';
                    }
                    directive = '<minispa-automation-testdata source="' + source + '"></minispa-automation-testdata>';
                } else {
                    //
                }

                $element.find('#minispa-directive').append($compile(directive)($scope));

            });

            console.log('Initializing LocFactory with url ' + dictionary);

            $http.get(dictionary).success(function(data) {
                LocFactory.init(data);
            }).error(function() {
                console.log('Error getting dictionary: ' + dictionary);
                LocFactory.init({});
            });

            Origin.auth.login().then(function() {
                console.log('login successfully');

                UserDataFactory.setAuthReady({
                    'isLoggedIn' : true
                });

                AppCommFactory.events.fire('auth:ready', {
                    'isLoggedIn' : true
                });

            }).catch(function(error) {
                console.log('login required: ' + error.stack);
            });

        }


        angular.module('testApp', ['ngSanitize', 'origin-i18n', 'origin-components'])
            .run(setComponentsBasePath)
            .controller('initController', initController);

        init();

    }());
