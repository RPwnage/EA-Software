/**
 * @file common/SingletonRegistryFactory.js
 */
(function() {
    'use strict';

    function SingletonRegistryFactory() {
        //use the opener if possible. All factories should be stored with the opener
        var rootWindow = window.opener || window,
            factoryDepot;

        try {
            //make sure these objects exist
            if (!rootWindow.OriginComponents) {
                rootWindow.OriginComponents = {};
            }
    
            if (!rootWindow.OriginComponents._factories) {
                rootWindow.OriginComponents._factories = {};
            }

        } catch (exp) {
            //Expect this for cross-origin windows
            rootWindow = window;
            rootWindow.OriginComponents = {};
            rootWindow.OriginComponents._factories = {};
        }

        factoryDepot = rootWindow.OriginComponents._factories;

        return {
            /**
             * Gets the instance of the factory to the caller
             * @param  {string}     key             The string id of the factory (make it the same as the function name)
             * @param  {Function}   factoryFunction The factory function that we want to instance or retrieve
             * @param  {Object}     context         The caller context.
             * @param  {Arguments}  params          The arguments for the factoryFunction. The arguments should be in the order the factoryFunction expects.
             * @return {Object}                     The factory object
             */
            get: function(key, factoryFunction, context, params) {

                //we will drop off the last entry in the params which should be the instance of SingletonRegistryFactory
                var args = [].slice.call(params, 0, params.length - 1);

                //see if we've registered this factory already, if we have then we just return the interface
                //if not we'll run the factory first
                //
                //if we are the parent and we already have something stored in the factory depot, we want to re run it again (happens with unit test)
                if ((typeof(factoryDepot[key]) === 'undefined') || !window.opener) {
                    if(rootWindow === window.opener) {
                        //we need to make sure that if we are a popup and we are instantiating a factory, it should be done on the main SPA 
                        var injector = rootWindow.angular.element(rootWindow.document.querySelector('.otk')).injector();
                        factoryDepot[key] =  injector.get(key);
                    } else
                    {
                        factoryDepot[key] = factoryFunction.apply(context, args);
                    }

                }
                return factoryDepot[key];
            } 
        };

    }

    /**
     * @ngdoc service
     * @name origin-components.factories.SingletonRegistryFactory
     * @description
     *
     * SingletonRegistryFactory keeps a map of all the factories we use in the components and stores it always with the main SPA. The
     * idea is that all windows would always use the instance of the factory thats stored with the SPA.
     */
    angular.module('origin-components')
        .factory('SingletonRegistryFactory', SingletonRegistryFactory);

}());
