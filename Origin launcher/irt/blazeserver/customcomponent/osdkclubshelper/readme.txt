OSDKClubsHelperComponent
----------------------------

The purpose of this component is to extend standard Blaze clubs functionality to specifically support
OSDK's use of clubs. The component contains commands that would otherwise not be supported by the clubs 
component as well as custom validation for clubs commands.

Validation:
This component adds hooks to the Blaze clubs component, such that it may be called for pre and post
validation of club commands. It will do stuff like check that the team ids set by the GM are valid based on maps in its config file.

Intra-club competition:
To support intra-club games modes (H2H or OTP), this component exposes new functionality for club GM's in the form of a
'wipe intra-club stats' RPC. This RPC will allow GM's to arbitrarily clear the stats for ongoing intra-club games.

Please see this link for more information on how validation is implemented:
 http://gos.online.ea.com/confluence/display/blaze/How+To+Customize+Stock+Blaze+Component+Methods


