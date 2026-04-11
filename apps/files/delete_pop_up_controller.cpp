#include "delete_pop_up_controller.h"
#include <apps/i18n.h>

namespace Files {

DeletePopUpController::DeletePopUpController(Invocation OkInvocation) :
  PopUpController(1, OkInvocation)
{
  m_contentView.setMessage(0, I18n::Message::ConfirmDelete);
}

}
