(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginOfflineFlyoutCtrl($scope, UtilFactory, EventsHelperFactory, AuthFactory) {

        var handles = [];

        var CONTEXT_NAMESPACE = 'origin-offline-flyout',
            isVisible = false;

        $scope.descriptionLoc = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'descriptionstr');

        function destroy() {
            EventsHelperFactory.detachAll(handles);
        }
        $scope.$on('$destroy', destroy);

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && onlineObj.isOnline) {
                setVisibility(false);
            }
        }

        function onClickedOfflineModeButton() {
            // Toggle the flyout
            setVisibility(!isVisible);
        }

        handles[handles.length] = AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
        handles[handles.length] = Origin.events.on(Origin.events.CLIENT_CLICKEDOFFLINEMODEBUTTON, onClickedOfflineModeButton);

        function setVisibility(visible) {
            isVisible = visible;
            if (visible) {
                document.querySelector('.origin-offline-flyout').classList.add('otktooltip-isvisible');
            }
            else {
                document.querySelector('.origin-offline-flyout').classList.remove('otktooltip-isvisible');
            }
        }
        this.setVisibility = setVisibility;

        if (!AuthFactory.isClientOnline()) {
            setVisibility(true);
        }
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originOfflineFlyout
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} descriptionstr - "Origin is in offline mode and would need to reconnect to reactivate the full spectrum of features. Go Online to reconnect Origin."
     * @description
     *
     * Offline Flyout
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-offline-flyout
     *            descriptionstr="Origin is in offline mode and would need to reconnect to reactivate the full spectrum of features. Go Online to reconnect Origin."
     *         ></origin-offline-flyout>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginOfflineFlyoutCtrl', OriginOfflineFlyoutCtrl)
        .directive('originOfflineFlyout', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginOfflineFlyoutCtrl',
                scope: {
                    descriptionStr: '@descriptionstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('offline/views/flyout.html'),

                link: function (scope, element, attrs, ctrl) {
                    element = element;
                    attrs = attrs;

                    // dismiss if the user clicks anywhere in the document except the reconnect button
                    $(document).on('click', function (e) {
                        var $target = $(e.target);
                        if (!$target.hasClass('origin-offline-reconnectbutton-button')) {
                            ctrl.setVisibility(false);
                        }
                    });

                }
            };
        });
}());
