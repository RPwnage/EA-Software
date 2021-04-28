(function() {
    'use strict';

    angular.module('origin-i18n', ['angularMoment'])
        // lets make these as constants so all constants can be in one file that is build
        // during build time or something.
        .constant('DISPLAY_FALLBACK', true)
        .constant('FALLBACK_PREFIX', 'miss')
        .constant('angularMomentConfig', {
            'statefulFilters': false
        });
}());
