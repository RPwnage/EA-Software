package com.ea.originx.automation.scripts.tools.parameterized;

import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameFamilyType;
import com.ea.originx.automation.scripts.tools.OADLCPlayButton;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OADLCPlayButton<br>
 * parameters:<br>
 * gameFamilyType: Crysis 3<br>
 *
 * @author palui
 */
public class OADLCPlayButtonCrysis3 extends OADLCPlayButton {

    @Test(groups = {"personalization", "client_only"})
    public void testDLCPlayButtonCrysis3(ITestContext context) throws Exception {
        testDLCPlayButton(context, GameFamilyType.CRYSIS_3);
    }
}
