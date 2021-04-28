angular.module('components', [])
    .controller('OriginComponentCtrl', function($scope, $http, $sce, $timeout, $compile) {
        var self = this;
        function onMarkupChange() {
            var html = $scope.markup,
                elm = $compile(html)($scope),
                example = self.node.find('.component-example')[0];
            example.innerHTML = '';
            angular.element(example).append(elm);
        }
        this.getTemplate = function() {
            $http.get($scope.componentData).success(function(response) {

                var elm = $compile(response.html)($scope),
                    example = self.node.find('.component-example')[0];

                example.innerHTML = '';
                angular.element(example).append(elm);

                $scope.markup = response.html;
                $scope.js = response.js || false;
                $scope.parameters = response.parameters || false;
                $scope.description = response.description || false;
                $scope.dependencies = response.dependencies || false;
                $scope.transclude = response.transclude || false;
            });
        };
        $scope.$watch('markup', onMarkupChange);
    })
    .directive('originComponent', function() {
        return {
            restrict: 'E',
            scope: {
                'componentData': '@',
                'title': '@'
            },
            controller: 'OriginComponentCtrl',
            templateUrl: 'templates/component.html',
            link: function(scope, element, attrs, ctrl) {
                ctrl.node = element;
                ctrl.getTemplate();
            }
        };
    })
    .controller('OriginElementCtrl', function($scope, $http, $sce, $timeout) {
        var self = this;
        function onMarkupChange() {
            $scope.html = $sce.trustAsHtml($scope.markup);
        }
        this.getTemplate = function() {
            $http.get($scope.componentData).success(function(response) {
                $scope.markup = response.html;
                $scope.html = $sce.trustAsHtml(response.html);
                $scope.description = response.description || false;
                $scope.js = response.js || false;
            });
        };
        $scope.$watch('markup', onMarkupChange);
    })
    .directive('originElement', function() {
        return {
            restrict: 'E',
            scope: {
                'componentData': '@',
                'title': '@'
            },
            controller: 'OriginElementCtrl',
            templateUrl: 'templates/element.html',
            link: function(scope, element, attrs, ctrl) {
                ctrl.node = element;
                ctrl.getTemplate();
            }
        };
    })
    .controller('OriginElementMiniCtrl', function($scope, $http, $sce, $timeout) {
        var self = this;
        function onMarkupChange() {
            $scope.html = $sce.trustAsHtml($scope.markup);
        }
        this.getTemplate = function() {
            $http.get($scope.componentData).success(function(response) {
                $scope.markup = response.html;
                $scope.html = $sce.trustAsHtml($scope.markup);
            });
        };
        $scope.$watch('markup', onMarkupChange);
    })
    .directive('originElementMini', function() {
        return {
            restrict: 'E',
            scope: {
                'componentData': '@',
                'title': '@'
            },
            controller: 'OriginElementMiniCtrl',
            templateUrl: 'templates/element-mini.html',
            link: function(scope, element, attrs, ctrl) {
                ctrl.node = element;
                ctrl.getTemplate();
            }
        };
    })
    .controller('OriginCodeBlockCtrl', function($scope, $http, $sce, $timeout, $rootScope) {

        var self = this,
            whoWatchesTheWatchmen;

        $scope.editing = false;

        function initClippy() {
            var copyTriggers = self.node.find('[data-clipboard-text]'),
                clippy, i, j;
            for (i=0, j=copyTriggers.length; i<j; i++) {
                clippy = new ZeroClipboard(copyTriggers[i]);
                clippy.on('ready', function(readyEvent) {
                    clippy.on('aftercopy', self.afterCopy);
                });
            }
        }

        function initCodeHighlight() {
            var codes = self.node.find('code'),
                i, j;
            for (i=0, j=codes.length; i<j; i++) {
                Prism.highlightElement(self.node.find('code')[i]);
            }
        }

        this.afterCopy = function(e) {
            var targ = e.target;
            targ.innerHTML = 'copied';
            targ.classList.add('componentsource-trigger-isactive');
            targ.blur();
            setTimeout(function() {
                targ.innerHTML = 'copy me';
                targ.classList.remove('componentsource-trigger-isactive');
            }, 3000);
        };

        this.init = function() {
            $timeout(function() {
                initClippy();
                initCodeHighlight();
            }, 100);
            whoWatchesTheWatchmen(); //deregister
        };

        $scope.handleChange = function() {
            var codes = self.node.find('code');
            $scope.html = $sce.trustAsHtml($scope.code);
        };

        $scope.edit = function(e) {
            var textarea =  self.node.find('textarea')[0];
            textarea.classList.add('component-edit-isactive');
            $scope.editing = true;
        };

        $scope.stopEditing = function() {
            var textarea =  self.node.find('textarea')[0];
            textarea.classList.remove('component-edit-isactive');
            $scope.editing = false;
            initCodeHighlight();
        };

        whoWatchesTheWatchmen = $scope.$watch('code', this.init);

    })
    .directive('originCodeBlock', function() {
        return {
            restrict: 'E',
            scope: {
                'code': '=',
                'language': '@'
            },
            controller: 'OriginCodeBlockCtrl',
            templateUrl: 'templates/codeblock.html',
            link: function(scope, element, attrs, ctrl) {
                ctrl.node = element;
            }
        };
    })
    /* add a test directive for a carousel for components */
    .directive('originCarousel', function() {
        return {
            restrict: 'E',
            scope: {},
            template: '<div id="carousel-example-generic" class="otkcarousel" data-ride="otkcarousel"><div class="otkcarousel-inner"><div class="otkcarousel-item otkcarousel-item-active"><img src="images/titanfall.png" alt=""><div class="otkcarousel-caption otkcarousel-caption-left"><h2 class="otktitle-page">Free Games</h2><p class="otkc">Get free full games on the house and in depth demos available through Game Time.</p><p class="otkc"><a href="#" class="otka">Learn More</a></p></div></div><div class="otkcarousel-item"><img src="images/titanfall.png" alt=""><div class="otkcarousel-caption otkcarousel-caption-center"><h2 class="otktitle-page">Unify your PC gaming life</h2><p class="otkc">Discover and play great games. The games you love and new discoveries for you.</p><p class="otkc"><a href="#" class="otka">Learn More</a></p></div></div><div class="otkcarousel-item"><img src="images/titanfall.png" alt=""><div class="otkcarousel-caption otkcarousel-caption-right"><h2 class="otktitle-page">Free Games</h2><p class="otkc">Get free full games on the house and in depth demos available through Game Time.</p><p class="otkc"><a href="#" class="otka">Learn More</a></p></div></div></div><ol class="otkcarousel-indicators"><li class="otkcarousel-indicator otkcarousel-indicator-active" data-target="#carousel-example-generic" data-slide-to="0"></li><li class="otkcarousel-indicator"data-target="#carousel-example-generic" data-slide-to="1"></li><li class="otkcarousel-indicator" data-target="#carousel-example-generic" data-slide-to="2"></li></ol><a class="otkcarousel-control otkcarousel-control-left" href="javascript:void(0);" data-target="#carousel-example-generic" data-slide="prev"><i class="otkicon otkicon-leftarrowcircle otkicon-leftarrow-carousel"></i></a><a class="otkcarousel-control otkcarousel-control-right" href="javascript:void(0);" data-target="#carousel-example-generic" data-slide="next"><i class="otkicon otkicon-rightarrowcircle otkicon-rightarrow-carousel"></i></a></div>'
        }
    });
