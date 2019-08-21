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

#ifndef __UTILS_H__
#define __UTILS_H__

typedef struct _Config {
  unsigned int address;
  unsigned int password;
  int has_password;
  int baudrate;
  int detect_pin;
  char serial[16];
} Config;


// 去除文件中所有空白字符(空格和制表符)
void trimSpaceInFile(const char* filename);

// 把字符串转为整型
void trim(const char* strIn, char* strOut);

// 把字符串转为整型
int toInt(const char* str);

// 把 *十六进制* 字符串转为无符号整型
unsigned int toUInt(const char* str);


#endif // __UTILS_H__
