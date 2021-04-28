/// \mainpage %Origin Client Source Documentation
///
///
/// \section docsorganization Client Source Docs Organization
///
/// This is the source code documentation for the %Origin Client. It exists in two parts:
/// <UL>
/// <LI>The <a href="pages.html"><b>Origin Client Source &mdash; Developers Guide</b></a> provides instructions on how to do
/// several common operations in the development of the %Origin client. Currently, this includes the following instructions: <UL>
/// <LI>\ref abcreatingARestService</LI>
/// <LI>\ref acusingTheEntitlementsApi</LI>
/// </UL> </LI>
/// <LI>The remainder of the documentation is a <b>Source Code Reference</b>, which provides documentation on: <UL>
///     <LI><a href="annotated.html">Data Structures</a></LI>
///     <LI><a href="hierarchy.html">Classes</a></LI>
///     <LI><a href="namespaces.html">Namespaces</a></LI>
///     <LI><a href="files.html">Source Code Files</a></LI>
///     </UL> </LI>
/// </UL>
///
///
/// \section linkstowiki Links to Relevant Wiki Documents
/// Throughout the Client Source documentation, there are links to relevant pages in the %Origin Internal wiki, which often
/// provide important introductory and conceptual background information to the Client Source sections and features. These
/// links are formatted in the following manner:
///
/// <span style="font-size:14pt;color:#404040;"><b>See the relevant wiki docs:</b></span>
/// <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/Origin+Client">Origin Client</a>
///
/// As Client development progresses, please be sure to add similarly formatted links to the relevant documentation in the wiki to the %Origin Client
/// Source Documentation.
///
///
/// \section namespacediagram Main Namespaces Hierarchy
///
/// The following diagram shows the hierarchy of the main namespaces in the Orgin Client Source Code. Each node is
/// hyperlinked to the namespace reference page in the documentation that describes that namespace in detail.
///
/// \dot
/// 
/// digraph Origin_Client {
/// graph [rankdir="LR"];
/// node [shape="box" fontname="Helvetica" fontsize="9" fontcolor="black" fillcolor="cadetblue1" style="filled"];
/// 
/// { rank = same; "node1"; "node34"; "node35"; "node36"; "Top Level\nNamespaces"; }
/// { rank = same; "node2"; "node3"; "node10"; "node11"; "node12"; "node20"; "node22"; "node23"; "node25"; "node26"; "node27"; "node28"; "node29"; "node33"; "node37"; "2nd Level\nNamespaces"; }
/// { rank = same; "node4"; "node5"; "node9"; "node13"; "node14"; "node15"; "node16"; "node17"; "node18"; "node19"; "node21"; "node24"; "node30"; "node31"; "node32"; "3rd Level\nNamespaces"; }
/// { rank = same; "node6"; "node7"; "node8"; "4th Level\nNamespaces"; }
/// 
/// "Top Level\nNamespaces" [fontname="Helvetica-Bold" fontsize="12", shape=plaintext, fillcolor=white]
/// "2nd Level\nNamespaces" [fontname="Helvetica-Bold" fontsize="12", shape=plaintext, fillcolor=white]
/// "3rd Level\nNamespaces" [fontname="Helvetica-Bold" fontsize="12", shape=plaintext, fillcolor=white]
/// "4th Level\nNamespaces" [fontname="Helvetica-Bold" fontsize="12", shape=plaintext, fillcolor=white]
/// "node1" [label="Origin" URL="\ref Origin"]
/// "node2" [label="Chat" URL="\ref Origin::Chat"]
/// "node3" [label="Client" URL="\ref Origin::Client"]
/// "node4" [label="CommandLine" URL="\ref Origin::Client::CommandLine"]
/// "node5" [label="JsInterface" URL="\ref Origin::Client::JsInterface"]
/// "node6" [label="ConversationEventDict" URL="\ref Origin::Client::JsInterface::ConversationEventDict"]
/// "node7" [label="ConversionHelpers" URL="\ref Origin::Client::JsInterface::ConversionHelpers"]
/// "node8" [label="JavaScript Bridge" fillcolor="goldenrod1" URL="./bridgedocgen/index.xhtml"]
/// "node9" [label="WebDispatcherRequestBuilder" URL="\ref Origin::Client::WebDispatcherRequestBuilder"]
/// "node10" [label="ContentUtils" URL="\ref Origin::ContentUtils"]
/// "node11" [label="Downloader\n(See secondary\nnamespace diagram)" fontcolor="blue" fillcolor="lightslateblue" URL="\ref Origin::Downloader"]
/// "node12" [label="Engine" URL="\ref Origin::Engine"]
/// "node13" [label="Achievements" URL="\ref Origin::Engine::Achievements"]
/// "node14" [label="CloudSaves\n(See secondary\nnamespace diagram)" fontcolor="blue" fillcolor="lightslateblue" URL="\ref Origin::Engine::CloudSaves"]
/// "node15" [label="Config" URL="\ref Origin::Engine::Config"]
/// "node16" [label="Content\n(See secondary\nnamespace diagram)" fontcolor="blue" fillcolor="lightslateblue" URL="\ref Origin::Engine::Content"]
/// "node17" [label="Debug" URL="\ref Origin::Engine::Debug"]
/// "node18" [label="MultiplayerInvite" URL="\ref Origin::Engine::MultiplayerInvite"]
/// "node19" [label="Social" URL="\ref Origin::Engine::Social"]
/// "node20" [label="Escalation" URL="\ref Origin::Escalation"]
/// "node21" [label="inner" URL="\ref Origin::Escalation::inner"]
/// "node22" [label="Platform" URL="\ref Origin::Platform"]
/// "node23" [label="SDK" URL="\ref Origin::SDK"]
/// "node24" [label="Lsx" URL="\ref Origin::SDK::Lsx"]
/// "node25" [label="Services\n(See secondary\nnamespace diagram)" fontcolor="blue" fillcolor="lightslateblue" URL="\ref Origin::Services"]
/// "node26" [label="StringUtils" URL="\ref Origin::StringUtils"]
/// "node27" [label="UIToolkit" URL="\ref Origin::UIToolkit"]
/// "node28" [label="URI" URL="\ref Origin::URI"]
/// "node29" [label="Util" URL="\ref Origin::Util"]
/// "node30" [label="EntitlementXmlUtil" URL="\ref Origin::Util::EntitlementXmlUtil"]
/// "node31" [label="NetUtil" URL="\ref Origin::Util::NetUtil"]
/// "node32" [label="XmlUtil" URL="\ref Origin::Util::XmlUtil"]
/// "node33" [label="Utilities" URL="\ref Origin::Utilities"]
/// "node34" [label="OriginCommonUI" URL="\ref OriginCommonUI"]
/// "node35" [label="OriginCommonUIUtils" URL="\ref OriginCommonUIUtils"]
/// "node36" [label="WebWidget" URL="\ref WebWidget"]
/// "node37" [label="NativeInterfaceRegistry" URL="\ref WebWidget::NativeInterfaceRegistry"]
/// edge [color=white];
/// "node1" -> "node34" -> "node35" -> "node36";
/// "Top Level\nNamespaces" -> "2nd Level\nNamespaces" -> "3rd Level\nNamespaces" -> "4th Level\nNamespaces";
/// edge [color=black];
/// "node1" -> "node2";
/// "node1" -> "node3" -> "node4";
/// "node3" -> "node5" -> "node6";
/// "node5" -> "node7";
/// "node5" -> "node8";
/// "node3" -> "node9";
/// "node1" -> "node10";
/// "node1" -> "node11";
/// "node1" -> "node12" -> "node13";
/// "node12" -> "node14";
/// "node12" -> "node15";
/// "node12" -> "node16";
/// "node12" -> "node17";
/// "node12" -> "node18";
/// "node12" -> "node19";
/// "node1" -> "node20" -> "node21";
/// "node1" -> "node22";
/// "node1" -> "node23" -> "node24";
/// "node1" -> "node25";
/// "node1" -> "node26";
/// "node1" -> "node27";
/// "node1" -> "node28";
/// "node1" -> "node29" -> "node30";
/// "node29" -> "node31";
/// "node29" -> "node32";
/// "node1" -> "node33";
/// "node36" -> "node37";
/// 
/// }
///
/// \enddot
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
///