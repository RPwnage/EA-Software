/**
 * @file store/oth/scripts/oth.js
 */
(function(){
    'use strict';


    var WidthEnumeration = {
        "full": "full",
        "half": "half",
        "third": "third",
        "quarter": "quarter"
    };

    var CONTEXT_NAMESPACE = 'origin-store-oth-program';
    var SIZE_CLASS_PREFIX = 'l-origin-store-column-';
    function originStoreOthProgram(ComponentsConfigFactory, DirectiveScope, PdpUrlFactory) {
        function originStoreOthProgramLink(scope, element) {
            var promise = DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, scope.ocdPath).then(function(){
                scope.size = scope.size || WidthEnumeration.full;
            });
            scope.$watchOnce('model.oth', function(){
                if (_.get(scope, ['model', 'oth']) === false){
                    element.remove();
                }else{
                    scope.sizeClass = scope.model.oth ? SIZE_CLASS_PREFIX + WidthEnumeration[scope.size] : '';    
                }
            });
            
            scope.goToPdp = function($event){
                $event.preventDefault();
                promise.then(function(){
                    PdpUrlFactory.goToPdp(scope.model);
                });
            };
            
            scope.getPdpUrl = PdpUrlFactory.getPdpUrl;
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                description: '@',
                freeText: '@',
                ocdPath: '@',
                offerDescriptionTitle: '@',
                offerDescription: '@',
                size: '@'
            },
            link: originStoreOthProgramLink,
            controller: 'OriginStoreFreegamesProgramCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/oth/views/oth.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreOthProgram
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} free-text Display something other than "FREE" for the discount
     * @param {LocalizedString} description The text for button (Optional).
     * @param {OcdPath} ocd-path OCD Path associated with this module.
     * @param {LocalizedString} offer-description-title The description title in the offer section.
     * @param {LocalizedString} offer-description The description in the offer section.
     * @param {WidthEnumeration} size size of oth. full for 1 per row, or half for 2 per row
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-oth-program
     *              description="Learn More"
     *              ocd-path="/battlefield/battlefield-4/digital-deluxe-edition"
     *              offer-description-title="What kind of city will you create?"
     *              offer-description="You are an architect, cityplanner, Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nulla vitae elit libero, a pharetra augue.">
     *          </origin-store-oth-program>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreOthProgram', originStoreOthProgram);
}());
