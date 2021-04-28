angular.module('origin-i18n-test', ['origin-i18n'])
    .controller('TestCtrl', function($scope, LocFactory) {

        function test(logStatement, locCall, shouldEqual) {
            console.log(logStatement + ': ' + (locCall === shouldEqual));
        }

        LocFactory.init('en-US', {
            'test': 'abcd',
            'playingnow': '%friends% are playing now',
            'pluraltest': '{0} No friends are playing now | {1} You have %friends% friend playing now',
            'pluralrangetest': '{0} No friends are playing now | {1} You have %friends% friend playing now | [2,10] You have %friends% friends playing now',
            'negativetest': '{-1} You owe me a dollar | {0} You owe me nothing | {1} I owe you a dollar',
            'exclusiverangetestleft': '{0} No friends are playing now | ]1,9] you have %friends% playing now',
            'exclusiverangetestright': '{0} No friends are playing now | [2,10[ you have %friends% playing now',
            'exclusiverangetestleftright': '{0} No friends are playing now | ]1,10[ you have %friends% playing now',
            'exclusiverangetestinfleft': ']-Inf,0] You owe me money | [1,10] you have money',
            'exclusiverangetestinfright': '{0} You have no money | [1,+Inf[ you have %dollars% dollars',
            'mygames': {
                'test': 'efgh',
                'playingnow': 'You have %friends% playing right now'
            }
        });

        // test getting things from the dictionary
        test('Testing a simple get', LocFactory.trans('test'), 'abcd');
        test('Testing a namespaced trans', LocFactory.trans('test', {}, 'mygames'), 'efgh');
        test('Testing a simple substitution', LocFactory.trans('playingnow', {'%friends%': 12}), '12 are playing now');
        test('Testing a namespaced substitution', LocFactory.trans('playingnow', {'%friends%': 13}, 'mygames'), 'You have 13 playing right now');

        // testing misses
        console.log('');
        test('Testing a miss', LocFactory.trans('testmiss'), 'misstestmiss');
        test('Testing a namespaced miss', LocFactory.trans('testmiss', {}, 'mygames'), 'misstestmiss');
        test('Testing a namespaced miss with a miss: ', LocFactory.trans('testmiss', {}, 'mygamesmiss'), 'misstestmiss');

        // testing numeric pluralization
        console.log('');
        test('Testing pluralization with numeric match', LocFactory.transChoice('pluraltest', 0, {'%friends%': 0}), 'No friends are playing now');
        test('Testing pluralization with numeric match and substitution', LocFactory.transChoice('pluraltest', 1, {'%friends%': 1}), 'You have 1 friend playing now');
        test('Testing pluralization miss', LocFactory.transChoice('pluraltestmiss', 0, {'%friends%': 1}), 'misspluraltestmiss');

        // testing negatives pluralization
        //'negativetest': '{-1} You owe me a dollar | {0} You owe me nothing | {1} I owe you a dollar',
        console.log('');
        test('Testing negatives with -1', LocFactory.transChoice('negativetest', -1), 'You owe me a dollar');
        test('Testing negatives with 0', LocFactory.transChoice('negativetest', 0), 'You owe me nothing');
        test('Testing negatives with 1', LocFactory.transChoice('negativetest', 1), 'I owe you a dollar');


        console.log('');
        test('Testing pluralization range with numeric match with no substitution', LocFactory.transChoice('pluralrangetest', 0, {'%friends%': 0}), 'No friends are playing now');
        test('Testing pluralization range with second numeric match with substitution', LocFactory.transChoice('pluralrangetest', 1, {'%friends%': 1}), 'You have 1 friend playing now');
        test('Testing pluralization range with lower bound with substitution', LocFactory.transChoice('pluralrangetest', 2, {'%friends%': 2}), 'You have 2 friends playing now');
        test('Testing pluralization range with upper bound with substitution', LocFactory.transChoice('pluralrangetest', 10, {'%friends%': 10}), 'You have 10 friends playing now');
        test('Testing pluralization range with number in range with substitution', LocFactory.transChoice('pluralrangetest', 5, {'%friends%': 5}), 'You have 5 friends playing now');
        test('Testing pluralization range with number out of range', LocFactory.transChoice('pluralrangetest', 11, {'%friends%': 11}), 'misspluralrangetest');

        console.log('');
        test('Testing exclusive range test left with number match',
            LocFactory.transChoice('exclusiverangetestleft', 0, {'%friends%': 0}),
            'No friends are playing now');
        test('Testing exlusive range test left with number out of range (lower)',
            LocFactory.transChoice('exclusiverangetestleft', 1, {'%friends%': 1}),
            'missexclusiverangetestleft');
        test('Testing exlusive range test left with number at lower range',
            LocFactory.transChoice('exclusiverangetestleft', 2, {'%friends%': 2}),
            'you have 2 playing now');
        test('Testing exlusive range test left with number in range',
            LocFactory.transChoice('exclusiverangetestleft', 5, {'%friends%': 5}),
            'you have 5 playing now');
        test('Testing exlusive range test left with number at higher range',
            LocFactory.transChoice('exclusiverangetestleft', 9, {'%friends%': 9}),
            'you have 9 playing now');
        test('Testing exlusive range test left with number out of range',
            LocFactory.transChoice('exclusiverangetestleft', 10, {'%friends%': 10}),
            'missexclusiverangetestleft');

        console.log('');
        test('Testing exclusive range test right with number match',
            LocFactory.transChoice('exclusiverangetestright', 0, {'%friends%': 0}),
            'No friends are playing now');
        test('Testing exlusive range test right with number out of range (lower)',
            LocFactory.transChoice('exclusiverangetestright', 1, {'%friends%': 1}),
            'missexclusiverangetestright');
        test('Testing exlusive range test right with number at lower range',
            LocFactory.transChoice('exclusiverangetestright', 2, {'%friends%': 2}),
            'you have 2 playing now');
        test('Testing exlusive range test right with number in range',
            LocFactory.transChoice('exclusiverangetestright', 5, {'%friends%': 5}),
            'you have 5 playing now');
        test('Testing exlusive range test right with number at higher range',
            LocFactory.transChoice('exclusiverangetestright', 9, {'%friends%': 9}),
            'you have 9 playing now');
        test('Testing exlusive range test right with number out of range',
            LocFactory.transChoice('exclusiverangetestright', 10, {'%friends%': 10}),
            'missexclusiverangetestright');

        console.log('');
        test('Testing exclusive range test leftright with number match',
            LocFactory.transChoice('exclusiverangetestleftright', 0, {'%friends%': 0}),
            'No friends are playing now');
        test('Testing exlusive range test leftright with number out of range (lower)',
            LocFactory.transChoice('exclusiverangetestleftright', 1, {'%friends%': 1}),
            'missexclusiverangetestleftright');
        test('Testing exlusive range test leftright with number at lower range',
            LocFactory.transChoice('exclusiverangetestleftright', 2, {'%friends%': 2}),
            'you have 2 playing now');
        test('Testing exlusive range test leftright with number in range',
            LocFactory.transChoice('exclusiverangetestleftright', 5, {'%friends%': 5}),
            'you have 5 playing now');
        test('Testing exlusive range test leftright with number at higher range',
            LocFactory.transChoice('exclusiverangetestleftright', 9, {'%friends%': 9}),
            'you have 9 playing now');
        test('Testing exlusive range test leftright with number out of range',
            LocFactory.transChoice('exclusiverangetestleftright', 10, {'%friends%': 10}),
            'missexclusiverangetestleftright');

        console.log('');
        test('Testing exclusive range test infinity lower bound in inf range',
            LocFactory.transChoice('exclusiverangetestinfleft', -5),
            'You owe me money');
        test('Testing exclusive range test infinity lower bound at upper bound of inf range',
            LocFactory.transChoice('exclusiverangetestinfleft', 0),
            'You owe me money');
        test('Testing exclusive range test infinity lower bound at lower bound in closed range',
            LocFactory.transChoice('exclusiverangetestinfleft', 1),
            'you have money');
        test('Testing exclusive range test infinity lower bound at upper bound in closed range',
            LocFactory.transChoice('exclusiverangetestinfleft', 10),
            'you have money');
        test('Testing exclusive range test infinity lower bound out of range',
            LocFactory.transChoice('exclusiverangetestinfleft', 11),
            'missexclusiverangetestinfleft');

        console.log('');
        test('Testing exclusive range test infinity upper bound out of range',
            LocFactory.transChoice('exclusiverangetestinfright', -1, {'%dollars%': -1}),
            'missexclusiverangetestinfright');
        test('Testing exclusive range test infinity upper bound numeric match',
            LocFactory.transChoice('exclusiverangetestinfright', 0, {'%dollars%': 0}),
            'You have no money');
        test('Testing exclusive range test infinity upper bound at lower bound',
            LocFactory.transChoice('exclusiverangetestinfright', 1, {'%dollars%': 1}),
            'you have 1 dollars');
        test('Testing exclusive range test infinity upper bound in range',
            LocFactory.transChoice('exclusiverangetestinfright', 100, {'%dollars%': 100}),
            'you have 100 dollars');
    });