(function() {

    Origin.init(function() {});
    Origin.AuthManager.login();


    bench = new Benchmark('foo', {

        // a flag to indicate the benchmark is deferred
        'defer': true,

        // benchmark test function
        'fn': function(deferred) {

            OriginBridge.games('OFB-EAST:109552419').startDownload();
        }
    });
})();