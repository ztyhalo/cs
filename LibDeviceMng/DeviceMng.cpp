#include "DeviceMng.h"
#include "libdefdebuglog.h"
DeviceMng::DeviceMng()
{
    SysMinKey = 0;
    SysMaxKey = 0;
    CfgList.clear();
    DriverTable.clear();
    InitFinishFlag = false;
}

DeviceMng::~DeviceMng()
{
    sysLogQD() << "LibDeviceMng exit start!";
    CfgList.clear();

    mDriverTable::iterator item;

    for (item = DriverTable.begin(); item != DriverTable.end(); ++item)
    {
        delete item.value();
        item.value() = (driver*) 0;
    }
    DriverTable.clear();
    sysLogQD() << "LibDeviceMng exit finish!";
}

DeviceMng* DeviceMng::pDeviceCmd = NULL;
DeviceMng* DeviceMng::GetDeviceMng()
{
    if (pDeviceCmd == NULL)
    {
        pDeviceCmd = new DeviceMng();
    }
    return pDeviceCmd;
}

void DeviceMng::loadCfgFile(const QString filePath)
{
    QDomDocument doc;
    QString      filepathname = filePath;
    QFile        file(filepathname);

    QString errorStr;
    int     errorLine;
    int     errorColumn;

    if (!doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn))
    {
        char buff[512] = {0};
        sprintf(buff,"LibDeviceMng loadCfgFile Line %d, column %d: %s",errorLine, errorColumn, errorStr.toLatin1().data());
        sysLogQE() << buff;
        return;
    }

    QDomElement doc_root = doc.documentElement();
    if (doc_root.tagName() != "DriverList")
    {
        sysLogQE() << "LibDeviceMng loadCfgFile The driver list config file is not valid";
        return;
    }

    CfgList.clear();
    QDomElement element;
    QDomNode    doc_node = doc_root.firstChild();
    while (!doc_node.isNull())
    {
        if (doc_node.toElement().tagName() == "Driver")
        {
            element = doc_node.toElement();
            sDeviceCfg tmpnode;
            tmpnode.id     = element.attribute("id").toInt();
            tmpnode.name   = element.attribute("drivername");
            tmpnode.script = element.attribute("startscript");
            CfgList.append(tmpnode);
        }
        doc_node = doc_node.nextSibling();
    }
}

bool DeviceMng::FindDriver(uint8_t id, driver** ppdriver)
{
    mDriverTable::iterator item;

    item = DriverTable.find(id);
    if ((item != DriverTable.end()) && (item.key() == id))
    {
        *ppdriver = item.value();
        return true;
    }
    *ppdriver = (driver*) 0;
    return false;
}

void DeviceMng::loadShareParam(const QString filePath)
{
    QSettings settings(filePath, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");

    QString value;

    settings.beginGroup("MEMERY_PARAM");
    value     = QString("SYS_KEY_MIN");
    SysMinKey = settings.value(value).toInt();
    value     = QString("SYS_KEY_MAX");
    SysMaxKey = settings.value(value).toInt();
    settings.endGroup();
    settings.deleteLater();
}

bool DeviceMng::CheckParamValidity(void)
{
    for (int i = 1; i < CfgList.size(); i++)
    {
        for (int j = 0; j < i; j++)
        {
            if (CfgList.at(j).id == CfgList.at(i).id)
            {
                sysLogQE() << "LibDeviceMng cfglist id exist!";
                return false;
            }
        }
    }

    if (SysMaxKey < SysMinKey)
    {
        sysLogQE() << "LibDeviceMng SysMaxKey < SysMinKey!";
        return false;
    }

    if ((SysMaxKey - SysMinKey + 1) <= (DRIVER_SHARE_KEY_NUM * CfgList.size() + DEVICEMNG_SHARE_KEY_NUM))
    {
        sysLogQE() << "LibDeviceMng SysKey num fail,SysMaxKey:" << SysMaxKey << "SysMinKey:" << SysMinKey
                 << "CfgList.size() :" << CfgList.size();
        return false;
    }

    return true;
}

int DeviceMng::GetDeviceMngKey(void)
{
    return SysMinKey;
}

bool DeviceMng::SetupDriver(uint32_t timeout)
{
    driver*        pdriver;
    int            keytemp;
    sDeviceCfg     cfg;
    struct timeval reporttime;
    MsgMng*        pMsgMng = MsgMng::GetMsgMng();

    gettimeofday(&reporttime, NULL);
    for (int i = 0; i < CfgList.size(); i++)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        keytemp  = SysMinKey + DEVICEMNG_SHARE_KEY_NUM + DRIVER_SHARE_KEY_NUM * (CfgList.at(i).id - 1);
        cfg.id   = CfgList.at(i).id;
        cfg.name = CfgList.at(i).name;
        //qDebug() << "LibDeviceMng device[" << i << "]=" << cfg.id << " init time:" << tv.tv_sec;
        pdriver = new driver(cfg.id, cfg.name, keytemp, keytemp + 1, keytemp + 2, keytemp + 4);
        DriverTable.insert(cfg.id, pdriver);
        if (!pMsgMng->InitGetInfo(cfg.id, timeout))
        {
            gettimeofday(&tv, NULL);
            sysLogQE() << "LibDeviceMng device[" << i << "]=" << cfg.id << " InitGetInfo error time:" << tv.tv_sec;
            return false;
        }
        if (!pdriver->Init())
        {
            gettimeofday(&tv, NULL);
            sysLogQE() << "LibDeviceMng device[" << i << "]=" << cfg.id << " Initmem error time:" << tv.tv_sec;
            return false;
        }
        if (timeout != 0)
        {
            if (timeout > timeruseMS(&reporttime))
                timeout = timeout - timeruseMS(&reporttime);
            else
            {
                timeout = 0;
                gettimeofday(&tv, NULL);
                sysLogQE() << "LibDeviceMng device[" << i << "]=" << cfg.id << " timeout error time:" << tv.tv_sec;
                return false;
            }
        }
        gettimeofday(&tv, NULL);
        sysLogQE() << "LibDeviceMng device[" << i << "]=" << cfg.id << " Init finish time:" << tv.tv_sec;
    }

    return true;
}

uint32_t DeviceMng::timeruseMS(struct timeval* old)
{
    struct timeval now;
    struct timeval temp;
    uint32_t       ms;

    gettimeofday(&now, NULL);
    temp.tv_usec = (now.tv_sec - old->tv_sec) * 1000000;
    temp.tv_usec = temp.tv_usec + now.tv_usec - old->tv_usec;
    ms           = temp.tv_usec % 1000;
    return ms;
}

int DeviceMng::Init(uint32_t waittime_ms)
{
    struct timeval reporttime;
    MsgMng*        pMsgMng = MsgMng::GetMsgMng();

    loadCfgFile(CFGXML_FILE_PATH);
    loadShareParam(CFGINI_FILE_PATH);
    if (!CheckParamValidity())
    {
        sysLogQE() << "LibDeviceMng param check fail!";
        return -2;
    }

    if (!pMsgMng->InitSendMail(SysMinKey, SysMinKey + 2, SysMinKey + 3))
    {
        sysLogQE() << "LibDeviceMng self msg init fail!";
        return -3;
    }

    gettimeofday(&reporttime, NULL);
    if (!pMsgMng->LoginRecvMail(waittime_ms))
    {
        sysLogQE() << "LibDeviceMng login recv msg fail!";
        return -4;
    }

    if (!pMsgMng->InitRecvMail())
    {
        sysLogQE() << "LibDeviceMng recv msg init fail!";
        return -5;
    }

    if ((waittime_ms <= timeruseMS(&reporttime)) && (waittime_ms != 0))
    {
        sysLogQE() << "LibDeviceMng init timeout!";
        return -7;
    }

    if (!SetupDriver((waittime_ms != 0) ? (waittime_ms - timeruseMS(&reporttime)) : 0))
    {
        sysLogQE() << "LibDeviceMng driver init fail!";
        return -6;
    }
    InitFinishFlag = true;
    sysLogQD() << "LibDeviceMng driver init InitFinishFlag true!";
    return 1;
}

uint32_t DeviceMng::GetAppid(
    uint8_t DriverID, uint8_t ParentDeviceID, uint8_t ChildDeviceID, uint8_t PointID, uint8_t type)
{
    if ((DriverID >= 64) || (type > 3))
        return 0;

    return (((uint32_t) (type << 30)) | ((uint32_t) (DriverID << 24)) | ((uint32_t) (ParentDeviceID << 16)) |
            ((uint32_t) (ChildDeviceID << 8)) | PointID);
}

void DeviceMng::ChangeAppid(
    uint32_t appid, uint8_t* DriverID, uint8_t* ParentDeviceID, uint8_t* ChildDeviceID, uint8_t* PointID, uint8_t* type)
{
    *DriverID       = (uint8_t) ((appid & 0x3f000000) >> 24);
    *ParentDeviceID = (uint8_t) ((appid & 0x00ff0000) >> 16);
    *ChildDeviceID  = (uint8_t) ((appid & 0x0000ff00) >> 8);
    *PointID        = (uint8_t) (appid & 0x000000ff);
    *type           = (uint8_t) ((appid & 0xc0000000) >> 30);
}
