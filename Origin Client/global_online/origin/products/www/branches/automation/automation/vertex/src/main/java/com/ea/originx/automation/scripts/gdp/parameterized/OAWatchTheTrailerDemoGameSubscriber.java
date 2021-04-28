package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAWatchTheTrailerDemoGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests clicking the secondary 'Watch Trailer' CTA opens a video player in
 * full-window mode for a demo game, subscriber account
 *
 * @author cdeaconu
 */
public class OAWatchTheTrailerDemoGameSubscriber extends OAWatchTheTrailerDemoGame{
    
    @TestRail(caseId = 3242467)
    @Test(groups = {"gdp", "full_regression"})
    public void testWatchTheTrailerDemoGameSubscriber(ITestContext context) throws Exception {
        testWatchTheTrailerDemoGame(context, TEST_TYPE.SUBSCRIBER);
    }
}