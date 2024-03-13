#ifndef SHM_H
#define SHM_H

#include "define.h"
#include "ShmObject.h"
#include "SemObject.h"

#define CTRL_AREA_MAX       256
#define DATA_AREA_POINT_MAX 4000
#define PARENT_DEVICE_MAX   100
#define CHILD_DEVICE_MAX    200
#define TYPE_DEVICE_MAX     4
#define SERIALIZE_FUNC(x, y, z, w)                                                                                     \
    (TYPE_DEVICE_MAX * PARENT_DEVICE_MAX * CHILD_DEVICE_MAX * z + TYPE_DEVICE_MAX * PARENT_DEVICE_MAX * y +            \
        TYPE_DEVICE_MAX * x + w)

typedef struct
{
    int    num; //共享内存状态时不用做序列号，用作输入/输出标致。1：输入  2：输出
    int    parentid;
    int    childid;
    int    pointid;
    double value;
    int    used; //用于标致输出点是否被占用 1：占用   0：未占用
} sDataUnit;

typedef struct
{
    sDataUnit data[CTRL_AREA_MAX];
    USHORT    write;
    USHORT    read;
    USHORT    count;
} sCtrlArea;

typedef QMap< int, int > mShmIndex;

class shm
{
  private:
    int        data_key;
    int        ctrl_key;
    int        data_cnt;
    int        ctrl_semkey;
    int        state_key;
    ShmObject* pDataShm;
    ShmObject* pCtrlShm;
    ShmObject* pStateShm;
    SemObject* pMsgSem;
    mShmIndex  IndexMap;

  public:
    shm(int inkey, int outkey, int outsem, int statekey);
    ~shm();
    bool    shm_create(int incnt, int statecnt);
    bool    shm_delete(void);
    bool    shm_init(void);
    bool    shm_read(int parentid, int childid, int pointid, int type, double* pvalue);
    bool    shm_ctrl(int parentid, int childid, int pointid, double value);
    bool    shm_write(int parentid, int childid, int pointid, int type, double value);
    double* shm_get_datapoint(int parentid, int childid, int pointid, int type);
    bool    shm_readstate(int childid, char* pvalue, int len);
    bool    shm_write_used(int parentid, int childid, int pointid, int type, int value);
    bool    shm_read_used(int parentid, int childid, int pointid, int type, int* pvalue);
    bool    shm_ctrl(uint8_t driid, int parentid, int childid, int pointid, double value);
};

#endif // SHM_H
