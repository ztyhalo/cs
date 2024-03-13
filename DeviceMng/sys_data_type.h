#ifndef SYS_DATA_TYPE_H
#define SYS_DATA_TYPE_H

#include "sys_param_type.h"

#define POINT_AREA_MAX             2000
#define OUTPUT_AREA_PER_DRIVER_MAX 255

typedef struct
{
    uint32_t        Num;
    STR_PointParam* pPointParam;
    uint32_t        value;
} STR_MemInfo_Point;

typedef struct
{
    uint32_t DriverId;
    uint32_t Num;
    uint32_t DeviceId;
    uint32_t PointId;
    uint32_t value;
} STR_MemOut_Point;

#endif // SYS_DATA_TYPE_H
