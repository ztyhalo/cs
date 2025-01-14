#include "MsgMng.h"
#include "libdefdebuglog.h"
#include <sys/types.h>
#include <signal.h>
#include <cerrno>
MsgMng::MsgMng()
{
    cancel = false;
    pthread_mutex_init(&RevTaskMutex, NULL);
    pthread_mutex_init(&WaitListMutex, NULL);
}

MsgMng::~MsgMng()
{
    mAppTable::iterator item;

    sysLogQD() << "DeviceMng **************~MsgMng begin";
    cancel = true;
    usleep(30000);
    DELETE(pinitsem);
    sysLogQD() << "DeviceMng **************DELETE(pinitsem);";
    DELETE(pAppMsg);
    sysLogQD() << "DeviceMng **************DELETE(pAppMsg)";
    DELETE(pDriverMsg);
    sysLogQD() << "DeviceMng **************DELETE(pDriverMsg)";
    DELETE(pAppTotalMsg);
    sysLogQD() << "DeviceMng **************DELETE(pAppTotalMsg)";
    DELETE(pResMsg);
    sysLogQD() << "DeviceMng **************DELETE(pResMsg)";

    pthread_cancel(DriverMsg_id);
    pthread_cancel(AppMsg_id);

    for (item = AppTable.begin(); item != AppTable.end(); ++item)
    {
        sysLogQD() << "DeviceMng **************delete item.value()";
        // item.value()->pmsg->delete_object();
        delete item.value();
        item.value() = (app*) 0;
    }
    AppTable.clear();
    WaitDriverList.clear();
    sysLogQD() << "DeviceMng **************~MsgMng end";
}

MsgMng* MsgMng::pMsgCmd = NULL;
MsgMng* MsgMng::GetMsgMng()
{
    if (pMsgCmd == NULL)
    {
        pMsgCmd = new MsgMng();
    }
    return pMsgCmd;
}

void* DriverMsg_task(void*)
{
    MsgMng* pMsgMng = MsgMng::GetMsgMng();

    while (1)
    {
        if (pMsgMng->cancel == true)
        {
            sysLogQD() << "DeviceMng DriverMsg_task cancel !";
            break;
        }
        pMsgMng->DriverMsgProcess();
        // sysLogQD() << "--------------------------------DriverMsg_task";
    }

    return NULL;
}

void* AppMsg_task(void*)
{
    MsgMng* pMsgMng = MsgMng::GetMsgMng();

    while (1)
    {
        if (pMsgMng->cancel == true)
        {
            sysLogQD() << "DeviceMng AppMsg_task cancel !";
            break;
        }
        pMsgMng->AppMsgProcess();
        // sysLogQD() << "--------------------------------AppMsg_task";
    }

    return NULL;
}

bool MsgMng::Init(int appkey, int driverkey, int totalkey, int initsemkey, int reskey)
{
    pResMsg      = new msg(reskey);
    pAppMsg      = new msg(appkey);
    pDriverMsg   = new msg(driverkey);
    pAppTotalMsg = new msg(totalkey);
    WaitDriverList.clear();
    AppTable.clear();

    if (!pResMsg->create_object())
    {
        sysLogQE() << "DeviceMng pResMsg create_object error";
        return false;
    }
    if (!pDriverMsg->create_object())
    {
        sysLogQE() << "DeviceMng pDriverMsg create_object error";
        return false;
    }
    if (!pAppMsg->create_object())
    {
        sysLogQE() << "DeviceMng pAppMsg create_object error";
        return false;
    }
    if (!pAppTotalMsg->create_object())
    {
        sysLogQE() << "pAppTotalMsg create_object error";
        return false;
    }

    pinitsem = new SemObject();
    if (!pinitsem->create_sem(initsemkey, 1, 1))
    {
        sysLogQE() << "DeviceMng pinitsem create_sem initsemkey:" << initsemkey;
        return false;
    }
    sysLogQD() << "DeviceMng msgmng init finish!";
    pthread_create(&DriverMsg_id, NULL, DriverMsg_task, NULL);
    pthread_create(&AppMsg_id, NULL, AppMsg_task, NULL);
    return true;
}

bool MsgMng::InsertWaitMsg(Type_MsgAddr& waitid, uint16_t type, sem_t* pack)
{
    sWaitMsg data;

    //    pthread_mutex_lock(&WaitListMutex);
    data.waitid = waitid;
    data.type   = type;
    data.pack   = pack;
    if (WaitDriverList.size() < WAIT_MSG_MAX)
    {
        WaitDriverList.append(data);
        //        pthread_mutex_unlock(&WaitListMutex);
        return true;
    }
    //    pthread_mutex_unlock(&WaitListMutex);
    return false;
}

bool MsgMng::CheckWaitMsg(Type_MsgAddr waitid, uint16_t type)
{
    lWaitList::iterator item;

    pthread_mutex_lock(&WaitListMutex);

    for (item = WaitDriverList.begin(); item != WaitDriverList.end(); ++item)
    {
        if (((*item).type == type) && ((*item).waitid.app == waitid.app))
        {
            pthread_mutex_unlock(&WaitListMutex);
            return true;
        }
    }
    pthread_mutex_unlock(&WaitListMutex);
    return false;
}

bool MsgMng::AckWaitMsg(Type_MsgAddr waitid, uint16_t type)
{
    lWaitList::iterator item;

    pthread_mutex_lock(&WaitListMutex);

    for (item = WaitDriverList.begin(); item != WaitDriverList.end(); ++item)
    {
        if (((*item).type == type) && ((*item).waitid.app == waitid.app))
        {
            sem_post((*item).pack);
            WaitDriverList.erase(item);
            pthread_mutex_unlock(&WaitListMutex);
            return true;
        }
    }
    pthread_mutex_unlock(&WaitListMutex);
    return false;
}

void MsgMng::AppMsgProcess(void)
{
    sMsgUnit pkt;
    uint16_t pkt_len;
    app*     papp;
    driver*  pdriver;
    int      key;

    if (!pAppMsg->ReceiveMsg(&pkt, &pkt_len, RECV_NOWAIT))
    {
        usleep(10000);
        return;
    }

    DeviceMng* pdevice = DeviceMng::GetDeviceMng();
    if (!pdevice->InitFinishFlag)
        return;

    pthread_mutex_lock(&RevTaskMutex);
    switch (pkt.type)
    {
        case MSG_TYPE_AppLogIn:
            sysLogQD() << "DeviceMng MSG_TYPE_AppLogIn enter!";
            if (pkt.source.app == GET_DEVMNG_ID)
            {
                sysLogQD() << "DeviceMng pkt.source.app == GET_DEVMNG_ID!";
                break;
            }
            if (IsAppMapExist(pkt.source.app))
            {
                pkt.dest.app   = pkt.source.app;
                pkt.source.app = BROADCAST_ID;
                pkt.data[0]    = MSG_ERROR_LogIn_Exist;
                sysLogQE() << "DeviceMng app login error:exist!";
                if (FindApp(pkt.dest.app, &papp))
                {
                    papp->pmsg->SendMsg(&pkt, ABNORMAL_MSG_LEN);
                }
                break;
            }
            if (AppTable.size() < APP_LOGIN_MAX)
            {
                DeviceMng* pdevice = DeviceMng::GetDeviceMng();
                key                = pdevice->OperateAppMsgKey(KEY_ADD, 0);
                if (key == 0)
                {
                    pkt.dest.app   = pkt.source.app;
                    pkt.source.app = BROADCAST_ID;
                    pkt.data[0]    = MSG_ERROR_LogIn_NOAPPID;
                    sysLogQE() << "DeviceMng app login error:no id!";
                    pAppTotalMsg->SendMsg(&pkt, ABNORMAL_MSG_LEN);
                    break;
                }

                bool isRecv = (bool) pkt.data[0];
                sysLogQD() << "isRecv:" << isRecv;

                app* apphandle = new app(pkt.source.app, key, isRecv);
                // 先清除之前废除的app
                for (auto iter = AppTable.begin(); iter != AppTable.end();)
                {
                    if (!isProcessExists(iter.key()))
                    {
                        iter = AppTable.erase(iter);
                    }
                    else
                    {
                        ++iter;
                    }
                }
                AppTable.insert(pkt.source.app, apphandle);

                for (auto iter : AppTable.keys())
                {
                    sysLogQD() << "AppTable key:" << iter;
                }

                pkt.dest.app   = pkt.source.app;
                pkt.source.app = BROADCAST_ID;
                pkt.data[0]    = MSG_ERROR_NoError;
                pkt.data[1]    = (uint8_t) ((key & 0xff000000) >> 24);
                pkt.data[2]    = (uint8_t) ((key & 0x00ff0000) >> 16);
                pkt.data[3]    = (uint8_t) ((key & 0x0000ff00) >> 8);
                pkt.data[4]    = (uint8_t) (key & 0x000000ff);
                pkt.data[5]    = (uint8_t) ((GET_DEVMNG_ID & 0xff000000) >> 24);
                pkt.data[6]    = (uint8_t) ((GET_DEVMNG_ID & 0x00ff0000) >> 16);
                pkt.data[7]    = (uint8_t) ((GET_DEVMNG_ID & 0x0000ff00) >> 8);
                pkt.data[8]    = (uint8_t) (GET_DEVMNG_ID & 0x000000ff);
                pAppTotalMsg->SendMsg(&pkt, NORMAL_MSG_LEN);
                sysLogQE() << "DeviceMng app login sucess!";
                break;
            }
            else
                sysLogQE() << "DeviceMng app login appid full!";
            break;

        case MSG_TYPE_AppLogOut:
            sysLogQD() << "DeviceMng app logout enter!";
            if (pkt.source.app == GET_DEVMNG_ID)
                break;
            if (FindApp(pkt.source.app, &papp))
            {
                DeviceMng* pdevice = DeviceMng::GetDeviceMng();
                pdevice->OperateAppMsgKey(KEY_SUB, papp->pmsg->GetMsgKey());
                delete papp;
                DeleteApp(pkt.source.app);
                sysLogQE() << "DeviceMng app logout sucess!";
            }
            break;

        default:
            sysLogQD() << "DeviceMng default enter!";
            DeviceMng* pdevice = DeviceMng::GetDeviceMng();
            if (!FindApp(pkt.source.app, &papp))
                break;

            if (!pdevice->FindDriver(pkt.dest.driver.id_driver, &pdriver))
            {
                pkt.dest.app   = pkt.source.app;
                pkt.source.app = GET_DEVMNG_ID;
                pkt.data[0]    = MSG_ERROR_Driver_NotExist;
                papp->pmsg->SendMsg(&pkt, ABNORMAL_MSG_LEN);
            }
            else
            {
                pdriver->pmsg->SendMsg(&pkt, pkt_len);
            }
            break;
    }
    pthread_mutex_unlock(&RevTaskMutex);
}

bool MsgMng::IsAppMapExist(uint32_t id)
{
    mAppTable::iterator item;

    item = AppTable.find(id);
    if ((item != AppTable.end()) && (item.key() == id))
    {
        return true;
    }
    return false;
}

bool MsgMng::FindApp(uint32_t id, app** ppapp)
{
    mAppTable::iterator item;

    item = AppTable.find(id);
    if ((item != AppTable.end()) && (item.key() == id))
    {
        *ppapp = item.value();
        return true;
    }
    *ppapp = (app*) 0;
    return false;
}

bool MsgMng::DeleteApp(uint32_t id)
{
    mAppTable::iterator item;

    item = AppTable.find(id);
    if ((item != AppTable.end()) && (item.key() == id))
    {
        AppTable.erase(item);
        return true;
    }
    return false;
}

void MsgMng::DriverMsgProcess(void)
{
    sMsgUnit pkt;
    uint16_t pkt_len;
    app*     papp;
    driver*  pdriver;

    if (!pDriverMsg->ReceiveMsg(&pkt, &pkt_len, RECV_NOWAIT))
    {
        usleep(10000);
        return;
    }

    pthread_mutex_lock(&RevTaskMutex);

    DeviceMng* pdevice = DeviceMng::GetDeviceMng();
    if (pkt.dest.app == GET_DEVMNG_ID)
    {
        switch (pkt.type)
        {
            case MSG_TYPE_DriverGetInfo:
                if (!CheckWaitMsg(pkt.source, pkt.type))
                    break;

                if (pdevice->FindDriver(pkt.source.driver.id_driver, &pdriver))
                {
                    pdriver->DriverInfo.TotalInCnt    = (uint16_t) (((uint16_t) pkt.data[0] << 8) | pkt.data[1]);
                    pdriver->DriverInfo.TotalOutCnt   = (uint16_t) (((uint16_t) pkt.data[2] << 8) | pkt.data[3]);
                    pdriver->DriverInfo.TotalStateCnt = (uint16_t) (((uint16_t) pkt.data[4] << 8) | pkt.data[5]);
                    // memcpy(&pdriver->DriverInfo,&pkt.data[0],sizeof(sDriverInfoType));
                    // qDebug() << "$$$DeviceMng MSG_TYPE_DriverGetInfo: " << pdriver->DriverInfo.TotalInCnt
                    //         << pdriver->DriverInfo.TotalOutCnt << pdriver->DriverInfo.TotalStateCnt;
                    AckWaitMsg(pkt.source, pkt.type);
                }
                break;

            case MSG_TYPE_DriverSendHeart:
                if (!CheckWaitMsg(pkt.source, pkt.type))
                    break;

                if (pdevice->FindDriver(pkt.source.driver.id_driver, &pdriver))
                {
                    AckWaitMsg(pkt.source, pkt.type);
                }
                break;
        }
    }
    else if (pkt.dest.app == BROADCAST_ID)
    {
        sysLogQD() << "+++++++++++++++++++++++++++pkt.dest.app == BROADCAST_ID";
        mAppTable::iterator item;

        for (item = AppTable.begin(); item != AppTable.end(); ++item)
        {
            pkt.dest.app = item.key();
            item.value()->pmsg->SendMsg(&pkt, pkt_len);
        }
    }
    else
    {
        sysLogQD() << "+++++++++++++++++++++++++++FindApp(pkt.dest.app, &papp";
        sysLogQD() << "+++++++++++++++++++++++++++dest.app:" << pkt.dest.app;
        for (auto iter : AppTable.keys())
        {
            sysLogQD() << "+++++++++++++++++++++++++++AppTable key:" << iter;
        }

        mAppTable::iterator item;
        for (item = AppTable.begin(); item != AppTable.end(); ++item)
        {
            if (item.value()->isRecv)
            {
                pkt.dest.app = item.key();
                item.value()->pmsg->SendMsg(&pkt, pkt_len);
            }
        }

        //        if (FindApp(pkt.dest.app, &papp))
        //        {
        //            papp->pmsg->SendMsg(&pkt, pkt_len);
        //        }
    }

    pthread_mutex_unlock(&RevTaskMutex);
}

bool MsgMng::BroadcastToApp(uint16_t type, uint8_t* data, uint16_t len)
{
    sMsgUnit            pkt;
    mAppTable::iterator item;

    if ((len > MSG_UNIT_LENGTH) || (data == (uint8_t*) 0))
        return false;

    pthread_mutex_lock(&RevTaskMutex);
    pkt.source.app = GET_DEVMNG_ID;
    pkt.type       = type;
    memcpy(&pkt.data[0], data, len);
    for (item = AppTable.begin(); item != AppTable.end(); ++item)
    {
        pkt.dest.app = item.key();
        item.value()->pmsg->SendMsg(&pkt, len);
    }
    pthread_mutex_unlock(&RevTaskMutex);
    return true;
}

bool MsgMng::isProcessExists(qint64 pid)
{
    if (kill(pid, 0) == -1)
    {
        if (errno == ESRCH)
        {
            return false;
        }
    }
    return true;
}
