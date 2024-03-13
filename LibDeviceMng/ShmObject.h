#ifndef SHMOBJECT_H
#define SHMOBJECT_H

#include <QSharedMemory>
#include "stdio.h"
#include <string.h>
#include <QDebug>

#define ERRORFAILED 0
#define ERROROK     1

typedef int SHSTATUS;
typedef enum
{
    ENUM_SHM_OPEN = 0,
    ENUM_SHM_CREATE
} eShmType;

class ShmObject
{
  private:
    QString       shm_key;
    int           shm_size;
    QSharedMemory shareMemory;

    SHSTATUS CreateShareBlock(QString key, int blockSize, eShmType mode, int isReadOnly);
    SHSTATUS DestroyShareBlock(void);

  public:
    ShmObject(int key, int blockSize);
    ~ShmObject();
    void  Lock(void);
    void  Unlock(void);
    bool  create_object(void);
    bool  delete_object(void);
    bool  read_object(int startsize, int typesize, int offset, void* pdata);
    bool  write_object(int startsize, int typesize, int offset, void* pdata);
    bool  nolockread_object(int startsize, int typesize, int offset, void* pdata);
    bool  nolockwrite_object(int startsize, int typesize, int offset, void* pdata);
    char* get_point(int startsize, int typesize, int offset);
};

#endif // SHMOBJECT_H
