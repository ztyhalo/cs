#include "ShmObject.h"
#include "libdefdebuglog.h"
ShmObject::ShmObject(int key, int blockSize)
{
    shm_key  = QString("%1").arg(key);
    shm_size = blockSize;
}

ShmObject::~ShmObject()
{
    sysLogQD() << "DeviceMng ShmObject exit start! ";
    delete_object();
    sysLogQD() << "DeviceMng ShmObject exit finish! ";
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
                sysLogQE() << "DeviceMng Unable to detach from shared memory.";
                return ERRORFAILED;
            }
        }
        if (!shareMemory.create(blockSize, isReadOnly ? QSharedMemory::ReadOnly : QSharedMemory::ReadWrite))
        {
            sysLogQE() << "DeviceMng can't create memory segment";
            sysLogQE() << shareMemory.error();
            return ERRORFAILED;
        }
    }
    else
    {
        if (!shareMemory.attach(isReadOnly ? QSharedMemory::ReadOnly : QSharedMemory::ReadWrite))
        {
            sysLogQE() << "DeviceMng can't attach share memory";
            return ERRORFAILED;
        }
    }

    return ERROROK;
}

SHSTATUS ShmObject::DestroyShareBlock(void)
{
    if (!shareMemory.detach())
    {
        sysLogQE() << "DeviceMng Unable to detach from shared memory.";
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
        return false;
    }
    Lock();
    memcpy((char*) pdata, ((char*) shareMemory.data() + startsize + typesize * offset), typesize);
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
