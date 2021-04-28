(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfileTabsCtrl($scope, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-tabs';

        $scope.achievementsLoc = UtilFactory.getLocalizedStr($scope.achievementsStr, CONTEXT_NAMESPACE, 'achievementsstr');
        $scope.friendsLoc = UtilFactory.getLocalizedStr($scope.friendsStr, CONTEXT_NAMESPACE, 'friendsstr');
        $scope.gamesLoc = UtilFactory.getLocalizedStr($scope.gamesStr, CONTEXT_NAMESPACE, 'gamesstr');

        $scope.activePage = 'achievements';

        function $tabByValue(value) {
            return $scope.$tabs.find('.otkpill[data-page=' + value + ']');
        }

        function valueForTab($tab) {
            return $tab.attr('data-page');
        }

        function stringForTab($tab) {
            return $tab.find('a').text();
        }

        function $getActiveTab() {
            return $scope.$tabs.find('.otkpill.otkpill-active');
        }

        function selectTab($tab) {
            // Remove previous active tab
            $scope.$tabs.find('.otkpill').removeClass('otkpill-active');

            // Set new active tab
            $tab.addClass('otkpill-active');

            // Switch pages
            $scope.activePage = $tab.attr('data-page');
            
            // Broadcast page switch
            $scope.$broadcast('activePageChange', $scope.activePage);

            // Update the underline size and position
            underlineTab($tab);

            $scope.$digest();
        }

        this.onClickedTab = function ($tab) {
            selectTab($tab);

            // Select the same value in the dropdown
            var value = valueForTab($tab);
            $scope.$dropdown.val(value);

            // Update the dropdown label
            var string = stringForTab($tab);
            $scope.$dropdownLabel.text(string);
        };

        this.onClickedDropdown = function (value, string) {
            // Change the dropdown label
            $scope.$dropdownLabel.text(string);

            // Select the same value on the tabs
            var $tab = $tabByValue(value);
            selectTab($tab);
        };

        function underlineTab($tab) {
            var tabX = $tab.position().left;
            var tabWidth = $tab.width();
            $scope.$tabs.find('.otknav-pills-bar').css({ 'width': tabWidth + 'px', 'transform': 'translate3d(' + tabX + 'px, 0px, 0px)' });
        }

        function underlineActiveTab() {
            var $activeTab = $getActiveTab();
            underlineTab($activeTab);
        }

        this.selectInitialTab = function () {
            underlineActiveTab();
        };

        $(window).resize(function () {
            // There is an issue with the OTK sliding nav bar. If you change
            // selected tabs using the dropdown and then resize the window
            // large enough to show the sliding navs again, the underline
            // won't be there. We can't adjust the underline bar while it's hidden.
            underlineActiveTab();
        });

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfileTabs
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} nucleusId nucleusId of the user
     * @param {LocalizedString} achievementsstr "ACHIEVEMENTS"
     * @param {LocalizedString} friendsstr "FRIENDS"
     * @param {LocalizedString} gamesstr "GAMES"
     * @description
     *
     * Profile Tabs
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-tabs nucleusid="123456789"
     *            achievementsstr="ACHIEVEMENTS"
     *            friendsstr="FRIENDS"
     *            gamesstr="GAMES"
     *         ></origin-profile-tabs>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfileTabsCtrl', OriginProfileTabsCtrl)
        .directive('originProfileTabs', function (ComponentsConfigFactory, $timeout) {

            return {
                restrict: 'E',
                controller: 'OriginProfileTabsCtrl',
                scope: {
                    nucleusId: '@nucleusid',
                    achievementsStr: '@achievementsstr',
                    friendsStr: '@friendsstr',
                    gamesStr: '@gamesstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/tabs.html'),
                link: function (scope, element, attrs, ctrl) {
                attrs = attrs;

                scope.$tabs = $(element).find('.origin-profile-tabs');
                scope.$dropdown = $(element).find('.origin-profile-dropdown');
                scope.$dropdownLabel = $(element).find('.origin-profile-navsection-dropdown .otkselect-label');

                // Clicked on one of the pill nav tabs
                scope.$tabs.on('click', '.otkpill', function () {
                    ctrl.onClickedTab($(this));
                });

                // Selected something from the dropdown
                $('.origin-profile-dropdown').change(function () {
                    var value = $(".origin-profile-dropdown option:selected").val();
                    var string = $(".origin-profile-dropdown option:selected").text();
                    ctrl.onClickedDropdown(value, string);
                });

                $timeout(function () {
                    // gotta wait until the layout is done before we can
                    // get the proper width of the first tab
                    ctrl.selectInitialTab();
                }, 250);

                $timeout(function () {
                    // In case of a page refresh
                    ctrl.selectInitialTab();
                }, 2000);

            }
        };

        });
}());

