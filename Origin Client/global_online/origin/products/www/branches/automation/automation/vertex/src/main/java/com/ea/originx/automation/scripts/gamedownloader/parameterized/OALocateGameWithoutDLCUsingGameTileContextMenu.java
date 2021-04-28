package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OALocateGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test Locate game without DLC using game tile context menu
 *
 * @author sunlee
 */
public class OALocateGameWithoutDLCUsingGameTileContextMenu extends OALocateGame {

    @TestRail(caseId = 383370)
    @Test(groups = {"gamelibrary", "gamedownloader", "client_only"})
    public void testLocateGameWithoutDLCUsingGameTileContextMenu(ITestContext context) throws Exception {
        testLocateGame(context, false, METHOD_TO_LOCATE_GAME.USE_GAME_TILE_CONTEXT_MENU, this.getClass().getName());
    }
}