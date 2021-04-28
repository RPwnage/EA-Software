// The following 4 lines are to allow for proper JSLinting.
//Uncomment it to run jshint but needs to be commented out before running minification and use
//'use strict';
//var utag = utag || {};
//var Origin = Origin || {};
//var $ = $ || {};

// only modify code below this comment block. code must be minified and
// prepended with javascript: before being used as a page bookmark

(function() {
    // Configure your tool.  Only config.selector must be changed.
    var config = {
        selector: '#OriginXExperimentCompanion',
        interval: 250,
        content: '',
        firstRun: false,

        // optional design config
        location: 'bottom', // top, bottom - default: bottom
        borderColor: 'black', // color - default: black
        backgroundColor: 'rgba(0,0,0,0.7)', // default: 'rgba(0,0,0,0.7)'
        textColor: 'white', // color = default: white
        fontSize: '13px' // default: 13px
    };


    // Do not change this block.  Contains basic functionality for toggling
    // elements, managing content, and initializing tool.
    function createOrToggleElement(){if(""===$(config.selector).text()){config.firstRun=!0;var a=document.createElement("div");a.id=config.selector.replace("#",""),document.getElementsByTagName("body")[0].appendChild(a),config.location=config.location||"bottom",config.borderColor=config.borderColor||"black",config.backgroundColor=config.backgroundColor||"rgba(0,0,0,0.7)",config.textColor=config.textColor||"white",config.fontSize=config.fontSize||"13px",$(config.selector).css({zIndex:"999999",position:"fixed",width:"100%",padding:"8px",backgroundColor:config.backgroundColor,color:config.textColor,fontSize:config.fontSize}),"bottom"===config.location?$(config.selector).css({borderTop:"5px Solid "+config.borderColor,bottom:"0"}):$(config.selector).css({borderBottom:"5px Solid "+config.borderColor,top:"0"})}else config.firstRun=!1,run(),$(config.selector).slideToggle(300)}function addContent(a,b){config.content=b?"":config.content||"",config.content+=a}function showContent(){$(config.selector).html(config.content)}function init(){createOrToggleElement(),config.firstRun&&setInterval(run,config.interval)}


    // Add logic and content management code here.  Use addContent(html) and
    // showContent() to add and display content.  Use addContent(html, true) to
    // start fresh content (should be first use).
    //
    // addContent(text, reset)
    // Used to add content to the view.  Can be text or HTML.  The second
    // paramter, reset, can be used to reset the content.
    //
    // showContent()
    // When this is called, the content will be placed within the container element.
    function run() {
        // start content with a reset
        addContent('<b>Origin X Experiment Companion</b><br>', true);

        // user info
        if (Origin.user.userPid() !== '') {
            addContent('<span><b>User:</b> '+Origin.user.originId()+' ('+Origin.user.userPid()+')</span><br>');
        }

        // url info
        var urlPath = window.location.pathname+window.location.search+window.location.hash;
        addContent('<span><b>URL:</b> '+urlPath+'</span><br>');
        addContent('<br>');

        //look for eax-experiment-bucket
        var expbucket = $('eax-experiment-bucket');

        //if it doesn't exist then early return
        if (!expbucket || expbucket.length === 0) {
            addContent ('<span>No Experiments were found on this page</span><br><br>');
            showContent();
            return;
        }

        var numbuckets = expbucket.length;
        var uniqueExp = [];
        for (var i = 0; i < numbuckets; i++) {
            var expName = expbucket.eq(i).attr('expname'),
                expVar = expbucket.eq(i).attr('expvariant'),
                expStr = expName;

                expStr += expVar ? '|'+expVar : '';

            if (uniqueExp.indexOf(expStr) < 0) {
                //if there's children then it means it wasn't ng-if'd out
                var numChildren = expbucket.eq(i).children().length,
                    activeStr = numChildren > 0 ? 'yes' : 'no';


                //check and see if we're in the experiment:
                addContent('<span><b>EXPERIMENT:</b> '+expName+' , <b>VARIANT:</b> '+expVar+' , <b>ACTIVE:</b></span>');
                addContent('<span style="color: #d0d0d0;"> '+activeStr+'</span><br>');
                uniqueExp.push(expStr);
            }
        }

        // inject new html into companion element
        showContent();
    }

    init();
})();