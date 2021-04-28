How to run jssdk API test using mockup data
  1. >cd jssdk
  2. >npm install
  3. >bower install
  4. >grunt
  5. >grunt functional-test --force
  

Note:
  1. Initialize jssdk with override URL, ex.
    var config = {
        env : 'qa',
        overrideUrls : {
            userPID : 'http://127.0.0.1:1400/data/pid?',
            userPersona : 'http://127.0.0.1:1400/data/{userId}/persona?',
            consolidatedEntitlements : 'http://127.0.0.1:1400/data/{userId}/consolidatedentitlements?',
            catalogInfo : 'http://127.0.0.1:1400/data/offer/{productId}/{locale}?',
            catalogInfoLMD : 'http://127.0.0.1:1400/data/offerLMD/{productId}?',
            avatarUrls : 'http://127.0.0.1:1400/data/avatar/{size}.xml?userIds={userIdList}',
            atomUsers : 'http://127.0.0.1:1400/data/atomUser/atomUserInfoByUserIds.xml?userIds={userIdList}',
            atomGameUsage : 'http://127.0.0.1:1400/data/{userId}/masterTitleId/{masterTitleId}/usage.xml?',
            xmppConfig : {
                wsHost : '127.0.0.1',
                wsPort : '5291',
                domain : '127.0.0.1',
                wsScheme : 'ws'
            }
        }
    };
    Origin.init(config);
  2. To start mockup server
     >cd automation/mockup
     >npm start
  3. To get HTML report, open origin/DL/web/jssdk/test/specRunner.html (this runner html is generated at above step 7) in browser to run test. Make sure to run mockup server in advance following point 2 above.
