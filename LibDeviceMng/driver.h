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
    COMSTATE_ABNORMAL,
    COMSTATE_MAX
} eComStateType;

class driver
{
  private:
    QString driver_name;
    int     driver_id;

  public:
    shm*            pshm;
    sDriverInfoType DriverInfo;
    eComStateType   ComState;

    driver(int id, QString& name, int shminkey, int shmoutkey, int shmoutsem, int shmstatekey);
    ~driver();
    bool    Init(void);
    double* GetDataPoint(uint8_t ParentDeviceId, uint8_t ChildDeviceId, uint8_t PointId, uint8_t type);
};

#endif // DRIVER_H
