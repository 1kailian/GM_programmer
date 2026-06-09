/****************************************************************************
@FILENAME  :   ftd_mw_log_manager.h
@BRIEF     :
@AUTHOR    :   
@DATE      :   2021/08/24 11:35:24
****************************************************************************/

#ifndef _FTD_MW_LOG_MANAGER__H_
#define _FTD_MW_LOG_MANAGER__H_

#include <stdio.h>
#include "ftd_data_model.h"

/******************************DEBUG LOG DEFINE******************************/
#ifndef LOG_TYPE
#define LOG_TYPE                                    LOG_LEVEL_OFF //selection the log level
#endif

/****************************LOG LEVEL DEFINE********************************/
#define LOG_LEVEL_OFF                               (0x00)
#define LOG_LEVEL_FATAL                             (0x01)
#define LOG_LEVEL_ERROR                             (0x02)
#define LOG_LEVEL_WARN                              (0x04)
#define LOG_LEVEL_INFO                              (0x10)
#define LOG_LEVEL_DEBUG                             (0x20)
#define LOG_LEVEL_TRACE                             (0x40)
#define LOG_LEVEL_ALL                               (0xFF)

#define LOG_PRINT_RAW                               printf

#if(LOG_TYPE >= LOG_LEVEL_FATAL)
#define JYMC_LOG_FATAL(fmt, ...)                                \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[FATL][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);

#define FTD_LOG_FATAL(fmt, ...)                                \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[FATL][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);
#else
#define JYMC_LOG_FATAL(fmt, ...)
#define FTD_LOG_FATAL(fmt, ...)
#endif

#if(LOG_TYPE >= LOG_LEVEL_ERROR)
#define JYMC_LOG_ERROR(fmt, ...)                                \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[ERRO][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);

#define FTD_LOG_ERROR(fmt, ...)                                \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[ERRO][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);
#else
#define JYMC_LOG_ERROR(fmt, ...)
#define FTD_LOG_ERROR(fmt, ...)
#endif

#if(LOG_TYPE >= LOG_LEVEL_WARN)
#define JYMC_LOG_WARN(fmt, ...)                                 \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[WARN][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);
#define FTD_LOG_WARN(fmt, ...)                                 \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[WARN][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);
#else
#define JYMC_LOG_WARN(fmt, ...)
#define FTD_LOG_WARN(fmt, ...)
#endif

#if(LOG_TYPE >= LOG_LEVEL_INFO)
#define JYMC_LOG_INFO(fmt, ...)                                 \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[INFO][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);
#define FTD_LOG_INFO(fmt, ...)                                 \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[INFO][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);
#else
#define JYMC_LOG_INFO(fmt,...)
#define FTD_LOG_INFO(fmt,...)
#endif

#if(LOG_TYPE >= LOG_LEVEL_DEBUG)
#define JYMC_LOG_DEBUG(fmt, ...)                                \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[DBUG][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);
#define FTD_LOG_DEBUG(fmt, ...)                                \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[DBUG][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);
#define FTD_LOG_DEBUG_BUFF(fmt, ...)                            \
do                                                              \
{                                                               \
    ftd_log_buffer(fmt, ##__VA_ARGS__);                         \
} while (0);
#else
#define JYMC_LOG_DEBUG(fmt, ...)
#define FTD_LOG_DEBUG(fmt, ...)
#define FTD_LOG_DEBUG_BUFF(fmt, ...)
#endif

#if(LOG_TYPE >= LOG_LEVEL_TRACE)
#define JYMC_LOG_TRACE(fmt, ...)                                \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[TRCE][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);
#define FTD_LOG_TRACE(fmt, ...)                                \
do                                                              \
{                                                               \
    LOG_PRINT_RAW("[TRCE][%s]<%d>:", __FUNCTION__, __LINE__);  \
    LOG_PRINT_RAW(fmt, ##__VA_ARGS__);                         \
    LOG_PRINT_RAW("\n");                                       \
} while (0);
#define FTD_LOG_TRACE_BUFF(fmt, ...)                            \
do                                                              \
{                                                               \
    ftd_log_buffer(fmt, ##__VA_ARGS__);                         \
} while (0);
#else
#define JYMC_LOG_TRACE(fmt, ...)
#define FTD_LOG_TRACE(fmt, ...)
#define FTD_LOG_TRACE_BUFF(fmt, ...)
#endif

#if (LOG_TYPE != LOG_LEVEL_OFF)
#define JYMC_LOG_RAW                                LOG_PRINT_RAW
#define FTD_LOG_RAW                                 LOG_PRINT_RAW
#endif

//void ftd_log_buffer(uint8_t * buf, uint16_t buff_size, uint16_t len);

#endif

