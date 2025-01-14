#ifndef MSGMNG_H
#define MSGMNG_H

#include "msg.h"
#include <semaphore.h>
#include <QList>
#include <QMap>
#include "DeviceMng.h"
#include <sys/time.h>
#include <QTimer>

#define WAIT_MSG_MAX         100
#define LOGIN_MSG_LEN        1
#define LOGIN_ACK_MSG_LEN    9
#define LOGIN_ACKERR_MSG_LEN 1
#define MSG_TIMEOUT_VALUE    2000
#define MSG_CHECK_TIME       100

typedef int (*ackfunctype)(void* pdata, unsigned int len);

typedef struct
{
    Type_MsgAddr waitid;
    uint16_t     type;
    sem_t*       pack;
    uint32_t     timeout_ms;
    ackfunctype  ackfunc;
} sWaitMsg;

typedef struct
{
    sMsgUnit MsgData;
    uint16_t MsgLen;
} sNotifyMsg;

typedef QList< sWaitMsg >   lWaitList;
typedef QList< sNotifyMsg > lNotifyList;

enum
{
    ACKSEM_Login = 0,
    ACKSEM_Info,
    ACKSEM_MAX
};

class MsgMng : public QObject
{
    Q_OBJECT

  private:
    pthread_t       RecvMsg_id;
    pthread_t       TotalMsg_id;
    msg*            pRecvMsg;
    msg*            pSendMsg;
    msg*            pTotalMsg;
    lWaitList       WaitRecvList;
    SemObject*      pInitSem;
    unsigned int    DeviceMngId;
    int             LoginKeyId;
    sem_t           AckSem[ACKSEM_MAX];
    pthread_mutex_t WaitListMutex;
    int             LoginSuccessFlag;
    sem_t           TotalProcessSem;
    bool            IsRecv;
    static MsgMng*  pMsgCmd;

    MsgMng();
    bool        CheckWaitMsg(Type_MsgAddr waitid, uint16_t type);
    bool        AckWaitMsg(Type_MsgAddr waitid, uint16_t type, uint8_t mode);
    void        timeraddMS(struct timeval* a, uint32_t ms);
    bool        WaitTimeMsgAck(uint32_t waittime_ms, sWaitMsg* pmsg);
    bool        InsertWaitMsg(sWaitMsg* waitmsg);
    ackfunctype GetWaitFunc(Type_MsgAddr waitid, uint16_t type);
    bool        SendMail(sMsgUnit& pkt, uint16_t pkt_len, ackfunctype func, uint32_t timeout);
    void        CheckTimeoutMsg(uint16_t intervaltime);

  private slots:
    void freshTimeoutData(void);

  public:
    QTimer*         freshTimer;
    sem_t           NotifySem;
    lNotifyList     NotifyList;
    bool            cancel;
    pthread_mutex_t NotifyListMutex;

    static MsgMng* GetMsgMng(void);

    ~MsgMng();
    void RecvMsgProcess(void);
    void TotalMsgProcess(void);
    bool InitSendMail(int sendkey, int totalkey, int totalmutexkey);
    bool LoginRecvMail(uint32_t waittime_ms);
    bool InitRecvMail(void);
    bool InitGetInfo(int driver_id, uint32_t timeout_ms);
    bool MsgSendProcess(Type_MsgAddr& addr, uint16_t msgtype, ackfunctype func, uint8_t* pdata, uint16_t len);
    void SetIsRecv(bool isRecv);
};

#endif // MSGMNG_H
