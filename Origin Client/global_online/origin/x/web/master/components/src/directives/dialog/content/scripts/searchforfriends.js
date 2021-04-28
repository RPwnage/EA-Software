/**
 * @file dialog/content/searchforfriends.js
 */
(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginDialogContentSearchforfriendsCtrl($scope, $state, UtilFactory, DialogFactory, AuthFactory) {

        var CONTEXT_NAMESPACE = 'origin-dialog-content-searchforfriends';

        $scope.titleLoc = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');
        $scope.descLoc = UtilFactory.getLocalizedStr($scope.descStr, CONTEXT_NAMESPACE, 'descstr');
        $scope.placeholderLoc = UtilFactory.getLocalizedStr($scope.placeholderStr, CONTEXT_NAMESPACE, 'placeholderstr');
        $scope.searchLoc = UtilFactory.getLocalizedStr($scope.searchStr, CONTEXT_NAMESPACE, 'searchstr');
        $scope.cancelLoc = UtilFactory.getLocalizedStr($scope.cancelStr, CONTEXT_NAMESPACE, 'cancelstr');

        this.search = function (searchTerm) {
            // waiting on Pub Scrum - $state.go('app.search', {'searchString' : searchTerm, 'includes': 'people'});
            DialogFactory.close('origin-dialog-content-searchforfriends-id');
            window.alert('Search for friends -> ' + searchTerm);
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
    * @param {LocalizedString} titlestr "Search for Friends"
    * @param {LocalizedString} descstr "Search for a friend by real name, email address or Origin ID."
    * @param {LocalizedString} placeholderstr "Search for Friends"
    * @param {LocalizedString} searchstr "Search"
    * @param {LocalizedString} cancelstr "Cancel"
    * @description
    *
    * search for friends dialog
    *
    * @example
    * <example module="origin-components">
    *     <file name="index.html">
    *         <origin-dialog-content-searchforfriends
    *            titlestr="Search for Friends"
    *            descstr="Search for a friend by real name, email address or Origin ID."
    *            placeholderstr="Search for Friends"
    *            searchstr="Search"
    *            cancelstr="Cancel"
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
                scope: {
                    titleStr: '@titlestr',
                    descStr: '@descstr',
                    placeholderStr: '@placeholderstr',
                    searchStr: '@searchstr',
                    cancelStr: '@cancelstr'
                },
                controller: 'OriginDialogContentSearchforfriendsCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/searchforfriends.html'),
                link: function (scope, element, attrs, ctrl) {

                    attrs = attrs;
                    ctrl = ctrl;

                    var $textInput = $(element).find('.dialog-content-searchforfriends-searchfield-input'),
                        $clearIcon = $(element).find('.dialog-content-searchforfriends-section-searchfield-area .otkicon-closecircle'),
                        $searchButton = $(element).find('.dialog-content-searchforfriends-section-footer .otkbtn-primary'),
                        $cancelButton = $(element).find('.dialog-content-searchforfriends-section-footer .otkbtn-light'),
                        $clearButton = $(element).find('.dialog-content-searchforfriends-section-searchfield-area .otkicon-closecircle');

                    var updateUI = function () {
                        var text = $textInput.val();
                        if (text.length) {
                            $searchButton.toggleClass("otkbtn-disabled", false);
                            $clearIcon.show();
                        }
                        else {
                            $searchButton.toggleClass("otkbtn-disabled", true);
                            $clearIcon.hide();
                        }
                    };

                    $textInput.keydown(function (event) {

                        var text = $textInput.val();

                        updateUI();

                        if ((event.keyCode || event.which) === 13) {
                            event.stopImmediatePropagation();
                            event.preventDefault();
                            if (text.length) {
                                ctrl.search(text);
                            }
                        }
                    });

                    $textInput.keyup(function () {
                        updateUI();
                    });

                    $searchButton.click(function () {
                        var text = $textInput.val();
                        if (text.length) {
                            ctrl.search(text);
                        }
                    });

                    $cancelButton.click(function () {
                        DialogFactory.close('origin-dialog-content-searchforfriends-id');
                    });

                    $clearButton.click(function () {
                        $textInput.val('');
                        $searchButton.toggleClass("otkbtn-disabled", true);
                        $clearIcon.hide();
                        $textInput.focus();
                    });

                    $textInput.focus();

                }

            };

        });

} ());

