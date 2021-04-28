package com.ea.originx.automation.scripts.tools.parameterized;

import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameFamilyType;
import com.ea.originx.automation.scripts.tools.OADLCPlayButton;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OADLCPlayButton<br>
 * parameters:<br>
 * gameFamilyType: Kingdoms of Amalur: Reckoning<br>
 *
 * @author palui
 */
public class OADLCPlayButtonKOAR extends OADLCPlayButton {

    @Test(groups = {"personalization", "client_only"})
    public void testDLCPlayButtonKOAR(ITestContext context) throws Exception {
        testDLCPlayButton(context, GameFamilyType.KINGDOMS_OF_AMALUR_RECKONING);
    }
}
