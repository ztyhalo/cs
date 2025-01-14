#ifndef APP_H
#define APP_H

#include <QObject>
#include "msg.h"

class app
{
  private:
    uint32_t app_id;

  public:
    msg* pmsg;
    bool isRecv;

    app(uint32_t id, int msgkey, bool isRecvIn);
    ~app();
    bool Init(void);
};

#endif // APP_H
