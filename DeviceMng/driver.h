#ifndef DRIVER_H
#define DRIVER_H

#include <QObject>
#include <semaphore.h>
#include "shm.h"
#include "msg.h"
#include <time.h>

typedef struct
{
    uint16_t TotalInCnt;
    uint16_t TotalOutCnt;
    uint16_t TotalStateCnt;
} sDriverInfoType;

typedef enum
{
    COMSTATE_NORMAL = 0,
    COMSTATE_ABNORMAL
} eComStateType;

class driver
{
  private:
    QString driver_name;
    int     driver_id;

    bool WaitSem(int time_10ms);
    bool Msg_GetInfo(void);

  public:
    shm*            pshm;
    msg*            pmsg;
    sem_t           AckSem;
    sDriverInfoType DriverInfo;
    eComStateType   ComState;

    driver(int id, QString& name, int shminkey, int shmoutkey, int shmoutsem, int msgkey, int shmstatekey);
    ~driver();
    bool Init(void);
    bool InitMsg(void);
    bool Msg_SendHeart(void);
};

#endif // DRIVER_H
