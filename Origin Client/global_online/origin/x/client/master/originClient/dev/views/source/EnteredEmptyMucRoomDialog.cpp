#include "EnteredEmptyMucRoomDialog.h"

#include "services/debug/DebugService.h"

#include "Qt/originpushbutton.h"
#include "Qt/originmsgbox.h"

namespace Origin
{
namespace Client
{

EnteredEmptyMucRoomDialog::EnteredEmptyMucRoomDialog()
    : UIToolkit::OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                   NULL, OriginWindow::MsgBox, QDialogButtonBox::Close)
{
    msgBox()->setup(UIToolkit::OriginMsgBox::NoIcon,
        tr("ebisu_client_group_chat_closed_uppercase"), 
        tr("ebisu_client_group_chat_closed_text"));

    ORIGIN_VERIFY_CONNECT(this, SIGNAL(rejected()), this, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close()));
}

}
}
