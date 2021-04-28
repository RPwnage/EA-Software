package com.ea.originx.automation.scripts.tools.parameterized;

import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameFamilyType;
import com.ea.originx.automation.scripts.tools.OADLCPlayButton;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OADLCPlayButton<br>
 * parameters:<br>
 * gameFamilyType: ME2<br>
 *
 * @author palui
 */
public class OADLCPlayButtonMassEffect2 extends OADLCPlayButton {

    @Test(groups = {"personalization", "client_only"})
    public void testDLCPlayButtonMassEffect2(ITestContext context) throws Exception {
        testDLCPlayButton(context, GameFamilyType.MASS_EFFECT_2);
    }
}
