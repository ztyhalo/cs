#ifndef SYS_DATA_TYPE
#define SYS_DATA_TYPE

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define POINT_PARAM_MAX  20
#define DEVICE_PARAM_MAX 20
#define DRIVER_PARAM_MAX 20

#define DRIVER_MAX_NUM 6
#define DEVICE_MAX_NUM 128
#define POINT_MAX_NUM  400

typedef unsigned char      uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int       uint32_t;

/*****************点类型定义**************/
typedef struct
{
    uint16_t TriggerMode; //触发方式：0电平变化、1上升沿、2下降沿、3上升/下降沿
    uint16_t FilterTime;  //去抖时间：XX秒
    uint16_t Enable;      //使能方式：0否、1是
} STR_PointType_SwitchIn;

typedef struct
{
    uint16_t ReportEn;  //浮动范围主动上报：0否、1是
    uint16_t ReportGap; //主动上报发送间隔：XX毫秒
    uint16_t ReportMax; //门槛最大值：XX HZ
    uint16_t ReportMin; //门槛最小值：XX HZ
    uint16_t Enable;    //使能方式：0否、1是
} STR_PointType_AnalogIn;

typedef struct
{
    uint16_t InitValue; //输出点初值：XX
    uint16_t Interlock; //与闭锁连锁：0否、1是
    uint16_t Type;      //常开/常闭选择：0常开、1常闭
    uint16_t Enable;    //使能方式：0否、1是
} STR_PointType_SwitchOut;

typedef struct
{
    uint16_t ScopeMax; //量程最大值：XX HZ
    uint16_t ScopeMin; //量程最小值：XX HZ
    uint16_t Enable;   //使能方式：0否、1是
} STR_PointType_AnalogOut;

typedef struct
{
    uint16_t RegType;     //寄存器区：0DO线圈、1DI离散输入、2AO保持、3AI输入寄存器
    uint16_t DataOrgType; //数据组织形式（与后一个寄存器组成32位数据）：0不组包、1此数据为高位、2此数据为低位
    uint16_t DataType;    //读写属性：0只读、1只写、2可读可写
    uint16_t ScopeMax;    //最大值：XX
    uint16_t ScopeMin;    //最小值：XX
    uint16_t InitValue;   //寄存器初值：XX
    uint16_t RegAddr;     //寄存器地址：XXXX
    uint16_t OffsetStart; //寄存器位置偏移_起始位：XX
    uint16_t OffsetEnd;   //寄存器位置偏移_结束位：XX
} STR_PointType_Modbus;

/*****************点结构定义**************/
enum
{
    ENUM_PointType_SwitchIn = 0,
    ENUM_PointType_AnalogIn,
    ENUM_PointType_SwitchOut,
    ENUM_PointType_AnalogOut,
    ENUM_PointType_Modbus,
    ENUM_PointType_MAX
};

typedef struct
{
    uint32_t type;                   //点的类型，包括：开关量输入/输出、模拟量输入/输出、modbus
    uint16_t param[POINT_PARAM_MAX]; //参数区，不同类型的参数见"点类型定义"
} STR_PointParam;

/*****************设备结构定义**************/
typedef union
{
    struct
    {
        uint32_t all_id : 16;
        uint32_t inparent_id : 8;
        uint32_t parent_id : 8;
    } BitName;
    uint32_t name;
} UNI_DeviceName;

typedef struct
{
    UNI_DeviceName  name;                         //设备名称编号
    uint32_t        point_count;                  //设备中包含的点的个数
    uint16_t        device_cfg[DEVICE_PARAM_MAX]; //设备参数区，存设备自身参数
    STR_PointParam* ppoint_param;                 //点的参数头指针
} STR_DeviceInfo;

/*****************驱动结构定义**************/
typedef struct
{
    uint32_t       name;                         //驱动名称编号
    uint32_t       device_count;                 //驱动中包含的设备的个数
    uint16_t       driver_cfg[DRIVER_PARAM_MAX]; //驱动参数区，存驱动自身参数
    STR_DeviceInfo device_param[DEVICE_MAX_NUM]; //每个设备的参数
} STR_DriverInfo;

/*****************参数结构定义**************/
typedef struct
{
    uint32_t       system_type;                  //系统类型，1—TK100系统，2—TK200系统
    uint32_t       driver_count;                 //驱动个数
    STR_DriverInfo driver_param[DRIVER_MAX_NUM]; //每个驱动的参数
} STR_ParamInfo;

#endif // SYS_DATA_TYPE
