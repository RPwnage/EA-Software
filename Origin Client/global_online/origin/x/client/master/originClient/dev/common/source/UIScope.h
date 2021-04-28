#ifndef _UISCOPE_H
#define _UISCOPE_H

namespace Origin
{
namespace Client
{

/// \brief Enumeration defining where UI elements should be placed
enum UIScope
{
    /// \brief UI should appear in the desktop client
    ClientScope,
    /// \brief UI should appear in the in-game overlay
    IGOScope
};

/// \brief - used to specify the initial show state of the client window
enum ClientViewWindowShowState
{
    ClientWindow_NO_ACTION,
    ClientWindow_SHOW_NORMAL,
    ClientWindow_SHOW_NORMAL_IF_CREATED,
    ClientWindow_SHOW_MINIMIZED,     //task bar
    ClientWindow_SHOW_MINIMIZED_SYSTEMTRAY,   //system tray
    ClientWindow_SHOW_MAXIMIZED
};

}
}

#endif

