/**
 * Jasmine functional test
 */

'use strict';

describe('StoreSearchQueryFactory', function () {
    var StoreSearchQueryFactory;

    beforeEach(function () {
        window.OriginComponents = {};
        angular.mock.module('origin-components');

        module(function ($provide) {
            $provide.factory('StoreFacetFactory', function () {
                return {
                    getFacets: function() {
                        return {
                            developer: true,
                            platform: true,
                            foo: true,
                            bar: true
                        };
                    }
                };
            });
            $provide.factory('$stateParams', function () {
                return {};
            });
            $provide.factory('$location', function () {
                return {};
            });
            $provide.factory('UrlHelper', function () {
                return {};
            });
            
        });
    });

    beforeEach(inject(function (_StoreSearchQueryFactory_) {
        StoreSearchQueryFactory = _StoreSearchQueryFactory_;
    }));

    describe('splitCommaDelimitedFacetQueryParam', function () {
        it('will split a valid facet query param', function () {
            var query = 'developer:monkey';
            var result = StoreSearchQueryFactory.splitCommaDelimitedFacetQueryParam(query);
            expect(result).toEqual([query]);
        });

        it('will keep a comma in a facet value', function () {
            var query = 'developer:monkey,-inc.';
            var result = StoreSearchQueryFactory.splitCommaDelimitedFacetQueryParam(query);
            expect(result).toEqual(['developer:monkey,-inc.']);
        });

        it('will keep multiple commas in a facet value', function () {
            var query = 'developer:monkey,-inc.,llc,megacorp,foo:red';
            var result = StoreSearchQueryFactory.splitCommaDelimitedFacetQueryParam(query);
            expect(result).toEqual(['developer:monkey,-inc.,llc,megacorp', 'foo:red']);
        });

        it('will return an empty array for undefined query', function () {
            var result = StoreSearchQueryFactory.splitCommaDelimitedFacetQueryParam();
            expect(result).toEqual([]);
        });

        it('will accept colons in the facet value', function () {
            var query = 'developer:mon:key,bar:moo,foo:this:that:and:the:other';
            var result = StoreSearchQueryFactory.splitCommaDelimitedFacetQueryParam(query);
            expect(result).toEqual(['developer:mon:key', 'bar:moo', 'foo:this:that:and:the:other']);
        });
    });

    describe('extractFacetValuesFromQuery', function() {
        it('should extract one valid facet query', function() {
            var queryParams = 'developer:monkey';
            var result = StoreSearchQueryFactory.extractFacetValuesFromQuery(queryParams);
            expect(result).toEqual([['developer', 'monkey']]);
        });

        it('should not split : in facet value', function() {
            var queryParams = 'developer:mon:key';
            var result = StoreSearchQueryFactory.extractFacetValuesFromQuery(queryParams);
            expect(result).toEqual([['developer', 'mon:key']]);
        });

        it('should extract one valid facet query and ignore an invalid facet query', function() {
            var queryParams = 'developer:monkey,badfacet:turtle';
            var result = StoreSearchQueryFactory.extractFacetValuesFromQuery(queryParams);
            expect(result).toEqual([['developer', 'monkey']]);
        });

        it('should extract multiple valid facet queries and ignore multiple invalid facet queries', function() {
            var queryParams = 'developer:monkey,badfacet1:turtle,foo:donkey,badfacet2:rabbit';
            var result = StoreSearchQueryFactory.extractFacetValuesFromQuery(queryParams);
            expect(result).toEqual([['developer', 'monkey'], ['foo', 'donkey']]);
        });

        it('should return an empty array if undefined queryParam', function() {
            var result = StoreSearchQueryFactory.extractFacetValuesFromQuery();
            expect(result).toEqual([]);
        });

        it('should handle multiple facets of the same category', function() {
            var queryParams = 'developer:monkey,developer:donkey,developer:turtle';
            var result = StoreSearchQueryFactory.extractFacetValuesFromQuery(queryParams);
            expect(result).toEqual([['developer', 'monkey'], ['developer', 'donkey'], ['developer', 'turtle']]);
        });

    });

});


