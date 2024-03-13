#ifndef MSGMNG_H
#define MSGMNG_H

#include "msg.h"
#include <semaphore.h>
#include <QList>
#include <QMap>
#include "app.h"
#include "DeviceMng.h"
#include <sys/time.h>

#define WAIT_MSG_MAX     100
#define APP_LOGIN_MAX    10
#define ABNORMAL_MSG_LEN 1
#define NORMAL_MSG_LEN   9

typedef struct
{
    Type_MsgAddr waitid;
    uint16_t     type;
    sem_t*       pack;
} sWaitMsg;
typedef QList< sWaitMsg >      lWaitList;
typedef QMap< uint32_t, app* > mAppTable;

class MsgMng
{
  private:
    pthread_t  DriverMsg_id;
    pthread_t  AppMsg_id;
    msg*       pResMsg;
    msg*       pDriverMsg;
    msg*       pAppMsg;
    msg*       pAppTotalMsg;
    lWaitList  WaitDriverList;
    mAppTable  AppTable;
    SemObject* pinitsem;

    MsgMng();
    bool IsAppMapExist(uint32_t id);
    bool FindApp(uint32_t id, app** ppapp);
    bool DeleteApp(uint32_t id);
    bool CheckWaitMsg(Type_MsgAddr waitid, uint16_t type);
    bool AckWaitMsg(Type_MsgAddr waitid, uint16_t type);

  public:
    pthread_mutex_t RevTaskMutex;
    pthread_mutex_t WaitListMutex;
    bool            cancel;
    static MsgMng*  pMsgCmd;

    static MsgMng* GetMsgMng(void);
    //    {
    //        static MsgMng gMsgMng;
    //        return &gMsgMng;
    //    }
    ~MsgMng();
    bool Init(int appkey, int driverkey, int totalkey, int initsemkey, int reskey);
    bool InsertWaitMsg(Type_MsgAddr& waitid, uint16_t type, sem_t* pack);
    void DriverMsgProcess(void);
    void AppMsgProcess(void);
    bool BroadcastToApp(uint16_t type, uint8_t* data, uint16_t len);
};

#endif // MSGMNG_H
