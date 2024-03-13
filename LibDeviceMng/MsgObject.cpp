#include "MsgObject.h"
#include "libdefdebuglog.h"
MsgObject::MsgObject(int key)
{
    msg_key = key;
}

MsgObject::~MsgObject()
{
    qDebug() << "LibDeviceMng ~ MsgObject: " << this;
    //    delete_object();
}

bool MsgObject::create_object(void)
{
    msg_id = msgget(msg_key, 0666);

    if (msg_id == -1)
    {
        return false;
    }
    memset(&msg_data, 0, sizeof(sMsgType));

    return true;
}

bool MsgObject::delete_object(void)
{
    if (msgctl(msg_id, IPC_RMID, 0) == -1)
        return false;
    return true;
}

bool MsgObject::send_object(void* pdata, int size)
{
    if ((size > MSG_OBJECT_LENGTH) || (pdata == NULL))
        return false;

    msg_data.msgtype = 1;
    memcpy(msg_data.msgtext, (char*) pdata, size);
    if (msgsnd(msg_id, &msg_data, size, IPC_NOWAIT) == -1)
        return false;
    return true;
}

bool MsgObject::receive_object(void* pdata, int* psize, int mode)
{
    int len;

    memset(&msg_data, 0, sizeof(sMsgType));
    len = msgrcv(msg_id, &msg_data, MSG_OBJECT_LENGTH, 1, (mode == RECV_WAIT) ? 0 : IPC_NOWAIT);
    if (len == -1)
    {
        sysLogQE() << "LibDeviceMng DeviceMngLib receive_object fail";
        return false;
    }

    *psize = len;
    memcpy((char*) pdata, msg_data.msgtext, len);
    return true;
}

int MsgObject::GetMsgKey(void)
{
    return msg_key;
}
