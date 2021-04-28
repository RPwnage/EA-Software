(function() {
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

    function retrieveAddMixJSSDKOverrideFile(url) {
        return new Promise(function(resolve) {
            $.get(url, function(data) {
                Origin.utils.mix(Origin.config, data);
                resolve();
            }).fail(function() {
                resolve();
            });

        });
    }

    function initJSSDK(locale) {
        return function() {
            return Origin.init(locale);
        };
    }

    function bootstrapTestApp() {
        console.log('initialized jssdk successfully');

        angular.element(document).ready(function() {
            Origin.performance.setMarker(OriginPerfConstant.markers.ANGULAR_BOOTSTRAP_START);
            Origin.performance.setMeasure(OriginPerfConstant.measure.HEAD_TO_ANGULAR_START, OriginPerfConstant.markers.HEAD_PARSED, OriginPerfConstant.markers.ANGULAR_BOOTSTRAP_START);
            angular.bootstrap(document, ['testApp']);

            var $div = $('<div ng-controller="initController"><div id="minispa-directive"></div></div>');
            $(document.body).append($div);

            angular.element(document).injector().invoke(function($compile) {
                var scope = angular.element($div).scope();
                $compile($div)(scope);
            });
        });

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
            locale = 'en-ca';
        }

        dictionary = 'https://dev.data1.origin.com/defaults/web-defaults/defaults.' + locale + '.can.config';

        if (!mockupServerUrl) {
            usingMockup = false;
        } else {
            usingMockup = true;
        }

        if (!account) {
            // this account has all entitlments which cover 'Cross section of different types of packages'
            account = 'minispadefault';
        }


        var jssdkOverrideURL = '/configs/environment/' + env + '/jssdk-nonorigin-config.json';

        if (usingMockup) {
            var mockupServerURL = 'http://' + mockupServerUrl + ':1400';
            var accountPath = '/data/' + account;
            jssdkOverrideURL = mockupServerURL + accountPath + '/jssdkconfig.json';
            dictionary = mockupServerURL + accountPath + '/translations/' + locale + '/components.json';
        }

        Origin.config.overrides = {
            env: env,
            cmsstage: ''
        };

        retrieveAddMixJSSDKOverrideFile(jssdkOverrideURL)
            .then(initJSSDK(locale))
            .then(bootstrapTestApp)
            .catch(function(error) {
                console.error('init ' + error.stack);
            });

    }

    function setComponentsBasePath(ComponentsConfigFactory) {
        ComponentsConfigFactory.setBasePath('bower_components/origin-components/');
    }

    function initController($element, $compile, $scope, $http, LocFactory, UserDataFactory, AppCommFactory) {

        console.log('Initializing LocFactory with url ' + dictionary);

        $http.get(dictionary).success(function(data) {
            LocFactory.init(data);
        }).error(function() {
            console.log('Error getting dictionary: ' + dictionary);
            LocFactory.init({});
        });

        // load dictionary
        console.log('LocFactory ready');

        var directive = '<minispa-testdata></minispa-testdata>';

        var page = getLocationPage();
        if ('minispa-handcraft.html' === page) {
            directive = '<minispa-handcraft></minispa-handcraft>';
        } else if ('minispa-automation.html' === page) {
            var source = getParameterByName('source');
            if (!source) {
                // use default
                source = 'minispa_data/default.xml';
            }
            directive = '<minispa-automation-testdata source="' + source + '"></minispa-automation-testdata>';
        } else {
            //
        }

        $element.find('#minispa-directive').append($compile(directive)($scope));

        
        Origin.auth.login().then(function() {
            console.log('login successfully');

            UserDataFactory.setAuthReady({
                'isLoggedIn': true
            });

            AppCommFactory.events.fire('auth:ready', {
                'isLoggedIn': true
            });

        }).catch(function(error) {
            console.log('login required: ' + error.stack);
        });

    }

    /**
    * Add the language to the html element
    */
    function initHtmlLang() {
        var originLocaleApi = OriginLocale.parse(window.location.href, 'en-us'); // jshint ignore:line
        angular.element('html').attr('lang', originLocaleApi.getLanguageCode());
    }
    

    angular.module('testApp', [
        'ngResource',
        'ngSanitize',
        'ngRoute',
        'ngTouch',
        'ngIdle',
        'ngclipboard',
        'angular-toArrayFilter',
        'ui.router',
        'origin-components',
        'origin-i18n',
        'eax-experiments'])
        .run(initHtmlLang)
        .run(setComponentsBasePath)
        .controller('initController', initController);

    init();

}());