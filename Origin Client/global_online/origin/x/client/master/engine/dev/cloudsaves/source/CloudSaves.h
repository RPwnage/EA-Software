#ifndef _CLOUDSAVES_CLOUDSAVES_H
#define _CLOUDSAVES_CLOUDSAVES_H

namespace Origin
{
namespace Engine
{
/// \brief The namespace for the %CloudSaves feature, which gives users the ability to store and access their game-generated data
/// from any Origin-equipped machine.
///
/// <span style="font-size:14pt;color:#404040;"><b>See the relevant wiki docs:</b></span> <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/Cloud+Storage">Cloud Storage</a> and  <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/Cloud+Storage+2.0">Cloud Storage 2.0</a>
/// 
/// A %CloudSaves namespaces diagram is shown below:
/// \dot
/// digraph g {
/// graph [rankdir="LR"];
/// node [shape="box" fontname="Helvetica" fontsize="9" fontcolor="black" fillcolor="cadetblue1" style="filled"];
/// edge [];
///
/// "node00" -> "node0" -> "node1" -> "node2";
/// "node1" -> "node3";
/// "node1" -> "node4";
/// "node4" [label="SaveFileCrawler" URL="\ref SaveFileCrawler"]
/// "node3" [label="LastSyncStore" URL="\ref LastSyncStore"]
/// "node2" [label="FilesystemSupport" URL="\ref FilesystemSupport"]
/// "node1" [label="CloudSaves" URL="\ref CloudSaves"]
/// "node0" [label="Engine" URL="\ref Engine"]
/// "node00" [label="Origin" URL="\ref Origin"]
/// }
/// \enddot
namespace CloudSaves
{
    void init();
}
}
}


#endif
