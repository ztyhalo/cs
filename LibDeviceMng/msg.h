#ifndef MSG_H
#define MSG_H

#include "MsgObject.h"
#include "msgtype.h"
#include <semaphore.h>
#include <QObject>
#include <pthread.h>

class msg : public MsgObject
{
  private:
    pthread_mutex_t SendMutex;

  public:
    msg(int key) : MsgObject(key)
    {
        pthread_mutex_init(&SendMutex, NULL);
    }
    ~msg();

    bool SendMsg(sMsgUnit* pdata, uint16_t size);
    bool ReceiveMsg(sMsgUnit* pdata, uint16_t* psize, int mode);
};

#endif // MSG_H
