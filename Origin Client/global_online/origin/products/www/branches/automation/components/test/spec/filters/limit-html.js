/**
 * Jasmine functional test
 */

'use strict';

describe('Limit Html', function() {
    var filter;

    beforeEach(function(){
        angular.mock.module('origin-components');
    });

    beforeEach(inject(function(_$filter_) {
        filter = _$filter_;
    }));

    describe('filter', function() {

        it('will trim a sentence to a given limit, rounding to the nearest sentence or whole word.', function() {
            var limit = filter('limitHtml');
            expect(limit('This is my sentence', 10, '...'))
                .toBe('This is...');

            expect(limit('This is my sentence', 15))
                .toBe('This is my');

            expect(limit('This is my sentence. It needs to be truncated', 25, '...'))
                .toBe('This is my sentence.');

            expect(limit('This is my sentence! It needs to be truncated', 20))
                .toBe('This is my sentence!');

            expect(limit('This is, my sentence', 10, '...'))
                .toBe('This is...');

        });

        it('will remove all html', function() {
            var limit = filter('limitHtml');
            expect(limit('<div>This is my sentence</div>', 10, '...'))
                .toBe('This is...');
    
            expect(limit(   '<i>Note: Currently this map pack&nbsp;<b>only works with the &lt;EADM&gt; &amp; &quot;disk&quot; versions of Mirrors Edge</b>.' +
                            'If you have purchased the game through another download service, you may receive an error message during ' +
                            'installation that will prevent use with the full game. We apologize for any disappointment or inconvenience ' +
                            'that this causes.</i><p><b>*Requires Mirrors Edge to play.</b></p><p>Time Trial is a popular mode in Mirrors' +
                            ' Edge that challenges players to find the fastest route across various maps. Players can upload their best time ' +
                            ' to the Mirrors Edge leaderboards for their friends to download and compete against. Players can see their ' +
                            ' friends "ghost" run as they race their way through the course to the top of the leaderboards.</p>', 190, '...'))
                .toEqual('Note: Currently this map pack only works with the <EADM> & "disk" versions of Mirrors Edge.');                

            expect(limit('<div>This <span>is <a href="whatev.html">my</a> <i>sentence</i></span></div>', 10, '...'))
                .toBe('This is...');
        });

        it('will only take text from the the first block level element', function() {
            var limit = filter('limitHtml');
            expect(limit('<ul><li>This <span>is <a href="whatev.html">my</a> <i>sentence.</i></span></li><li>This is another sentence.</li></ul>', 25, '...'))
                .toBe('This is my sentence.');
        });

        it('will only take text from the the first block level element or br tag', function() {
            var limit = filter('limitHtml');
            expect(limit('This is my sentence.<br>This is my sentenc again.', 25, '...', true))
                .toBe('This is my sentence.');
        });

        it('will handle foreign languages', function() {
            var limit = filter('limitHtml');
            expect(limit('คือเกมจำลองชีวิตที่มีคนตั้งตารอมหาศาลซึ่งให้คุณเล่นสนุกกับชีวิ', 10, '...'))
                .toBe('คือเกมจ...');

            expect(limit('คือเกมจำลองชีวิตที่มีคนตั้งตารอมหาศาลซึ่งให้คุณเล่นสนุกกับชีวิ', 10))
                .toBe('คือเกมจำลอ');

            expect(limit('<div>คือเกมจำลองชีวิตที่มีคนตั้งตารอมหาศาลซึ่งให้คุณเล่นสนุกกับชีวิ</div>', 10, '...'))
                .toBe('คือเกมจ...');

            // Real use case from Korean Battlefield 4
            expect(limit(   'Battlefield 4™의 비길 데 없는 파괴를 경험하세요. 인터렉티브 환경에서 전술 보상이 가득한 전면전의 화려한 혼돈을 ' + 
                            '경험하세요. 적을 가리는 건물을 파괴하고 포함 뒤에서 습격을 이끄세요. 더 많은 것을 할 자유가 있습니다. 자신의 장점을 ' +
                            '활용하고 자신만의 길을 개척하여 승리할 수 있습니다. 비교할 수 없습니다. 전면전의 화려한 혼란에 몰입하세요. <br><br>' + 
                            '<b>주요 특징: </b><br><br><b>역동적인 전장. </b>사용자의 액션에 실시간으로 반응하는 인터렉티브 환경을 이용하여 배를' +
                            ' 난파시키거나 거리를 점령하세요. 적은 예측할 수 없을 것입니다. <br><br><b>더 많은 탑승장비, 더 많은 파괴, 더 많은 자유. ' +
                            '</b>이전보다 더 많은 탑승장비와 파괴로 비길 데 없는 수준의 전면전을 경험하세요. 전장의 거대한 영역과 규모 덕분에 자유롭게 자신의 ' +
                            '장점을 활용하고 자신만의 길을 개척하여 승리할 수 있습니다. <br><br><b>상륙 작전을 개시하세요. </b>포병을 배치하고 폭풍 같은 ' +
                            '전투에서 적을 쓰러뜨리세요. 새롭고 격렬한 해상 기반 탑승장비 전투에서 육, 해, 공을 정복하세요. <br><br><b>강렬한 싱글 플레이어 ' +
                            '캠페인. </b>Battlefield 4™는 캐릭터 기반의 캠페인을 제공하며, 상하이에서 미국인 VIP의 대피로 시작하여 집에 되돌아가려는 분대의 ' +
                            '고군분투기를 따라갑니다.', 190, '...'))
                .toBe(  'Battlefield 4™의 비길 데 없는 파괴를 경험하세요. 인터렉티브 환경에서 전술 보상이 가득한 전면전의 화려한 혼돈을 경험하세요. 적을 가리는 건물을 ' +
                        '파괴하고 포함 뒤에서 습격을 이끄세요. 더 많은 것을 할 자유가 있습니다. 자신의 장점을 활용하고 자신만의 길을 개척하여 승리할 수 있습니다. 비교할 수 없습니다.');
        });

    });
});