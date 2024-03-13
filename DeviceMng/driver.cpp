#include "driver.h"
#include "MsgMng.h"
#include "libdefdebuglog.h"
driver::driver(int id, QString& name, int shminkey, int shmoutkey, int shmoutsem, int msgkey, int shmstatekey)
{
    driver_id   = id;
    driver_name = name;
    ComState    = COMSTATE_NORMAL;

    pshm = new shm(shminkey, shmoutkey, shmoutsem, shmstatekey);
    pmsg = new msg(msgkey);
    sem_init(&AckSem, 0, 0);
}

driver::~driver()
{
    sysLogQD() << "DeviceMng driver exit start! ";
    DELETE(pshm);
    DELETE(pmsg);
    sysLogQD() << "DeviceMng driver exit finish! ";
}

bool driver::InitMsg(void)
{
    if (!pmsg->create_object())
    {
        sysLogQE() << "DeviceMng driver init :create_object error";
        return false;
    }
    return true;
}

bool driver::Init(void)
{
    if (!Msg_GetInfo())
    {
        sysLogQE() << "DeviceMng driver init :Msg_GetInfo!";
        return false;
    }

    sysLogQD() << "DeviceMng driver init :shm_create:" << DriverInfo.TotalInCnt + DriverInfo.TotalOutCnt
             << "    ,shm_state:" << DriverInfo.TotalStateCnt << " !";

    if (!pshm->shm_create(DriverInfo.TotalInCnt + DriverInfo.TotalOutCnt, DriverInfo.TotalStateCnt))
    {
        sysLogQE() << "DeviceMng driver init fail:shm_create:" << DriverInfo.TotalInCnt + DriverInfo.TotalOutCnt
                 << "    ,shm_state:" << DriverInfo.TotalStateCnt << " !";
        return false;
    }
    return true;
}

bool driver::WaitSem(int time_10ms)
{
    int ret;

    for (int i = 0; i < time_10ms; i++)
    {
        usleep(10000);
        ret = sem_trywait(&AckSem);
        if (ret >= 0)
            return true;
    }
    return false;
}

bool driver::Msg_GetInfo(void)
{
    sMsgUnit pkt;
    MsgMng*  pMsgMng = MsgMng::GetMsgMng();

    memset(&pkt, 0, sizeof(sMsgUnit));
    pkt.source.app            = GET_DEVMNG_ID;
    pkt.dest.driver.id_driver = driver_id;
    pkt.type                  = MSG_TYPE_DriverGetInfo;

    pthread_mutex_lock(&pMsgMng->WaitListMutex);
    if (pmsg->SendMsg(&pkt, 0) == false)
    {
        sysLogQE() << "DeviceMng pmsg->SendMsg(&pkt,0) == false";
        pthread_mutex_unlock(&pMsgMng->WaitListMutex);
        return false;
    }

    if (!pMsgMng->InsertWaitMsg(pkt.dest, pkt.type, &AckSem))
    {
        sysLogQE() << "DeviceMng !pMsgMng->InsertWaitMsg(pkt.dest,pkt.type,&AckSem)";
        pthread_mutex_unlock(&pMsgMng->WaitListMutex);
        return false;
    }
    pthread_mutex_unlock(&pMsgMng->WaitListMutex);

    if (!WaitSem(500))
    {
        sysLogQE() << "DeviceMng !WaitSem(500)";
        return false;
    }
    return true;
}

bool driver::Msg_SendHeart(void)
{
    sMsgUnit pkt;
    MsgMng*  pMsgMng = MsgMng::GetMsgMng();

    memset(&pkt, 0, sizeof(sMsgUnit));
    pkt.source.app            = GET_DEVMNG_ID;
    pkt.dest.driver.id_driver = driver_id;
    pkt.type                  = MSG_TYPE_DriverSendHeart;

    pthread_mutex_lock(&pMsgMng->WaitListMutex);
    if (pmsg->SendMsg(&pkt, 0) == false)
    {
        pthread_mutex_unlock(&pMsgMng->WaitListMutex);
        return false;
    }

    if (!pMsgMng->InsertWaitMsg(pkt.dest, pkt.type, &AckSem))
    {
        pthread_mutex_unlock(&pMsgMng->WaitListMutex);
        return false;
    }
    pthread_mutex_unlock(&pMsgMng->WaitListMutex);
    return true;
}
