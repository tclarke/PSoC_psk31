/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include "varicode.h"

#define USBFS_DEVICE    (0u)

uint16 vcode = 0;
int zcnt = 2;

void send_psk_1(void)
{
    ; // do nothing, just wait a tick
}

void send_psk_0(void)
{
    // Change phase
    uint8 val = Phase_Read();
    if (val == 0)  Phase_Write(0xff);
    else           Phase_Write(0x00);
}

void send_ascii(uint8 ascii)
{
    // lookup varicode
    for (;;) {
        uint8 istate = CyEnterCriticalSection();
        if (zcnt == 2) {
            CyExitCriticalSection(istate);
            break;
        }
        CyExitCriticalSection(istate);
        CyDelay(1);
    }
//    SND_CTL_Write(0xff);
    //OutputSwitch_FastSelect(0);
    uint8 istate = CyEnterCriticalSection();
    vcode = varicode[ascii];
    zcnt = 0;
    CyExitCriticalSection(istate);
    for (;;) {
        uint8 istate = CyEnterCriticalSection();
        if (zcnt == 2) {
            CyExitCriticalSection(istate);
            break;
        }
        CyExitCriticalSection(istate);
        CyDelay(1);
    }
    //OutputSwitch_Disconnect(0);
//    SND_CTL_Write(0x00);
}

CY_ISR(BaudInterrupt)
{
	/* Read Status register in order to clear the sticky Terminal Count (TC) bit 
	 * in the status register. Note that the function is not called, but rather 
	 * the status is read directly.
	 */
   	BaudTimer_STATUS;
    
    uint8 istate = CyEnterCriticalSection();
    if (zcnt < 2) {
        if (vcode & 0x8000) {
            send_psk_1();
            zcnt = 0;
        } else {
            send_psk_0();
            zcnt++;
        }
        vcode <<= 1;
    }
    CyExitCriticalSection(istate);
}

int main(void)
{
    uint8 in_buffer[64];
    
    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    Baud_Isr_StartEx(BaudInterrupt);
    BaudTimer_Start();
    USBUART_1_Start(USBFS_DEVICE, USBUART_1_5V_OPERATION);
    SineDAC_Start();
    //OutputSwitch_Start();
    
    for(;;)
    {
        /* Place your application code here. */
        
        if (0u != USBUART_1_IsConfigurationChanged()) {
            if (0u != USBUART_1_GetConfiguration()) {
                USBUART_1_CDC_Init();
                USBUART_1_PutString("Welcome to PSK-31\r\nEnter characters to send\r\n\r\n");
            }
        }
        if (0u != USBUART_1_GetConfiguration()) {
            if (USBUART_1_DataIsReady()) {
                uint16 cnt = USBUART_1_GetAll(in_buffer);
                for (uint16 i = 0; i < cnt; ++i) {
                    if (in_buffer[i] & 0x80) { // varicode is only valid for 7-bit ascii
                        continue;
                    }
                    send_ascii(in_buffer[i]);
                    USBUART_1_PutChar(in_buffer[i]);
                }
            }
        }
    }
}

/* [] END OF FILE */
