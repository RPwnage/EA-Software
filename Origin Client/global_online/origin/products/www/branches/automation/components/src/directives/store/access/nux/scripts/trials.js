
/**
 * @file store/access/nux/scripts/trials.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-nux-trials',
        BANNER_CLASS = '.origin-store-access-nux-trials-banner',
        HOVERED_CLASS = 'origin-store-access-nux-trials-banner-hovered';

    function OriginStoreAccessNuxTrialsCtrl($scope, UtilFactory, OcdPathFactory, $element, DialogFactory, LocalStorageFactory, CheckoutFactory, ComponentsConfigHelper, storeAccessNux) {
        var count = (_.size($element.parent().children()) - 1);

        $scope.getPillCount = function() {
            return new Array(count);
        };

        $scope.currentIndex = $element.index();

        function applyModel(data) {
            $scope.model = _.first(_.values(data));
            $scope.formattedDescription = UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, '', {'%game%': $scope.model.displayName});
            $scope.infobubbleContent =  '<origin-store-game-rating></origin-store-game-rating>'+
                                        '<origin-store-game-legal></origin-store-game-legal>';
        }

        /* This adds a hover class when the infobubble is hovered */
        $scope.$on('infobubble-show', function() {
            $element
                .find(BANNER_CLASS)
                .addClass(HOVERED_CLASS);
        });

        /* this removes the class */
        $scope.$on('infobubble-hide', function() {
            $element
                .find(BANNER_CLASS)
                .removeClass(HOVERED_CLASS);
        });

        if($scope.trial) {
            OcdPathFactory
                .get([$scope.trial])
                .attachToScope($scope, applyModel);
        }

        $scope.getItNowBtn = UtilFactory.getLocalizedStr($scope.getItNow, CONTEXT_NAMESPACE, 'get-it-now');

        $scope.nextStage = function() {
            if($element.index() !== count) {
                $element
                    .removeClass(storeAccessNux.class)
                    .next()
                    .addClass(storeAccessNux.class);
                LocalStorageFactory.set(storeAccessNux.key, $element.index()+1);
            } else {
                DialogFactory.close(storeAccessNux.id);
                LocalStorageFactory.delete(storeAccessNux.key);
                if (ComponentsConfigHelper.isOIGContext()) {
                    CheckoutFactory.close();
                }
            }
        };

        if(LocalStorageFactory.get(storeAccessNux.key) === $element.index()) {
            $element.addClass(storeAccessNux.class);
        }
    }

    function originStoreAccessNuxTrials(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                titleStr: '@',
                btn: '@',
                description: '@',
                image: '@',
                trial: '@',
                getItNow: '@'
            },
            controller: 'OriginStoreAccessNuxTrialsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/nux/views/trials.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessNuxTrials
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {OcdPath} trial the OCD path for the game in this module
     * @param {LocalizedString} title-str The title for this module
     * @param {LocalizedString} btn The button text for this module
     * @param {LocalizedString} description The description for this module
     * @param {LocalizedString} get-it-now The text for the entitlement button
     * @param {ImageUrl} image The background image for this module.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-nux-trials
     *          title-str="blah"
     *          image="http://someimage.jpg"
     *          btn="Lets Go"
     *          description="some text"
     *          trial="/bf/bf4/edition"
     *          get-it-now="Get it Now">
     *     </origin-store-access-nux-trials>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessNuxTrialsCtrl', OriginStoreAccessNuxTrialsCtrl)
        .directive('originStoreAccessNuxTrials', originStoreAccessNuxTrials);
}());
