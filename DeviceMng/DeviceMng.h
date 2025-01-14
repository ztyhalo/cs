#ifndef DEVICEMNG_H
#define DEVICEMNG_H

#include <QObject>
#include <QDomDocument>
#include <QFile>
#include <QMap>
#include "driver.h"
#include <QSettings>

#ifdef I386
#define CFGXML_FILE_PATH "/opt/config/devicemng/DeviceManager.xml"
#define CFGINI_FILE_PATH "/opt/config/devicemng/devicemng.ini"
#else
#define CFGXML_FILE_PATH "/opt/config/devicemng/DeviceManager.xml"
#define CFGINI_FILE_PATH "/opt/config/devicemng/devicemng.ini"
#endif

#define DEVICEMNG_SHARE_KEY_NUM 4 // 消息占用4个key
#define DRIVER_SHARE_KEY_NUM    5 // 每个驱动占用5个key

typedef enum
{
    KEY_ADD = 0,
    KEY_SUB
} eOperateKeyType;

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
    lCfgType     CfgList;
    mDriverTable DriverTable;
    int          SysResKey;
    int          SysMinKey;
    int          SysMaxKey;
    int          AppStartKey;
    QList< int > AppUseKeyList;

    DeviceMng();
    void ShellSetupDriver(sDeviceCfg& cfg, int key, int drivermsgkey);

  public:
    bool              InitFinishFlag;
    static DeviceMng* pDeviceCmd;

    static DeviceMng* GetDeviceMng(void);
    //    {
    //        static DeviceMng gDeviceMng;
    //        return &gDeviceMng;
    //    }
    ~DeviceMng();
    void loadCfgFile(const QString filePath);
    void loadShareParam(const QString filePath);
    int  OperateAppMsgKey(eOperateKeyType mode, int key);
    bool FindDriver(uint8_t id, driver** ppdriver);
    bool CheckParamValidity(void);
    int  GetDeviceMngKey(void);
    int  GetDeviceMngResKey(void);
    bool SetupDriver(void);
    void SendHeartToDriver(void);
    void DriverHeartMng(void);
};

#endif // DEVICEMNG_H
