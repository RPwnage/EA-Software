package com.ea.originx.automation.scripts.tools.parameterized;

import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameFamilyType;
import com.ea.originx.automation.scripts.tools.OADLCPlayButton;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OADLCPlayButton<br>
 * parameters:<br>
 * gameFamilyType: MOH War Fighter<br>
 *
 * @author palui
 */
public class OADLCPlayButtonMOHWarfighter extends OADLCPlayButton {

    @Test(groups = {"personalization", "client_only"})
    public void testDLCPlayButtonMOHWarfighter(ITestContext context) throws Exception {
        testDLCPlayButton(context, GameFamilyType.MOH_WAR_FIGHTER);
    }
}
