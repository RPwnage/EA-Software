package com.ea.originx.automation.scripts.tools.parameterized;

import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameFamilyType;
import com.ea.originx.automation.scripts.tools.OADLCPlayButton;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OADLCPlayButton - SIMS4<br>
 * parameters:<br>
 * gameFamilyType: SIMS4<br>
 *
 * @author palui
 */
public class OADLCPlayButtonSims4 extends OADLCPlayButton {

    @Test(groups = {"personalization", "client_only"})
    public void testDLCPlayButtonSims4(ITestContext context) throws Exception {
        testDLCPlayButton(context, GameFamilyType.SIMS4);
    }
}
