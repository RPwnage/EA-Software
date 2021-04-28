package com.ea.originx.automation.scripts.tools.parameterized;

import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameFamilyType;
import com.ea.originx.automation.scripts.tools.OADLCPlayButton;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OADLCPlayButton<br>
 * parameters:<br>
 * gameFamilyType: ME3<br>
 *
 * @author palui
 */
public class OADLCPlayButtonMassEffect3 extends OADLCPlayButton {

    @Test(groups = {"personalization", "client_only"})
    public void testDLCPlayButtonMassEffect3(ITestContext context) throws Exception {
        testDLCPlayButton(context, GameFamilyType.MASS_EFFECT_3);
    }
}
