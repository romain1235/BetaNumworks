#ifndef FILES_DELETE_POP_UP_CONTROLLER_H
#define FILES_DELETE_POP_UP_CONTROLLER_H

#include <escher/pop_up_controller.h>
#include <escher/invocation.h>

namespace Files {

class DeletePopUpController : public PopUpController {
public:
  DeletePopUpController(Invocation OkInvocation);
};

}

#endif
