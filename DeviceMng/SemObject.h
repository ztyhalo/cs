#ifndef SEMOBJECT_H
#define SEMOBJECT_H

#include <sys/sem.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <QDebug>

union semun
{
    int              val;
    struct semid_ds* buf;
    unsigned short*  array;
};

class SemObject
{
  private:
    int semkey;
    int semid;

  public:
    SemObject();
    ~SemObject();
    bool create_sem(key_t key, int val, int mode);
    bool init_sem(int init_value);
    bool del_sem(void);
    bool sem_p(void);
    bool sem_v(void);
    int  sem_get(void);
};

#endif // SEMOBJECT_H
