/*
  Copyright (c) 2019  Leopard-C

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*/

#ifndef __AS608_H__
#define __AS608_H__

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef unsigned char uchar;
typedef unsigned int uint;

/*******************************BEGIN**********************************
 * 全局变量
*/
extern int   g_fd;          // 全局变量，文件描述符，即open()函数打开串口的返回值
extern int   g_detect_pin;  // 全局变量，GPIO引脚号，检测是模块上否有手指
extern int   g_verbose;     // 全局变量，输出信息的详细程度
extern int   g_has_password;// 全局变量，是否设置了密码
extern char  g_error_desc[128]; // 全局变量，错误代码的含义
extern uchar g_error_code;      // 全局变量，模块返回的确认码，如果函数返回值不为true，读取此变量
/*
**********************************END********************************/


/********************************BEGIN*******************************
 * 模块参数变量
*/
extern uint PS_STATUS;        // 状态寄存器 0
extern uint PS_MODEL;         // 传感器类型 0-15
extern uint PS_CAPACITY;      // 指纹容量，300
extern uint PS_LEVEL;         // 安全等级 1/2/3/4/5，默认为3
extern uint PS_PACKET_SIZE;   // 数据包大小 32/64/128/256 bytes，默认为128
extern uint PS_BAUD_RATE;     // 波特率系数 

extern uint PS_CHIP_ADDR;      // 设备(芯片)地址
extern uint PS_PASSWORD;       // 通信密码
 
extern char PS_PRODUCT_SN[12];       // 产品型号
extern char PS_SOFTWARE_VERSION[12]; // 软件版本号
extern char PS_MANUFACTURER[12];     // 厂家名称
extern char PS_SENSOR_NAME[12];      // 传感器名称
/*
 *********************************END*****************************/


#ifdef __cplusplus
extern "C" {
#endif

extern bool PS_Setup(uint chipAddr, uint password);       // 0x00000000 ~ 0xffffffff

extern bool PS_GetImage();
extern bool PS_GenChar(uchar bufferID);
extern bool PS_Match(int* pScore);
extern bool PS_Search(uchar bufferID, int startPageID, int count, int* pPageID, int* pScore);
extern bool PS_RegModel();
extern bool PS_StoreChar(uchar bufferID, int pageID);
extern bool PS_LoadChar(uchar bufferID, int pageID);
extern bool PS_UpChar(uchar bufferID, const char* filename);
extern bool PS_DownChar(uchar bufferID, const char* filename);
extern bool PS_UpImage(const char* filename);
extern bool PS_DownImage(const char* filename);
extern bool PS_DeleteChar(int startpageID, int count);
extern bool PS_Empty();
extern bool PS_WriteReg(int regID, int value);
extern bool PS_ReadSysPara();
extern bool PS_Enroll(int* pPageID);
extern bool PS_Identify(int* pPageID, int* pScore);
extern bool PS_SetPwd(uint passwd);   // 4字节无符号整数
extern bool PS_VfyPwd(uint passwd);   // 4字节无符号整数
extern bool PS_GetRandomCode(uint* pRandom);
extern bool PS_SetChipAddr(uint newAddr);
extern bool PS_ReadINFpage(uchar* pInfo, int size/*>=512*/);
extern bool PS_WriteNotepad(int notePageID, uchar* pContent, int contentSize);
extern bool PS_ReadNotepad(int notePageID, uchar* pContent, int contentSize);
extern bool PS_HighSpeedSearch(uchar bufferID, int startPageID, int count, int* pPageID, int* pScore);
extern bool PS_ValidTempleteNum(int* pValidN);
extern bool PS_ReadIndexTable(int* indexList, int size);

// 封装函数
extern bool PS_SetBaudRate(int value);
extern bool PS_SetSecureLevel(int level);
extern bool PS_SetPacketSize(int size);
extern bool PS_GetAllInfo();
extern bool PS_Flush();

// 获得错误代码g_error_code的含义，并赋值给g_error_desc
extern char* PS_GetErrorDesc(); 

#ifdef __cplusplus
}
#endif

#endif // __AS608_H__
