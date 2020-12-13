/*
*********************************************************************************************************
*	                                  
*	ģ������ : GUI����������
*	�ļ����� : MainTask.c
*	��    �� : V1.0
*	˵    �� : ����emWin��RTOS��ʽ��ֲ���ԡ�
*              
*	�޸ļ�¼ :
*		�汾��   ����         ����          ˵��
*		V1.0    2016-07-16   Eric2013  	    �װ�    
*                                     
*	Copyright (C), 2015-2020, ���������� www.armfly.com
*
*********************************************************************************************************
*/
#include "MainTask.h"



/*
*********************************************************************************************************
*	                                     �궨��
*********************************************************************************************************
*/
#define ID_FRAMEWIN_0 (GUI_ID_USER + 0x00)
#define ID_BUTTON_0 (GUI_ID_USER + 0x01)
#define ID_SLIDER_0 (GUI_ID_USER + 0x02)


/*
*********************************************************************************************************
*	                       GUI_WIDGET_CREATE_INFO��������
*********************************************************************************************************
*/
static const GUI_WIDGET_CREATE_INFO _aDialogCreate[] = {
  { FRAMEWIN_CreateIndirect, "Framewin", ID_FRAMEWIN_0, 0, 0, 800, 480, 0, 0x64, 0 },
  { BUTTON_CreateIndirect, "Button", ID_BUTTON_0, 130, 28, 147, 35, 0, 0x0, 0 },
  { SLIDER_CreateIndirect, "Slider", ID_SLIDER_0, 133, 118, 137, 25, 0, 0x0, 0 },
};

/*
*********************************************************************************************************
*	�� �� ��: _cbDialog
*	����˵��: �Ի���ص�����		
*	��    ��: pMsg  �ص����� 
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void _cbDialog(WM_MESSAGE * pMsg) 
{
	WM_HWIN hItem;
	int     NCode;
	int     Id;


	switch (pMsg->MsgId) 
	{
		case WM_INIT_DIALOG:
			//
			// ��ʼ�� 'Framewin'
			//
			hItem = pMsg->hWin;
			FRAMEWIN_SetFont(hItem, GUI_FONT_32B_ASCII);
			FRAMEWIN_SetTextAlign(hItem, GUI_TA_HCENTER | GUI_TA_VCENTER);
			FRAMEWIN_SetText(hItem, "armfly");
		
			//
			// ��ʼ�� 'Button'
			//
			hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_0);
			BUTTON_SetFont(hItem, GUI_FONT_24B_ASCII);
			BUTTON_SetText(hItem, "armfly");
			break;
		
		case WM_NOTIFY_PARENT:
			Id    = WM_GetId(pMsg->hWinSrc);
			NCode = pMsg->Data.v;
			switch(Id) 
			{
				case ID_BUTTON_0:
					switch(NCode) 
					{
						case WM_NOTIFICATION_CLICKED:
							break;
						
						case WM_NOTIFICATION_RELEASED:
							break;
					}
					break;
				
				case ID_SLIDER_0:
					switch(NCode) 
					{
						case WM_NOTIFICATION_CLICKED:
							break;
						
						case WM_NOTIFICATION_RELEASED:
							break;
						
						case WM_NOTIFICATION_VALUE_CHANGED:
							break;
					}
					break;
			}
			break;
			
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

/*
*********************************************************************************************************
*	�� �� ��: CreateFramewin
*	����˵��: �����Ի���		
*	��    ��: ��
*	�� �� ֵ: ���ضԻ�����
*********************************************************************************************************
*/
WM_HWIN CreateFramewin(void) 
{
	WM_HWIN hWin;

	hWin = GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), _cbDialog, WM_HBKWIN, 0, 0);
	return hWin;
}


/*
*********************************************************************************************************
*	�� �� ��: MainTask
*	����˵��: GUI������
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void MainTask(void) 
{
	
	/* ��ʼ�� */
	GUI_Init();
	
	/*
	 ���ڶ໺��ʹ����ڴ��豸������˵��
	   1. ʹ�ܶ໺���ǵ��õ����º������û�Ҫ��LCDConf_Lin_Template.c�ļ��������˶໺�壬���ô˺�������Ч��
		  WM_MULTIBUF_Enable(1);
	   2. ����ʹ��ʹ���ڴ��豸�ǵ��ú�����WM_SetCreateFlags(WM_CF_MEMDEV);
	   3. ���emWin�����ö໺��ʹ����ڴ��豸��֧�֣���ѡһ���ɣ����������ѡ��ʹ�ö໺�壬ʵ��ʹ��
		  STM32F429BIT6 + 32λSDRAM + RGB565/RGB888ƽ̨���ԣ��໺�������Ч�Ľ��ʹ����ƶ����߻���ʱ��˺��
		  �У�����Ч����������ԣ�ͨ��ʹ�ܴ���ʹ���ڴ��豸���������ġ�
	   4. ����emWin����Ĭ���ǿ��������塣
	*/
	WM_MULTIBUF_Enable(1);
	
	/*
       ����У׼����Ĭ����ע�͵��ģ���������ҪУ׼������������У׼������û���ҪУ׼�������Ļ���ִ��
	   �˺������ɣ��Ὣ����У׼�������浽EEPROM���棬�Ժ�ϵͳ�ϵ���Զ���EEPROM������ء�
	*/
    //TOUCH_Calibration();
	
	/* ��ʾ�α� */
	GUI_CURSOR_Show();
	
	/* �����Ի��� */
	CreateFramewin();
		
	while(1) 
	{
		GUI_Delay(500);
	}
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
