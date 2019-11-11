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

#include "../as608.h"
#include "./utils.h"

#include <wiringPi.h>
#include <wiringSerial.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

extern AS608 g_as608;
extern int g_fd;
extern int g_verbose;
extern char  g_error_desc[128];
extern uchar g_error_code;

int  g_argc = 0;   // 参数个数，g_argc = argc - g_option_count
int  g_option_count = 0; // 选项个数-v、-h等
char g_command[16] = { 0 };     // 即argv[1]
Config g_config;   // 配置文件 结构体，定义在"./utils.h"头文件中

void printConfig();
void printUsage();
bool readConfig();  // 读取文件到 g_config
bool writeConfig(); // 将 g_config 写入文件
void asyncConfig();
void priorAnalyseArgv(int argc, char* argv[]);
void analyseArgv(int argc, char* argv[]);

// 因为as608.h内的函数执行失败而退出程序
bool PS_Exit() {
  printf("ERROR! code=%02X, desc=%s\n", g_error_code, PS_GetErrorDesc());
  exit(2);
  return true;
}

// 程序退出时执行的工作，关闭串口等
void atExitFunc() {
  if (g_verbose == 1)
    printf("Exit\n");
  if (g_fd > 0)
    serialClose(g_fd); 
}


/*******************************************************************
 *
 * main()
 *
*******************************************************************/

int main(int argc, char *argv[]) //int serialOpen (const char *device, const int baud)
{
  // 1.读取配置文件，获得芯片地址和通信密码
  if (!readConfig())
    exit(1);

  // 2.优先解析的内容，如参数选项、配置本地文件等
  priorAnalyseArgv(argc, argv);
  
  if (g_verbose == 1)
    printConfig();
  
  // 3.初始化wiringPi库
  if (-1 == wiringPiSetup()) {
    printf("wiringPi setup failed!\n");
    return 1;
  }

  // 4.检测是否有手指放上的GPIO端口，设为输入模式
  pinMode(g_config.detect_pin, INPUT);

  // 5.打开串口
	if((g_fd = serialOpen(g_config.serial, g_config.baudrate)) < 0)	{
		fprintf(stderr,"Unable to open serial device: %s\n", strerror(errno));
		return 1;
	}

  // 6.注册退出函数(打印一些信息、关闭串口等)
  atexit(atExitFunc);

  // 7.初始化 AS608 模块
  // 地址 密码
  PS_Setup(g_config.address, g_config.password) ||  PS_Exit();

  // 8.主处理函数，解析普通命令(argv[1])，
  analyseArgv(argc, argv);

	return 0;
}


/***************************************************************
 *
 * 函数定义
 *
***************************************************************/

// 打印配置文件内容到屏幕上
void printConfig() {
  printf("address=%08x\n", g_config.address);
  if (g_config.has_password)
    printf("password=%08x\n", g_config.password);
  else
    printf("password=none(no password)\n");
  printf("serial_file=%s\n",   g_config.serial);
  printf("baudrate=%d\n",   g_config.baudrate);
  printf("detect_pin=%d\n",   g_config.detect_pin);
}

// 同步g_config变量内容和其他变量内容
void asyncConfig() {
  g_as608.detect_pin   = g_config.detect_pin;
  g_as608.has_password = g_config.has_password;
  g_as608.password     = g_config.password;
  g_as608.chip_addr    = g_config.address;
  g_as608.baud_rate    = g_config.baudrate;
}

// 读取配置文件
bool readConfig() {
  FILE* fp;

  // 获取用户主目录
  char filename[256] = { 0 };
  sprintf(filename, "%s/.fpconfig", getenv("HOME"));
  
  // 主目录下的配置文件
  if (access(filename, F_OK) == 0) { 
    trimSpaceInFile(filename);
    fp = fopen(filename, "r");
  }
  else {
    // 如果配置文件不存在，就在主目录下创建配置文件，并写入默认配置
    // 设置默认值
    g_config.address = 0xffffffff;
    g_config.password= 0x00000000;
    g_config.has_password = 0;
    g_config.baudrate = 9600;
    g_config.detect_pin = 1; 
    strcpy(g_config.serial, "/dev/ttyAMA0");

    writeConfig();

    printf("Please config the address and password in \"~/.fpconfig\"\n");
    printf("  fp cfgaddr   0x[address]\n");
    printf("  fp cfgpwd    0x[password]\n");
    printf("  fp cfgserial [serialFile]\n");
    printf("  fp cfgbaud   [rate]\n");
    printf("  fp cfgpin    [GPIO_pin]\n");
    return false;
  }

  char key[16] = { 0 };
  char value[16] = { 0 };
  char line[32] = { 0 };

  char *tmp;
  while (!feof(fp)) {
    fgets(line, 32, fp);
    
    // 分割字符串，得到key和value
    if (tmp = strtok(line, "="))
      trim(tmp, key);
    else
      continue;
    if (tmp = strtok(NULL, "="))
      trim(tmp, value);
    else
      continue;
    while (!tmp)
      tmp = strtok(NULL, "=");

    // 如果数值以 0x 开头
    int offset = 0;
    if (value[0] == '0' && (value[1] == 'x' || value[1] == 'X'))
      offset = 2;

    if (strcmp(key, "address") == 0) {
      g_config.address = toUInt(value+offset);
    }
    else if (strcmp(key, "password") == 0) {
      if (strcmp(value, "none") == 0 || strcmp(value, "false") == 0) {
        g_config.has_password = 0; // 无密码
      }
      else {
        g_config.has_password = 1; // 有密码
        g_config.password = toUInt(value+offset);
      }
    }
    else if (strcmp(key, "serial") == 0) {
      int len = strlen(value);
      if (value[len-1] == '\n')
        value[len-1] = 0;
      strcpy(g_config.serial, value);
    }
    else if (strcmp(key, "baudrate") == 0) {
      g_config.baudrate = toInt(value);
    }
    else if (strcmp(key, "detect_pin") == 0) {
      g_config.detect_pin = toInt(value);
    }
    else {
      printf("Unknown key:%s\n", key);
      fclose(fp);
      return false;
    }

  } // end while(!feof(fp))

  asyncConfig();

  fclose(fp);
  return true;
}

/*
 * 写配置文件
*/
bool writeConfig() {
  // 获取用户主目录
  char filename[256] = { 0 };
  sprintf(filename, "%s/.fpconfig", getenv("HOME"));
  
  FILE* fp = fp = fopen(filename, "w+");
  if (!fp) {
    printf("Write config file error!\n");
    exit(0);
  }

  fprintf(fp, "address=0x%08x\n", g_config.address);
  if (g_config.has_password)
    fprintf(fp, "password=0x%08x\n", g_config.password);
  else
    fprintf(fp, "password=none\n");
  fprintf(fp, "baudrate=%d\n", g_config.baudrate);
  fprintf(fp, "detect_pin=%d\n", g_config.detect_pin);
  fprintf(fp, "serial=%s\n", g_config.serial);

  fclose(fp);
}

// 检查参数数量是否正确是否
bool checkArgc(int argcNum) {
  if (argcNum == g_argc)
    return true;
  else if (argcNum == 2)
    printf("ERROR! \"%s\" accept no parameter\n", g_command);
  else if (argcNum == 3)
    printf("ERROR! \"%s\" accept 1 parameter\n", g_command);
  else if (argcNum > 3)
    printf("ERROR! \"%s\" accept %d parameters\n", g_command, argcNum);

  exit(1);
}

// 匹配argv[1], 即g_command
bool match(const char* str) {
  if (strcmp(str, g_command) == 0)
    return true;
  else
    return false;
}

// 重置电阻屏检测，防止用户两次录指纹之间没有拿开手指
// TODO
bool reset(int seconds) {
  for (int i = 0; i < 1000*seconds; ++i) {
    if (digitalRead(g_as608.detect_pin) == LOW) {
      return true;
    }

    // 如果seconds秒内，没有检测到指纹，返回false
    usleep(1000);
  }

  return false;
}

// 需要优先解析的命令，如配置文件的修改、选项解析等
// 不需要与模块通信
void priorAnalyseArgv(int argc, char* argv[]) {
  if (argc < 2) {
    printUsage();
    exit(1);
  }

  // 检查选项  -v -h
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0) {
      printUsage();
      g_option_count++;
      exit(0);
    }
    else if (strcmp(argv[i], "-v") == 0) {
      g_verbose = 1;
      g_option_count++;
    }
  }
  
  g_argc = argc - g_option_count;
  strcpy(g_command, argv[1]);

  if (match("cfg")) {
    printConfig();
    exit(0);
  }

  // 配置通信地址
  else if (match("cfgaddr")) {
    checkArgc(3);
    g_config.address = toUInt(argv[2]);
    writeConfig();
    exit(0);
  }

  // 配置通信密码
  else if (match("cfgpwd")) {
    checkArgc(3);
    g_config.password = toUInt(argv[2]);
    g_config.has_password = 1;
    writeConfig();
    exit(0);
  }

  // 配置串口号
  else if (match("cfgserial")) {
    checkArgc(3);
    strcpy(g_config.serial, argv[2]);
    writeConfig();
    exit(0);
  }

  else if (match("cfgbaud")) {
    checkArgc(3);
    g_config.baudrate = toInt(argv[2]);
    writeConfig();
    exit(0);
  }

  else if (match("cfgpin")) {
    checkArgc(3);
    g_config.detect_pin = toInt(argv[2]);
    writeConfig();
    exit(0);
  }
}

// 主处理函数，解析命令
void analyseArgv(int argc, char* argv[]) {

  if (match("add")) {
    checkArgc(3);
    printf("Please put your finger on the module.\n");
    if (!PS_GetImage()) 
      PS_Exit();
    if (!PS_GenChar(1))
      PS_Exit();

    printf("Please raise your finger\n");

    int try_time = 5;       // 尝试五次录入第二次指纹，如果一直无法和第一次匹配,exit
    while (try_time--) {
      // 判断用户是否抬起了手指，
      for (int i = 0; i < 3; ++i) {
        if (reset(2)) {
         printf("Please put your finger again\n");
         break;
        }
        else
          printf("Plese raise your finger!\n");
      }

      PS_GetImage() || PS_Exit();
      PS_GenChar(2) || PS_Exit();

      int score = 0;
      if (!PS_Match(&score))
        printf("Not matched, raise your finger and put it on again.\n");
      else {
        printf("Matched! score=%d\n", score);
        break;
      }
    }
    
    if (g_error_code != 0x00)
      PS_Exit();

    // 合并特征文件
    PS_RegModel() || PS_Exit();
    PS_StoreChar(2, toInt(argv[2])) || PS_Exit();

    printf("OK! New fingerprint saved to pageID=%d\n", toInt(argv[2]));
  }

  else if (match("enroll")) {
    checkArgc(2);

    int count = 0;
    printf("Please put your finger on the moudle\n");
    while (digitalRead(g_as608.detect_pin) == LOW) {
      delay(1);
      if ((count++) > 5000) {
        printf("Not detected the finger!\n");
        exit(2);
      }
    }
        
    int pageID = 0;
    PS_Enroll(&pageID) || PS_Exit();

    printf("OK! New fingerprint saved to pageID=%d\n", pageID);
  }
  
  else if (match("info")) {
    checkArgc(2);
    PS_GetAllInfo() || PS_Exit();

    printf("Product SN:        %s\n", g_as608.product_sn);
    printf("Software version:  %s\n", g_as608.software_version);
    printf("Manufacture:       %s\n", g_as608.manufacture);
    printf("Sensor model:      %d\n", g_as608.model);
    printf("Sensor name:       %s\n", g_as608.sensor_name);
    printf("Status register:   %d\n", g_as608.status);
    printf("Database capacity: %d\n", g_as608.capacity);
    printf("Secure level(1-5): %d\n", g_as608.secure_level);
    printf("Device address:    0x%08x\n", g_as608.chip_addr);
    printf("Packet size:       %u bytes\n", g_as608.packet_size);
    printf("Baud rate:         %d\n", g_as608.baud_rate);
  }

  else if (match("empty")) {
    checkArgc(2);
    printf("Confirm to empty database: (Y/n)? ");
    fflush(stdout);
    char cmd;
    scanf("%c", &cmd);
    if (cmd == 'n' || cmd ==  'N') {
      printf("Canceled!\n");
      exit(3);
    }

    PS_Empty() || PS_Exit();
    printf("OK!\n");
  }

  else if (match("delete")) {
    int startPageID = 0;
    int count = 0;

    // 判断参数个数
    if (g_argc == 3) {
      startPageID = toInt(argv[2]);
      count = 1;
      printf("Confirm to delete fingerprint %d: (Y/n)? ", startPageID);
    }
    else if (argc == 4) {
      startPageID = toInt(argv[2]);
      count = toInt(argv[3]);
      printf("Confirm to delete fingerprint %d-%d: (Y/n)? ", startPageID, startPageID+count-1);
    }
    else {
      printf("Command \"delete\" accept 1 or 2 parameter\n");
      printf("  Usage: fp delete startPageID [count]\n");
      exit(1);
    }

    // 询问是否继续
    fflush(stdout);
    char cmd = getchar();
    if (cmd == 'n' || cmd ==  'N') {
      printf("Canceled!\n");
      exit(0);
    }

    PS_DeleteChar(startPageID, count) || PS_Exit();
    printf("OK!\n");
  }

  else if (match("count")) {
    checkArgc(2);
    int count = 0;
    PS_ValidTempleteNum(&count) || PS_Exit();
    printf("%d\n", count);
  }

  else if (match("search")) {
    checkArgc(2);

    printf("Please put your finger on the module.\n");
    PS_GetImage() || PS_Exit();
    PS_GenChar(1) || PS_Exit();

    int pageID = 0, score = 0;
    if (!PS_Search(1, 0, 300, &pageID, &score))
      PS_Exit();
    else
      printf("Matched! pageID=%d score=%d\n", pageID, score);
  }

  else if (match("hsearch")) {  // high speed search
    checkArgc(2);

    printf("Please put your finger on the module.\n");
    PS_GetImage() || PS_Exit();
    PS_GenChar(1) || PS_Exit();

    int pageID = 0, score = 0;
    if (!PS_HighSpeedSearch(1, 0, 300, &pageID, &score))
      PS_Exit();
    else
      printf("Matched! pageID=%d score=%d\n", pageID, score);
  }

  else if (match("identify")) {
    checkArgc(2);
    int pageID = 0;
    int score = 0;
    
    // 检测手指是否存在
    int count = 0;
    printf("Please put your finger on the moudle\n");
    while (digitalRead(g_as608.detect_pin) == LOW) {
      delay(1);
      if ((count++) > 5000) {
        printf("Not detected the finger!\n");
        exit(2);
      }
    }

    PS_Identify(&pageID, &score) || PS_Exit();
    printf("Matched! pageID=%d score=%d\n", pageID, score);
  }

  // 列出指纹列表
  else if (match("list")) {
    checkArgc(2);
    int indexList[512] = { 0 };
    PS_ReadIndexTable(indexList, 512) ||  PS_Exit();

    int i = 0;
    for (i = 0; i < 300; ++i) {
      if (indexList[i] == -1)
        break;
      printf("%d\n", indexList[i]);
    }
    if (i == 0) {
      printf("The database is empty!\n");
    }
  }

  else if (match("getimage")) {
    checkArgc(2);
    printf("Please put your finger on the module.\n");
    PS_GetImage() || PS_Exit();
    printf("OK!\n");
  }

  else if (match("genchar")) {
    checkArgc(3);
    PS_GenChar(toInt(argv[2])) || PS_Exit();
    printf("OK!\n");
  }

  else if (match("regmodel")) {
    checkArgc(2);
    PS_RegModel() ||  PS_Exit();
    printf("OK!\n");
  }

  else if (match("storechar")) {
    checkArgc(4);
    PS_StoreChar(toInt(argv[2]), toInt(argv[3])) ||  PS_Exit();
    printf("OK! Stored in pageID=%d\n", toInt(argv[3]));
  }

  else if (match("loadchar")) {
    checkArgc(4);
    PS_LoadChar(toInt(argv[2]), toInt(argv[3])) || PS_Exit();
    printf("OK! Loaded to bufferID=%d\n", toInt(argv[2]));
  }

  else if (match("match")) {
    checkArgc(2);
    int score = 0;
    PS_Match(&score) || PS_Exit();
    printf("Matched! Score=%d\n", score);
  }

  else if (match("random")) {
    checkArgc(2);
    uint randomNum = 0; // 必须是unsigned int，否则可能会溢出
    PS_GetRandomCode(&randomNum) ||  PS_Exit();
    printf("%u\n", randomNum);
  }
  
  else if (match("upchar")) {
    checkArgc(4);
    PS_UpChar(toInt(argv[2]), argv[3]) ||  PS_Exit();
    printf("OK!\n");
  }

  else if (match("downchar")) {
    checkArgc(4);
    PS_DownChar(toInt(argv[2]), argv[3]) || PS_Exit();
    printf("OK!\n");
  }

  else if (match("upimage")) {
    checkArgc(3);
    PS_UpImage(argv[2]) || PS_Exit();
    printf("OK!\n");
  }

  else if (match("downimage")) {
    checkArgc(3);
    PS_DownImage(argv[2]) ||  PS_Exit();
    printf("OK!\n");
  }

  else if (match("flush")) {
    checkArgc(2);
    PS_Flush();
    printf("OK!\n");
  }

  else if (match("writereg")) {
    checkArgc(4);
    PS_WriteReg(toInt(argv[2]), toInt(argv[3])) ||  PS_Exit();
    printf("OK!\n");
  }

  else if (match("baudrate")) {
    if (g_argc == 2) {
      printf("%d\n", g_as608.baud_rate);
      exit(0);
    }
    else if (g_argc == 3) {
      PS_SetBaudRate(toInt(argv[2])) ||  PS_Exit();
    }
    else {
      printf("Command \"baudrate\" accept 1 parameter at most\n");
      exit(1);
    }

    printf("OK!\n");
  }

  else if (match("level")) {
    if (g_argc == 2) {
      printf("%d\n", g_as608.secure_level);
      exit(0);
    }
    else if (g_argc == 3) {
      PS_SetSecureLevel(toInt(argv[2])) ||  PS_Exit();
    }
    else {
      printf("Command \"level\" accept 1 parameter at most\n");
      exit(1);
    }

    printf("OK!\n");
  }

  else if (match("packetsize")) {
    if (g_argc == 2) {
      printf("%d\n", g_as608.packet_size);
      exit(0);
    }
    else if (g_argc == 3) {
      PS_SetPacketSize(toInt(argv[2])) ||  PS_Exit();
    }
    else {
      printf("Command \"packetsize\" accept 1 parameter at most\n");
      exit(1);
    }

    printf("OK!\n");
  }
  
  else if (match("address")) {
    if (g_argc == 2) {
      printf("0x%08x\n", g_as608.chip_addr);
      exit(0);
    }
    else if (g_argc == 3) {
      PS_SetChipAddr(toUInt(argv[2])) || PS_Exit();
      g_config.address = toUInt(argv[2]);
      if (writeConfig())
        printf("New chip address is 0x%08x\n", g_as608.chip_addr);
    }
    else {
      printf("Command \"address\" accept 1 parameter at most\n");
      exit(1);
    }

    printf("OK!\n");
  }

  else if (match("setpwd")) {
    checkArgc(3);
    PS_SetPwd(toUInt(argv[2])) ||  PS_Exit();
    g_config.has_password = 1;
    g_config.password = toUInt(argv[2]);
    if (writeConfig())
      printf("New password is 0x%08x\n", g_as608.password);
  }

  else if (match("vfypwd")) {
    checkArgc(3);
    PS_VfyPwd(toUInt(argv[2])) ||  PS_Exit();
    printf("OK!\n");
  }

  else if (match("readinf")) {
    checkArgc(3);
    uchar buf[513] = { 0 };
    PS_ReadINFpage(buf, 512) ||  PS_Exit();

    // 写入文件
    FILE* fp = fopen(argv[2], "w+");
    if (!fp) {
      printf("Open file error\n");
      exit(1);
    }
    fwrite(buf, 1, 512, fp);
    fclose(fp);

    printf("%s\n", buf);
  }

  else if (match("writenote")) {
    char buf[128] = { 0 };
    if (g_argc == 3) {
      printf("Please write note: (max lenght is 31 !)\n");
      fgets(buf, 128, stdin);
    }
    else if (g_argc == 4) {
      memcpy(buf, argv[3], 32);
    }
    else {
      printf("Command \"writenote\" accept 1 or 2 parameter\n");
      exit(1);
    }

    if (strlen(buf) > 31) {   // 如果输入的字符多于31个
      printf("Too long, continute to save the first 31 characters? (Y/n): ");
      char c = 0;
      scanf("%c", &c);
      if (c != 'y' && c != 'Y') {
        printf("Canceled!\n");
        exit(0);
      }
    }

    PS_WriteNotepad(toInt(argv[2]), buf, 32) ||  PS_Exit();

    printf("OK\n");
  }

  else if (match("readnote")) {
    checkArgc(3);
    uchar buf[33] = { 0 }; 
    PS_ReadNotepad(toInt(argv[2]), buf, 32) || PS_Exit();
    printf("%s\n", buf);
  }


  else {
    printf("Unknown parameter \"%s\"\n", argv[1]);
    exit(1);
  }
}


// 打印程序使用说明
void printUsage() {
  printf("A command line program to interact with AS608 module.\n\n");
  printf("Usage:\n  ./fp [command] [param] [option]\n");
  printf("\nAvailable Commands:\n");

  printf("-------------------------------------------------------------------------\n");
  printf("  command  | parm     | description\n");
  printf("-------------------------------------------------------------------------\n");
  printf("  cfgaddr   [addr]     Config address in local config file\n");
  printf("  cfgpwd    [pwd]      Config password in local config file\n");
  printf("  cfgserial [serialFile] Config serial port in local config file. Default:/dev/ttyAMA0\n");
  printf("  cfgbaud   [rate]     Config baud rate in local config file\n");
  printf("  cfgpin    [GPIO_pin] Config GPIO pin to detect finger in local confilg file\n\n");

  printf("  add       [pID]      Add a new fingerprint to database. (Read twice) \n");
  printf("  enroll    []         Add a new fingerprint to database. (Read only once)\n");
  printf("  delete    [pID {count}]  Delete one or contiguous fingerprints.\n");
  printf("  empty     []         Empty the database.\n");
  printf("  search    []         Collect fingerprint and search in database.\n");
  printf("  identify  []         Search\n");
  printf("  count     []         Get the count of registered fingerprints.\n");
  printf("  list      []         Show the registered fingerprints list.\n");
  printf("  info      []         Show the basic parameters of the module.\n");
  printf("  random    []         Generate a random number.(0~2^32)\n\n");

  printf("  getimage  []         Collect a fingerprint and store to ImageBuffer.\n");
  printf("  upimage   [filename] Download finger image to ras-pi in ImageBuffer of the module\n");
  printf("  downimage [filename] Upload finger image to module\n");
  printf("  genchar   [cID]      Generate fingerprint feature from ImageBuffer.\n");
  printf("  match     []         Accurate comparison of CharBuffer1 and CharBuffer2\n");
  printf("                         feature files.\n");
  printf("  regmodel  []         Merge the characteristic file in CharBuffer1 and\n");
  printf("                         CharBuffer2 and then generate the template, the\n");
  printf("                         results are stored in CharBuffer1 and CharBuffer2.\n");
  printf("  storechar [cID pID]  Save the template file in CharBuffer1 or CharBuffer2\n");
  printf("                         to the flash database location with the PageID number\n");
  printf("  loadchar  [cID pID]  Reads the fingerprint template with the ID specified\n");
  printf("                         in the flash database into the template buffer,\n");
  printf("                         CharBuffer1 or CharBuffer2\n");
  printf("  readinf   [filename] Read the FLASH Info Page (512bytes), and save to file\n");
  printf("  writenote     [page {note}]   Write note loacted in pageID=page\n");
  printf("  readnote      [page]          Read note loacted in pageID=page\n");
  printf("  upchar        [cID filename]  Download feature file in CharBufferID to ras-pi\n");
  printf("  downchar      [cID filename]  Upload feature file in loacl disk to module\n");
  printf("  setpwd        [pwd]           Set password\n");
  printf("  vfypwd        [pwd]           Verify password\n");
  printf("  packetsize    [{size}]        Show or Set data packet size\n");
  printf("  baudrate      [{rate}]        Show or Set baud rate\n");
  printf("  level         [{level}]       Show or Set secure level(1~5)\n");
  printf("  address       [{addr}]        Show or Set secure level(1~5)\n");
  
  printf("\nAvaiable options:\n");
  printf("  -h    Show help\n");
  printf("  -v    Shwo details while excute the order\n");

  printf("\nUsage:\n  ./fp [command] [param] [option]\n\n");
}

