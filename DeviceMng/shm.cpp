#include "shm.h"
#include "libdefdebuglog.h"
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
    sysLogQD() << "devicemng shm exit start! ";
    DELETE(pDataShm);
    DELETE(pCtrlShm);
    DELETE(pStateShm);
    DELETE(pMsgSem);
    IndexMap.clear();
    sysLogQD() << "devicemng shm exit finish! ";
}

bool shm::shm_create(int incnt, int statecnt)
{
    bool ret = true;

    data_cnt = (incnt > DATA_AREA_POINT_MAX) ? DATA_AREA_POINT_MAX : incnt;

    pMsgSem = new SemObject();
    ret &= pMsgSem->create_sem(ctrl_semkey, 0, 0);

    if (data_cnt == 0)
    {
        return false;
    }

    sysLogQD() << "DeviceMng shm_create sDataUnit";
    pDataShm = new ShmObject(data_key, (sizeof(sDataUnit)) * data_cnt);
    ret &= pDataShm->create_object();

    sysLogQD() << "DeviceMng shm_create sCtrlArea";
    pCtrlShm = new ShmObject(ctrl_key, sizeof(sCtrlArea));
    ret &= pCtrlShm->create_object();

    sysLogQD() << "DeviceMng shm_create statecnt";
    pStateShm = new ShmObject(state_key, statecnt);
    ret &= pStateShm->create_object();

    if (ret == false)
    {
        return ret;
    }

    sysLogQD() << "DeviceMng shm_create shm_init";
    ret = shm_init();
    sysLogQD() << "DeviceMng shm_init ret=" << ret;
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
    sysLogQD() << "DeviceMng shm_init data_cnt:" << data_cnt;
    for (int i = 0; i < data_cnt; i++)
    {
        if (!(pDataShm->read_object(0, sizeof(sDataUnit), i, (void*) (&temp))))
        {
            sysLogQE() << "DeviceMng shm_init fail";
            return false;
        }
        //sysLogQD() << "DeviceMngshm_init  parentid:" << temp.parentid << "temp.childid:" << temp.childid << "temp.pointid"
        //         << temp.pointid << "i: " << i << " SERIALIZE_FUNC(temp.parentid,temp.childid,temp.pointid) = "
        //         << SERIALIZE_FUNC(temp.parentid, temp.childid, temp.pointid);
        IndexMap.insert(SERIALIZE_FUNC(temp.parentid, temp.childid, temp.pointid), i);
    }
    sysLogQD() << "DeviceMng shm_init sucess";
    return true;
}

bool shm::shm_read(int parentid, int childid, int pointid, double* pvalue)
{
    mShmIndex::Iterator item;
    sDataUnit           temp;

    if (IndexMap.isEmpty())
    {
        sysLogQE() << "DeviceMng IndexMap.isEmpty";
        return false;
    }
    item = IndexMap.find(SERIALIZE_FUNC(parentid, childid, pointid));
    if ((item != IndexMap.end()) && (item.key() == SERIALIZE_FUNC(parentid, childid, pointid)))
    {
        if (!(pDataShm->read_object(0, sizeof(sDataUnit), item.value(), (void*) (&temp))))
        {
            sysLogQE() << "DeviceMng read_object.fall";
            return false;
        }
        else
        {
            *pvalue = temp.value;
            return true;
        }
    }
    else
    {
        sysLogQD() << "DeviceMng read_object.nofind " << item.key() << "parentid = " << parentid
                 << " SERIALIZE_FUNC(parentid,childid,pointid) " << SERIALIZE_FUNC(parentid, childid, pointid);
        return false;
    }
}

bool shm::shm_ctrl(int parentid, int childid, int pointid, double value)
{
    USHORT    write;
    sDataUnit data;
    bool      ret;

    ret = pCtrlShm->read_object((sizeof(sDataUnit) * CTRL_AREA_MAX), sizeof(USHORT), 0, (void*) (&write));
    if ((ret == false) || (write >= CTRL_AREA_MAX))
        return false;

    data.num      = write;
    data.parentid = parentid;
    data.childid  = childid;
    data.pointid  = pointid;
    data.value    = value;
    ret           = pCtrlShm->write_object(0, sizeof(sDataUnit), write, &data);
    if (ret == false)
    {
        return false;
    }
    write += 1;
    if (write == CTRL_AREA_MAX)
        write = 0;
    ret = pCtrlShm->write_object((sizeof(sDataUnit) * CTRL_AREA_MAX), sizeof(USHORT), 0, (void*) (&write));
    if (ret == false)
    {
        return false;
    }

    pMsgSem->sem_v();
    return true;
}

bool shm::shm_write(int parentid, int childid, int pointid, double value)
{
    mShmIndex::Iterator item;
    sDataUnit           temp;

    if (IndexMap.isEmpty())
    {
        sysLogQE() << "DeviceMng IndexMap Empty";
        return false;
    }
    item = IndexMap.find(SERIALIZE_FUNC(parentid, childid, pointid));
    if ((item != IndexMap.end()) && (item.key() == SERIALIZE_FUNC(parentid, childid, pointid)))
    {
        if (!(pDataShm->read_object(0, sizeof(sDataUnit), item.value(), (void*) (&temp))))
        {
            sysLogQE() << "DeviceMng pDataShm read_object error";
            return false;
        }
        temp.value = value;
        if (!(pDataShm->write_object(0, sizeof(sDataUnit), item.value(), (void*) (&temp))))
        {
            sysLogQE() << "DeviceMng pDataShm write_object error";
        }
        return true;
    }
    else
    {
        sysLogQE() << "DeviceMng IndexMap not found parentid:" << parentid <<
                                         " childid:" << childid  <<
                                          " pointid" << pointid;
        return false;
    }
}

bool shm::shm_readstate(char* pvalue, int len)
{
    bool ret;

    ret = pStateShm->read_object(0, len, 0, (void*) pvalue);
    return ret;
}
