/// \page na-intguide-commerce Integrating Origin Commerce
/// \brief This page discusses the integration of the Origin Commerce feature, which allows the local user to browse
/// store offerings, purchase games and add-ons, and query and redeem the local user's product entitlements.
/// 
/// <img align="right" alt="Commerce User Experience" title="Commerce User Experience" src="OriginSDKCommerceInteractions.png"/>
/// 
/// Origin wraps the Nucleus commerce API (including OFB for catalog management) to deliver offers information to games,
/// including microcontent offerings.
/// 
/// Game purchases are presented by Origin, but the purchases are handled by an external Store, which handles Payment Card
/// Industry (PCI) compliance, and which offers a different set of payment options per territory.
/// 
/// Offers are not shown to a player prior to game launch, but they may be shown when a player quits a game or when a
/// user starts the Origin Client.
/// 
/// The Origin SDK supports the use of both the In-Game Overlay (IGO) to display the standard store, and the
/// construction of custom in-game stores using the APIs. Using the Origin SDK's Commerce API, you can do the following:
/// <ul>
/// 	<li>Show the store IGO, (OriginShowStoreUI), which lists a title's offers.</li>
/// 	<li>Get the catalogue category hierarchy (OriginQueryCategories) and the offers (OriginQueryOffers).</li>
/// 	<li>Get the details of a product purchase (OriginQueryInvoice) when the game gets a purchase event where the
///       player may have redeemed a code or made a purchase and needs the game to unlock the content.</li>
/// 	<li>Show the purchase screen for a list of products (OriginShowCheckoutUI). The user may have used an in-fiction
///       store to browse items but needs to confirm the purchase using an IGO.</li>
/// 	<li>Get the entitlement list for the player's products (OriginQueryEntitlements). This is for CMS entitlements,
///       (used by the EA Store and the Origin Store), rather than Nucleus entitlements.</li>
/// 	<li>Check which DLC the user has installed (OriginQueryResource).</li>
/// 	<li>Show the code redemption IGO, (OriginShowCodeRedemptionUI), which allows users to enter Keymaster codes to
///       unlock content.</li>
/// </ul>
/// 
/// The following code example illustrates how to purchase a product via lockbox. Selecting products, purchasing them,
/// then consuming the purchased entitlement involves multiple steps. To complete the purchase process, the
/// user is transferred to an online store to enter payment details. Once the product is purchased, the user will
/// receive a product entitlement which can be consumed. These examples show synchronous calls so that the complete
/// workflow can be shown in a single function, but asynchronous functions are also available.
/// 
/// \include exampleCommerce.txt
/// 
/// This next code example shows how a user who has purchased a product can consume their entitlements. This example
/// also uses synchronous calls so that the complete workflow can be shown in a single function, but again asynchronous
/// functions are available.
/// 
/// \include exampleCommerceConsumeEntitlements.txt
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
