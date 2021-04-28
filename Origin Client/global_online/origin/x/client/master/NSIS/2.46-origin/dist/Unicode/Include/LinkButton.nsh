!ifndef _LINKBUTTON__INC
!define _LINKBUTTON__INC

/* ********************
** LinkButton_SetCallback
** --------------------
Sets error flag on error
*/
!define LinkButton_SetCallback '!insertmacro _LinkButton_SetCallback GetFunctionAddress '
!macro _LinkButton_SetCallback _func _hwnd _CB _tmpvar _outvar
${_func} ${_tmpvar} ${_CB}
LinkButton::BindLinkToAction /NOUNLOAD ${_hwnd} ${_tmpvar}
pop ${_outvar}
!macroend

!endif ;_LINKBUTTON__INC
