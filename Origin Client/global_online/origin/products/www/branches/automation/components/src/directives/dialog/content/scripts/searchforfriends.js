/**
 * @file dialog/content/searchforfriends.js
 */
(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginDialogContentSearchforfriendsCtrl($scope, $state, DialogFactory, AuthFactory) {

        $scope.onInputChanged = function () {
            var textLength = $scope.searchText.length;

            // enable/disable the search button
            DialogFactory.update({ acceptEnabled: textLength });

            // hide/show the clear button
            if ($scope.searchText.length) {
                $scope.$clearIcon.show();
            }
            else {
                $scope.$clearIcon.hide();
            }
        };

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && !onlineObj.isOnline) {
                DialogFactory.close('origin-dialog-content-searchforfriends-id');
            }
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChanged);
        }

        //listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
        $scope.$on('$destroy', onDestroy);

    }

    /**
    * @ngdoc directive
    * @name origin-components.directives:originDialogContentSearchforfriends
    * @restrict E
    * @element ANY
    * @scope
    * @param {string} placeholderstr "Search for Friends"
    * @description
    * @param {LocalizedString} searchfriendstitle "Search for friends"
    * @param {LocalizedString} searchfriendsdescription "Search for friends by their Public ID, real name or email address."
    * @param {LocalizedString} searchfriendsinputplaceholdertext "Search for friends"
    * @param {LocalizedString} searchfriendssearchbuttontext "Search"
    * @param {LocalizedString} searchfriendscancelbuttontext "Cancel" 
    * search for friends dialog
    *
    * @example
    * <example module="origin-components">
    *     <file name="index.html">
    *         <origin-dialog-content-searchforfriends
    *            placeholderstr="Search for Friends"
    *         ></origin-dialog-content-searchforfriends>
    *     </file>
    * </example>
    *
    */

    angular.module('origin-components')
        .controller('OriginDialogContentSearchforfriendsCtrl', OriginDialogContentSearchforfriendsCtrl)
        .directive('originDialogContentSearchforfriends', function (ComponentsConfigFactory, DialogFactory) {

            return {
                restrict: 'E',
                require: ['^originDialogContentPrompt', 'originDialogContentSearchforfriends'],
                scope: {
                    placeholderstr: '@placeholderstr',
                },
                controller: 'OriginDialogContentSearchforfriendsCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/searchforfriends.html'),
                link: function (scope, element, attrs, ctrl) {

                    attrs = attrs;

                    scope.$textInput = $(element).find('.otkinput > input');
                    scope.$clearIcon = $(element).find('.otkinput > .otkinput-clear');

                    // Hit ENTER key
                    var promptCtrl = ctrl[0];
                    scope.$textInput.keydown(function (event) {
                        if (scope.searchText.length) {
                            promptCtrl.handleButtonPress(event);
                        }
                    });

                    // Clicked the Clear button
                    scope.$clearIcon.click(function () {
                        scope.$textInput.val('');
                        scope.$clearIcon.hide();
                        scope.$textInput.focus();
                        DialogFactory.update({ acceptEnabled: false });
                    });

                    scope.$textInput.focus();

                    promptCtrl.setContentDataFunc(function () {
                        return {
                            searchText: scope.searchText
                        };
                    });
                }

            };

        });

} ());

