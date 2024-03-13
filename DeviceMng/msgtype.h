#ifndef MSGTYPE_H
#define MSGTYPE_H

#include "define.h"
#include "MsgObject.h"
#include <unistd.h>

#define MSG_UNIT_LENGTH   MSG_OBJECT_LENGTH
#define MSG_UNIT_HEAD_LEN 10

#define GET_DEVMNG_ID ((unsigned int) getpid())
#define BROADCAST_ID  0

typedef union
{
    struct
    {
        uint8_t id_point;
        uint8_t id_child;
        uint8_t id_parent;
        uint8_t id_driver;
    } driver;
    uint32_t app;
} Type_MsgAddr;

// define type
enum
{
    MSG_TYPE_DriverGetInfo = 0,
    MSG_TYPE_DriverSendHeart,
    MSG_TYPE_AppLogIn,
    MSG_TYPE_AppLogOut,
    MSG_TYPE_AppReportDriverComNormal,
    MSG_TYPE_AppReportDriverComAbnormal,
    MSG_TYPE_MAX
};

// define abnormal error (data[0])
enum
{
    MSG_ERROR_NoError = 0,
    MSG_ERROR_LogIn_Exist,
    MSG_ERROR_LogIn_NOAPPID,
    MSG_ERROR_Driver_NotExist,
    MSG_ERROR_MAX
};

typedef struct
{
    Type_MsgAddr source;
    Type_MsgAddr dest;
    uint16_t     type;
    uint8_t      data[MSG_UNIT_LENGTH];
} sMsgUnit;

#endif // MSGTYPE_H
