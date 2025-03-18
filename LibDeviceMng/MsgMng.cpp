#include "MsgMng.h"
#include <QDateTime>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "libdefdebuglog.h"
#include "libcommon.h"
MsgMng::MsgMng()
{
    DeviceMngId      = BROADCAST_ID;
    LoginSuccessFlag = 0;
    RecvMsg_id       = 0;
    TotalMsg_id      = 0;
    cancel           = false;
    pRecvMsg         = NULL;
    pSendMsg         = NULL;
    pTotalMsg        = NULL;
    pInitSem         = NULL;
    freshTimer       = NULL;
    pthread_mutex_init(&WaitListMutex, NULL);
    pthread_mutex_init(&NotifyListMutex, NULL);

    for (int i = 0; i < ACKSEM_MAX; i++)
        sem_init(&AckSem[i], 0, 0);
    sem_init(&NotifySem, 0, 0);
    sem_init(&TotalProcessSem, 0, 0);
    freshTimer = new QTimer();
    connect(freshTimer, SIGNAL(timeout()), this, SLOT(freshTimeoutData()));
}

MsgMng::~MsgMng()
{
    sysLogQD() << "LibDeviceMng  exit start !";
    sysLogQD() << "LibDeviceMng  RecvMsg cancel = true !";
    Type_MsgAddr addr;
    MsgSendProcess(addr, MSG_TYPE_AppLogOut, NULL, NULL, 0);
    cancel = true;
    if (RecvMsg_id)
    {
        pthread_cancel(RecvMsg_id);
        pthread_join(RecvMsg_id, NULL);
    }
    if (TotalMsg_id)
    {
        pthread_cancel(TotalMsg_id);
        pthread_join(TotalMsg_id, NULL);
    }

    sysLogQD() << "LibDeviceMng task exit ok !";
    DELETE(pRecvMsg);
    DELETE(pSendMsg);
    DELETE(pTotalMsg);
    WaitRecvList.clear();
    DELETE(pInitSem);
    DELETE(freshTimer);
    NotifyList.clear();
    sysLogQD() << "LibDeviceMng exit finish !";
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

void* RecvMsg_task(void*)
{
    MsgMng* pMsgMng = MsgMng::GetMsgMng();

    while (1)
    {
        if (pMsgMng->cancel == true)
        {
            sysLogQD() << "LibDeviceMng RecvMsg cancel !";
            break;
        }

        pMsgMng->RecvMsgProcess();
    }
    sysLogQD() << "LibDeviceMng RecvMsg_task exit!";
    return NULL;
}

void* TotalMsg_task(void*)
{
    MsgMng* pMsgMng = MsgMng::GetMsgMng();

    while (1)
    {
        DeviceMng* pdevice = DeviceMng::GetDeviceMng();
        if (pdevice->InitFinishFlag)
            break;
        pMsgMng->TotalMsgProcess();
    }
    sysLogQD() << "LibDeviceMng TotalMsg_task exit!";
    return NULL;
}

bool MsgMng::InitSendMail(int sendkey, int totalkey, int totalmutexkey)
{
    pSendMsg  = new msg(sendkey);
    pTotalMsg = new msg(totalkey);
    pInitSem  = new SemObject();

    WaitRecvList.clear();

    if (!pInitSem->create_sem(totalmutexkey))
    {
        sysLogQE() << "LibDeviceMng TotalMsg_task exit!";
        return false;
    }
    if (!pSendMsg->create_object())
    {
        sysLogQE() << "LibDeviceMng TotalMsg_task exit!";
        return false;
    }
    if (!pTotalMsg->create_object())
    {
        sysLogQE() << "LibDeviceMng TotalMsg_task exit!";
        return false;
    }
    pthread_create(&TotalMsg_id, NULL, TotalMsg_task, NULL);
    freshTimer->start(MSG_CHECK_TIME);
    return true;
}

void MsgMng::timeraddMS(struct timeval* a, uint32_t ms)
{
    a->tv_usec += ms * 1000;
    if (a->tv_usec >= 1000000)
    {
        a->tv_sec += a->tv_usec / 1000000;
        a->tv_usec %= 1000000;
    }
}

bool MsgMng::WaitTimeMsgAck(uint32_t waittime_ms, sWaitMsg* pmsg)
{
    struct timeval  now;
    struct timespec outtime;
    int             ret;

    if (waittime_ms > 0)
    {
        //        qDebug()<<"WaitTimeMsgAck waittime_ms:"<<waittime_ms<<" -- "<<QDateTime::currentMSecsSinceEpoch();
        gettimeofday(&now, NULL);

        //        QString timestr1;
        //        timestr1.sprintf("now:%ld.%ld, ", now.tv_sec, now.tv_usec);
        timeraddMS(&now, waittime_ms);
        outtime.tv_sec  = now.tv_sec;
        outtime.tv_nsec = now.tv_usec * 1000;
        int count       = 0;
        while (count < 5)
        {
            ret = sem_timedwait(pmsg->pack, &outtime);
            if (ret == -1)
            {
                sysLogQD() << "LibDeviceMng WaitTimeMsgAck errno:" << errno << ",count:" << count;
            }
            count++;
            if (ret == -1 && errno == EINTR) // if sem_timedwait returns -1 and errno is EINTR, recall sem_timedwait
            {
                continue;
            }
            else
                break;
        }
        //        QString timestr2;
        //        timestr2.sprintf("add:%ld.%ld, outtime:%ld.%ld", now.tv_sec, now.tv_usec, outtime.tv_sec,
        //        outtime.tv_nsec); qDebug()<<"WaitTimeMsgAck timestr:"<<timestr1<<timestr2; struct timeval afterwait;
        //        gettimeofday(&afterwait, NULL);
        //        QString timestr3;
        //        timestr3.sprintf("afterwait:%ld.%ld", afterwait.tv_sec, afterwait.tv_usec);
        //        qDebug()<<"WaitTimeMsgAck timestr3:"<<timestr3;
        if (ret == -1)
        {
            //            qDebug()<<"WaitTimeMsgAck timeout! -- "<<QDateTime::currentMSecsSinceEpoch();
            AckWaitMsg(pmsg->waitid, pmsg->type, 1);
            return false;
        }
        //        else
        //            qDebug()<<"WaitTimeMsgAck wait success! -- "<<QDateTime::currentMSecsSinceEpoch();
    }
    else
    {
        ret = sem_wait(pmsg->pack);
        if (ret == -1)
        {
            sysLogQE() << "LibDeviceMng WaitTimeMsgAck recv error!";
            AckWaitMsg(pmsg->waitid, pmsg->type, 1);
            return false;
        }
    }

    return true;
}

bool MsgMng::LoginRecvMail(uint32_t waittime_ms)
{
    sMsgUnit pkt;
    sWaitMsg msg;

    sysLogQD() << "LibDeviceMng LoginRecvMail pend!" << GET_APP_ID;
    LoginSuccessFlag = 0;
    pInitSem->sem_p();

    sem_post(&TotalProcessSem);
    pkt.dest.app   = DeviceMngId;
    pkt.source.app = GET_APP_ID;
    pkt.type       = MSG_TYPE_AppLogIn;
    pkt.data[0]    = IsRecv;
    sysLogQE() << "LibDeviceMng IsRecv:" << IsRecv;
    LoginKeyId = 0;

    pthread_mutex_lock(&WaitListMutex);
    if (!pSendMsg->SendMsg(&pkt, LOGIN_MSG_LEN))
    {
        pthread_mutex_unlock(&WaitListMutex);
        sysLogQE() << "LibDeviceMng LoginRecvMail post!" << GET_APP_ID << "SendMsg fail!";
        pInitSem->sem_v();
        return false;
    }

    msg.waitid     = pkt.dest;
    msg.type       = pkt.type;
    msg.pack       = &AckSem[ACKSEM_Login];
    msg.timeout_ms = waittime_ms + 1000;
    msg.ackfunc    = 0;
    sysLogQD() << "LibDeviceMng InsertWaitMsg --1";
    if (!InsertWaitMsg(&msg))
    {
        sysLogQE() << "LibDeviceMng Send finish wait ack!";
        pthread_mutex_unlock(&WaitListMutex);
        return false;
    }
    pthread_mutex_unlock(&WaitListMutex);

    if (!WaitTimeMsgAck(waittime_ms, &msg))
    {
        sysLogQE() << "LibDeviceMng LoginRecvMail post!" << GET_APP_ID << "waittime msg ack!";
        pInitSem->sem_v();
        return false;
    }
    if ((LoginKeyId == 0) || (LoginSuccessFlag == 0))
    {
        sysLogQE() << "LibDeviceMng LoginRecvMail post!" << GET_APP_ID << "login key fail!";
        pInitSem->sem_v();
        return false;
    }

    qDebug() << "LoginRecvMail post!" << GET_APP_ID << "success!";
    pInitSem->sem_v();
    return true;
}

bool MsgMng::InitRecvMail(void)
{
    pRecvMsg = new msg(LoginKeyId);
    NotifyList.clear();

    if (!pRecvMsg->create_object())
        return false;

    pthread_create(&RecvMsg_id, NULL, RecvMsg_task, NULL);
    return true;
}

bool MsgMng::InsertWaitMsg(sWaitMsg* waitmsg)
{
    sWaitMsg data;

    //    pthread_mutex_lock(&WaitListMutex);
    memcpy(&data, waitmsg, sizeof(sWaitMsg));
    data.timeout_ms = (data.timeout_ms == 0) ? MSG_TIMEOUT_VALUE : data.timeout_ms;
    if (WaitRecvList.size() < WAIT_MSG_MAX)
    {
        WaitRecvList.append(data);
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

    for (item = WaitRecvList.begin(); item != WaitRecvList.end(); ++item)
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

ackfunctype MsgMng::GetWaitFunc(Type_MsgAddr waitid, uint16_t type)
{
    lWaitList::iterator item;

    pthread_mutex_lock(&WaitListMutex);

    for (item = WaitRecvList.begin(); item != WaitRecvList.end(); ++item)
    {
        if (((*item).type == type) && ((*item).waitid.app == waitid.app))
        {
            pthread_mutex_unlock(&WaitListMutex);
            return (*item).ackfunc;
        }
    }
    pthread_mutex_unlock(&WaitListMutex);
    return 0;
}

bool MsgMng::AckWaitMsg(Type_MsgAddr waitid, uint16_t type, uint8_t mode)
{
    lWaitList::iterator item;

    pthread_mutex_lock(&WaitListMutex);

    for (item = WaitRecvList.begin(); item != WaitRecvList.end(); ++item)
    {
        if (((*item).type == type) && ((*item).waitid.app == waitid.app))
        {
            if (!mode)
                sem_post((*item).pack);
            WaitRecvList.erase(item);
            //            qDebug()<<"DeviceMngLib WaitRecvList sucess:"<<waitid.app<<" --
            //            "<<QDateTime::currentMSecsSinceEpoch();
            pthread_mutex_unlock(&WaitListMutex);
            return true;
        }
    }
    pthread_mutex_unlock(&WaitListMutex);
    return false;
}

bool MsgMng::SendMail(sMsgUnit& pkt, uint16_t pkt_len, ackfunctype func, uint32_t timeout)
{
    sWaitMsg msg;

    pthread_mutex_lock(&WaitListMutex);
    if (!pSendMsg->SendMsg(&pkt, pkt_len))
    {
        pthread_mutex_unlock(&WaitListMutex);
        return false;
    }

    msg.waitid     = pkt.dest;
    msg.type       = pkt.type;
    msg.pack       = 0;
    msg.timeout_ms = (timeout == 0) ? MSG_TIMEOUT_VALUE : timeout;
    msg.ackfunc    = func;
    if (!InsertWaitMsg(&msg))
    {
        pthread_mutex_unlock(&WaitListMutex);
        return false;
    }
    pthread_mutex_unlock(&WaitListMutex);
    return true;
}

bool MsgMng::InitGetInfo(int driver_id, uint32_t timeout_ms)
{
    sMsgUnit pkt;
    sWaitMsg msg;

    memset(&pkt, 0, sizeof(sMsgUnit));
    pkt.source.app            = GET_APP_ID;
    pkt.dest.driver.id_driver = driver_id;
    pkt.type                  = MSG_TYPE_DriverGetInfo;

    pthread_mutex_lock(&WaitListMutex);
    if (!pSendMsg->SendMsg(&pkt, 0))
    {
        pthread_mutex_unlock(&WaitListMutex);
        return false;
    }

    msg.waitid     = pkt.dest;
    msg.type       = pkt.type;
    msg.pack       = &AckSem[ACKSEM_Info];
    msg.timeout_ms = timeout_ms + 1000;
    msg.ackfunc    = 0;
    if (!InsertWaitMsg(&msg))
    {
        pthread_mutex_unlock(&WaitListMutex);
        return false;
    }
    pthread_mutex_unlock(&WaitListMutex);

    if (!WaitTimeMsgAck(timeout_ms, &msg))
        return false;
    return true;
}

void MsgMng::CheckTimeoutMsg(uint16_t intervaltime)
{
    lWaitList::iterator item;

    pthread_mutex_lock(&WaitListMutex);

    for (item = WaitRecvList.begin(); item != WaitRecvList.end(); ++item)
    {
        if ((*item).timeout_ms <= intervaltime)
        {
            WaitRecvList.erase(item);
        }
        else
        {
            (*item).timeout_ms = (*item).timeout_ms - intervaltime;
        }
    }
    pthread_mutex_unlock(&WaitListMutex);
}

void MsgMng::freshTimeoutData(void)
{
    DeviceMng* pdevice = DeviceMng::GetDeviceMng();
    if (!pdevice->InitFinishFlag)
        return;
    CheckTimeoutMsg(MSG_CHECK_TIME);
}

bool MsgMng::MsgSendProcess(Type_MsgAddr& addr, uint16_t msgtype, ackfunctype func, uint8_t* pdata, uint16_t len)
{
    sMsgUnit pkt;
    bool     ret = true;

    memset(&pkt, 0, sizeof(sMsgUnit));
    switch (msgtype)
    {
        case MSG_TYPE_AppLogOut:
            pkt.source.app = GET_APP_ID;
            pkt.dest.app   = DeviceMngId;
            pkt.type       = MSG_TYPE_AppLogOut;
            SendMail(pkt, 0, func, 0);
            cancel = true;
            // pthread_join(RecvMsg_id,NULL);
            break;

        case MSG_TYPE_AppGetIOParam:
            pkt.source.app = GET_APP_ID;
            pkt.dest.app   = addr.app;
            pkt.type       = MSG_TYPE_AppGetIOParam;
            SendMail(pkt, 0, func, 0);
            break;

        case MSG_TYPE_AppSetIOParam:
            pkt.source.app = GET_APP_ID;
            pkt.dest.app   = addr.app;
            pkt.type       = MSG_TYPE_AppSetIOParam;
            memcpy(pkt.data, pdata, len);
            SendMail(pkt, len, func, 0);
            break;

        case MSG_TYPE_DriverGetInfo:
            pkt.source.app = GET_APP_ID;
            pkt.dest.app   = addr.app;
            pkt.type       = MSG_TYPE_DriverGetInfo;
            SendMail(pkt, 0, func, 0);
            break;

        default:
            break;
    }
    return ret;
}

void MsgMng::TotalMsgProcess(void)
{
    sMsgUnit pkt;
    uint16_t pkt_len;

    if (sem_wait(&TotalProcessSem) == -1)
    {
        USLEEP(10000);
        return;
    }

    if (!pTotalMsg->ReceiveMsg(&pkt, &pkt_len, RECV_WAIT))
    {
        USLEEP(10000);
        return;
    }

    DeviceMng* pdevice = DeviceMng::GetDeviceMng();
    if (pdevice->InitFinishFlag)
        return;

    if ((pkt.dest.app != GET_APP_ID) && (pkt.dest.app != BROADCAST_ID))
    {
        sysLogQE() << "LibDeviceMng recv addr error!";
        return;
    }

    switch (pkt.type)
    {
        case MSG_TYPE_AppLogIn:
            if (!CheckWaitMsg(pkt.source, pkt.type))
            {
                break;
            }

            if ((pkt_len == LOGIN_ACK_MSG_LEN) && (pkt.data[0] == MSG_ERROR_NoError))
            {
                DeviceMngId = ((int) pkt.data[5] << 24) | ((int) pkt.data[6] << 16) | ((int) pkt.data[7] << 8) |
                              (int) pkt.data[8];
                LoginKeyId = ((int) pkt.data[1] << 24) | ((int) pkt.data[2] << 16) | ((int) pkt.data[3] << 8) |
                             (int) pkt.data[4];
                LoginSuccessFlag = 1;
            }
            else
            {
                sysLogQE() << "LibDeviceMng TotalMsgProcess app login ack fail!";
            }
            AckWaitMsg(pkt.source, pkt.type, 0);
            break;

        default:
            break;
    }
}

void MsgMng::RecvMsgProcess(void)
{
    sMsgUnit    pkt;
    uint16_t    pkt_len;
    driver*     pdriver;
    ackfunctype func;
    sNotifyMsg  notifypkt;

    if (!pRecvMsg->ReceiveMsg(&pkt, &pkt_len, RECV_WAIT))
    {
        sysLogQE() << "LibDeviceMng Test ReceiveMsg fail!";
        USLEEP(10000);
        return;
    }

    DeviceMng* pdevice = DeviceMng::GetDeviceMng();
    if ((pkt.dest.app != GET_APP_ID) && (pkt.dest.app != BROADCAST_ID))
    {
        sysLogQE() << "LibDeviceMng Test pkt.dest.app fail,pkt.dest.app:" << pkt.dest.app;
        return;
    }

    switch (pkt.type)
    {
        case MSG_TYPE_DriverGetInfo:
        {
            sysLogQE() << "LibDeviceMng Test MSG_TYPE_DriverGetInfo";
            bool ret_checkwait = CheckWaitMsg(pkt.source, pkt.type);
            //            qDebug()<<"Test MSG_TYPE_DriverGetInfo ret_checkwait:"<<ret_checkwait;
            if (!ret_checkwait)
                break;

            if (!pdevice->InitFinishFlag)
            {
                //                qDebug()<<"Test MSG_TYPE_DriverGetInfo --1"<<" --
                //                "<<QDateTime::currentMSecsSinceEpoch();
                if (pdevice->FindDriver(pkt.source.driver.id_driver, &pdriver))
                {
                    pdriver->DriverInfo.TotalInCnt    = (uint16_t) (((uint16_t) pkt.data[0] << 8) | pkt.data[1]);
                    pdriver->DriverInfo.TotalOutCnt   = (uint16_t) (((uint16_t) pkt.data[2] << 8) | pkt.data[3]);
                    pdriver->DriverInfo.TotalStateCnt = (uint16_t) (((uint16_t) pkt.data[4] << 8) | pkt.data[5]);
                    //                    qDebug()<<" pdriver->DriverInfo.TotalStateCnt =
                    //                    "<(uint16_t)(((uint16_t)pkt.data[4]<<8)|pkt.data[5]);
                    // memcpy(&pdriver->DriverInfo,&pkt.data[0],sizeof(sDriverInfoType));
                    /*bool ret_act = */ AckWaitMsg(pkt.source, pkt.type, 0);
                    //                    qDebug()<<"Test MSG_TYPE_DriverGetInfo --2 ret_act:"<<ret_act<<" --
                    //                    "<<QDateTime::currentMSecsSinceEpoch();
                }
            }
            else
            {
                //              qDebug()<<"Test MSG_TYPE_DriverGetInfo --3";
                func = GetWaitFunc(pkt.source, pkt.type);
                //               qDebug()<<"Test MSG_TYPE_DriverGetInfo --4";
                if (func != 0)
                    func((void*) (&pkt.data[0]), pkt_len);
                //              qDebug()<<"Test MSG_TYPE_DriverGetInfo --5";
            }
        }
        break;

        case MSG_TYPE_AppReportDriverComNormal:
            sysLogQE() << "LibDeviceMng Test MSG_TYPE_AppReportDriverComNormal";
            if ((pkt.source.app == DeviceMngId) && (pkt_len == 1))
            {
                if (pdevice->FindDriver(pkt.data[0], &pdriver))
                {
                    pdriver->ComState = COMSTATE_NORMAL;
                }
            }
            break;

        case MSG_TYPE_AppGetIOParam:
        case MSG_TYPE_AppSetIOParam:
            sysLogQE() << "LibDeviceMng Test MSG_TYPE_AppGetIOParam";
            if (!CheckWaitMsg(pkt.source, pkt.type))
                break;
            func = GetWaitFunc(pkt.source, pkt.type);
            if (func != 0)
                func((void*) (&pkt.data[0]), pkt_len);
            break;

        case MSG_TYPE_AppReportDriverComAbnormal:
            sysLogQE() << "LibDeviceMng Test MSG_TYPE_AppReportDriverComAbnormal";
            if ((pkt.source.app == DeviceMngId) && (pkt_len == 1))
            {
                if (pdevice->FindDriver(pkt.data[0], &pdriver))
                {
                    pdriver->ComState = COMSTATE_ABNORMAL;
                }
            }

        default:
            if (NotifyList.size() < WAIT_MSG_MAX)
            {
                pthread_mutex_lock(&NotifyListMutex);
                memcpy(&notifypkt.MsgData, &pkt, sizeof(sMsgUnit));
                notifypkt.MsgLen = pkt_len;
                NotifyList.append(notifypkt);
                pthread_mutex_unlock(&NotifyListMutex);
                sem_post(&NotifySem);

                //sysLogQD() << "------------------------------------NotifyList.append";
            }
            break;
    }
}
void MsgMng::SetIsRecv(bool isRecv)
{
    IsRecv = isRecv;
}
