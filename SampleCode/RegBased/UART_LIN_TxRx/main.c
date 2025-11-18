/****************************************************************************
 * @file     main.c
 * @version  V3.00
 * @brief    Transmit LIN frame including header and response in UART LIN mode.
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "M0518.h"


#define PLLCON_SETTING  CLK_PLLCON_50MHz_HXT
#define PLL_CLOCK   50000000


#define LIN_ID      0x30

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
static volatile int32_t g_i32RxCounter;
static uint8_t g_u8ReceiveData[9];

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void UART_FunctionTest(void);
int32_t LIN_Tx_FunctionTest(void);
void LIN_Rx_FunctionTest(void);
void UART1_IRQHandler(void);
void SYS_Init(void);
void UART0_Init(void);
void UART1_Init(void);
uint8_t ComputeChksumValue(uint8_t *pu8Buf, uint32_t u32ByteCnt);
void TestItem(void);
uint8_t GetParityValue(uint32_t u32id);


void UART1_IRQHandler(void)
{
    volatile uint32_t u32IntSts = UART1->ISR;
    uint32_t u32Data;

    if(u32IntSts & UART_ISR_LIN_INT_Msk)
    {
        if(UART1->LIN_SR & UART_LIN_SR_LINS_HDET_F_Msk)
        {
            /* Clear LIN slave header detection flag */
            UART1->LIN_SR = UART_LIN_SR_LINS_HDET_F_Msk;
            printf("\n LIN Slave Header detected ");
        }

        if(UART1->LIN_SR & (UART_LIN_SR_LINS_HERR_F_Msk | UART_LIN_SR_LINS_IDPERR_F_Msk | UART_LIN_SR_BIT_ERR_F_Msk))
        {
            /* Clear LIN error flag */
            UART1->LIN_SR = (UART_LIN_SR_LINS_HERR_F_Msk | UART_LIN_SR_LINS_IDPERR_F_Msk | UART_LIN_SR_BIT_ERR_F_Msk);
            printf("\n LIN error detected ");
        }
    }

    if(u32IntSts & UART_ISR_RDA_INT_Msk)
    {
        u32Data = UART1->RBR;
        g_u8ReceiveData[g_i32RxCounter] = (uint8_t)u32Data;
        g_i32RxCounter++;
    }

}

void SYS_Init(void)
{
	uint32_t u32TimeOutCnt;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable Internal RC 22.1184MHz clock */
    CLK->PWRCON |= CLK_PWRCON_OSC22M_EN_Msk;

    /* Waiting for Internal RC clock ready */
    u32TimeOutCnt = __HIRC;
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_OSC22M_STB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLK_S_Msk)) | CLK_CLKSEL0_HCLK_S_HIRC;
    CLK->CLKDIV = (CLK->CLKDIV & (~CLK_CLKDIV_HCLK_N_Msk)) | CLK_CLKDIV_HCLK(1);

    /* Set PLL to Power-down mode */
    CLK->PLLCON |= CLK_PLLCON_PD_Msk;

    /* Enable external XTAL 12MHz clock */
    CLK->PWRCON |= CLK_PWRCON_XTL12M_EN_Msk;

    /* Waiting for external XTAL clock ready */
    u32TimeOutCnt = __HIRC;
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_XTL12M_STB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* Set core clock as PLL_CLOCK from PLL */
    CLK->PLLCON = PLLCON_SETTING;
    u32TimeOutCnt = __HIRC;
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_PLL_STB_Msk))
		if(--u32TimeOutCnt == 0) break;
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLK_S_Msk)) | CLK_CLKSEL0_HCLK_S_PLL;

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate PllClock, SystemCoreClock and CyclesPerUs automatically. */
    //SystemCoreClockUpdate();
    PllClock        = PLL_CLOCK;            // PLL
    SystemCoreClock = PLL_CLOCK / 1;        // HCLK
    CyclesPerUs     = PLL_CLOCK / 1000000;  // For CLK_SysTickDelay()

    /* Enable UART module clock */
    CLK->APBCLK |= (CLK_APBCLK_UART0_EN_Msk | CLK_APBCLK_UART1_EN_Msk);

    /* Select UART module clock source */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_UART_S_Msk)) | CLK_CLKSEL1_UART_S_HXT;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    /* Set GPB multi-function pins for UART1 RXD and TXD */

    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk |
                      SYS_GPB_MFP_PB4_Msk | SYS_GPB_MFP_PB5_Msk);

    SYS->GPB_MFP |= (SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD |
                     SYS_GPB_MFP_PB4_UART1_RXD | SYS_GPB_MFP_PB5_UART1_TXD);

}

void UART0_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART0 */
    SYS->IPRSTC2 |=  SYS_IPRSTC2_UART0_RST_Msk;
    SYS->IPRSTC2 &= ~SYS_IPRSTC2_UART0_RST_Msk;

    /* Configure UART0 and set UART0 Baudrate */
    UART0->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 115200);
    UART0->LCR = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
}

void UART1_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART1 */
    SYS->IPRSTC2 |=  SYS_IPRSTC2_UART1_RST_Msk;
    SYS->IPRSTC2 &= ~SYS_IPRSTC2_UART1_RST_Msk;

    /* Configure UART1 and set UART1 Baudrate */
    UART1->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 115200);
    UART1->LCR = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
}

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/

int32_t main(void)
{

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 */
    UART0_Init();

    /* Init UART1 */
    UART1_Init();

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/

    printf("\n\nCPU @ %dHz\n", SystemCoreClock);

    printf("\n\nUART Sample Program\n");

    /* UART sample LIN function */
    UART_FunctionTest();

    while(1);

}

/*---------------------------------------------------------------------------------------------------------*/
/* Compute Parity Value                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t GetParityValue(uint32_t u32id)
{
    uint32_t u32Res = 0, ID[6], p_Bit[2], mask = 0;

    for(mask = 0; mask < 6; mask++)
        ID[mask] = (u32id & (1 << mask)) >> mask;

    p_Bit[0] = (ID[0] + ID[1] + ID[2] + ID[4]) % 2;
    p_Bit[1] = (!((ID[1] + ID[3] + ID[4] + ID[5]) % 2));

    u32Res = u32id + (p_Bit[0] << 6) + (p_Bit[1] << 7);
    return (uint8_t)u32Res;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Compute CheckSum Value                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t ComputeChksumValue(uint8_t *pu8Buf, uint32_t u32ByteCnt)
{
    uint32_t i, CheckSum = 0;

    for(i = 0 ; i < u32ByteCnt; i++)
    {
        CheckSum += pu8Buf[i];
        if(CheckSum >= 256)
            CheckSum -= 255;
    }
    return (uint8_t)(255 - CheckSum);
}


/*---------------------------------------------------------------------------------------------------------*/
/*  LIN Function Test                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void TestItem(void)
{
    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  UART LIN Demo Function Select                                                 |\n");
    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  1 : Demo LIN Tx Function                                                      |\n");
    printf("|  2 : Demo LIN Rx Function                                                      |\n");
    printf("+--------------------------------------------------------------------------------+\n");
    printf("| Quit                                                                   - [ESC] |\n");
    printf("+--------------------------------------------------------------------------------+\n\n");
    printf("Please Select key (1~2): ");

    /* Forces a write of all user-space buffered data for the given output */
    fflush(stdout);
}

void UART_FunctionTest(void)
{
    uint32_t u32Item;

    do
    {
        TestItem();
        u32Item = (uint32_t)getchar();
        printf("%c\n", u32Item);

        switch(u32Item)
        {
            case '1':
                if(LIN_Tx_FunctionTest() < 0)
                    u32Item = 27;
                break;

            case '2':
                LIN_Rx_FunctionTest();
                break;

            default:
                break;
        }
    }
    while(u32Item != 27);

    printf("\nUART Demo Program End\n");

}

int32_t LIN_Tx_FunctionTest(void)
{
    uint32_t i, u32TimeOutCnt;

    /*
        The sample code will send a LIN header with ID is 0x30 and response field.
        The response field with 8 data bytes and checksum without including ID.
    */

    uint8_t au8TestPattern[9] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x0}; /* 8 data byte + 1 byte checksum */

    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  UART LIN Tx Function Test                                                     |\n");
    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  Description :                                                                 |\n");
    printf("|    The sample code will send a LIN header with ID is 0x30 and response field   |\n");
    printf("+--------------------------------------------------------------------------------+\n");

    /* Set UART Configuration, LIN Max Speed is 20K */
    UART1->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 9600);

    /* Switch back to LIN Function */
    UART1->FUN_SEL = UART_FUNC_SEL_LIN;

    /* Send Header */
    UART1->LIN_CTL = UART_LIN_CTL_LIN_LIN_PID(LIN_ID) | UART_LIN_CTL_LIN_HEAD_SEL_BREAK_SYNC_ID |
                     UART_LIN_CTL_LIN_BS_LEN(1) | UART_LIN_CTL_LIN_BKFL(12) | UART_LIN_CTL_LIN_IDPEN_Msk;
    /* LIN TX Send Header Enable */
    UART1->LIN_CTL |= UART_LIN_CTL_LIN_SHD_Msk;
    /* Wait until break field, sync field and ID field transfer completed */
    u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
    while((UART1->LIN_CTL & UART_LIN_CTL_LIN_SHD_Msk) == UART_LIN_CTL_LIN_SHD_Msk)
    {
        if(--u32TimeOutCnt == 0)
        {
            printf("Wait for UART LIN header transfer completed time-out!\n");
            return -1;
        }
    }

    /* Compute checksum without ID and fill checksum value to au8TestPattern[8] */
    au8TestPattern[8] = ComputeChksumValue(&au8TestPattern[0], 8);
    for(i = 0; i < 9; i++)
    {
        u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
        while(!(UART1->FSR & UART_FSR_TE_FLAG_Msk))    /* Wait Tx empty */
        {
            if(--u32TimeOutCnt == 0)
            {
                printf("Wait for UART Tx empty time-out!\n");
                return -1;
            }
        }

        UART1->THR = au8TestPattern[i]; /* Send UART Data from buffer */
    }

    printf("\n UART LIN Tx Function Demo End !!\n\n");

    return 0;
}

void LIN_Rx_FunctionTest(void)
{
    /*
        The sample code will detect LIN header(break+sync+ID) and receive response field.
        The response field with 8 data bytes and checksum without including ID.
    */

    uint32_t u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */

    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  UART LIN Rx Function Test                                                     |\n");
    printf("+--------------------------------------------------------------------------------+\n");
    printf("|  Description :                                                                 |\n");
    printf("|    The sample code will receive a LIN response field data                      |\n");
    printf("+--------------------------------------------------------------------------------+\n");

    /* Reset Rx FIFO */
    UART1->FCR |= UART_FCR_RFR_Msk;
    while(UART1->FCR & UART_FCR_RFR_Msk)
        if(--u32TimeOutCnt == 0) break;

    g_i32RxCounter = 0;

    /* Set UART Configuration, LIN Max Speed is 20K */
    UART1->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 9600);

    /* Switch back to LIN Function */
    UART1->FUN_SEL = UART_FUNC_SEL_LIN;

    /* Enable RDA and LIN Interrupt */
    UART_ENABLE_INT(UART1, (UART_IER_RDA_IEN_Msk | UART_IER_LIN_IEN_Msk));
    NVIC_EnableIRQ(UART1_IRQn);

    /* Receive Header: break+sync+ID */
    UART1->LIN_CTL = UART_LIN_CTL_LIN_LIN_PID(LIN_ID) | UART_LIN_CTL_LIN_HEAD_SEL_BREAK_SYNC_ID | UART_LIN_CTL_LINS_HDET_EN_Msk |
                     UART_LIN_CTL_LIN_IDPEN_Msk | UART_LIN_CTL_LIN_MUTE_EN_Msk | UART_LIN_CTL_LINS_EN_Msk;

    while(g_i32RxCounter < 9);

    printf("\n Receive Data:\n [ 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x 0x%x ] \n",
           g_u8ReceiveData[0], g_u8ReceiveData[1], g_u8ReceiveData[2], g_u8ReceiveData[3],
           g_u8ReceiveData[4], g_u8ReceiveData[5], g_u8ReceiveData[6], g_u8ReceiveData[7],
           g_u8ReceiveData[8]);

    NVIC_DisableIRQ(UART1_IRQn);

    printf("\n UART LIN Rx Function Demo End !!\n\n");
}
