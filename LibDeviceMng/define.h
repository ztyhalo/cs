#ifndef DEFINE_H
#define DEFINE_H

// using namespace std;

typedef unsigned char      uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int       uint32_t;

typedef char           BOOL;
typedef unsigned char  UCHAR;
typedef char           CHAR;
typedef unsigned short USHORT;
typedef short          SHORT;

typedef unsigned long ULONG;
typedef long          LONG;

#define DELETE(pp)                                                                                                     \
    {                                                                                                                  \
        if (pp)                                                                                                        \
        {                                                                                                              \
            delete (pp);                                                                                               \
            (pp) = NULL;                                                                                               \
        }                                                                                                              \
    }

#endif // DEFINE_H
