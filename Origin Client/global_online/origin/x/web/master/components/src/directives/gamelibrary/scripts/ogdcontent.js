/**
 * file gamelibrary/ogdcontent.js
 */
(function() {

    'use strict';

    function OriginGamelibraryOgdContentCtrl($scope, AppCommFactory) {
        /**
         * WIP: Mockup of menu content in OCD
         */
        $scope.nav = [
            {
                'origin-gamelibrary-ogd-content-panel': {
                    title: 'Overview',
                    friendlyname: 'overview',
                    items: [

                    ]
                }
            }, {
                'origin-gamelibrary-ogd-content-panel': {
                    title: 'DLC',
                    friendlyname: 'battlefield-downloadable-content',
                    items: [

                    ]
                }
            },
            {
                'origin-gamelibrary-ogd-content-panel': {
                    title: 'Theme Packs!!!',
                    friendlyname: 'themes',
                    items: [

                    ]
                }
            }
        ];

        /**
         * WIP: Mockup of OCD theme content
         */
        if($scope.offerId === 'OFB-EAST:41420') {
            $scope.theme = {
                tabtextcolor: 'rgba(193,64,22,0.7)',
                tabactiveselectioncolor: 'cornflowerblue',
                tabactivetextcolor: '#FF6600',
                tabhorizontalrulecolor: '#24789D'
            };
        }

        /**
         * Map of the styles to apply using a stylesheet override
         * @type {Object}
         */
        var themeMap = {
            tabtextcolor: {
                getHtml: function(color) {
                    return '.expandedview-content .tabs .otknav > li > a{color: ' + color + ';}';
                }
            },
            tabactiveselectioncolor: {
                getHtml: function(color) {
                    var html =  '.expandedview-content .tabs .otknav > li > a:hover,';
                    html += 'expandedview-content .tabs .otknav > li.otkpill-active > a:hover{color: ' + color + ';}';

                    return html;
                }
            },
            tabactivetextcolor: {
                getHtml: function(color) {
                    return '.expandedview-content .tabs .otknav .otkpill-active:after{ background-color: ' + color + ';}';
                }
            },
            tabhorizontalrulecolor: {
                getHtml: function(color) {
                    return '.expandedview-content .ogdtabbar {border-bottom: 1px solid ' + color + ';}';
                }
            }
        };

        /**
         * Update the topic from a rebroadcasted uirouter event
         * @param  {Object} event    event
         * @param  {Object} toState  $state
         * @param  {Object} toParams $stateParams
         */
        function updateTopic(event, toState, toParams) {
            if(toParams.topic) {
                $scope.topic = toParams.topic;
                $scope.$digest();
            }
        }

        /**
         * Mapping function for extracting topics from a list of panels.
         * @param  {Object} panel an instance of a panel
         * @return {string} the friendlyname
         */
        function getTopicList(panel) {
            return panel['origin-gamelibrary-ogd-content-panel'].friendlyname;
        }

        /**
         * Validate the tab route and redirect to the first tab in the panel set
         * if it's not a valid deeplink
         */
        $scope.validateRoute = function(topic, offerId) {
            var topics = $scope.nav.map(getTopicList);

            if(topics.length > 0 && topics.indexOf(topic) === -1) {
                AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary.ogd.topic', {
                    offerid: offerId,
                    topic: topics[0]
                });
            }
        };

        /**
         * Determine if the selected tab is active
         * @param  {string}  tabId The current tab id
         * @return {Boolean}       true if active tab
         */
        $scope.isActiveTab = function(tabId) {
            return ($scope.topic === tabId) ? 'true' : 'false';
        };

        /**
         * Place tabs into the tab container if there is more than one tab
         *
         * @param {Object[]} data the OCD data feed
         * @return {string} HTML for tabs
         */
        $scope.generateContentTabs = function(data) {
            var tabs = '';

            if(data && data.length > 1) {
                tabs += '<origin-gamelibrary-ogd-tab-bar>';
                for(var i = 0; i < data.length; i++) {
                    tabs += '<origin-gamelibrary-ogd-tab-bar-item ';
                    tabs += 'offerid="' + $scope.offerId + '" ';
                    tabs += 'title="' + data[i]['origin-gamelibrary-ogd-content-panel'].title + '" ';
                    tabs += 'friendlyname="' + data[i]['origin-gamelibrary-ogd-content-panel'].friendlyname + '" ';
                    tabs += 'active="{{isActiveTab(\'' + data[i]['origin-gamelibrary-ogd-content-panel'].friendlyname + '\')}}">';
                    tabs += '</origin-gamelibrary-ogd-tab-bar-item>';
                }
                tabs += '</origin-gamelibrary-ogd-tab-bar>';
            }

            return tabs;
        };

        /**
         * Generate markup for the content panels
         *
         * @param {Object[]} data the OCD data feed
         * @return {string} HTML or content panels
         */
        $scope.generateContentPanels = function(data) {
            var panels = '';

            if(data && data.length > 0) {
                for(var i = 0; i < data.length; i++) {
                    panels += '<div ng-if="topic === \'' + data[i]['origin-gamelibrary-ogd-content-panel'].friendlyname + '\'"><h2>' + data[i]['origin-gamelibrary-ogd-content-panel'].title + '</h2></div>';
                }
            }

            return panels;
        };

        /**
         * Generate a stylesheet
         * @param  {Object} attrs the element's override attrs
         * @return {string} html
         */
        $scope.generateStylesheet = function(attrs) {
            var html = '<style type="text/css">';
            var index;

            for(index in themeMap) {
                if(attrs[index] && themeMap[index]) {
                    html += themeMap[index].getHtml(attrs[index]);
                } else if ($scope.theme && themeMap[index] && $scope.theme[index]) {
                    html += themeMap[index].getHtml($scope.theme[index]);
                }
            }

            html += '</style>';

            return html;
        };

        $scope.validateRoute($scope.topic, $scope.offerId);

        AppCommFactory.events.on('uiRouter:stateChangeStart', updateTopic);

        $scope.$on('$destroy', function(){
            AppCommFactory.events.off('uiRouter:stateChangeStart', updateTopic);
        });
    }

    function originGamelibraryOgdContent(ComponentsConfigFactory, $compile) {
        /**
         * Link function
         * @param  {Object} scope   link scope
         * @param  {Object} element link element
         */
        function originGamelibraryOgdContentLink(scope, element, attrs) {
            /**
             * Update the dom regions from OCD data
             */
            function updateDom() {
                element.prepend($compile(scope.generateStylesheet(attrs))(scope));
                element.find('.expandedview-content .tabs')
                    .append($compile(scope.generateContentTabs(scope.nav))(scope));
                element.find('.expandedview-content .content')
                    .append($compile(scope.generateContentPanels(scope.nav))(scope));
            }

            updateDom();
        }

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                topic: '@topic',
                tabhorizontalrulecolor: '@tabhorizontalrulecolor',
                tabactiveselectioncolor: '@tabactiveselectioncolor',
                tabactivetextcolor: '@tabactivetextcolor',
                tabtextcolor: '@tabtextcolor',
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdcontent.html'),
            link: originGamelibraryOgdContentLink,
            controller: OriginGamelibraryOgdContentCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdContent
     * @restrict E
     * @element ANY
     * @scope
     * @param {OfferId} offerid the offer id
     * @param {string} topic the topic name to use as a default
     * @param {string|OCD} tabhorizontalrulecolor color of the tab bar horizontal rule theme
     * @param {string|OCD} tabactiveselectioncolor the tab bar active selection underline theme
     * @param {string|OCD} tabactivetextcolor The color of the hover and active text
     * @param {string|OCD} tabtextcolor the color of inactive text in the tab bar
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdContentCtrl', OriginGamelibraryOgdContentCtrl)
        .directive('originGamelibraryOgdContent', originGamelibraryOgdContent);
})();