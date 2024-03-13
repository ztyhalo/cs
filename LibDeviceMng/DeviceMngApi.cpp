#include "DeviceMngApi.h"
// #include <QApplication>
#include "libdefdebuglog.h"
void SignalFunc(int var)
{
    Q_UNUSED(var);
    MsgMng*    pMsgMng    = MsgMng::GetMsgMng();
    DeviceMng* pDeviceMng = DeviceMng::GetDeviceMng();
    DELETE(pMsgMng);
    DELETE(pDeviceMng);
}

void* ProcessMsg_task(void*)
{
    DeviceMngApi* pApi = DeviceMngApi::GetDeviceMngApi();
    sMsgUnit      processmsgdata;
    uint16_t      processmsglen;

    while (1)
    {
        memset(&processmsgdata, 0, sizeof(sMsgUnit));
        processmsglen = 0;

        if (!pApi->wait_msg(&processmsgdata, &processmsglen, WAIT_MSG_BLOCK))
        {
            break;
        }
        else
        {
            lProcessList::iterator item;
            for (item = pApi->processList.begin(); item != pApi->processList.end(); ++item)
            {
                if (((*item).type == processmsgdata.type) &&
                    ((*item).driverid == processmsgdata.source.driver.id_driver))
                {
                    (*item).callfunc(processmsgdata.data, processmsglen);
                }
            }
        }
    }
    sysLogQD() << "LibDeviceMng ProcessMsg_task exit!";
    return NULL;
}

double __get_us(struct timeval t)
{
    return (t.tv_sec * 1000000 + t.tv_usec);
}

int DeviceMngApi::init_data(uint32_t waittime_ms, uint8_t select)
{
    int ret;

    pDeviceMng = DeviceMng::GetDeviceMng();
    pMsgMng    = MsgMng::GetMsgMng();
    struct timeval start, end;
    gettimeofday(&start, NULL);
    ret = pDeviceMng->Init(waittime_ms);
    gettimeofday(&end, NULL);
    suseconds_t msec = end.tv_usec - start.tv_usec;
    time_t      sec  = end.tv_sec - start.tv_sec;
    char buff[512] = {0};
    sprintf(buff,"LibDeviceMng time used:%u.%us",sec, msec);
    sysLogQD() << buff;

    if (select == 0)
        pthread_create(&ProcessMsg_id, NULL, ProcessMsg_task, NULL);

    return ret;
}

DeviceMngApi::DeviceMngApi()
{
    processList.clear();
}

DeviceMngApi* DeviceMngApi::pCmd = NULL;
DeviceMngApi* DeviceMngApi::GetDeviceMngApi()
{
    if (pCmd == NULL)
    {
        pCmd = new DeviceMngApi();
    }
    return pCmd;
}

DeviceMngApi::~DeviceMngApi()
{
    sysLogQD() << "LibDeviceMng normal exit***! addr:" << this;
    DELETE(pMsgMng);
    DELETE(pDeviceMng);
    processList.clear();
    sysLogQD() << "<LibDeviceMng normal exit finish!";
}

uint32_t DeviceMngApi::get_point_appid(
    uint8_t DriverId, uint8_t ParentDeviceId, uint8_t ChildDeviceId, uint8_t PointId, uint8_t type)
{
    return (pDeviceMng->GetAppid(DriverId, ParentDeviceId, ChildDeviceId, PointId, type));
}

bool DeviceMngApi::ctrl_data(CONTROL_DATA_T control_data, double value)
{
    driver* pdriver;
    bool    ret;

    if (!pDeviceMng->FindDriver(control_data.DriverId, &pdriver))
    {
        sysLogQE() << "LibDeviceMng FindDriver fail!";
        return false;
    }
    pdriver->pshm->shm_write_used(
        control_data.ParentDeviceId, control_data.ChildDeviceId, control_data.cmd, control_data.type, 1);
    ret = pdriver->pshm->shm_ctrl(
        control_data.DriverId, control_data.ParentDeviceId, control_data.ChildDeviceId, control_data.cmd, value);
    return ret;
}

double* DeviceMngApi::get_data_point(uint32_t AppId)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;
    double* pvalue;

    pDeviceMng->ChangeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    if (!pDeviceMng->FindDriver(DriverId, &pdriver))
    {
        sysLogQE() << "LibDeviceMng FindDriver fail!";
        return 0;
    }

    pvalue = pdriver->GetDataPoint(ParentDeviceId, ChildDeviceId, PointId, type);
    return pvalue;
}

bool DeviceMngApi::read_data(uint32_t AppId, double* value)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;
    bool    ret;

    pDeviceMng->ChangeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    if (!pDeviceMng->FindDriver(DriverId, &pdriver))
    {
        sysLogQE() << "LibDeviceMng FindDriver fail!";
        return false;
    }
    ret = pdriver->pshm->shm_read(ParentDeviceId, ChildDeviceId, PointId, type, value);
    //   qDebug()<<"DeviceMngApi read_data, appid:  ret:"<<AppId<<"value:"<<*value <<ret;
    return ret;
}

bool DeviceMngApi::write_data(uint32_t AppId, double value)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;
    bool    ret;

    pDeviceMng->ChangeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    if (!pDeviceMng->FindDriver(DriverId, &pdriver))
    {
        sysLogQE() << "LibDeviceMng FindDriver fail!";
        return false;
    }
    ret = pdriver->pshm->shm_write(ParentDeviceId, ChildDeviceId, PointId, type, value);
    return ret;
}

bool DeviceMngApi::read_ctrl_used(uint32_t AppId, int* value)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;

    pDeviceMng->ChangeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    if (!pDeviceMng->FindDriver(DriverId, &pdriver))
    {
        sysLogQE() << "LibDeviceMng FindDriver fail!";
        return false;
    }
    if (!pdriver->pshm->shm_read_used(ParentDeviceId, ChildDeviceId, PointId, type, value))
    {
        sysLogQE() << "LibDeviceMng read_ctrl_used shm_read_used fail!";
        return false;
    }
    return true;
}

bool DeviceMngApi::ctrl_data(uint32_t AppId, double value)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;
    bool    ret;

    pDeviceMng->ChangeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    if (!pDeviceMng->FindDriver(DriverId, &pdriver))
    {
        sysLogQE() << "LibDeviceMng FindDriver fail!";
        return false;
    }
    pdriver->pshm->shm_write_used(ParentDeviceId, ChildDeviceId, PointId, type, 1);
    ret = pdriver->pshm->shm_ctrl(DriverId, ParentDeviceId, ChildDeviceId, PointId, value);
    return ret;
}

bool DeviceMngApi::ctrl_data_block(uint32_t AppId, double value, uint32_t overtime_10ms)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;
    bool    ret;
    int     used, ticks, WaitTicks;

    //    qDebug()<<"DeviceMngApi ChangeAppid ctrl_data start";
    pDeviceMng->ChangeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    //    qDebug()<<"DeviceMngApi ChangeAppid AppId:"<<AppId;
    if (!pDeviceMng->FindDriver(DriverId, &pdriver))
    {
        sysLogQE() << "LibDeviceMng FindDriver fail!";
        return false;
    }
    //    qDebug()<<"DeviceMngApi FindDriver success!";

    GET_SYS_TIME_MS(ticks);
    WaitTicks = ticks + overtime_10ms * 10;

    while (ticks <= WaitTicks)
    {
        if (!pdriver->pshm->shm_read_used(ParentDeviceId, ChildDeviceId, PointId, type, &used))
        {
            sysLogQE() << "LibDeviceMng ctrl_data shm_read_used fail!";
            return false;
        }

        if (used == 0)
        {
            ret = pdriver->pshm->shm_ctrl(DriverId, ParentDeviceId, ChildDeviceId, PointId, value);
            if (ret)
                pdriver->pshm->shm_write_used(ParentDeviceId, ChildDeviceId, PointId, type, 1);

            return ret;
        }
        usleep(10);
        GET_SYS_TIME_MS(ticks);
    }
    sysLogQD() << "LibDeviceMng ctrl_data wait shm_read_used over time!";
    return false;
}

bool DeviceMngApi::get_param(uint32_t AppId, backcallfunc func)
{
    bool         ret;
    Type_MsgAddr addr;

    addr.app = AppId;
    ret      = pMsgMng->MsgSendProcess(addr, MSG_TYPE_AppGetIOParam, (ackfunctype) func, NULL, 0);
    return ret;
}

bool DeviceMngApi::set_param(uint32_t AppId, void* paramlist, uint16_t len, eEffectType mode, backcallfunc func)
{
    bool         ret;
    Type_MsgAddr addr;
    uint8_t      data[MSG_UNIT_LENGTH];

    addr.app = AppId;
    len      = (len < MSG_UNIT_LENGTH) ? len : MSG_UNIT_LENGTH - 1;
    data[0]  = mode;
    memcpy(&data[1], (uint8_t*) paramlist, len);
    ret = pMsgMng->MsgSendProcess(addr, MSG_TYPE_AppSetIOParam, (ackfunctype) func, data, len + 1);
    return ret;
}

bool DeviceMngApi::get_deviceinfo(uint8_t DriverId, backcallfunc func)
{
    bool         ret;
    Type_MsgAddr addr;

    addr.app              = 0;
    addr.driver.id_driver = DriverId;
    ret                   = pMsgMng->MsgSendProcess(addr, MSG_TYPE_DriverGetInfo, (ackfunctype) func, NULL, 0);
    return ret;
}

bool DeviceMngApi::wait_msg(sMsgUnit* recvmsg, uint16_t* msglen, eWaitMsgType mode)
{
    int                   ret;
    lNotifyList::iterator item;

    if (mode == WAIT_MSG_BLOCK)
    {
        ret = sem_wait(&pMsgMng->NotifySem);
    }
    else
    {
        ret = sem_trywait(&pMsgMng->NotifySem);
    }

    if (ret < 0)
    {
        sysLogQD()<<"LibDeviceMng sem_trywait  ret <0";
        return false;
    }

    if (pMsgMng->NotifyList.size() <= 0)
    {
        sysLogQD() << "LibDeviceMng  pMsgMng->NotifyList.size() =" << pMsgMng->NotifyList.size();
        return false;
    }

    pthread_mutex_lock(&pMsgMng->NotifyListMutex);
    item = pMsgMng->NotifyList.begin();
    memcpy(recvmsg, &((*item).MsgData), sizeof(sMsgUnit));
    *msglen = (*item).MsgLen;
    pMsgMng->NotifyList.erase(item);
    pthread_mutex_unlock(&pMsgMng->NotifyListMutex);
    return true;
}

bool DeviceMngApi::read_state(uint8_t DriverId, int childid, char* value, uint16_t len)
{
    driver* pdriver;
    bool    ret;

    //DriverId大于10表示是中继的，对于中继的共享内存统一在主控cs1共享内存下延伸。
    if(DriverId > 10)
    {
        DriverId = 1;
    }


    if (!pDeviceMng->FindDriver(DriverId, &pdriver))
    {
        sysLogQE() << "LibDeviceMng FindDriver fail!";
        return false;
    }
    ret = pdriver->pshm->shm_readstate(childid, value, len);
    return ret;
}

bool DeviceMngApi::add_msgcall(uint8_t DriverId, uint16_t type, backcallfunc func)
{
    sProcessMsg msg;

    if (processList.size() < PROCESS_MSG_MAX)
    {
        msg.driverid = DriverId;
        msg.type     = type;
        msg.callfunc = func;
        processList.append(msg);
        return true;
    }
    return false;
}

void DeviceMngApi::bussness_start_imform(void)
{
    CONTROL_DATA_T cv_id_t;

    int ctrl               = 1;
    cv_id_t.DriverId       = 1;
    cv_id_t.ParentDeviceId = 0;
    cv_id_t.ChildDeviceId  = 0;
    cv_id_t.cmd            = 21;
    cv_id_t.type           = 2;
    ctrl_data(cv_id_t, ctrl);
    cv_id_t.DriverId = 2;
    ctrl_data(cv_id_t, ctrl);
    cv_id_t.DriverId = 3;
    ctrl_data(cv_id_t, ctrl);
}
