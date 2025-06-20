#ifndef DEVICEMNGAPI_H
#define DEVICEMNGAPI_H

#include "DeviceMng.h"


#define PROCESS_MSG_MAX 20

typedef enum
{
    PARAM_EFFECT_IMMEDIATELY = 0,
    PARAM_EFFECT_MOMENT
} eEffectType;



#define GET_SYS_TIME_MS(x)                                                                                             \
{                                                                                                                  \
        struct timeval tv;                                                                                             \
        gettimeofday(&tv, NULL);                                                                                       \
        x = tv.tv_sec * 1000 + tv.tv_usec / 1000;                                                                      \
}

enum
{
    ERR_NONE             = 1,
    ERR_RET              = -1,
    ERR_PARAM            = -2,
    ERR_INIT_SENDMAIL    = -3,
    ERR_LOGIN_MAIL       = -4,
    ERR_INIT_RECVMAIL    = -5,
    ERR_INIT_DRIVERSHARE = -6,
    ERR_INIT_TIMEOUT     = -7,
    ERR_MAX
};

typedef int (*backcallfunc)(void* pdata, unsigned int len);

typedef struct
{
    uint16_t     driverid;
    uint16_t     type;
    backcallfunc callfunc;
} sProcessMsg;

typedef enum
{
    REPORT_CONTROL_CMD_DUANWEI = 18,
    REPORT_CONTROL_CMD_GET_C_V = 20
} REPORT_CONTROL_CMD;

typedef struct
{
    uint8_t DriverId;
    uint8_t ParentDeviceId;
    uint8_t ChildDeviceId;
    uint8_t cmd;
    uint8_t type;
} CONTROL_DATA_T;

typedef QList< sProcessMsg > lProcessList;

class DeviceMngApi:public Pth_Class
{
  private:
    DeviceMngApp *     m_pAppDevMng;
    // pthread_t          ProcessMsg_id;
    DeviceMngApi();
    static DeviceMngApi* pCmd;
    class AutoRelease
    {
      public:
        ~AutoRelease()
        {
            zprintf3("DeviceMngApi auto release!\n");
            if(pCmd != NULL)
            {
                delete pCmd;
                pCmd = NULL;
            }
        }
    };
    static AutoRelease g_release;

  public:
    lProcessList            processList;
    static DeviceMngApi*    GetDeviceMngApi(void);

    ~DeviceMngApi();

    int      init_data(uint32_t waittime_ms, uint8_t select = 0, bool isRecv = true);
    uint32_t get_point_appid(
        uint8_t DriverId, uint8_t ParentDeviceId, uint8_t ChildDeviceId, uint8_t PointId, uint8_t type = 1);
    double* get_data_point(uint32_t AppId);
    bool    read_data(uint32_t AppId, double* value);
    bool    write_data(uint32_t AppId, double value);
    bool    ctrl_data(uint32_t AppId, double value);
    bool    ctrl_data(CONTROL_DATA_T control_data, double value);
    bool    get_param(uint32_t AppId, backcallfunc func);
    bool    set_param(uint32_t AppId, void* paramlist, uint16_t len, eEffectType mode, backcallfunc func);
    bool    get_deviceinfo(uint8_t DriverId, backcallfunc func);
    bool    wait_msg(sMsgUnit* recvmsg, uint16_t* msglen, eWaitMsgType mode);
    bool    read_state(uint8_t DriverId, int childid, char* value, uint16_t len);
    bool    add_msgcall(uint8_t DriverId, uint16_t type, backcallfunc func);
    bool    ctrl_data_block(uint32_t AppId, double value, uint32_t overtime_10ms);
    bool    read_ctrl_used(uint32_t AppId, int* value);
    void    bussness_start_imform(void);
    void    stopWaitMsg(void);
    void    run();
};

#endif // DEVICEMNGAPI_H
