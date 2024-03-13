#include "driver.h"
#include "libdefdebuglog.h"

driver::driver(int id, QString& name, int shminkey, int shmoutkey, int shmoutsem, int shmstatekey)
{
    driver_id   = id;
    driver_name = name;
    ComState    = COMSTATE_NORMAL;

    pshm = new shm(shminkey, shmoutkey, shmoutsem, shmstatekey);
}

driver::~driver()
{
    sysLogQD() << "LibDeviceMng driver exit start! ";
    DELETE(pshm);
    sysLogQD() << "LibDeviceMng driver exit finish! ";
}

bool driver::Init(void)
{
    if (!pshm->shm_create(DriverInfo.TotalInCnt + DriverInfo.TotalOutCnt, DriverInfo.TotalStateCnt))
    {
        sysLogQE() << "LibDeviceMng shm_create error";
        return false;
    }
    sysLogQD() << "LibDeviceMng driver init shm_create size: " << DriverInfo.TotalInCnt << DriverInfo.TotalOutCnt
             << DriverInfo.TotalStateCnt;
    return true;
}

double* driver::GetDataPoint(uint8_t ParentDeviceId, uint8_t ChildDeviceId, uint8_t PointId, uint8_t type)
{
    return (pshm->shm_get_datapoint(ParentDeviceId, ChildDeviceId, PointId, type));
}
