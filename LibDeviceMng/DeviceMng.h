#ifndef DEVICEMNG_H
#define DEVICEMNG_H

#include <QObject>
#include <QDomDocument>
#include <QFile>
#include <QMap>
#include "driver.h"
#include <QSettings>
#include "MsgMng.h"

#ifdef I386
#define CFGXML_FILE_PATH "/opt/config/devicemng/DeviceManager.xml"
#define CFGINI_FILE_PATH "/opt/config/devicemng/devicemng.ini"
#else
#define CFGXML_FILE_PATH "/opt/config/devicemng/DeviceManager.xml"
#define CFGINI_FILE_PATH "/opt/config/devicemng/devicemng.ini"
#endif

#define DEVICEMNG_SHARE_KEY_NUM 4
#define DRIVER_SHARE_KEY_NUM    5

#define GET_SYS_TIME_MS(x)                                                                                             \
    {                                                                                                                  \
        struct timeval tv;                                                                                             \
        gettimeofday(&tv, NULL);                                                                                       \
        x = tv.tv_sec * 1000 + tv.tv_usec / 1000;                                                                      \
    }

typedef struct
{
    int     id;
    QString name;
    QString script;
} sDeviceCfg;
typedef QList< sDeviceCfg >  lCfgType;
typedef QMap< int, driver* > mDriverTable;

class DeviceMng
{
  private:
    lCfgType          CfgList;
    mDriverTable      DriverTable;
    int               SysMinKey;
    int               SysMaxKey;
    static DeviceMng* pDeviceCmd;

    DeviceMng();
    uint32_t timeruseMS(struct timeval* old);

  public:
    bool InitFinishFlag;

    static DeviceMng* GetDeviceMng(void);
    ~DeviceMng();
    void     loadCfgFile(const QString filePath);
    void     loadShareParam(const QString filePath);
    bool     FindDriver(uint8_t id, driver** ppdriver);
    bool     CheckParamValidity(void);
    int      GetDeviceMngKey(void);
    bool     SetupDriver(uint32_t timeout);
    int      Init(uint32_t waittime_ms);
    uint32_t GetAppid(uint8_t DriverID, uint8_t ParentDeviceID, uint8_t ChildDeviceID, uint8_t PointID, uint8_t type);
    void     ChangeAppid(uint32_t appid, uint8_t* DriverID, uint8_t* ParentDeviceID, uint8_t* ChildDeviceID,
            uint8_t* PointID, uint8_t* type);
};

#endif // DEVICEMNG_H
