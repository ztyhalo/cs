#include "shm.h"
#include "libdefdebuglog.h"
uint32_t gSemNum;
shm::shm(int inkey, int outkey, int outsem, int statekey)
{
    data_key    = inkey;
    ctrl_key    = outkey;
    ctrl_semkey = outsem;
    data_cnt    = 0;
    state_key   = statekey;
}

shm::~shm()
{
    sysLogQD() << "LibDeviceMng shm exit start! ";
    DELETE(pDataShm);
    DELETE(pCtrlShm);
    DELETE(pStateShm);
    DELETE(pMsgSem);
    IndexMap.clear();
    sysLogQD() << "LibDeviceMng shm exit finish! ";
}

bool shm::shm_create(int incnt, int statecnt)
{
    bool ret = true;

    data_cnt = (incnt > DATA_AREA_POINT_MAX) ? DATA_AREA_POINT_MAX : incnt;

    pMsgSem = new SemObject();
    ret &= pMsgSem->create_sem(ctrl_semkey);
    if (data_cnt == 0)
        return false;
    pDataShm = new ShmObject(data_key, (sizeof(sDataUnit)) * data_cnt); //输入输出口状态
    ret &= pDataShm->create_object();
    pCtrlShm = new ShmObject(ctrl_key, sizeof(sCtrlArea)); //输入输出口控制指令缓存
    ret &= pCtrlShm->create_object();
    pStateShm = new ShmObject(state_key, statecnt); //沿线设备状态
    ret &= pStateShm->create_object();
    if (ret == false)
    {
        sysLogQE() << "LibDeviceMng shm::shm_create error";
        return ret;  
    }

    ret = shm_init();
    return ret;
}

bool shm::shm_delete(void)
{
    bool ret = true;
    ret &= pMsgSem->del_sem();
    ret &= pDataShm->delete_object();
    ret &= pCtrlShm->delete_object();
    ret &= pStateShm->delete_object();
    return ret;
}

bool shm::shm_init(void)
{
    sDataUnit temp;

    IndexMap.clear();
    for (int i = 0; i < data_cnt; i++)
    {
        if (!(pDataShm->read_object(0, sizeof(sDataUnit), i, (void*) (&temp))))
            return false;
        IndexMap.insert(SERIALIZE_FUNC(temp.parentid, temp.childid, temp.pointid, temp.num), i);
    }
    return true;
}

bool shm::shm_read(int parentid, int childid, int pointid, int type, double* pvalue)
{
    mShmIndex::Iterator item;
    sDataUnit           temp;

    if (IndexMap.isEmpty())
        return false;

    item = IndexMap.find(SERIALIZE_FUNC(parentid, childid, pointid, type));
    //    qDebug()<<"shm_read parentid,childid,pointid"<<parentid<<childid<<pointid;
    if ((item != IndexMap.end()) && (item.key() == SERIALIZE_FUNC(parentid, childid, pointid, type)))
    {
        //        qDebug()<<"shm_read read_object item.value():"<<item.value();
        if (!(pDataShm->read_object(0, sizeof(sDataUnit), item.value(), (void*) (&temp))))
            return false;
        else
        {
            *pvalue = temp.value;
            return true;
        }
    }
    else
        return false;
}

bool shm::shm_ctrl(int parentid, int childid, int pointid, double value)
{
    USHORT    count;
    USHORT    write;
    sDataUnit data;
    bool      ret;

    //  qDebug()<<"DeviceMngLib shm_ctrl 1";
    ret = pCtrlShm->read_object((sizeof(sDataUnit) * CTRL_AREA_MAX), sizeof(USHORT), 0, (void*) (&write));
    //   qDebug()<<"DeviceMngLib shm_ctrl 2";
    if ((ret == false) || (write >= CTRL_AREA_MAX))
    {
        sysLogQE() << "LibDeviceMng DeviceMngLib shm_ctrl read_object fail:write:" << write;
        return false;
    }

    ret = pCtrlShm->read_object((sizeof(sDataUnit) * CTRL_AREA_MAX), sizeof(USHORT), 2, (void*) (&count));

    if ((ret == false) || (count >= CTRL_AREA_MAX))
    {
        if (ret == false)
        {
            sysLogQE()<<"LibDeviceMng zty read cout fail!";
            return false;
        }
        int cout = pMsgSem->sem_get();
        char buff[512] = {0};
        sprintf(buff,"LibDeviceMng shm_ctrl read_object fail:count:%d sem %d", count, cout);
        sysLogQE() << buff;
        return false;
    }

    data.num      = write;
    data.parentid = parentid;
    data.childid  = childid;
    data.pointid  = pointid;
    data.value    = value;
    ret           = pCtrlShm->write_object(0, sizeof(sDataUnit), write, &data);
    if (ret == false)
    {
        sysLogQE() << "LibDeviceMng shm_ctrl write_object fail:data:" << write;
        return false;
    }
    write += 1;
    count++;
    if (write == CTRL_AREA_MAX)
        write = 0;
    ret = pCtrlShm->write_object((sizeof(sDataUnit) * CTRL_AREA_MAX), sizeof(USHORT), 0, (void*) (&write));
    if (ret == false)
    {
        sysLogQE() << "LibDeviceMng shm_ctrl write_object fail:write:" << write;
        return false;
    }

    ret = pCtrlShm->write_object((sizeof(sDataUnit) * CTRL_AREA_MAX), sizeof(USHORT), 2, (void*) (&count));
    if (ret == false)
    {
        sysLogQE()<<"LibDeviceMng shm_ctrl write_object fail:write:"<< count;
        return false;
    }
    // qDebug()<<"apptest ctrl write:"<<write;
    // if(parentid ==3)
    pMsgSem->sem_v();
    gSemNum++;
    return true;
}

bool shm::shm_ctrl(uint8_t driid, int parentid, int childid, int pointid, double value)
{
    USHORT    count;
    USHORT    write;
    sDataUnit data;
    bool      ret;

    pCtrlShm->Lock();
    ret = pCtrlShm->nolockread_object((sizeof(sDataUnit) * CTRL_AREA_MAX), sizeof(USHORT), 0, (void*) (&write));
    if ((ret == false) || (write >= CTRL_AREA_MAX))
    {
        pCtrlShm->Unlock();
        sysLogQE()<< "LibDeviceMng shm_ctrl read_object fail:write:" << write;
        return false;
    }

    ret = pCtrlShm->nolockread_object((sizeof(sDataUnit) * CTRL_AREA_MAX), sizeof(USHORT), 2, (void*) (&count));

    if ((ret == false) || (count >= CTRL_AREA_MAX))
    {
        if (ret == false)
        {
            pCtrlShm->Unlock();
            sysLogQE() << "LibDeviceMng read cout fail";
            return false;
        }
        pCtrlShm->Unlock();
        int cout = pMsgSem->sem_get();
        char buff[512] = {0};
        sprintf(buff,"LibDeviceMng shm_ctrl read_object fail:count:%d sem %d", count, cout);
        sysLogQE() << buff;
        return false;
    }

    data.num      = write;
    data.parentid = parentid;
    data.childid  = childid;
    data.pointid  = pointid;
    data.value    = value;
    ret           = pCtrlShm->nolockwrite_object(0, sizeof(sDataUnit), write, &data);
    if (ret == false)
    {
        pCtrlShm->Unlock();
        sysLogQE()<< "LibDeviceMng shm_ctrl write_object fail:data:"<< write;
        return false;
    }
    write += 1;
    count++;
    if (write == CTRL_AREA_MAX)
        write = 0;
    ret = pCtrlShm->nolockwrite_object((sizeof(sDataUnit) * CTRL_AREA_MAX), sizeof(USHORT), 0, (void*) (&write));
    if (ret == false)
    {
        pCtrlShm->Unlock();
        sysLogQE()<<"LibDeviceMng shm_ctrl write_object fail:write:"<< write;
        return false;
    }
    ret = pCtrlShm->nolockwrite_object((sizeof(sDataUnit) * CTRL_AREA_MAX), sizeof(USHORT), 2, (void*) (&count));
    if (ret == false)
    {
        pCtrlShm->Unlock();
        sysLogQE() << "LibDeviceMng shm_ctrl write_object fail:write:"<< count;
        return false;
    }
    pCtrlShm->Unlock();
    pMsgSem->sem_v();
    if (driid == 3)
    {
        gSemNum++;
    }
    return true;
}

bool shm::shm_write(int parentid, int childid, int pointid, int type, double value)
{
    mShmIndex::Iterator item;
    sDataUnit           temp;

    if (IndexMap.isEmpty())
        return false;

    item = IndexMap.find(SERIALIZE_FUNC(parentid, childid, pointid, type));
    if ((item != IndexMap.end()) && (item.key() == SERIALIZE_FUNC(parentid, childid, pointid, type)))
    {
        if (!(pDataShm->read_object(0, sizeof(sDataUnit), item.value(), (void*) (&temp))))
            return false;

        temp.value = value;
        if (!(pDataShm->write_object(0, sizeof(sDataUnit), item.value(), (void*) (&temp))))
            return false;
        return true;
    }
    else
        return false;
}

double* shm::shm_get_datapoint(int parentid, int childid, int pointid, int type)
{
    mShmIndex::Iterator item;
    char*               point;

    if (IndexMap.isEmpty())
        return 0;

    item = IndexMap.find(SERIALIZE_FUNC(parentid, childid, pointid, type));
    if ((item != IndexMap.end()) && (item.key() == SERIALIZE_FUNC(parentid, childid, pointid, type)))
    {
        point = pDataShm->get_point(0, sizeof(sDataUnit), item.value());
        if (point == 0)
            return 0;
        else
        {
            return (double*) (point + offsetof(sDataUnit, value));
        }
    }
    else
        return 0;
}

bool shm::shm_readstate(int childid, char* pvalue, int len)
{
    bool ret;
    //childid  表示偏移几个结构体
    //len      单个结构体的大小
    ret = pStateShm->read_object(childid * len, len, 0, (void*) pvalue);
    return ret;
}

bool shm::shm_read_used(int parentid, int childid, int pointid, int type, int* pvalue)
{
    mShmIndex::Iterator item;
    sDataUnit           temp;

    if (IndexMap.isEmpty())
        return false;

    item = IndexMap.find(SERIALIZE_FUNC(parentid, childid, pointid, type));
    //    qDebug()<<"shm_read parentid,childid,pointid"<<parentid<<childid<<pointid;
    if ((item != IndexMap.end()) && (item.key() == SERIALIZE_FUNC(parentid, childid, pointid, type)))
    {
        //        qDebug()<<"shm_read read_object item.value():"<<item.value();
        if (!(pDataShm->read_object(0, sizeof(sDataUnit), item.value(), (void*) (&temp))))
            return false;
        else
        {
            *pvalue = temp.used;
            return true;
        }
    }
    else
        return false;
}

bool shm::shm_write_used(int parentid, int childid, int pointid, int type, int value)
{
    mShmIndex::Iterator item;
    sDataUnit           temp;

    if (IndexMap.isEmpty())
        return false;

    pCtrlShm->Lock();
    item = IndexMap.find(SERIALIZE_FUNC(parentid, childid, pointid, type));
    if ((item != IndexMap.end()) && (item.key() == SERIALIZE_FUNC(parentid, childid, pointid, type)))
    {
        if (!(pDataShm->nolockread_object(0, sizeof(sDataUnit), item.value(), (void*) (&temp))))
        {
            pCtrlShm->Unlock();
            return false;
        }

        temp.used = value;
        if (!(pDataShm->nolockwrite_object(0, sizeof(sDataUnit), item.value(), (void*) (&temp))))
        {
            pCtrlShm->Unlock();
            return false;
        }
        pCtrlShm->Unlock();
        return true;
    }
    else
    {
        pCtrlShm->Unlock();
        return false;
    }
}
