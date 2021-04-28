package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAWatchTheTrailerNoDemoGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests clicking the secondary 'Watch Trailer' CTA opens a video player in
 * full-window mode for anonymous user
 *
 * @author cdeaconu
 */
public class OAWatchTheTrailerNoDemoGameAnonymous extends OAWatchTheTrailerNoDemoGame{
    
    @TestRail(caseId = 3064068)
    @Test(groups = {"gdp", "full_regression", "browser_only"})
    public void testWatchTheTrailerNoDemoGameAnonymous(ITestContext context) throws Exception {
        testWatchTheTrailerNoDemoGame(context, TEST_TYPE.ANONYMOUS_USER);
    }
}