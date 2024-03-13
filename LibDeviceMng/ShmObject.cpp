#include "ShmObject.h"
#include "libdefdebuglog.h"
ShmObject::ShmObject(int key, int blockSize)
{
    shm_key  = QString("%1").arg(key);
    shm_size = blockSize;
}

ShmObject::~ShmObject()
{
    sysLogQD() << "LibDeviceMng ShmObject exit start! ";
    delete_object();
    sysLogQD() << "LibDeviceMng ShmObject exit finish! ";
}

SHSTATUS ShmObject::CreateShareBlock(QString key, int blockSize, eShmType mode, int isReadOnly)
{
    shareMemory.setKey(key);
    if (mode == ENUM_SHM_CREATE)
    {
        if (shareMemory.isAttached())
        {
            // 将该进程与共享内存段分离
            if (!shareMemory.detach())
            {
                sysLogQE() << "LibDeviceMng Unable to detach from shared memory.";
                return ERRORFAILED;
            }
        }
        if (!shareMemory.create(blockSize, isReadOnly ? QSharedMemory::ReadOnly : QSharedMemory::ReadWrite))
        {
            sysLogQE() << "LibDeviceMng can't create memory segment:" << shareMemory.error();
            return ERRORFAILED;
        }
    }
    else
    {
        if (!shareMemory.attach(isReadOnly ? QSharedMemory::ReadOnly : QSharedMemory::ReadWrite))
        {
            sysLogQE() << "LibDeviceMng can't attach share memory";
            return ERRORFAILED;
        }
    }

    return ERROROK;
}

SHSTATUS ShmObject::DestroyShareBlock(void)
{
    if (!shareMemory.detach())
    {
        sysLogQE() << "LibDeviceMng Unable to detach from shared memory.";
        return ERRORFAILED;
    }
    return ERRORFAILED;
}

bool ShmObject::create_object(void)
{
    bool ret;

    ret = CreateShareBlock(shm_key, shm_size, ENUM_SHM_OPEN, 0);
    return ret;
}

bool ShmObject::delete_object(void)
{
    bool ret;

    ret = DestroyShareBlock();
    return ret;
}

void ShmObject::Lock(void)
{
    shareMemory.lock();
}

void ShmObject::Unlock(void)
{
    shareMemory.unlock();
}

bool ShmObject::read_object(int startsize, int typesize, int offset, void* pdata)
{
    if ((startsize + typesize * (offset + 1)) > shm_size)
    {
        //        qDebug()<<"startsize + typesize*(offset+1)) >shm_size : "<<(startsize + typesize*(offset+1))<< ">"<<
        //        shm_size;
        return false;
    }

    Lock();
    //    qDebug()<<"DeviceMngLib read_object 4:offset:"<<startsize+typesize*offset<<"typesize:"<<typesize;
    memcpy((char*) pdata, ((char*) shareMemory.data() + startsize + typesize * offset), typesize);
    //    qDebug()<<"DeviceMngLib read_object 5";
    Unlock();

    return true;
}

bool ShmObject::write_object(int startsize, int typesize, int offset, void* pdata)
{
    if ((startsize + typesize * (offset + 1)) > shm_size)
    {
        return false;
    }

    Lock();
    memcpy(((char*) shareMemory.data() + startsize + typesize * offset), (char*) pdata, typesize);
    Unlock();

    return true;
}

bool ShmObject::nolockread_object(int startsize, int typesize, int offset, void* pdata)
{
    if ((startsize + typesize * (offset + 1)) > shm_size)
    {
        return false;
    }

    //    qDebug()<<"DeviceMngLib read_object 4:offset:"<<startsize+typesize*offset<<"typesize:"<<typesize;
    memcpy((char*) pdata, ((char*) shareMemory.data() + startsize + typesize * offset), typesize);
    //    qDebug()<<"DeviceMngLib read_object 5";

    return true;
}

bool ShmObject::nolockwrite_object(int startsize, int typesize, int offset, void* pdata)
{
    if ((startsize + typesize * (offset + 1)) > shm_size)
    {
        return false;
    }

    memcpy(((char*) shareMemory.data() + startsize + typesize * offset), (char*) pdata, typesize);

    return true;
}

char* ShmObject::get_point(int startsize, int typesize, int offset)
{
    if ((startsize + typesize * (offset + 1)) > shm_size)
    {
        return 0;
    }

    return ((char*) shareMemory.data() + startsize + typesize * offset);
}
