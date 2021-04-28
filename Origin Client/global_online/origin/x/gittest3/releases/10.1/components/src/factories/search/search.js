(function () {
    "use strict";

    function SearchFactory() {

        var searchProviders = {},
            categories = {},
            defaultIncludes = [];

        function addSearchProvider(provider, data) {
            searchProviders[provider.area] = provider;
            categories[provider.area] = data;
            defaultIncludes.push(provider.area);
        }

        function copyIncludes(ctx, includes, excludes) {
            var i, incl, excl;

            if (excludes) {
                excl = excludes.split(',');
            } else {
                excl = [];
            }

            ctx.includes = [];
            if (!includes) {
                for (i = 0; i < defaultIncludes.length; i = i + 1) {
                    if (excl.indexOf(defaultIncludes[i]) < 0) {
                        ctx.includes.push(defaultIncludes[i]);
                    }
                }
            } else {
                incl = includes.split(',');
                for (i = 0; i < incl.length; i = i + 1) {
                    if (excl.indexOf(incl[i]) < 0) {
                        ctx.includes.push(incl[i]);
                    }
                }
            }
        }

        function createSearchContext() {
            return {
                categories: categories,
                includes: {},
                searchString: '',
                results: {},
                activeSearchId: 0,
                currentSearchId: 0,
                searchCount: 0,
                events: new Origin.utils.Communicator(),
                set: function (ctx, area, id, data) {
                    if (area && ctx.activeSearchId === id) {
                        ctx.results[area] = data;
                        ctx.searchCount = ctx.searchCount - 1;
                        ctx.events.fire('search:updated');
                        if (ctx.searchCount <= 0) {
                            ctx.events.fire('search:finished');
                        }
                    } else {
                        console.log('setSearchResults: Search Results Dropped' + data);
                    }
                },
                getResults: function(filter) {
                    if(filter && filter !== '' && this.results.hasOwnProperty(filter)) {
                        return this.results[filter];
                    }
                    return this.results;
                }

            };
        }

        function search(ctx, options) {
            if (ctx) {
                ctx.activeSearchId = ctx.activeSearchId + 1;
                ctx.searchCount = 0;
                ctx.searchString = options.searchString;

                copyIncludes(ctx, options.includes, options.excludes);

                setTimeout(function () {
                    var i, area;
                    ctx.currentSearchId = ctx.currentSearchId + 1;

                    if (ctx.currentSearchId === ctx.activeSearchId) {
                        for (i = 0; i < ctx.includes.length; i = i + 1) {
                            area = ctx.includes[i];
                            ctx.results[area] = undefined;
                            ctx.searchCount = ctx.searchCount + 1;
                            searchProviders[area].search(ctx, ctx.activeSearchId, options[area]);
                        }
                    } else {
                        console.log('Not searching for ' + options.searchString);
                    }
                }, 500); // Wait 500 ms to prevent spamming the search engine.

                ctx.events.fire('search:started');
            } else {
                console.log('search: unknown source');
            }
        }

        return {
            addSearchProvider: addSearchProvider,
            createSearchContext: createSearchContext,
            search: search
        };
    }

    angular.module('origin-components')
        .factory('SearchFactory', SearchFactory);
}());