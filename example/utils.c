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

#include "./utils.h"

#include <string.h>
#include <stdio.h>

// 去除文件中的空格
// 仅适合本例，用于处理配置文件，数据量很小
void trimSpaceInFile(const char* filename) {
  FILE* fp = fopen(filename, "r");
  if (!fp)
    return;

  char lineBuf[64] = { 0 };
  char writeBuf[1024] = { 0 }; // 除了空白字符全部读入writeBuf中，包括换行符'\n'
  int offset = 0;
  while (!feof(fp)) {
    fgets(lineBuf, 64, fp);
    if (feof(fp))
      break;
    for (int i = 0, len = strlen(lineBuf); i < len; ++i) {
      if (lineBuf[i] != ' ' && lineBuf[i] != '\t') {
        if (lineBuf[i] == '\n' && offset > 0 && writeBuf[offset-1] == '\n')
          continue;
        writeBuf[offset++] = lineBuf[i];
      }
    }
  }

  fclose(fp);
  fp = fopen(filename, "w+"); // 覆盖写入
  if (!fp)
    return;
  fwrite(writeBuf, 1, strlen(writeBuf), fp);
  fclose(fp);
}

// 去除字符串首尾空格 和 换行符
void trim(const char* strIn, char* strOut) {
  int i = 0;
  int j = strlen(strIn) - 1;
  while (strIn[i] == ' ')
    ++i;
  while (strIn[j] == ' ' || strIn[j] == '\n')
    --j;
  strncpy(strOut, strIn+i, j-i+1);
  strOut[j-i+1] = 0;
}

// 把字符串转为整型
int toInt(const char* str) {
  int ret = 0;
  sscanf(str, "%d", &ret);
  return ret;
}

// 把 *十六进制* 字符串转为无符号整型
unsigned int toUInt(const char* str) {
  unsigned int ret = 0;
  if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    sscanf(str+2, "%x", &ret);
  else
    sscanf(str, "%x", &ret);

  return ret;
}

