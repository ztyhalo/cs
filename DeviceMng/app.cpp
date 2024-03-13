#include "app.h"
#include "libdefdebuglog.h"

app::app(uint32_t id, int msgkey)
{
    app_id = id;
    pmsg   = new msg(msgkey);
    Init();
}

app::~app()
{
    sysLogQD() << "DeviceMng**************delete app";
    pmsg->delete_object();
}

bool app::Init(void)
{
    if (!pmsg->create_object())
        return false;
    return true;
}
