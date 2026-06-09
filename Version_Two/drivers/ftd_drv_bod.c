#include "stdio.h"
#include "NuMicro.h"

static void (*ftd_drv_bod_callback_funtion)(void) = NULL;

// BOD 中断服务函数
void BOD_IRQHandler(void)
{
    /* Clear BOD Interrupt Flag */
    SYS_CLEAR_BOD_INT_FLAG();

    if(ftd_drv_bod_callback_funtion != NULL) {
        ftd_drv_bod_callback_funtion();
    }

    // ========== 关键：立即设置写保护 ==========
    // 禁止任何后续的 Flash 写入/擦除操作
//    ftd_mw_ring_buffer_set_write_protect(1);


    // // 保存关键数据到 RAM（不能操作 Flash）
    // volatile uint32_t* critical_ram = (volatile uint32_t*)0x20000000;

    // // 注意：无法直接访问私有变量，可以通过添加一个获取函数或直接读取地址
    // // 这里演示如何保存必要信息
    // critical_ram[0] = 0xDEADBEEF;  // 掉电标志
    // // 如果需要保存写入计数，可以在 ring_buffer 中添加一个获取函数

    // // 进入低功耗模式，等待复位
    // __WFI();
}

// BOD initialize
void ftd_drv_bod_init(void)
{
    SYS_UnlockReg();
    /* Enable Brown-out detector function */
    SYS_ENABLE_BOD();

    /* Set Brown-out detector voltage level as 3.0V */
    SYS_SET_BOD_LEVEL(SYS_BODCTL_BODVL_3_0V);

    /* Enable Brown-out detector interrupt function */
    SYS_DISABLE_BOD_RST();

    /* Enable Brown-out detector and Power-down wake-up interrupt */
    NVIC_EnableIRQ(BOD_IRQn);
    NVIC_EnableIRQ(PWRWU_IRQn);

    SYS_LockReg();


}

// BOD set callback function
void ftd_drv_bod_set_callback(void (*callback)(void))
{
    ftd_drv_bod_callback_funtion = callback;
}
