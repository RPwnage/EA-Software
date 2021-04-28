/**
 * This file should not be processed as it contains no angular directive syntax
 */
var foo = function(str, ComponentsConfigFactory) {
    return {
        templateUrl: ComponentsConfigFactory.getTemplatePath('views/game.html')
    };
};

foo();