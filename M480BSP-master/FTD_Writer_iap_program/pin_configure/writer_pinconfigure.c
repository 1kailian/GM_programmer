/****************************************************************************
 * @file     FTDWriter_PinConfigure.c
 * @version  v1.35.3
 * @Date     Wed Dec 10 2025 11:33:59 GMT+0800 (中国标准时间)
 * @brief    NuMicro generated code file
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2013-2025 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

/********************
MCU:M487KIDAE(LQFP128)
********************/

#include "M480.h"


void FTDWriter_PinConfigure_init_emac(void)
{
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA7MFP_Msk | SYS_GPA_MFPL_PA6MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA7MFP_EMAC_RMII_CRSDV | SYS_GPA_MFPL_PA6MFP_EMAC_RMII_RXERR);
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC8MFP_Msk);
    SYS->GPC_MFPH |= (SYS_GPC_MFPH_PC8MFP_EMAC_RMII_REFCLK);
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC7MFP_Msk | SYS_GPC_MFPL_PC6MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC7MFP_EMAC_RMII_RXD0 | SYS_GPC_MFPL_PC6MFP_EMAC_RMII_RXD1);
    SYS->GPE_MFPH &= ~(SYS_GPE_MFPH_PE12MFP_Msk | SYS_GPE_MFPH_PE11MFP_Msk | SYS_GPE_MFPH_PE10MFP_Msk | SYS_GPE_MFPH_PE9MFP_Msk | SYS_GPE_MFPH_PE8MFP_Msk);
    SYS->GPE_MFPH |= (SYS_GPE_MFPH_PE12MFP_EMAC_RMII_TXEN | SYS_GPE_MFPH_PE11MFP_EMAC_RMII_TXD1 | SYS_GPE_MFPH_PE10MFP_EMAC_RMII_TXD0 | SYS_GPE_MFPH_PE9MFP_EMAC_RMII_MDIO | SYS_GPE_MFPH_PE8MFP_EMAC_RMII_MDC);

    return;
}

void FTDWriter_PinConfigure_deinit_emac(void)
{
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA7MFP_Msk | SYS_GPA_MFPL_PA6MFP_Msk);
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC8MFP_Msk);
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC7MFP_Msk | SYS_GPC_MFPL_PC6MFP_Msk);
    SYS->GPE_MFPH &= ~(SYS_GPE_MFPH_PE12MFP_Msk | SYS_GPE_MFPH_PE11MFP_Msk | SYS_GPE_MFPH_PE10MFP_Msk | SYS_GPE_MFPH_PE9MFP_Msk | SYS_GPE_MFPH_PE8MFP_Msk);

    return;
}




void FTDWriter_PinConfigure_init_ice(void)
{
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF1MFP_Msk | SYS_GPF_MFPL_PF0MFP_Msk);
    SYS->GPF_MFPL |= (SYS_GPF_MFPL_PF1MFP_ICE_CLK | SYS_GPF_MFPL_PF0MFP_ICE_DAT);

    return;
}

void FTDWriter_PinConfigure_deinit_ice(void)
{
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF1MFP_Msk | SYS_GPF_MFPL_PF0MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_pa(void)
{
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA12MFP_Msk | SYS_GPA_MFPH_PA11MFP_Msk | SYS_GPA_MFPH_PA10MFP_Msk | SYS_GPA_MFPH_PA9MFP_Msk | SYS_GPA_MFPH_PA8MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA12MFP_GPIO | SYS_GPA_MFPH_PA11MFP_GPIO | SYS_GPA_MFPH_PA10MFP_GPIO | SYS_GPA_MFPH_PA9MFP_GPIO | SYS_GPA_MFPH_PA8MFP_GPIO);
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA5MFP_Msk | SYS_GPA_MFPL_PA4MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA5MFP_GPIO | SYS_GPA_MFPL_PA4MFP_GPIO);

    return;
}

void FTDWriter_PinConfigure_deinit_pa(void)
{
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA12MFP_Msk | SYS_GPA_MFPH_PA11MFP_Msk | SYS_GPA_MFPH_PA10MFP_Msk | SYS_GPA_MFPH_PA9MFP_Msk | SYS_GPA_MFPH_PA8MFP_Msk);
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA5MFP_Msk | SYS_GPA_MFPL_PA4MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_pb(void)
{
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB15MFP_Msk | SYS_GPB_MFPH_PB14MFP_Msk | SYS_GPB_MFPH_PB11MFP_Msk | SYS_GPB_MFPH_PB10MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk | SYS_GPB_MFPH_PB8MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB15MFP_GPIO | SYS_GPB_MFPH_PB14MFP_GPIO | SYS_GPB_MFPH_PB11MFP_GPIO | SYS_GPB_MFPH_PB10MFP_GPIO | SYS_GPB_MFPH_PB9MFP_GPIO | SYS_GPB_MFPH_PB8MFP_GPIO);

    return;
}

void FTDWriter_PinConfigure_deinit_pb(void)
{
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB15MFP_Msk | SYS_GPB_MFPH_PB14MFP_Msk | SYS_GPB_MFPH_PB11MFP_Msk | SYS_GPB_MFPH_PB10MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk | SYS_GPB_MFPH_PB8MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_pc(void)
{
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC14MFP_Msk | SYS_GPC_MFPH_PC12MFP_Msk | SYS_GPC_MFPH_PC11MFP_Msk);
    SYS->GPC_MFPH |= (SYS_GPC_MFPH_PC14MFP_GPIO | SYS_GPC_MFPH_PC12MFP_GPIO | SYS_GPC_MFPH_PC11MFP_GPIO);

    return;
}

void FTDWriter_PinConfigure_deinit_pc(void)
{
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC14MFP_Msk | SYS_GPC_MFPH_PC12MFP_Msk | SYS_GPC_MFPH_PC11MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_pd(void)
{
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD13MFP_Msk | SYS_GPD_MFPH_PD9MFP_Msk | SYS_GPD_MFPH_PD8MFP_Msk);
    SYS->GPD_MFPH |= (SYS_GPD_MFPH_PD13MFP_GPIO | SYS_GPD_MFPH_PD9MFP_GPIO | SYS_GPD_MFPH_PD8MFP_GPIO);

    return;
}

void FTDWriter_PinConfigure_deinit_pd(void)
{
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD13MFP_Msk | SYS_GPD_MFPH_PD9MFP_Msk | SYS_GPD_MFPH_PD8MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_pe(void)
{
    SYS->GPE_MFPH &= ~(SYS_GPE_MFPH_PE15MFP_Msk | SYS_GPE_MFPH_PE14MFP_Msk | SYS_GPE_MFPH_PE13MFP_Msk);
    SYS->GPE_MFPH |= (SYS_GPE_MFPH_PE15MFP_GPIO | SYS_GPE_MFPH_PE14MFP_GPIO | SYS_GPE_MFPH_PE13MFP_GPIO);
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE7MFP_Msk | SYS_GPE_MFPL_PE6MFP_Msk | SYS_GPE_MFPL_PE5MFP_Msk | SYS_GPE_MFPL_PE4MFP_Msk | SYS_GPE_MFPL_PE3MFP_Msk | SYS_GPE_MFPL_PE2MFP_Msk | SYS_GPE_MFPL_PE1MFP_Msk | SYS_GPE_MFPL_PE0MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE7MFP_GPIO | SYS_GPE_MFPL_PE6MFP_GPIO | SYS_GPE_MFPL_PE5MFP_GPIO | SYS_GPE_MFPL_PE4MFP_GPIO | SYS_GPE_MFPL_PE3MFP_GPIO | SYS_GPE_MFPL_PE2MFP_GPIO | SYS_GPE_MFPL_PE1MFP_GPIO | SYS_GPE_MFPL_PE0MFP_GPIO);

    return;
}

void FTDWriter_PinConfigure_deinit_pe(void)
{
    SYS->GPE_MFPH &= ~(SYS_GPE_MFPH_PE15MFP_Msk | SYS_GPE_MFPH_PE14MFP_Msk | SYS_GPE_MFPH_PE13MFP_Msk);
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE7MFP_Msk | SYS_GPE_MFPL_PE6MFP_Msk | SYS_GPE_MFPL_PE5MFP_Msk | SYS_GPE_MFPL_PE4MFP_Msk | SYS_GPE_MFPL_PE3MFP_Msk | SYS_GPE_MFPL_PE2MFP_Msk | SYS_GPE_MFPL_PE1MFP_Msk | SYS_GPE_MFPL_PE0MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_pf(void)
{
    SYS->GPF_MFPH &= ~(SYS_GPF_MFPH_PF11MFP_Msk | SYS_GPF_MFPH_PF10MFP_Msk | SYS_GPF_MFPH_PF9MFP_Msk | SYS_GPF_MFPH_PF8MFP_Msk);
    SYS->GPF_MFPH |= (SYS_GPF_MFPH_PF11MFP_GPIO | SYS_GPF_MFPH_PF10MFP_GPIO | SYS_GPF_MFPH_PF9MFP_GPIO | SYS_GPF_MFPH_PF8MFP_GPIO);

    return;
}

void FTDWriter_PinConfigure_deinit_pf(void)
{
    SYS->GPF_MFPH &= ~(SYS_GPF_MFPH_PF11MFP_Msk | SYS_GPF_MFPH_PF10MFP_Msk | SYS_GPF_MFPH_PF9MFP_Msk | SYS_GPF_MFPH_PF8MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_pg(void)
{
    SYS->GPG_MFPL &= ~(SYS_GPG_MFPL_PG4MFP_Msk | SYS_GPG_MFPL_PG3MFP_Msk | SYS_GPG_MFPL_PG2MFP_Msk);
    SYS->GPG_MFPL |= (SYS_GPG_MFPL_PG4MFP_GPIO | SYS_GPG_MFPL_PG3MFP_GPIO | SYS_GPG_MFPL_PG2MFP_GPIO);

    return;
}

void FTDWriter_PinConfigure_deinit_pg(void)
{
    SYS->GPG_MFPL &= ~(SYS_GPG_MFPL_PG4MFP_Msk | SYS_GPG_MFPL_PG3MFP_Msk | SYS_GPG_MFPL_PG2MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_ph(void)
{
    SYS->GPH_MFPH &= ~(SYS_GPH_MFPH_PH11MFP_Msk | SYS_GPH_MFPH_PH10MFP_Msk | SYS_GPH_MFPH_PH9MFP_Msk | SYS_GPH_MFPH_PH8MFP_Msk);
    SYS->GPH_MFPH |= (SYS_GPH_MFPH_PH11MFP_GPIO | SYS_GPH_MFPH_PH10MFP_GPIO | SYS_GPH_MFPH_PH9MFP_GPIO | SYS_GPH_MFPH_PH8MFP_GPIO);
    SYS->GPH_MFPL &= ~(SYS_GPH_MFPL_PH7MFP_Msk | SYS_GPH_MFPL_PH6MFP_Msk | SYS_GPH_MFPL_PH5MFP_Msk | SYS_GPH_MFPL_PH4MFP_Msk);
    SYS->GPH_MFPL |= (SYS_GPH_MFPL_PH7MFP_GPIO | SYS_GPH_MFPL_PH6MFP_GPIO | SYS_GPH_MFPL_PH5MFP_GPIO | SYS_GPH_MFPL_PH4MFP_GPIO);

    return;
}

void FTDWriter_PinConfigure_deinit_ph(void)
{
    SYS->GPH_MFPH &= ~(SYS_GPH_MFPH_PH11MFP_Msk | SYS_GPH_MFPH_PH10MFP_Msk | SYS_GPH_MFPH_PH9MFP_Msk | SYS_GPH_MFPH_PH8MFP_Msk);
    SYS->GPH_MFPL &= ~(SYS_GPH_MFPL_PH7MFP_Msk | SYS_GPH_MFPL_PH6MFP_Msk | SYS_GPH_MFPL_PH5MFP_Msk | SYS_GPH_MFPL_PH4MFP_Msk);

    return;
}


void FTDWriter_PinConfigure_init_uart0(void)
{
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB13MFP_Msk | SYS_GPB_MFPH_PB12MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB13MFP_UART0_TXD | SYS_GPB_MFPH_PB12MFP_UART0_RXD);

    return;
}

void FTDWriter_PinConfigure_deinit_uart0(void)
{
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB13MFP_Msk | SYS_GPB_MFPH_PB12MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_uart1(void)
{
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD11MFP_Msk | SYS_GPD_MFPH_PD10MFP_Msk);
    SYS->GPD_MFPH |= (SYS_GPD_MFPH_PD11MFP_UART1_TXD | SYS_GPD_MFPH_PD10MFP_UART1_RXD);

    return;
}

void FTDWriter_PinConfigure_deinit_uart1(void)
{
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD11MFP_Msk | SYS_GPD_MFPH_PD10MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_uart2(void)
{
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC13MFP_Msk);
    SYS->GPC_MFPH |= (SYS_GPC_MFPH_PC13MFP_UART2_TXD);
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD12MFP_Msk);
    SYS->GPD_MFPH |= (SYS_GPD_MFPH_PD12MFP_UART2_RXD);

    return;
}

void FTDWriter_PinConfigure_deinit_uart2(void)
{
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC13MFP_Msk);
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD12MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_uart3(void)
{
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC10MFP_Msk | SYS_GPC_MFPH_PC9MFP_Msk);
    SYS->GPC_MFPH |= (SYS_GPC_MFPH_PC10MFP_UART3_TXD | SYS_GPC_MFPH_PC9MFP_UART3_RXD);

    return;
}

void FTDWriter_PinConfigure_deinit_uart3(void)
{
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC10MFP_Msk | SYS_GPC_MFPH_PC9MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_uart4(void)
{
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF7MFP_Msk | SYS_GPF_MFPL_PF6MFP_Msk);
    SYS->GPF_MFPL |= (SYS_GPF_MFPL_PF7MFP_UART4_TXD | SYS_GPF_MFPL_PF6MFP_UART4_RXD);

    return;
}

void FTDWriter_PinConfigure_deinit_uart4(void)
{
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF7MFP_Msk | SYS_GPF_MFPL_PF6MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_uart5(void)
{
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB5MFP_Msk | SYS_GPB_MFPL_PB4MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB5MFP_UART5_TXD | SYS_GPB_MFPL_PB4MFP_UART5_RXD);

    return;
}

void FTDWriter_PinConfigure_deinit_uart5(void)
{
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB5MFP_Msk | SYS_GPB_MFPL_PB4MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_usb(void)
{
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA15MFP_Msk | SYS_GPA_MFPH_PA14MFP_Msk | SYS_GPA_MFPH_PA13MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA15MFP_USB_OTG_ID | SYS_GPA_MFPH_PA14MFP_USB_D_P | SYS_GPA_MFPH_PA13MFP_USB_D_N);

    return;
}

void FTDWriter_PinConfigure_deinit_usb(void)
{
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA15MFP_Msk | SYS_GPA_MFPH_PA14MFP_Msk | SYS_GPA_MFPH_PA13MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_x32(void)
{
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF5MFP_Msk | SYS_GPF_MFPL_PF4MFP_Msk);
    SYS->GPF_MFPL |= (SYS_GPF_MFPL_PF5MFP_X32_IN | SYS_GPF_MFPL_PF4MFP_X32_OUT);

    return;
}

void FTDWriter_PinConfigure_deinit_x32(void)
{
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF5MFP_Msk | SYS_GPF_MFPL_PF4MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init_xt1(void)
{
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF3MFP_Msk | SYS_GPF_MFPL_PF2MFP_Msk);
    SYS->GPF_MFPL |= (SYS_GPF_MFPL_PF3MFP_XT1_IN | SYS_GPF_MFPL_PF2MFP_XT1_OUT);

    return;
}

void FTDWriter_PinConfigure_deinit_xt1(void)
{
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF3MFP_Msk | SYS_GPF_MFPL_PF2MFP_Msk);

    return;
}

void FTDWriter_PinConfigure_init(void)
{
    //SYS->GPA_MFPH = 0xEEE00000UL;
    //SYS->GPA_MFPL = 0x33004444UL;
    //SYS->GPB_MFPH = 0x00660000UL;
    //SYS->GPB_MFPL = 0x11771199UL;
    //SYS->GPC_MFPH = 0x00700773UL;
    //SYS->GPC_MFPL = 0x33444444UL;
    //SYS->GPD_MFPH = 0x0B073300UL;
    //SYS->GPE_MFPH = 0x00033333UL;
    //SYS->GPE_MFPL = 0x00000000UL;
    //SYS->GPF_MFPH = 0x00000000UL;
    //SYS->GPF_MFPL = 0x66AAAAEEUL;
    //SYS->GPG_MFPH = 0x33333330UL;
    //SYS->GPG_MFPL = 0x00000000UL;
    //SYS->GPH_MFPH = 0x00000000UL;
    //SYS->GPH_MFPL = 0x00000000UL;

    FTDWriter_PinConfigure_init_ice();
    FTDWriter_PinConfigure_init_pa();
    FTDWriter_PinConfigure_init_pb();
    FTDWriter_PinConfigure_init_pc();
    FTDWriter_PinConfigure_init_pd();
    FTDWriter_PinConfigure_init_pe();
    FTDWriter_PinConfigure_init_pf();
    FTDWriter_PinConfigure_init_pg();
    FTDWriter_PinConfigure_init_ph();
    FTDWriter_PinConfigure_init_uart0();
    FTDWriter_PinConfigure_init_uart1();
    FTDWriter_PinConfigure_init_uart2();
    FTDWriter_PinConfigure_init_uart3();
    FTDWriter_PinConfigure_init_uart4();
    FTDWriter_PinConfigure_init_uart5();
    FTDWriter_PinConfigure_init_usb();
    FTDWriter_PinConfigure_init_x32();
    FTDWriter_PinConfigure_init_xt1();

    return;
}

void FTDWriter_PinConfigure_deinit(void)
{
    FTDWriter_PinConfigure_deinit_ice();
    FTDWriter_PinConfigure_deinit_pa();
    FTDWriter_PinConfigure_deinit_pb();
    FTDWriter_PinConfigure_deinit_pc();
    FTDWriter_PinConfigure_deinit_pd();
    FTDWriter_PinConfigure_deinit_pe();
    FTDWriter_PinConfigure_deinit_pf();
    FTDWriter_PinConfigure_deinit_pg();
    FTDWriter_PinConfigure_deinit_ph();
    FTDWriter_PinConfigure_deinit_uart0();
    FTDWriter_PinConfigure_deinit_uart1();
    FTDWriter_PinConfigure_deinit_uart2();
    FTDWriter_PinConfigure_deinit_uart3();
    FTDWriter_PinConfigure_deinit_uart4();
    FTDWriter_PinConfigure_deinit_uart5();
    FTDWriter_PinConfigure_deinit_usb();
    FTDWriter_PinConfigure_deinit_x32();
    FTDWriter_PinConfigure_deinit_xt1();

    return;
}

/*** (C) COPYRIGHT 2013-2025 Nuvoton Technology Corp. ***/
