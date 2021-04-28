(function() {
    'use strict';

    angular.module('origin-i18n', ['angularMoment', 'origin-i18n-default-dictionary'])
        // lets make these as constants so all constants can be in one file that is build
        // during build time or something.
        .constant('FALLBACK_PREFIX', 'miss')
        .constant('angularMomentConfig', {
            'statefulFilters': false
        });
}());
