#include "DeviceMngApi.h"
#include "timers.h"
#include <signal.h>

void SignalFunc(int var)
{
    Q_UNUSED(var);

    DeviceMngApp* pDeviceMng = DeviceMngApp::getDeviceMngApp();
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
            lProcessList::iterator item = pApi->processList.begin();
            for (; item != pApi->processList.end(); ++item)
            {
                if (((*item).type == processmsgdata.type) &&
                    ((*item).driverid == processmsgdata.source.driver.id_driver))
                {
                    (*item).callfunc(processmsgdata.data, processmsglen);
                }
            }
        }
    }
    zprintf3("LibDeviceMng ProcessMsg_task exit!\n");
    return NULL;
}

double __get_us(struct timeval t)
{
    return (t.tv_sec * 1000000 + t.tv_usec);
}

int DeviceMngApi::init_data(uint32_t waittime_ms, uint8_t select, bool isRecv)
{

    int ret;

    zprintf1("LibDeviceMng init_data begin:\n");
    m_pAppDevMng = DeviceMngApp::getDeviceMngApp();

    m_pAppDevMng->m_pMngApp->setIsRecv(isRecv);            //干什么用的

    struct timeval start, end;
    gettimeofday(&start, NULL);
    ret = m_pAppDevMng->initApp();
    gettimeofday(&end, NULL);
    suseconds_t msec      = end.tv_usec - start.tv_usec;
    time_t      sec       = end.tv_sec - start.tv_sec;
    char        buff[512] = {0};
    sprintf(buff, "LibDeviceMng time used:%u.%us", sec, msec);
    zprintf3("%s!\n", buff);

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
    zprintf3("LibDeviceMng normal exit start!\n");
    DELETE(m_pAppDevMng);
    // DELETE(pDeviceMng);
    processList.clear();
    zprintf3("LibDeviceMng normal exit end!\n");
}

uint32_t DeviceMngApi::get_point_appid(
    uint8_t DriverId, uint8_t ParentDeviceId, uint8_t ChildDeviceId, uint8_t PointId, uint8_t type)
{
    return (m_pAppDevMng->getAppid(DriverId, ParentDeviceId, ChildDeviceId, PointId, type));
}

bool DeviceMngApi::ctrl_data(CONTROL_DATA_T control_data, double value)
{
    driver* pdriver;
    bool    ret;

    if (!m_pAppDevMng->m_pMngApp->findDriver(control_data.DriverId, &pdriver))
    {
        zprintf1("LibDeviceMng FindDriver fail!\n");
        return false;
    }
    pdriver->m_pShm->shm_write_used(
        control_data.ParentDeviceId, control_data.ChildDeviceId, control_data.cmd, control_data.type, 1);
    ret = pdriver->m_pShm->shm_ctrl(
        control_data.DriverId, control_data.ParentDeviceId, control_data.ChildDeviceId, control_data.cmd, value);
    return ret;
}

double* DeviceMngApi::get_data_point(uint32_t AppId)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;
    double* pvalue = NULL;

    m_pAppDevMng->changeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    if (!m_pAppDevMng->m_pMngApp->findDriver(DriverId, &pdriver))
    {
        zprintf1("LibDeviceMng FindDriver fail!\n");
        return NULL;
    }

           // pvalue = pdriver->m_pShm->getDataPoint(ParentDeviceId, ChildDeviceId, PointId, type);
    pvalue = pdriver->m_pShm->shm_get_datapoint(ParentDeviceId, ChildDeviceId, PointId, type);
    return pvalue;
}

bool DeviceMngApi::read_data(uint32_t AppId, double* value)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;
    bool    ret;

    m_pAppDevMng->changeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    if (!m_pAppDevMng->m_pMngApp->findDriver(DriverId, &pdriver))
    {
        zprintf1("LibDeviceMng FindDriver fail!\n");
        return false;
    }
    ret = pdriver->m_pShm->shm_read(ParentDeviceId, ChildDeviceId, PointId, type, value);
    return ret;
}

bool DeviceMngApi::write_data(uint32_t AppId, double value)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;
    bool    ret;

    m_pAppDevMng->changeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    if (!m_pAppDevMng->m_pMngApp->findDriver(DriverId, &pdriver))
    {
        zprintf1("LibDeviceMng FindDriver fail!\n");
        return false;
    }
    ret = pdriver->m_pShm->shm_write(ParentDeviceId, ChildDeviceId, PointId, type, value);
    return ret;
}

bool DeviceMngApi::read_ctrl_used(uint32_t AppId, int* value)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;

    m_pAppDevMng->changeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    if (!m_pAppDevMng->m_pMngApp->findDriver(DriverId, &pdriver))
    {
        zprintf1("LibDeviceMng FindDriver fail!\n");
        return false;
    }
    if (pdriver->m_pShm->shm_read_used(ParentDeviceId, ChildDeviceId, PointId, type, value))
        return true;
    else
    {
        zprintf1("LibDeviceMng read_ctrl_used shm_read_used fail!\n");
        return false;
    }

}

bool DeviceMngApi::ctrl_data(uint32_t AppId, double value)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;
    bool    ret;
    zprintf1("ctrl data appid 0x%x!\n", AppId);
    m_pAppDevMng->changeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);
    if (!m_pAppDevMng->m_pMngApp->findDriver(DriverId, &pdriver))
    {
        zprintf1("LibDeviceMng FindDriver fail!\n");
        return false;
    }
    zprintf1("ctrl data driver %d point %d.%d.%d type %d!\n", DriverId, ParentDeviceId, ChildDeviceId, PointId, type);
    pdriver->m_pShm->shm_write_used(ParentDeviceId, ChildDeviceId, PointId, type, 1);
    ret = pdriver->m_pShm->shm_ctrl(DriverId, ParentDeviceId, ChildDeviceId, PointId, value);
    return ret;
}

bool DeviceMngApi::ctrl_data_block(uint32_t AppId, double value, uint32_t overtime_10ms)
{
    uint8_t DriverId, ParentDeviceId, ChildDeviceId, PointId, type;
    driver* pdriver;
    bool    ret;
    int     used, ticks, WaitTicks;

    m_pAppDevMng->changeAppid(AppId, &DriverId, &ParentDeviceId, &ChildDeviceId, &PointId, &type);

    if (!m_pAppDevMng->m_pMngApp->findDriver(DriverId, &pdriver))
    {
        zprintf1("LibDeviceMng func %s FindDriver fail!\n", __func__);
        return false;
    }


    GET_SYS_TIME_MS(ticks);
    WaitTicks = ticks + overtime_10ms * 10;

    while (ticks <= WaitTicks)
    {
        if (pdriver->m_pShm->shm_read_used(ParentDeviceId, ChildDeviceId, PointId, type, &used))
        {
            if (used == 0)
            {
                ret = pdriver->m_pShm->shm_ctrl(DriverId, ParentDeviceId, ChildDeviceId, PointId, value);
                if (ret)
                    pdriver->m_pShm->shm_write_used(ParentDeviceId, ChildDeviceId, PointId, type, 1);

                return ret;
            }
            USLEEP(10);
            GET_SYS_TIME_MS(ticks);

        }
        else
        {
            zprintf1("LibDeviceMng ctrl_data shm_read_used fail func %s !\n", __func__);
            return false;
        }
    }
    zprintf1("LibDeviceMng ctrl_data wait shm_read_used over time!\n");
    return false;
}

bool DeviceMngApi::get_param(uint32_t AppId, backcallfunc func)
{
    bool         ret;
    Type_MsgAddr addr;

    addr.app = AppId;
    ret      = m_pAppDevMng->m_pMngApp->msgSendProcess(addr, MSG_TYPE_AppGetIOParam, (ackfunctype) func, NULL, 0);
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
    ret = m_pAppDevMng->m_pMngApp->msgSendProcess(addr, MSG_TYPE_AppSetIOParam, (ackfunctype) func, data, len + 1);
    return ret;
}

bool DeviceMngApi::get_deviceinfo(uint8_t DriverId, backcallfunc func)
{
    bool         ret;
    Type_MsgAddr addr;

    addr.app              = 0;
    addr.driver.id_driver = DriverId;
    ret = m_pAppDevMng->m_pMngApp->msgSendProcess(addr, MSG_TYPE_DriverGetInfo, (ackfunctype) func, NULL, 0);
    return ret;
}

bool DeviceMngApi::wait_msg(sMsgUnit* recvmsg, uint16_t* msglen, eWaitMsgType mode)
{
    return m_pAppDevMng->m_pMngApp->wait_msg(recvmsg, msglen, mode);
}

bool DeviceMngApi::read_state(uint8_t DriverId, int childid, char* value, uint16_t len)
{
    driver* pdriver;
    bool    ret;

           // DriverId大于10表示是中继的，对于中继的共享内存统一在主控cs1共享内存下延伸。
    if (DriverId > 10)
    {
        DriverId = 1;
    }

    if (!m_pAppDevMng->m_pMngApp->findDriver(DriverId, &pdriver))
    {
        zprintf1("LibDeviceMng FindDriver fail!\n");
        return false;
    }
    ret = pdriver->m_pShm->shm_readstate(childid, value, len);
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
