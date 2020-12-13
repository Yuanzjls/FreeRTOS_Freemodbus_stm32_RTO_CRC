/**
  ******************************************************************************
  * @file    lcdconf.c
  * @author  MCD Application Team
  * @brief   This file implements the configuration for the GUI library
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "LCDConf.h"
#include "GUI_Private.h"

/** @addtogroup LCD CONFIGURATION
* @{
*/

/** @defgroup LCD CONFIGURATION
* @brief This file contains the LCD Configuration
* @{
*/

/** @defgroup LCD CONFIGURATION_Private_TypesDefinitions
* @{
*/
/**
* @}
*/

/** @defgroup LCD CONFIGURATION_Private_Defines
* @{
*/
#undef  LCD_SWAP_XY
#undef  LCD_MIRROR_Y

#define LCD_SWAP_XY     1
#define LCD_MIRROR_Y    1


#define XSIZE_PHYS      800
#define YSIZE_PHYS      480

#define ZONES           4
#define VSYNC           1
#define VBP             1
#define VFP             1
#define VACT            YSIZE_PHYS
#define HSYNC           1
#define HBP             1
#define HFP             1
#define HACT            XSIZE_PHYS/ZONES      /* !!!! SCREEN DIVIDED INTO 2 AREAS !!!! */


#define NUM_BUFFERS      3 /* Number of multiple buffers to be used */
#define NUM_VSCREENS     1 /* Number of virtual screens to be used */

#define BK_COLOR GUI_DARKBLUE

#undef  GUI_NUM_LAYERS
#define GUI_NUM_LAYERS 2

#define COLOR_CONVERSION_0 GUICC_M565
#define DISPLAY_DRIVER_0   GUIDRV_LIN_16


#if (GUI_NUM_LAYERS > 1)
#define COLOR_CONVERSION_1 GUICC_M1555I
#define DISPLAY_DRIVER_1   GUIDRV_LIN_16

#endif

#ifndef   XSIZE_PHYS
#error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
#error Physical Y size of display is not defined!
#endif
#ifndef   NUM_VSCREENS
#define NUM_VSCREENS 1
#else
#if (NUM_VSCREENS <= 0)
#error At least one screeen needs to be defined!
#endif
#endif
#if (NUM_VSCREENS > 1) && (NUM_BUFFERS > 1)
#error Virtual screens and multiple buffers are not allowed!
#endif


#define LCD_LAYER0_FRAME_BUFFER  ((uint32_t)0xC0400000)
#define LCD_LAYER1_FRAME_BUFFER  ((uint32_t)0xC0800000)

/**
* @}
*/

#define __DSI_MASK_TE()   (GPIOJ->AFR[0] &= (0xFFFFF0FFU))   /* Mask DSI TearingEffect Pin*/
#define __DSI_UNMASK_TE() (GPIOJ->AFR[0] |= ((uint32_t)(GPIO_AF13_DSI) << 8)) /* UnMask DSI TearingEffect Pin*/

/**
* @}
*/
/** @defgroup LCD CONFIGURATION_Private_Variables
* @{
*/

LTDC_HandleTypeDef    hltdc_disco;
DSI_HandleTypeDef     hdsi_disco;

uint8_t pPage[]       = {0x00, 0x00, 0x01, 0xDF}; /*   0 -> 479 */


uint8_t pCols[ZONES][4] =
{
#if (ZONES == 4 )
  {0x00, 0x00, 0x00, 0xC7}, /*   0 -> 199 */
  {0x00, 0xC8, 0x01, 0x8F}, /* 200 -> 399 */
  {0x01, 0x90, 0x02, 0x57}, /* 400 -> 599 */
  {0x02, 0x58, 0x03, 0x1F}, /* 600 -> 799 */
#elif (ZONES == 2 )
  {0x00, 0x00, 0x01, 0x8F}, /*   0 -> 399 */
  {0x01, 0x90, 0x03, 0x1F}  
#elif (ZONES == 1 )
  {0x00, 0x00, 0x03, 0x1F}, /*   0 -> 799 */
#endif  
};

static LCD_LayerPropTypedef          layer_prop[GUI_NUM_LAYERS];
volatile int32_t LCD_ActiveRegion    = 1;
volatile int32_t LCD_Refershing      = 0;

static const LCD_API_COLOR_CONV * apColorConvAPI[] =
{
  COLOR_CONVERSION_0,
#if GUI_NUM_LAYERS > 1
  COLOR_CONVERSION_1,
#endif
};

U32 LCD_Addr[GUI_NUM_LAYERS] = {LCD_LAYER0_FRAME_BUFFER, LCD_LAYER1_FRAME_BUFFER};

/**
* @}
*/

/** @defgroup LCD CONFIGURATION_Private_FunctionPrototypes
* @{
*/
static U32      GetPixelformat(U32 LayerIndex);
static U32      GetBufferSize(U32 LayerIndex);

static void     DMA2D_CopyBufferWithAlpha(U32 LayerIndex, void * pSrc, void * pDst, U32 xSize, U32 ySize, U32 OffLineSrc, U32 OffLineDst);
static void     DMA2D_CopyBuffer         (U32 LayerIndex, void * pSrc, void * pDst, U32 xSize, U32 ySize, U32 OffLineSrc, U32 OffLineDst);
static void     DMA2D_FillBuffer(U32 LayerIndex, void * pDst, U32 xSize, U32 ySize, U32 OffLine, U32 ColorIndex);

static void     LCD_LL_Init(void);
static void     LCD_LL_Reset(void);
static void     LTDC_Init(void);
void            BSP_LCD_MspInit(void);

static void     LCD_LL_CopyBuffer(int LayerIndex, int IndexSrc, int IndexDst);
static void     LCD_LL_CopyRect(int LayerIndex, int x0, int y0, int x1, int y1, int xSize, int ySize);
static void     LCD_LL_FillRect(int LayerIndex, int x0, int y0, int x1, int y1, U32 PixelIndex);
static void     LCD_LL_DrawBitmap8bpp(int LayerIndex, int x, int y, U8 const * p, int xSize, int ySize, int BytesPerLine);
void     LCD_LL_DrawBitmap16bpp(int LayerIndex, int x, int y, U16 const * p, int xSize, int ySize, int BytesPerLine);
static void     LCD_LL_DrawBitmap32bpp(int LayerIndex, int x, int y, U8 const * p,  int xSize, int ySize, int BytesPerLine);


/**
* @brief  DCS or Generic short/long write command
* @param  NbParams: Number of parameters. It indicates the write command mode:
*                 If inferior to 2, a long write command is performed else short.
* @param  pParams: Pointer to parameter values table.
* @retval HAL status
*/
void DSI_IO_WriteCmd(uint32_t NbrParams, uint8_t *pParams)
{
  if(NbrParams <= 1)
  {
    HAL_DSI_ShortWrite(&hdsi_disco, 0, DSI_DCS_SHORT_PKT_WRITE_P1, pParams[0], pParams[1]);
  }
  else
  {
    HAL_DSI_LongWrite(&hdsi_disco,  0, DSI_DCS_LONG_PKT_WRITE, NbrParams, pParams[NbrParams], pParams);
  } 
}

/**
  * @brief  BSP LCD Reset
  *         Hw reset the LCD DSI activating its XRES signal (active low for some time)
  *         and desactivating it later.
  */
static void LCD_LL_Reset(void)
{
  GPIO_InitTypeDef  gpio_init_structure;
  
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  
  /* Configure the GPIO on PK7 */
  gpio_init_structure.Pin   = GPIO_PIN_15;
  gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Pull  = GPIO_PULLUP;
  gpio_init_structure.Speed = GPIO_SPEED_HIGH;
  
  HAL_GPIO_Init(GPIOJ, &gpio_init_structure);
  
  /* Activate XRES active low */
  HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_15, GPIO_PIN_RESET);
  
  HAL_Delay(20); /* wait 20 ms */
  
  /* Desactivate XRES */
  HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_15, GPIO_PIN_SET);
  
  /* Wait after releasing RESX before sending commands */
  HAL_Delay(10); /* wait 10 ms */

}

/**
* @brief  Initialize the BSP LCD Msp.
* Application can surcharge if needed this function implementation
*/
void BSP_LCD_MspInit(void)
{
  /** @brief Enable the LTDC clock */
  __HAL_RCC_LTDC_CLK_ENABLE();
  
  /** @brief Toggle Sw reset of LTDC IP */
  __HAL_RCC_LTDC_FORCE_RESET();
  __HAL_RCC_LTDC_RELEASE_RESET();
  
  /** @brief Enable the DMA2D clock */
  __HAL_RCC_DMA2D_CLK_ENABLE();
  
  /** @brief Toggle Sw reset of DMA2D IP */
  __HAL_RCC_DMA2D_FORCE_RESET();
  __HAL_RCC_DMA2D_RELEASE_RESET();
  
  /** @brief Enable DSI Host and wrapper clocks */
  __HAL_RCC_DSI_CLK_ENABLE();
  
  /** @brief Soft Reset the DSI Host and wrapper */
  __HAL_RCC_DSI_FORCE_RESET();
  __HAL_RCC_DSI_RELEASE_RESET();
  
  /** @brief NVIC configuration for DSI interrupt that is now enabled */
  HAL_NVIC_SetPriority(DSI_IRQn, 0xF, 0);
  HAL_NVIC_EnableIRQ(DSI_IRQn);
}

/**
* @brief  Initialize the LTDC
* @param  None
* @retval None
*/
static void LTDC_Init(void)
{
  /* DeInit */
  HAL_LTDC_DeInit(&hltdc_disco);

  /* LTDC Config */
  /* Timing and polarity */
  hltdc_disco.Init.HorizontalSync = HSYNC;
  hltdc_disco.Init.VerticalSync = VSYNC;
  hltdc_disco.Init.AccumulatedHBP = HSYNC+HBP;
  hltdc_disco.Init.AccumulatedVBP = VSYNC+VBP;
  hltdc_disco.Init.AccumulatedActiveH = VSYNC+VBP+VACT;
  hltdc_disco.Init.AccumulatedActiveW = HSYNC+HBP+HACT;
  hltdc_disco.Init.TotalHeigh = VSYNC+VBP+VACT+VFP;
  hltdc_disco.Init.TotalWidth = HSYNC+HBP+HACT+HFP;

  /* background value */
  hltdc_disco.Init.Backcolor.Blue = 0;
  hltdc_disco.Init.Backcolor.Green = 0;
  hltdc_disco.Init.Backcolor.Red = 0;

  /* Polarity */
  hltdc_disco.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc_disco.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc_disco.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc_disco.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc_disco.Instance = LTDC;

  HAL_LTDC_Init(&hltdc_disco);
}

/**
* @brief  Initialize the LCD Controller.
* @param  None
* @retval None
*/
static void LCD_LL_Init(void) 
{   
  RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;
  GPIO_InitTypeDef          GPIO_Init_Structure;
  static DSI_CmdCfgTypeDef         CmdCfg;
  static DSI_LPCmdTypeDef          LPCmd;
  static DSI_PLLInitTypeDef        PLLInit;
  static DSI_PHY_TimerTypeDef      PhyTimings;

  /* Toggle Hardware Reset of the DSI LCD using
  * its XRES signal (active low) */
  LCD_LL_Reset();

  /* Call first MSP Initialize only in case of first initialization
  * This will set IP blocks LTDC, DSI and DMA2D
  * - out of reset
  * - clocked
  * - NVIC IRQ related to IP blocks enabled
  */
  BSP_LCD_MspInit();

#if GUI_NUM_LAYERS < 2
  /* LCD clock configuration */
  /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
  /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 375 Mhz */
  /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 375 MHz / 3 = 125 MHz */
  /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_2 = 125 MHz / 2 = 62.5 MHz */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 375;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 3;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
#else
  /* LCD clock configuration */
  /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
  /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 280 Mhz */
  /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 280 MHz / 7 = 40 MHz */
  /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_2 = 40 MHz / 2 = 20 MHz */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 280;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 7;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
#endif
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

  /* Base address of DSI Host/Wrapper registers to be set before calling De-Init */
  hdsi_disco.Instance = DSI;

  HAL_DSI_DeInit(&(hdsi_disco));

  PLLInit.PLLNDIV  = 100;
  PLLInit.PLLIDF   = DSI_PLL_IN_DIV5;
  PLLInit.PLLODF  = DSI_PLL_OUT_DIV1;

  hdsi_disco.Init.NumberOfLanes = DSI_TWO_DATA_LANES;
  hdsi_disco.Init.TXEscapeCkdiv = 0x4;
  HAL_DSI_Init(&(hdsi_disco), &(PLLInit));

  /* Configure the DSI for Command mode */
  CmdCfg.VirtualChannelID      = 0;
  CmdCfg.HSPolarity            = DSI_HSYNC_ACTIVE_HIGH;
  CmdCfg.VSPolarity            = DSI_VSYNC_ACTIVE_HIGH;
  CmdCfg.DEPolarity            = DSI_DATA_ENABLE_ACTIVE_HIGH;
  CmdCfg.ColorCoding           = DSI_RGB565;
  CmdCfg.CommandSize           = HACT;
  CmdCfg.TearingEffectSource   = DSI_TE_EXTERNAL;
  CmdCfg.TearingEffectPolarity = DSI_TE_RISING_EDGE;
  CmdCfg.VSyncPol              = DSI_VSYNC_FALLING;
  CmdCfg.AutomaticRefresh      = DSI_AR_DISABLE;
  CmdCfg.TEAcknowledgeRequest  = DSI_TE_ACKNOWLEDGE_ENABLE;
  HAL_DSI_ConfigAdaptedCommandMode(&hdsi_disco, &CmdCfg);

  LPCmd.LPGenShortWriteNoP    = DSI_LP_GSW0P_ENABLE;
  LPCmd.LPGenShortWriteOneP   = DSI_LP_GSW1P_ENABLE;
  LPCmd.LPGenShortWriteTwoP   = DSI_LP_GSW2P_ENABLE;
  LPCmd.LPGenShortReadNoP     = DSI_LP_GSR0P_ENABLE;
  LPCmd.LPGenShortReadOneP    = DSI_LP_GSR1P_ENABLE;
  LPCmd.LPGenShortReadTwoP    = DSI_LP_GSR2P_ENABLE;
  LPCmd.LPGenLongWrite        = DSI_LP_GLW_ENABLE;
  LPCmd.LPDcsShortWriteNoP    = DSI_LP_DSW0P_ENABLE;
  LPCmd.LPDcsShortWriteOneP   = DSI_LP_DSW1P_ENABLE;
  LPCmd.LPDcsShortReadNoP     = DSI_LP_DSR0P_ENABLE;
  LPCmd.LPDcsLongWrite        = DSI_LP_DLW_ENABLE;
  HAL_DSI_ConfigCommand(&hdsi_disco, &LPCmd);
  
  /* Initialize LTDC */
  LTDC_Init();

  /* Start DSI */
  HAL_DSI_Start(&(hdsi_disco));

  /* Initialize the OTM8009A LCD Display IC Driver (KoD LCD IC Driver)
  *  depending on configuration set in 'hdsivideo_handle'.
  */

    /* Send Display off DCS Command to display */
  HAL_DSI_ShortWrite(&(hdsi_disco),
                     0,
                     DSI_DCS_SHORT_PKT_WRITE_P1,
                     OTM8009A_CMD_DISPOFF,
                     0x00);

  /* Configure DSI PHY HS2LP and LP2HS timings */
  PhyTimings.ClockLaneHS2LPTime = 35;
  PhyTimings.ClockLaneLP2HSTime = 35;
  PhyTimings.DataLaneHS2LPTime = 35;
  PhyTimings.DataLaneLP2HSTime = 35;
  PhyTimings.DataLaneMaxReadTime = 0;
  PhyTimings.StopWaitTime = 10;
  HAL_DSI_ConfigPhyTimer(&hdsi_disco, &PhyTimings);

  OTM8009A_Init(OTM8009A_FORMAT_RBG565, LCD_ORIENTATION_LANDSCAPE);

  LPCmd.LPGenShortWriteNoP    = DSI_LP_GSW0P_DISABLE;
  LPCmd.LPGenShortWriteOneP   = DSI_LP_GSW1P_DISABLE;
  LPCmd.LPGenShortWriteTwoP   = DSI_LP_GSW2P_DISABLE;
  LPCmd.LPGenShortReadNoP     = DSI_LP_GSR0P_DISABLE;
  LPCmd.LPGenShortReadOneP    = DSI_LP_GSR1P_DISABLE;
  LPCmd.LPGenShortReadTwoP    = DSI_LP_GSR2P_DISABLE;
  LPCmd.LPGenLongWrite        = DSI_LP_GLW_DISABLE;
  LPCmd.LPDcsShortWriteNoP    = DSI_LP_DSW0P_DISABLE;
  LPCmd.LPDcsShortWriteOneP   = DSI_LP_DSW1P_DISABLE;
  LPCmd.LPDcsShortReadNoP     = DSI_LP_DSR0P_DISABLE;
  LPCmd.LPDcsLongWrite        = DSI_LP_DLW_DISABLE;
  HAL_DSI_ConfigCommand(&hdsi_disco, &LPCmd);

  HAL_DSI_ConfigFlowControl(&hdsi_disco, DSI_FLOW_CONTROL_BTA);


  /* Enable GPIOJ clock */
  __HAL_RCC_GPIOJ_CLK_ENABLE();

  /* Configure DSI_TE pin from MB1166 : Tearing effect on separated GPIO from KoD LCD */
  /* that is mapped on GPIOJ2 as alternate DSI function (DSI_TE)                      */
  /* This pin is used only when the LCD and DSI link is configured in command mode    */
  /* Not used in DSI Video mode.                                                      */
  GPIO_Init_Structure.Pin       = GPIO_PIN_2;
  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
  GPIO_Init_Structure.Pull      = GPIO_NOPULL;
  GPIO_Init_Structure.Speed     = GPIO_SPEED_HIGH;
  GPIO_Init_Structure.Alternate = GPIO_AF13_DSI;
  HAL_GPIO_Init(GPIOJ, &GPIO_Init_Structure);

  DMA2D_FillBuffer(0, (U32 *)LCD_LAYER0_FRAME_BUFFER, XSIZE_PHYS,YSIZE_PHYS, 0, 0x00);
  
  /* Refresh the display */
  HAL_DSI_Refresh(&hdsi_disco);
}


/**
* @brief  Initialize the LCD layers.
* @param  LayerIndex : layer Index.
* @retval None
*/
static void LCD_LL_LayerInit(U32 LayerIndex, U32 address)
{
  LTDC_LayerCfgTypeDef  Layercfg;
  
  /* Layer Init */
  Layercfg.WindowX0 = 0;
  Layercfg.WindowX1 = HACT;
  Layercfg.WindowY0 = 0;
  Layercfg.WindowY1 = YSIZE_PHYS;
  Layercfg.PixelFormat = GetPixelformat(LayerIndex);
  Layercfg.FBStartAdress = address;
  Layercfg.Alpha = 255;
  Layercfg.Alpha0 = 0;
  Layercfg.Backcolor.Blue = 0;
  Layercfg.Backcolor.Green = 0;
  Layercfg.Backcolor.Red = 0;
  Layercfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  Layercfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  Layercfg.ImageWidth = HACT;
  Layercfg.ImageHeight = YSIZE_PHYS;
  
  HAL_LTDC_ConfigLayer(&hltdc_disco, &Layercfg, LayerIndex);
}



/**
* @brief  Return Pixel format for a given layer
* @param  LayerIndex : Layer Index
* @retval Status ( 0 : 0k , 1: error)
*/
static U32 GetPixelformat(U32 LayerIndex)
{
  if (LayerIndex == 0)
  {
    return LTDC_PIXEL_FORMAT_RGB565;
  }
  else
  {
    return LTDC_PIXEL_FORMAT_ARGB1555;
  }
}

/**
* @brief  Return Pixel format for a given layer
* @param  LayerIndex : Layer Index
* @retval Status ( 0 : 0k , 1: error)
*/
static void DMA2D_CopyBuffer(U32 LayerIndex, void * pSrc, void * pDst, U32 xSize, U32 ySize, U32 OffLineSrc, U32 OffLineDst)
{
  U32 PixelFormat;

  PixelFormat = GetPixelformat(LayerIndex);
  DMA2D->CR      = 0x00000000UL | (1 << 9);

  /* Set up pointers */
  DMA2D->FGMAR   = (U32)pSrc;
  DMA2D->OMAR    = (U32)pDst;
  DMA2D->FGOR    = OffLineSrc;
  DMA2D->OOR     = OffLineDst;
  
  /* Set up pixel format */
  DMA2D->FGPFCCR = PixelFormat;
  
  /*  Set up size */
  DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize;
  
  DMA2D->CR     |= DMA2D_CR_START;
  
  /* Wait until transfer is done */
  while (DMA2D->CR & DMA2D_CR_START)
  {
  }
}

/*********************************************************************
*
*       CopyBuffer
*/
static void DMA2D_CopyBufferWithAlpha(U32 LayerIndex, void * pSrc, void * pDst, U32 xSize, U32 ySize, U32 OffLineSrc, U32 OffLineDst)
{
  uint32_t PixelFormat;

  PixelFormat = GetPixelformat(LayerIndex);
  DMA2D->CR      = 0x00000000UL | (1 << 9) | (0x2 << 16);

  /* Set up pointers */
  DMA2D->FGMAR   = (U32)pSrc;
  DMA2D->OMAR    = (U32)pDst;
  DMA2D->BGMAR   = (U32)pDst;
  DMA2D->FGOR    = OffLineSrc;
  DMA2D->OOR     = OffLineDst;
  DMA2D->BGOR     = OffLineDst;
  
  /* Set up pixel format */
  DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888;
  DMA2D->BGPFCCR = PixelFormat;
  DMA2D->OPFCCR = PixelFormat;
  
  /*  Set up size */
  DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize;
  
  DMA2D->CR     |= DMA2D_CR_START;
  
  /* Wait until transfer is done */
  while (DMA2D->CR & DMA2D_CR_START)
  {
  }
}

/**
* @brief  Fill Buffer
* @param  LayerIndex : Layer Index
* @param  pDst:        pointer to destination
* @param  xSize:       X size
* @param  ySize:       Y size
* @param  OffLine:     offset after each line
* @param  ColorIndex:  color to be used.
* @retval None.
*/
static void DMA2D_FillBuffer(U32 LayerIndex, void * pDst, U32 xSize, U32 ySize, U32 OffLine, U32 ColorIndex)
{
  
  U32 PixelFormat;

  PixelFormat = GetPixelformat(LayerIndex);
  
  /* Set up mode */
  DMA2D->CR      = 0x00030000UL | (1 << 9);
  DMA2D->OCOLR   = ColorIndex;
  
  /* Set up pointers */
  DMA2D->OMAR    = (U32)pDst;
  
  /* Set up offsets */
  DMA2D->OOR     = OffLine;
  
  /* Set up pixel format */
  DMA2D->OPFCCR  = PixelFormat;
  
  /*  Set up size */
  DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize;
  
  DMA2D->CR     |= DMA2D_CR_START;
  
  /* Wait until transfer is done */
  while (DMA2D->CR & DMA2D_CR_START)
  {
  }
}


/**
* @brief  Get buffer size
* @param  LayerIndex : Layer Index
* @retval None.
*/
static U32 GetBufferSize(U32 LayerIndex)
{
  return (layer_prop[LayerIndex].xSize * layer_prop[LayerIndex].ySize * layer_prop[LayerIndex].BytesPerPixel);
}

/**
* @brief  Customized copy buffer
* @param  LayerIndex : Layer Index
* @param  IndexSrc:    index source
* @param  IndexDst:    index destination
* @retval None.
*/
static void LCD_LL_CopyBuffer(int LayerIndex, int IndexSrc, int IndexDst) {
  U32 BufferSize, AddrSrc, AddrDst;
  
  BufferSize = GetBufferSize(LayerIndex);
  AddrSrc    = layer_prop[LayerIndex].address + BufferSize * IndexSrc;
  AddrDst    = layer_prop[LayerIndex].address + BufferSize * IndexDst;
  DMA2D_CopyBuffer(LayerIndex, (void *)AddrSrc, (void *)AddrDst, layer_prop[LayerIndex].xSize, layer_prop[LayerIndex].ySize, 0, 0);
  layer_prop[LayerIndex].buffer_index = IndexDst;
}

/**
* @brief  Copy rectangle
* @param  LayerIndex : Layer Index
* @param  x0:          X0 position
* @param  y0:          Y0 position
* @param  x1:          X1 position
* @param  y1:          Y1 position
* @param  xSize:       X size.
* @param  ySize:       Y size.
* @retval None.
*/
static void LCD_LL_CopyRect(int LayerIndex, int x0, int y0, int x1, int y1, int xSize, int ySize)
{
  U32 BufferSize, AddrSrc, AddrDst;

  BufferSize = GetBufferSize(LayerIndex);
  AddrSrc = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].pending_buffer + (y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].pending_buffer + (y1 * layer_prop[LayerIndex].xSize + x1) * layer_prop[LayerIndex].BytesPerPixel;
  DMA2D_CopyBuffer(LayerIndex, (void *)AddrSrc, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].xSize - xSize, 0);
}

/**
* @brief  Fill rectangle
* @param  LayerIndex : Layer Index
* @param  x0:          X0 position
* @param  y0:          Y0 position
* @param  x1:          X1 position
* @param  y1:          Y1 position
* @param  PixelIndex:  Pixel index.
* @retval None.
*/
static void LCD_LL_FillRect(int LayerIndex, int x0, int y0, int x1, int y1, U32 PixelIndex)
{
  U32 BufferSize, AddrDst;
  int xSize, ySize;
  
  if (GUI_GetDrawMode() == GUI_DM_XOR) 
  {
    LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, NULL);
    LCD_FillRect(x0, y0, x1, y1);
    LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, (void(*)(void))LCD_LL_FillRect);
  }
  else
  {
    xSize = x1 - x0 + 1;
    ySize = y1 - y0 + 1;
    BufferSize = GetBufferSize(LayerIndex);
    AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;
    DMA2D_FillBuffer(LayerIndex, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].xSize - xSize, PixelIndex);
  }
}

/**
* @brief  Draw indirect color bitmap
* @param  pSrc: pointer to the source
* @param  pDst: pointer to the destination
* @param  OffSrc: offset source
* @param  OffDst: offset destination
* @param  PixelFormatDst: pixel format for destination
* @param  xSize: X size
* @param  ySize: Y size
* @retval None
*/
static void DMA2D_DrawBitmapL8(void * pSrc, void * pDst,  U32 OffSrc, U32 OffDst, U32 PixelFormatDst, U32 xSize, U32 ySize)
{
  /* Set up mode */
  DMA2D->CR      = 0x00010000UL | (1 << 9);         /* Control Register (Memory to memory with pixel format conversion and TCIE) */

  /* Set up pointers */
  DMA2D->FGMAR   = (U32)pSrc;                       /* Foreground Memory Address Register (Source address) */
  DMA2D->OMAR    = (U32)pDst;                       /* Output Memory Address Register (Destination address) */

  /* Set up offsets */
  DMA2D->FGOR    = OffSrc;                          /* Foreground Offset Register (Source line offset) */
  DMA2D->OOR     = OffDst;                          /* Output Offset Register (Destination line offset) */

  /* Set up pixel format */
  DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_L8;             /* Foreground PFC Control Register (Defines the input pixel format) */
  DMA2D->OPFCCR  = PixelFormatDst;                  /* Output PFC Control Register (Defines the output pixel format) */

  /* Set up size */
  DMA2D->NLR     = (U32)(xSize << 16) | ySize;      /* Number of Line Register (Size configuration of area to be transfered) */

  /* Execute operation */
  DMA2D->CR     |= DMA2D_CR_START;                               /* Start operation */

  /* Wait until transfer is done */
  while (DMA2D->CR & DMA2D_CR_START)
  {
  }
}

/**
* @brief  Draw 16bpp bitmap file
* @param  LayerIndex: Layer Index
* @param  x:          X position
* @param  y:          Y position
* @param  p:          pointer to destination address
* @param  xSize:      X size
* @param  ySize:      Y size
* @param  BytesPerLine
* @retval None
*/
void LCD_LL_DrawBitmap16bpp(int LayerIndex, int x, int y, U16 const * p, int xSize, int ySize, int BytesPerLine)
{
  U32 BufferSize, AddrDst;
  int OffLineSrc, OffLineDst;

  BufferSize = GetBufferSize(LayerIndex);
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = (BytesPerLine / 2) - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  DMA2D_CopyBuffer(LayerIndex, (void *)p, (void *)AddrDst, xSize, ySize, OffLineSrc, OffLineDst);
}

static void LCD_LL_DrawBitmap32bpp(int LayerIndex, int x, int y, U8 const * p, int xSize, int ySize, int BytesPerLine)
{
  U32 BufferSize, AddrDst;
  int OffLineSrc, OffLineDst;

  BufferSize = GetBufferSize(LayerIndex);
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = (BytesPerLine / 4) - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  DMA2D_CopyBufferWithAlpha(LayerIndex, (void *)p, (void *)AddrDst, xSize, ySize, OffLineSrc, OffLineDst);
}

/**
* @brief  Draw 8bpp bitmap file
* @param  LayerIndex: Layer Index
* @param  x:          X position
* @param  y:          Y position
* @param  p:          pointer to destination address
* @param  xSize:      X size
* @param  ySize:      Y size
* @param  BytesPerLine
* @retval None
*/
static void LCD_LL_DrawBitmap8bpp(int LayerIndex, int x, int y, U8 const * p, int xSize, int ySize, int BytesPerLine)
{
  U32 BufferSize, AddrDst;
  int OffLineSrc, OffLineDst;
  U32 PixelFormat;

  BufferSize = GetBufferSize(LayerIndex);
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = BytesPerLine - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  PixelFormat = GetPixelformat(LayerIndex);
  DMA2D_DrawBitmapL8((void *)p, (void *)AddrDst, OffLineSrc, OffLineDst, PixelFormat, xSize, ySize);
}


/**
* LCD_SetUpdateRegion
*/
void LCD_SetUpdateRegion(int idx)
{
  HAL_DSI_LongWrite(&hdsi_disco, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pCols[idx]);
}

/**
* @brief  Tearing Effect DSI callback.
* @param  hdsi: pointer to a DSI_HandleTypeDef structure that contains
*               the configuration information for the DSI.
* @retval None
*/
void HAL_DSI_TearingEffectCallback(DSI_HandleTypeDef *hdsi)
{
  uint32_t index = 0;

  if(!LCD_Refershing)
  {
    for(index = 0; index < GUI_NUM_LAYERS; index ++)
    {
      if(layer_prop[index].pending_buffer >= 0)
      {
        GUI_MULTIBUF_ConfirmEx(index,  layer_prop[index].pending_buffer);
        layer_prop[index].pending_buffer = -1;
      } 
    }
    LCD_Refershing = 1;
    LCD_ActiveRegion = 1;
    HAL_DSI_Refresh(hdsi);
  }
}  

/**
* @brief  End of Refresh DSI callback.
* @param  hdsi: pointer to a DSI_HandleTypeDef structure that contains
*               the configuration information for the DSI.
* @retval None
*/
void HAL_DSI_EndOfRefreshCallback(DSI_HandleTypeDef *hdsi)
{
  uint32_t index = 0;

  if(LCD_ActiveRegion < ZONES )
  {
    LCD_Refershing = 1;
    /* Disable DSI Wrapper */
    __HAL_DSI_WRAPPER_DISABLE(hdsi);
    for(index = 0; index < GUI_NUM_LAYERS; index ++)
    {
      /* Update LTDC configuaration */
      LTDC_LAYER(&hltdc_disco, index)->CFBAR  = LCD_Addr[index] + LCD_ActiveRegion  * HACT * 2;
    }
    __HAL_LTDC_RELOAD_CONFIG(&hltdc_disco);
    __HAL_DSI_WRAPPER_ENABLE(hdsi);
    LCD_SetUpdateRegion(LCD_ActiveRegion++);
    /* Refresh the right part of the display */
    HAL_DSI_Refresh(hdsi);

  }
  else
  {
    LCD_Refershing = 0;

    __HAL_DSI_WRAPPER_DISABLE(&hdsi_disco);
    for(index = 0; index < GUI_NUM_LAYERS; index ++)
    {
      LTDC_LAYER(&hltdc_disco, index)->CFBAR  = LCD_Addr[index];
    }

    __HAL_LTDC_RELOAD_CONFIG(&hltdc_disco);
    __HAL_DSI_WRAPPER_ENABLE(&hdsi_disco);
    LCD_SetUpdateRegion(0); 
  }
}

/**
* Request TE at scanline.
*/
void LCD_ReqTear(void)
{
  static uint8_t ScanLineParams[2];
#if (ZONES == 4 )
  uint16_t scanline = 283;
#elif (ZONES == 2 )
  uint16_t scanline = 200;
#endif
  ScanLineParams[0] = scanline >> 8;
  ScanLineParams[1] = scanline & 0x00FF;
  
  HAL_DSI_LongWrite(&hdsi_disco, 0, DSI_DCS_LONG_PKT_WRITE, 2, 0x44, ScanLineParams);
  /* set_tear_on */
  HAL_DSI_ShortWrite(&hdsi_disco, 0, DSI_DCS_SHORT_PKT_WRITE_P1, OTM8009A_CMD_TEEON, 0x00);
}




/**
* @brief  Called during the initialization process in order to set up the
*          display driver configuration
* @param  None
* @retval None
*/
void LCD_X_Config(void) 
{
  U32 i;
  
  /* At first initialize use of multiple buffers on demand */
#if (NUM_BUFFERS > 1)
  for (i = 0; i < GUI_NUM_LAYERS; i++) 
  {
    GUI_MULTIBUF_ConfigEx(i, NUM_BUFFERS);
  }
#endif
  
  /* Set display driver and color conversion for 1st layer */
  GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER_0, COLOR_CONVERSION_0, 0, 0);
  
  /* Set size of 1st layer */
  if (LCD_GetSwapXYEx(0)) {
    LCD_SetSizeEx (0, YSIZE_PHYS, XSIZE_PHYS);
    LCD_SetVSizeEx(0, YSIZE_PHYS * NUM_VSCREENS, XSIZE_PHYS);
  } else {
    LCD_SetSizeEx (0, XSIZE_PHYS, YSIZE_PHYS);
    LCD_SetVSizeEx(0, XSIZE_PHYS, YSIZE_PHYS * NUM_VSCREENS);
  }
#if (GUI_NUM_LAYERS > 1)

  /* Set display driver and color conversion for 2nd layer */
  GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER_1, COLOR_CONVERSION_1, 0, 1);

  /* Set size of 2nd layer */
  if (LCD_GetSwapXYEx(1)) {
    LCD_SetSizeEx (1, YSIZE_PHYS, XSIZE_PHYS);
    LCD_SetVSizeEx(1, YSIZE_PHYS * NUM_VSCREENS, XSIZE_PHYS);
  } else {
    LCD_SetSizeEx (1, XSIZE_PHYS, YSIZE_PHYS);
    LCD_SetVSizeEx(1, XSIZE_PHYS, YSIZE_PHYS * NUM_VSCREENS);
  }
#endif


  /*Initialize GUI Layer structure */
  layer_prop[0].address = LCD_LAYER0_FRAME_BUFFER;

#if (GUI_NUM_LAYERS > 1)
  layer_prop[1].address = LCD_LAYER1_FRAME_BUFFER;
#endif
   /* Setting up VRam address and custom functions for CopyBuffer-, CopyRect- and FillRect operations */
  for (i = 0; i < GUI_NUM_LAYERS; i++) 
  {

    layer_prop[i].pColorConvAPI = (LCD_API_COLOR_CONV *)apColorConvAPI[i];

    layer_prop[i].pending_buffer = -1;

    /* Remember color depth for further operations */
    layer_prop[i].BytesPerPixel = LCD_GetBitsPerPixelEx(i) >> 3;

    /* Set custom functions for several operations */
    LCD_SetDevFunc(i, LCD_DEVFUNC_COPYBUFFER, (void(*)(void))LCD_LL_CopyBuffer);
    LCD_SetDevFunc(i, LCD_DEVFUNC_COPYRECT,   (void(*)(void))LCD_LL_CopyRect);

    /* Filling via DMA2D does only work with 16bpp or more */
    LCD_SetDevFunc(i, LCD_DEVFUNC_FILLRECT, (void(*)(void))LCD_LL_FillRect);
    LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_8BPP, (void(*)(void))LCD_LL_DrawBitmap8bpp);
    LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_16BPP, (void(*)(void))LCD_LL_DrawBitmap16bpp);
    LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_32BPP, (void(*)(void))LCD_LL_DrawBitmap32bpp);

    /* Set VRAM address */
    LCD_SetVRAMAddrEx(i, (void *)(layer_prop[i].address));
  }
  
  LCD_LL_Init ();
  
  LCD_LL_LayerInit(0, LCD_LAYER0_FRAME_BUFFER);
#if (GUI_NUM_LAYERS > 1)
  LCD_LL_LayerInit(1, LCD_LAYER1_FRAME_BUFFER);
#endif
  
  HAL_DSI_LongWrite(&hdsi_disco, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pCols[0]);
  HAL_DSI_LongWrite(&hdsi_disco, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_PASET, pPage);

  
  /* Update pitch : the draw is done on the whole physical X Size */
  HAL_LTDC_SetPitch(&hltdc_disco, XSIZE_PHYS, 0);
#if (GUI_NUM_LAYERS > 1)
  HAL_LTDC_SetPitch(&hltdc_disco, XSIZE_PHYS, 1);
#endif
  
  HAL_Delay(50);

  LCD_ReqTear();

      /* Send Display off DCS Command to display */
  HAL_DSI_ShortWrite(&(hdsi_disco),
                     0,
                     DSI_DCS_SHORT_PKT_WRITE_P1,
                     OTM8009A_CMD_DISPON,
                     0x00);
}

/**
* @brief  This function is called by the display driver for several purposes.
*         To support the according task the routine needs to be adapted to
*         the display controller. Please note that the commands marked with
*         'optional' are not cogently required and should only be adapted if
*         the display controller supports these features
* @param  LayerIndex: Index of layer to be configured
* @param  Cmd       :Please refer to the details in the switch statement below
* @param  pData     :Pointer to a LCD_X_DATA structure
* @retval Status (-1 : Error,  0 : Ok)
*/
int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData)
{
  int r = 0;
  U32 addr;
  int xPos, yPos;
  U32 Color;
  
  LCD_X_SHOWBUFFER_INFO * p;
  switch (Cmd)
  {
  case LCD_X_SETORG:
    addr = layer_prop[LayerIndex].address + ((LCD_X_SETORG_INFO *)pData)->yPos * layer_prop[LayerIndex].xSize * layer_prop[LayerIndex].BytesPerPixel;
    HAL_LTDC_SetAddress(&hltdc_disco, addr, LayerIndex);
    break;

  case LCD_X_SHOWBUFFER:

    p = (LCD_X_SHOWBUFFER_INFO *)pData;
    LCD_Addr[LayerIndex] = layer_prop[LayerIndex].address + \
      layer_prop[LayerIndex].xSize * layer_prop[LayerIndex].ySize * layer_prop[LayerIndex].BytesPerPixel * p->Index;
    layer_prop[LayerIndex].pending_buffer = p->Index;
    break;

  case LCD_X_ON:
    __HAL_LTDC_ENABLE(&hltdc_disco);
    break;

  case LCD_X_OFF:
    __HAL_LTDC_DISABLE(&hltdc_disco);
    break;

  case LCD_X_SETVIS:
    if(((LCD_X_SETVIS_INFO *)pData)->OnOff  == ENABLE )
    {
      __HAL_DSI_WRAPPER_DISABLE(&hdsi_disco);
      __HAL_LTDC_LAYER_ENABLE(&hltdc_disco, LayerIndex);
      __HAL_DSI_WRAPPER_ENABLE(&hdsi_disco);
    }
    else
    {
      __HAL_DSI_WRAPPER_DISABLE(&hdsi_disco);
      __HAL_LTDC_LAYER_DISABLE(&hltdc_disco, LayerIndex);
      __HAL_DSI_WRAPPER_ENABLE(&hdsi_disco);

    }
    __HAL_LTDC_RELOAD_CONFIG(&hltdc_disco);

    HAL_DSI_Refresh(&hdsi_disco);
    break;

  case LCD_X_SETPOS:
    HAL_LTDC_SetWindowPosition(&hltdc_disco,
                               ((LCD_X_SETPOS_INFO *)pData)->xPos,
                               ((LCD_X_SETPOS_INFO *)pData)->yPos,
                               LayerIndex);
    break;

  case LCD_X_SETSIZE:
    GUI_GetLayerPosEx(LayerIndex, &xPos, &yPos);
    layer_prop[LayerIndex].xSize = ((LCD_X_SETSIZE_INFO *)pData)->xSize;
    layer_prop[LayerIndex].ySize = ((LCD_X_SETSIZE_INFO *)pData)->ySize;
    HAL_LTDC_SetWindowPosition(&hltdc_disco, xPos, yPos, LayerIndex);
    break;

  case LCD_X_SETALPHA:
    HAL_LTDC_SetAlpha(&hltdc_disco, ((LCD_X_SETALPHA_INFO *)pData)->Alpha, LayerIndex);
    break;

  case LCD_X_SETCHROMAMODE:
    if(((LCD_X_SETCHROMAMODE_INFO *)pData)->ChromaMode != 0)
    {
      HAL_LTDC_EnableColorKeying(&hltdc_disco, LayerIndex);
    }
    else
    {
      HAL_LTDC_DisableColorKeying(&hltdc_disco, LayerIndex);
    }
    break;

  case LCD_X_SETCHROMA:

    Color = ((((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0xFF0000) >> 16) |\
      (((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0x00FF00) |\
        ((((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0x0000FF) << 16);

    HAL_LTDC_ConfigColorKeying(&hltdc_disco, Color, LayerIndex);
    break;

  default:
    r = -1;
  }
  return r;
}


/**
* @brief  This function handles DSI Handler.
* @param  None
* @retval None
*/
void DSI_IRQHandler(void)
{
  HAL_DSI_IRQHandler(&hdsi_disco);
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

