/****************************************************************************
@FILENAME:  ftd_app_writer_manager.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.28
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "stdint.h"
#include "NuMicro.h"
#include "ftd_mw_display_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_data_model.h"

#define MGR_LOG_WARN   JYMC_LOG_WARN
#define MGR_LOG_INFO   JYMC_LOG_INFO
#define MGR_LOG_DBUG   JYMC_LOG_DBUG

extern CHANNEL_BURN_STATUS st_channel_burn_status_test;

void ftd_app_writer_manager_init(void)
{
    MGR_LOG_INFO("ftd_app_writer_manager_init");
}

void ftd_app_writer_manager_process(void)
{
    bool b_update_display = false;

    if(b_update_display)
        ftd_mw_display_manager_sync_channel_burn_status(&st_channel_burn_status_test);
}

void ftd_app_writer_manager_deinit(void)
{
    MGR_LOG_INFO("ftd_app_writer_manager_deinit");

}

