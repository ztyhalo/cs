#include "MsgObject.h"
#include "libdefdebuglog.h"
MsgObject::MsgObject(int key)
{
    msg_key = key;
}

MsgObject::~MsgObject()
{
    sysLogQD() << "DeviceMng ~ MsgObject: " << this;
    delete_object();
}

bool MsgObject::create_object(void)
{
    // 尝试删除消息队列
    if ((msg_id = msgget(msg_key, 0)) != -1)
    {
        msgctl(msg_id, IPC_RMID, NULL);
    }

    msg_id = msgget(msg_key, 0666 | IPC_CREAT);

    if (msg_id == -1)
    {
        sysLogQE() << "DeviceMng msgobject create error:" << strerror(errno);
        return false;
    }
    memset(&msg_data, 0, sizeof(sMsgType));

    return true;
}

bool MsgObject::delete_object(void)
{
    if (msgctl(msg_id, IPC_RMID, 0) == -1)
    {
        sysLogQE() << "DeviceMng msgobject delete error:" << strerror(errno);
        return false;
    }
    return true;
}

bool MsgObject::send_object(void* pdata, int size)
{
    if ((size > MSG_OBJECT_LENGTH) || (pdata == NULL))
        return false;

    memset(&msg_data, 0, sizeof(sMsgType));
    msg_data.msgtype = 1;
    memcpy(msg_data.msgtext, (char*) pdata, size);

    //qDebug() << "------msg send start-----------------------------------------------";
    //qDebug() << "DeviceMng send_object send msg_id:" << msg_id << " len: " << size;
    //    qDebug()<<QString::fromStdString(msg_data.msgtext);
    //qDebug() << "------msg send end------";

    if (msgsnd(msg_id, &msg_data, size, IPC_NOWAIT) == -1)
    {
        sysLogQE() << "DeviceMng msgobject send error:" << strerror(errno);
        return false;
    }
    return true;
}

bool MsgObject::receive_object(void* pdata, int* psize, int mode)
{
    int len;

    memset(&msg_data, 0, sizeof(sMsgType));
    len = msgrcv(msg_id, &msg_data, MSG_OBJECT_LENGTH, 1, (mode == RECV_WAIT) ? 0 : IPC_NOWAIT);
    if (len == -1)
    {
        sysLogQE() << "DeviceMng msgobject receive error:" << strerror(errno);
        return false;
    }

    *psize = len;
    memcpy((char*) pdata, msg_data.msgtext, len);
    //sysLogQD() << "DeviceMng receive_object recv msg_id:" << msg_id << " len: " << len;
    return true;
}

int MsgObject::GetMsgKey(void)
{
    return msg_key;
}
