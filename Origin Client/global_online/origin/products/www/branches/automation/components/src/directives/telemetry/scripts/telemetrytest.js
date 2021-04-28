
/**
 * @file /telemetry/scripts/telemetrytest.js
 */
(function(){
    'use strict';
    function OriginTelemetryTestCtrl($scope, LogFactory) {
        $scope.pinEvents = [
            {
                id: 'pin-login',
                name: 'Login/Logout',
                comment: 'Send Login & Logout',
                jsFunc: Origin.telemetry.sendLoginEvent,
                jsFuncName: 'sendLoginEvent',
                jsArgs: [
                    {
                        name: 'action',
                        type: 'select',
                        values: ['login', 'logout'],
                        default: 'login'
                    },
                    {
                        name: 'type',
                        type: 'select',
                        values: ['SPA', 'client'],
                        default: 'SPA'
                    },
                    {
                        name: 'status',
                        type: 'select',
                        values: ['success', 'failure'],
                        default: 'success'
                    },
                    {
                        name: 'statuscode',
                        placeholder: 'Ex: normal',
                        type: 'text'
                    }
                ],
                jsParams: []
            },
            {
                id: 'pin-boot',
                name: 'Boot',
                comment: 'Send Client Start/Client Shutdown',
                jsFunc: Origin.telemetry.sendStandardPinEvent,
                jsFuncName: 'sendStandardPinEvent',
                jsArgs: [
                    {
                        name: 'trackerType',
                        type: 'const',
                        values: 'TRACKER_MARKETING'
                    },
                    {
                        name: 'action',
                        type: 'const',
                        values: 'boot_start'
                    },
                    {
                        name: 'source',
                        type: 'select',
                        values: [
                            'client',
                            'web' 
                        ]
                    },
                    {
                        name: 'action',
                        type: 'select',
                        values: [ 'success', 'error' ]
                    },
                    {
                        name: 'params',
                        placeholder: 'JSON object',
                        type: 'json'
                    }
                ],

                jsParams: []
            },
            {
                id: 'pin-pageview',
                name: 'Page View',
                comment: 'Send Page View',
                jsFunc: Origin.telemetry.sendPageView,
                jsFuncName: 'sendPageView',
                jsArgs: [
                    {
                        name: 'uri',
                        type: 'text',
                        placeholder: 'Ex. /home'
                    },
                    {
                        name: 'title',
                        type: 'text',
                        placeholder: 'Ex. app.home'
                    },
                    {
                        name: 'params',
                        placeholder: 'JSON object',
                        type: 'json'
                    }
                ],
                jsParams: []
            },
            {
                id: 'pin-marketing',
                name: 'Marketing Event',
                comment: 'Send Marketing Event Notification',
                jsFunc: Origin.telemetry.sendMarketingEvent,
                jsFuncName: 'sendMarketingEvent',
                jsArgs: [
                    {
                        name: 'category',
                        type: 'text',
                        placeholder: 'Ex. click'
                    },
                    {
                        name: 'action',
                        type: 'text',
                        placeholder: 'Ex. ????'
                    },
                    {
                        name: 'label',
                        type: 'text',
                        placeholder: 'Ex. ????'
                    },
                    {
                        name: 'value',
                        type: 'text',
                        placeholder: 'Ex. ????'
                    },
                    {
                        name: 'params',
                        placeholder: 'JSON object',
                        type: 'json'
                    }
                ],
                jsParams: []
            },
            {
                id: 'pin-friends',
                name: 'Friends Event',
                comment: 'Send Friends Event',
                jsFunc: Origin.telemetry.sendTelemetryEvent,
                jsFuncName: 'sendTelemetryEvent',
                jsArgs: [
                    {
                        name: 'trackerType',
                        type: 'const',
                        values: 'TRACKER_MARKETING'
                    },
                    {
                        name: 'trackerType',
                        type: 'const',
                        values: 'friends'
                    },
                    {
                        name: 'action',
                        type: 'text',
                        placeholder: 'Ex. ????'
                    },
                    {
                        name: 'label',
                        type: 'text',
                        placeholder: 'Ex. ????'
                    },
                    {
                        name: 'params',
                        placeholder: 'JSON object',
                        type: 'json'
                    }
                ],
                jsParams: []
            },
            {
                id: 'pin-transaction',
                name: 'Transaction Event',
                comment: 'Send Transaction Event: Buy games, subs, refund, etc...',
                jsFunc: Origin.telemetry.sendTransactionEvent,
                jsFuncName: 'sendTransactionEvent',
                jsArgs: [
                    {
                        name: 'transactionId',
                        type: 'text',
                        placeholder: 'Ex. 1000216789513'
                    },
                    {
                        name: 'storeId',
                        type: 'select',
                        values: ['web', 'client'],
                        default: 'web'
                    },
                    {
                        name: 'currency',
                        type: 'select',
                        values: ['USD', 'AUD', 'HKD', 'INR', 'NZD', 'SGD', 'ZAR', 'JPY', 'KRW',
                            'THB', 'TWD', 'DKK', 'EUR', 'GPB', 'NOK', 'PLN', 'RUB', 'SEK', 'BRL'],
                        default: 'USD' 
                    },
                    {
                        name: 'revenue',
                        type: 'text',
                        placeholder: 'Ex. 59.95 including tax'
                    },
                    {
                        name: 'tax',
                        type: 'text',
                        placeholder: 'Ex. 5.75'
                    },
                    {
                        name: 'transactionItems',
                        type: 'array',
                        keys: [
                            {
                                name: 'offerId',
                                key: 'id',
                                type: 'text',
                                placeholder: 'Ex. OFB_EAST:1234'
                            },
                            {
                                name: 'gaCategory',
                                key: 'gaCategory',
                                type: 'text',
                                placeholder: 'Ex. ????'
                            },
                            {
                                name: 'pinCategory',
                                key: 'pinCategory',
                                type: 'text',
                                placeholder: 'Ex. ????'
                            },
                            {
                                name: 'name',
                                key: 'name',
                                type: 'text',
                                placeholder: 'Ex. Dragon Age'
                            },
                            {
                                name: 'price',
                                key: 'price',
                                type: 'text',
                                placeholder: 'Ex. 59.95'
                            },
                            {
                                name: 'revenueModel',
                                key: 'revenueModel',
                                type: 'select',
                                values: [
                                    'full_game', 'pdlc', 'mtx', 'virtual'
                                ],
                                default: 'full_game'
                            },
                        ]
                    },
                ],
                jsParams: []
                //jsParams: ['', '', '', '', [new Origin.telemetry.TransactionItem('a', 'b', 'c', 'd', 'e', 'f')]]
            },
            {
                id: 'pin-registration',
                name: 'Register Event',
                comment: 'Event for new user creation.',
                jsFunc: Origin.telemetry.sendTelemetryEvent,
                jsFuncName: 'sendTelemetryEvent',
                jsArgs: [
                    {
                        name: 'trackerType',
                        type: 'const',
                        values: 'TRACKER_MARKETING'
                    },
                    {
                        name: 'trackerType',
                        type: 'const',
                        values: 'registration'
                    },
                    {
                        name: 'source',
                        type: 'text',
                        placeholder: 'Ex. ????'
                    },
                    {
                        name: 'status',
                        type: 'text',
                        placeholder: 'Ex. ????'
                    },
                    {
                        name: 'params',
                        placeholder: 'JSON object',
                        type: 'json'
                    }
                ],
                jsParams: []
            },
            {
                id: 'pin-download',
                name: 'Game Download Event',
                comment: 'Event when user downloads a game',
                jsFunc: Origin.telemetry.sendTelemetryEvent,
                jsFuncName: 'sendTelemetryEvent',
                jsArgs: [
                    {
                        name: 'trackerType',
                        type: 'const',
                        values: 'TRACKER_MARKETING'
                    },
                    {
                        name: 'trackerType',
                        type: 'const',
                        values: 'transaction'
                    },
                    {
                        name: 'action',
                        type: 'text',
                        placeholder: 'Ex. ????'
                    },
                    {
                        name: 'offerid',
                        type: 'text',
                        placeholder: 'Ex. Origin.OFR.50.0000462'
                    },
                    {
                        name: 'params',
                        placeholder: 'JSON object',
                        type: 'json'
                    }
                ],
                jsParams: []
            }
        ];

        $scope.gaEvents = [

        ];

        function selectExample(example) {
            $scope.currentExample = example;
            $scope.executeCmd = '';
            
            // update default
            $scope.currentExample.jsParams = [];
            $scope.currentExample.jsArgs.forEach(function(arg, i) {
                $scope.currentExample.jsParams[i] = arg.default;
            });
        }
        $scope.selectExample = selectExample;

        function change(code) {
            $scope.sampleCode = code;
        }
        $scope.change = change;

        function getCmd(params) {
            if ($scope.currentExample) {
                var info = $scope.currentExample;
                params = _.map(params, function(value) {
                    return JSON.stringify(value);
                });
                var paramStr = params.join(', ');
                return 'Origin.telemetry.' + info.jsFuncName + '(' + paramStr + ')';
            } else {
                return '';
            }
        }

        function execute(info) {
            var params = info.jsParams;
            params = _.map(params, function(value, i) {
                var type = info.jsArgs[i].type;
                if (type === 'json') {
                    try {
                        if (value) {
                            return JSON.parse(value);
                        }
                    } catch (e) {
                        window.alert('JSON error: ' + e.toString());
                    }
                } else if (type === 'const') {
                    return info.jsArgs[i].values;
                } else {
                    return value;
                }
            });
            var cmd = getCmd(params);
            var result = info.jsFunc.apply(null, params);
            LogFactory.log('telemetry-test:', cmd, ' -> ', result);
            $scope.executeCmd = cmd;
            $scope.newItem = {};
            $scope.items = [];
        }
        $scope.execute = execute;
        
        function addItem(index, item) {
            //console.log(JSON.stringify(item));
            var items;
            if (_.isArray($scope.currentExample.jsParams[index])) {
                items = $scope.currentExample.jsParams[index];
            } else {
                items = [];
                $scope.currentExample.jsParams[index] = items;
            }
            items.push(
                new Origin.telemetry.TransactionItem(
                    item.offerId, item.gaCategory, item.pinCategory, item.name, item.price, item.revenueModel
                )
            ); 
        }
        $scope.addItem = addItem;
        
        function removeItem(paramIndex, index) {
            $scope.currentExample.jsParams[paramIndex].splice(index, 1);
        }
        $scope.removeItem = removeItem;

        $scope.newItem = {};
        selectExample($scope.pinEvents[0]);
    }

    function originTelemetryTest(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {},
            controller: 'OriginTelemetryTestCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('telemetry/views/telemetrytest.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originTelemetryTest
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-telemetry-test />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginTelemetryTestCtrl', OriginTelemetryTestCtrl)
        .directive('originTelemetryTest', originTelemetryTest);
}());
