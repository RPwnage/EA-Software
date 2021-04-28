package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAWatchTheTrailerNoDemoGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests clicking the secondary 'Watch Trailer' CTA opens a video player in
 * full-window mode for a subscriber account
 *
 * @author cdeaconu
 */
public class OAWatchTheTrailerNoDemoGameSubscriber extends OAWatchTheTrailerNoDemoGame{
    
    @TestRail(caseId = 3134290)
    @Test(groups = {"gdp", "full_regression"})
    public void testWatchTheTrailerNoDemoGameSubscriber(ITestContext context) throws Exception {
        testWatchTheTrailerNoDemoGame(context, TEST_TYPE.SUBSCRIBER);
    }
}