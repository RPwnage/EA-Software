/**
* This file is meant to make our documentation more accessible
* @file documentation.js
*/
(function() {
	'use strict';
    var links = {
        'FAQ': 'https://confluence.ea.com/display/EBI/Origin+X+Development+FAQ',
        'Best Practices Guide': 'https://confluence.ea.com/display/EBI/Origin+X+-+Coding+Standard+and+Style+Guide',
        'Coding Standard': 'https://confluence.ea.com/pages/viewpage.action?spaceKey=EBI&title=Origin+X+-+JavaScript+Coding+Standard'
    };

    console.info('^( \'.\' )>\n\nHeeeeey there fellow Origin X Developer!\nAre you looking for some help in builing out Origin X?\nTake a look at our documentation.\n--------------------------------------\n');
    for(var i in links) {
        console.info('%c%s : %c%s', 'font-weight:bold;', i, 'font-weight:normal;', links[i]); 
    }
}());
