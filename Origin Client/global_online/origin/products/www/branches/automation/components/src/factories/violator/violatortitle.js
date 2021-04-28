/**
 * @file factories/violator/violatortitle.js
 */
(function() {

    'use strict';

    function ViolatorTitleFactory(DateHelperFactory, UtilFactory, ComponentsConfigFactory, $compile, ScopeHelper) {
        /**
         * If a time remaining is set, create a string to represent the time in natural language
         * @param {Object} scope the scope object
         * @param {string} CONTEXT_NAMESPACE the contextnamespace
         * @param {Object} timeData a DateHelper countdown object
         * @return {string} the HTML directive to substitute
         */
        function getTimeRemainingString(scope, CONTEXT_NAMESPACE, timeData) {
            var orderedTimeUnits = ['days', 'hours', 'minutes'],
                tokenObject = {},
                timeUnitValue = 0,
                time = '',
                templateName = '';

            //loop through days/hours/minutes and find the first non zero value, if found break early
            //this algorithm tests "less than" the value being localized, so the integer value passed to the
            //the pluralization engine needs to be bumped forward by 1
            for (var i = 0, j = orderedTimeUnits.length; i < j; i++) {
                timeUnitValue = timeData[orderedTimeUnits[i]];

                if (timeUnitValue > 0) {
                    tokenObject['%' + orderedTimeUnits[i] + '%'] = timeUnitValue + 1;
                    templateName = orderedTimeUnits[i] + 'template';

                    time = UtilFactory.getLocalizedStr(
                        scope[templateName],
                        CONTEXT_NAMESPACE,
                        templateName,
                        tokenObject,
                        timeUnitValue + 1
                    );

                    break;
                }
            }

            return UtilFactory.getLocalizedStr(scope.title, CONTEXT_NAMESPACE, 'title-str', {
                '%time%': time
            });
        }

        /**
         * Create a live countdown element
         *
         * @param scope the scope object
         * @return html tag to include for the countdown timer
         */
        function getEndDateElement(scope) {
            return [
                '<origin-game-violator-timer enddate="',
                scope.enddate,
                '" format="',
                scope.timerformat,
                '"></origin-game-violator-timer>'
            ].join('');
        }

        /**
         * Fetch the HTML string used for the violator label
         *
         * @param {Object} scope the scope to analyze
         * @param {string} CONTEXT_NAMESPACE the context namespace
         */
        function getTitle(scope, CONTEXT_NAMESPACE) {
            if (scope.livecountdown === 'fixed-date') {
                return UtilFactory.getLocalizedStr(
                    scope.title,
                    CONTEXT_NAMESPACE,
                    'title-str',
                    {
                        '%enddate%': getEndDateElement(scope)
                    }
                );
            } else if (scope.livecountdown === 'limited') {
                return getTimeRemainingString(
                    scope,
                    CONTEXT_NAMESPACE,
                    {
                        days: 0,
                        hours: Math.floor((scope.timeremaining / 60) / 60),
                        minutes: Math.floor((scope.timeremaining / 60) % 60)
                    }
                );
            } else if (scope.livecountdown === 'days-hours-minutes') {
                return getTimeRemainingString(
                    scope,
                    CONTEXT_NAMESPACE,
                    DateHelperFactory.getCountdownDataFromTimeRemaining(scope.timeremaining * 1000)
                );
            }

            return scope.title;
        }

        /**
         * Append the compiled title string to the violator html
         *
         * @param {Object} scope the scope to analyze
         * @param {Object} elem the element to update
         * @param {string} CONTEXT_NAMESPACE the context namespace
         * @param {string} className the class name to target
         */
        function appendTitle(scope, elem, CONTEXT_NAMESPACE, className) {
            var compiledTitle = $compile([
                    '<span>',
                    getTitle(scope, CONTEXT_NAMESPACE),
                    '</span>'
                ].join(''))(scope),
                target = elem.find(className);

            target.append(compiledTitle);

            ScopeHelper.digestIfDigestNotInProgress(scope);
        }

        return {
            getTitle: getTitle,
            appendTitle: appendTitle
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ViolatorTitleFactory

     * @description
     *
     * The dynamic violator title builder that is shared between all violator contexts
     */
    angular.module('origin-components')
        .factory('ViolatorTitleFactory', ViolatorTitleFactory);
}());
