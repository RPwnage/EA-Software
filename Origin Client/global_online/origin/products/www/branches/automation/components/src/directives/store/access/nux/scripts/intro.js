
/**
 * @file /store/access/nux/scripts/intro.js
 */
/* global moment */
(function(){
    'use strict';
    
    var SUBS_SUCCESS_REDIRECT = 'app.home_loggedin';

    function OriginStoreAccessNuxIntroCtrl($scope, UtilFactory, SubscriptionFactory, $element, DialogFactory, LocalStorageFactory, CheckoutFactory, ComponentsConfigHelper, storeAccessNux, AppCommFactory) {
        var date = moment(SubscriptionFactory.getNextBillingDate()).format('LL'),
            count = (_.size($element.parent().children()) - 1);

        $scope.formattedDescription = UtilFactory.getLocalizedStr($scope.description, '', '', {'%date%': date});

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

        if(!LocalStorageFactory.get(storeAccessNux.key) || LocalStorageFactory.get(storeAccessNux.key) === $element.index()) {
            LocalStorageFactory.set(storeAccessNux.key, $element.index());
            $element.addClass(storeAccessNux.class);
        }

        function onStateChange(event, toState) {
            if (toState.name !== SUBS_SUCCESS_REDIRECT) {
                AppCommFactory.events.off('uiRouter:stateChangeStart', onStateChange);
                DialogFactory.close(storeAccessNux.id);
            }
        }

        // end nux if user navigates pages
        AppCommFactory.events.on('uiRouter:stateChangeStart', onStateChange);
    }
    function originStoreAccessNuxIntro(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                titleStr: '@',
                logo: '@',
                btn: '@',
                description: '@'
            },
            controller: 'OriginStoreAccessNuxIntroCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/access/nux/views/intro.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessNuxIntro
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} logo The logo asset above the copy
     * @param {LocalizedString} title-str The title for this module
     * @param {LocalizedString} btn The button text for this module
     * @param {LocalizedString} description The description for this module
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-nux-intro
     *          title-str="blah"
     *          logo="http://someimage.jog"
     *          btn="Lets Go"
     *          description="some text">
     *     </origin-store-access-nux-intro>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessNuxIntroCtrl', OriginStoreAccessNuxIntroCtrl)
        .directive('originStoreAccessNuxIntro', originStoreAccessNuxIntro)
        .constant('storeAccessNux', {
            'key': 'storeAccessNuxStage',
            'class': 'origin-store-access-nux-active',
            'id': 'access-nux',
            'template': 'nux'
        });
}());
