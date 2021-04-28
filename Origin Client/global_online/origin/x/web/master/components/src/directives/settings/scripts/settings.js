/**
 * @file settings.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings',
        OVERFLOW_MENU_WIDTH = 110;

    function OriginSettingsCtrl($scope, $stateParams, UtilFactory) {
        var visibleInSPA = [1,0,0,0,0,0,1,1,0];
    
        $scope.changeSavedStr = UtilFactory.getLocalizedStr($scope.changeSavedStr, CONTEXT_NAMESPACE, 'change-saved');

        $scope.tabNames = [];
        $scope.tabNames[1] = UtilFactory.getLocalizedStr($scope.accountstr, CONTEXT_NAMESPACE, "tab-account");
        $scope.tabNames[2] = UtilFactory.getLocalizedStr($scope.applicationstr, CONTEXT_NAMESPACE, "tab-application");
        $scope.tabNames[3] = UtilFactory.getLocalizedStr($scope.diagnosticsstr, CONTEXT_NAMESPACE, "tab-diagnostics");
        $scope.tabNames[4] = UtilFactory.getLocalizedStr($scope.installsstr, CONTEXT_NAMESPACE, "tab-installs");
        $scope.tabNames[5] = UtilFactory.getLocalizedStr($scope.notificationsstr, CONTEXT_NAMESPACE, "tab-notifications");
        $scope.tabNames[6] = UtilFactory.getLocalizedStr($scope.oigstr, CONTEXT_NAMESPACE, "tab-oig");
        $scope.tabNames[7] = UtilFactory.getLocalizedStr($scope.paymentstr, CONTEXT_NAMESPACE, "tab-payment");
        $scope.tabNames[8] = UtilFactory.getLocalizedStr($scope.subscriptionsstr, CONTEXT_NAMESPACE, "tab-subscriptions");
        $scope.tabNames[9] = UtilFactory.getLocalizedStr($scope.voicestr, CONTEXT_NAMESPACE, "tab-voice");
        
        $scope.voiceSupported = Origin.voice.supported();

        $scope.pages = {
            application: "application",
            diagnostics: "diagnostics",
            installs: "installs",
            notifications: "notifications",
            oig: "oig",
            payment: "payment",
            subscriptions: "subscriptions",
            voice: "voice"
        };

        // for now assume that if we have extra parameters on the window it's an OIG window
        $scope.inOIG = false;
        var windowParams = window.extraParams;
        if (windowParams) {
            $scope.inOIG = true;
        }
        
        this.triggerSavedToasty = function() {
            document.querySelector(".origin-toast-dialog").classList.add("origin-toast-dialog-display");
            setTimeout(function(){
                document.querySelector(".origin-toast-dialog").classList.remove("origin-toast-dialog-display");
                }, 1200);
        };

        // If a specific page was passed with the URL, then go to that page (ie "#/settings?page=voice")
        // Else, we go to the application page
        this.page = (!!$stateParams.page) ? $stateParams.page : $scope.pages.application;

        this.isSet = function(checkPage) {
            return this.page === checkPage;
        };

        this.setPage = function(setPage) {
            this.page = setPage;
            // This is to remove any page parameters when you click a tab
            window.location = '#/settings';

            if (setPage === $scope.pages.voice) {
                Origin.client.voice.setInVoiceSettings(true);
            }
            else {
                Origin.client.voice.setInVoiceSettings(false);
            }
        };
        
        this.triggerSavedToasty = function() {
            document.querySelector(".origin-toast-dialog").classList.add("origin-toast-dialog-display");
            setTimeout(function(){
                document.querySelector(".origin-toast-dialog").classList.remove("origin-toast-dialog-display");
                }, 1200);
        };
        
        this.isClientRunning = function() {
            return Origin.client.isEmbeddedBrowser();
        };
        
        $scope.manageOverflowTabs = function(){
            var tabArea = document.querySelector('.origin-settings-tablist'),
                tabList = document.querySelectorAll('.origin-settings-tab-ref'),
                tabWidth = 0,
                index = 0,
                lastVisible,
                inClient = $scope.debugIsAlwaysVisible || Origin.client.isEmbeddedBrowser();
                
            $scope.overflowStart = 10;
            $scope.tabsInOverflow = 0;

            // find last visible
            lastVisible = tabList.length;
            while ((lastVisible > 0) && !inClient && !visibleInSPA[lastVisible-1]){
                lastVisible--;
          }

            angular.forEach(tabList, function(item) {
                var offset = OVERFLOW_MENU_WIDTH;
                index++;
                if (index === lastVisible){ // last visible tab shouldn't need the Overflow menu width added
                    offset = 100;
                }                
                if (inClient || visibleInSPA[index-1]){
                    tabWidth += item.offsetWidth;
                    if ((tabWidth + offset) >= tabArea.clientWidth){
                        $scope.tabsInOverflow++;
                        if ($scope.overflowStart === 10){
                            $scope.overflowStart = index; // put in first 
                        }
                    }
          }
        });
            console.log("manageOverflowTabs: tabWidth = " + tabWidth + " tabArea = " + tabArea.clientWidth + " tabsInOverflow = " + $scope.tabsInOverflow + " overflowStart = " + $scope.overflowStart);
            $scope.$digest();
        };
        
        // handle overflow on window resize
        $(window).resize( UtilFactory.throttle(function() {
            $scope.manageOverflowTabs();
        }, 300));
    }

    function originSettings(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                descriptionStr: '@descriptionstr',
                titleStr: '@titlestr'
            },
            controller: 'OriginSettingsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/settings.html'),
            controllerAs:'OriginSettingsCtrl',
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettings
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} titlestr the title string
     * @param {string} descriptionstr the description str
     * @description
     *
     * Settings section 
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-footer titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-home-footer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsCtrl', OriginSettingsCtrl)
        .directive('originSettings', originSettings);
}());
