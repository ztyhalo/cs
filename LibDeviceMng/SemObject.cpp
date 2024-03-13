#include "SemObject.h"
#include "libdefdebuglog.h"
SemObject::SemObject()
{
}

SemObject::~SemObject()
{
    sysLogQD() << "LibDeviceMng SemObject exit! ";
    //   del_sem();
}

bool SemObject::create_sem(key_t key)
{
    semkey = key;
    semid  = semget(key, 1, 0666);

    if (semid == -1)
    {
        sysLogQE() << "LibDeviceMng SemObject exit! ";
        /*        semid = semget(key, 1,  IPC_CREAT|0666);
                if (semid == -1)
                {
                    qDebug("Sem Create Error!\n");
                    return false;
                }

                if(init_sem(val) == -1)
                {
                    return false;
                }*/
    }
    return true;
}

// 将信号量设置为init_value
bool SemObject::init_sem(int init_value)
{
    union semun sem_union;
    sem_union.val = init_value;
    if (semctl(semid, 0, SETVAL, sem_union) == -1)
    {
        sysLogQE() << "LibDeviceMng Sem init error";
        return false;
    }
    return true;
}

// 删除信号量
bool SemObject::del_sem(void)
{
    union semun sem_union;
    if (semctl(semid, 0, IPC_RMID, sem_union) == -1)
    {
        sysLogQE()<<"LibDeviceMng Sem delete";
        return false;
    }
    return true;
}

// 对sem_id执行p操作
bool SemObject::sem_p(void)
{
    struct sembuf sem_buf;
    sem_buf.sem_num = 0;  //信号量编号
    sem_buf.sem_op  = -1; // P操作
    sem_buf.sem_flg = 0;  // SEM_UNDO;//系统退出前未释放信号量，系统自动释放
    if (semop(semid, &sem_buf, 1) == -1)
    {
        sysLogQE()<<"LibDeviceMng Sem P operation";
        return false;
    }
    return true;
}

// 对sem_id执行V操作
bool SemObject::sem_v(void)
{
    struct sembuf sem_buf;
    sem_buf.sem_num = 0;
    sem_buf.sem_op  = 1; // V操作
    sem_buf.sem_flg = 0; // SEM_UNDO;
    if (semop(semid, &sem_buf, 1) == -1)
    {
        sysLogQE()<<"LibDeviceMng Sem V operation";
        return false;
    }
    return true;
}

int SemObject::sem_get(void)
{
    return semctl(semid, 0, GETVAL, 0);
}
