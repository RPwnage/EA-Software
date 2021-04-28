package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAWatchTheTrailerDemoGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests clicking the secondary 'Watch Trailer' CTA opens a video player in
 * full-window mode for a demo game, non-subscriber account
 *
 * @author cdeaconu
 */
public class OAWatchTheTrailerDemoGameNonSubscriber extends OAWatchTheTrailerDemoGame{
    
    @TestRail(caseId = 3242469)
    @Test(groups = {"gdp", "full_regression"})
    public void testWatchTheTrailerDemoGameNonSubscriber(ITestContext context) throws Exception {
        testWatchTheTrailerDemoGame(context, TEST_TYPE.NON_SUBSCRIBER);
    }
}