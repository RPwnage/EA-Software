package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAMoveGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test move game with DLC to same drive using game tile context menu
 *
 * @author sunlee
 */
public class OAMoveGameWithDLCToSameDriveUsingGameTileContextMenu extends OAMoveGame {

    @TestRail(caseId = 4380090)
    @Test(groups = {"gamelibrary", "gamedownloader", "client_only"})
    public void testMoveGameWithDLCToSameDriveUsingGameTileContextMenu(ITestContext context) throws Exception {
        testMoveGame(context, true, true, METHOD_TO_MOVE_GAME.USE_GAME_TILE_CONTEXT_MENU, this.getClass().getName());
    }
}