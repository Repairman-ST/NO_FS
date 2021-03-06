
/*
*********************************************************************************************************
*                           该问价负责操作wifi模块，但是并没有写所有的接口，只实现了TCP相关的
*                          所有函数都不是线程安全，所以多线程使用时需要上锁
*   
*********************************************************************************************************
*/


#include "bsp_esp8266.h"
#include  <string.h>
#include  <stdlib.h>


/*
*********************************************************************************************************
*                                       LOCAL VARIABLES
*********************************************************************************************************
*/

static     char                   wifi_buf[WIFI_BUFF_LEN];                     //存储模块返回的信息

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


static      void     BSP_ESP8266_Rst(void);
static      void     WriteCmd(char *cmd);
static      void     str_append(char *str_d, char *str_s);
static      void     WriteData(uint8_t *dat, uint16_t len);
static      uint16_t ReadData(char *data, OS_TICK timeOut);




/*
*********************************************************************************************************
*********************************************************************************************************
**                                         GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

// 该函数提供一个短暂的延时，由初始化函数调用，保证每次手动复位后
// wifi模块都能正常启动，其负责拉低复位引脚一段时间
static void init_delay(uint8_t count)
{
	uint32_t time;
	
	
	while (count--) {
		time = 0x2FFFF;
		while (time--);
	}
}



/*
*********************************************************************************************************
*                                        BSP_ESP8266_Init()
*
* Description : 初始与该模块链接的串口，设置相应的波特率
*
* Return(s)   : none
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_ESP8266_Init(void)
{	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// 设置引脚时钟
	RCC_AHB1PeriphClockCmd(WIFI_EN_GPIO_RCC, ENABLE);
	
	// 模式配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;        //普通输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;       //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;    
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;         //上拉
	
	// 配置使能引脚，并且使能模块
	GPIO_InitStructure.GPIO_Pin = WIFI_EN_PIN;             
	GPIO_Init(WIFI_EN_GPIO_PORT, &GPIO_InitStructure);  

	// 保证每次手动复位都能得到重启
	GPIO_ResetBits(WIFI_EN_GPIO_PORT, WIFI_EN_PIN); 
	init_delay(80);
	GPIO_SetBits(WIFI_EN_GPIO_PORT, WIFI_EN_PIN);  
	ReadData(wifi_buf, 1000);                             //等待开机结束    
	init_delay(120);
	
	// 初始化串口
	BSP_UART_Init(WIFI_UART_PORT, WIFI_UART_BAUD);
	init_delay(2);
}



/*
*********************************************************************************************************
**                                         服务器模式相关函数
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                                        BSP_ESP8266_Server_Init()
*
* Description : 将wifi模块配置为TCP服务器，模式，但是最多只能连接5个设备，无数据交互8s后主动断开
*
* Return(s)   : none
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

void BSP_ESP8266_Server_Init(void) 
{
	WriteCmd("ATE0");                                                  // 不回显 
	ReadData(wifi_buf, 1000);                                          // 等待模块回应
	
	WriteCmd("AT+CWMODE_CUR=2");                                       // 设置为热点模式 
	ReadData(wifi_buf, 1000);                                          // 等待模块回应
	
	// 配置热点
	WriteCmd(AP_CONFIG);
	ReadData(wifi_buf, 1000);                                          // 等待模块回应
	
	WriteCmd("AT+CIPMUX=1");                                           // 服务器必须为多连接模式
	ReadData(wifi_buf, 1000);                                          // 等待模块回应
	
	WriteCmd(AP_SERVER_CONFIG);                                        // 开启服务器服务器
	ReadData(wifi_buf, 1000);                                          // 等待模块回应
	
	WriteCmd("AT+CIPSTO=8");                                           // 无数据交互8s后断开，最多连接5个设备
	ReadData(wifi_buf, 1000);                                          // 等待模块回应
}


/*
*********************************************************************************************************
*                                        BSP_ESP8266_Server_Read()
*
* Description : 模块作为服务器的读数据函数，会永久阻塞当有新设备连接或者断开时会返回0(调用时要保证在服务器模式)
*
* Argument(s) : data存储读取到的数据，id是通信设备的编号[0,4]
*
* Return(s)   : 0 表示有设备连接或者断开，n表示读取到的数据长度
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/
uint16_t BSP_ESP8266_Server_Read(uint8_t *data, uint8_t *id, OS_TICK timeout) 
{
	char     lenStr[5];                                             // 所以读取个数不能超9999byte
	uint16_t index = 0;                                             // 索引
	uint16_t dataLen = 0, cpyIndex = 0, readLen;
	
	readLen = ReadData(wifi_buf, timeout);                         // 永久等待
	if (0 == readLen)
		return 0;
	
	// 表示客户端连接或者断开,真实数据重视回车加换行开始
	if(wifi_buf[0] != 0x0D || wifi_buf[1] != 0x0A || wifi_buf[2] != '+' || wifi_buf[3] != 'I')
		return 0;
	
	// 取得连接的ID，范围为0-4，直接偏移因为前面数据固定
	*id = wifi_buf[7]-'0';
	
	// 取得有效数据长度，直接偏移因为前面数据固定
	for (index = 9, dataLen = 0; ':' != wifi_buf[index]; index++, dataLen++) {
		lenStr[dataLen] = wifi_buf[index];
	}
	index++;                                                      // 使其指向真实数据
	lenStr[dataLen] = 0;                                          // 做成字符串,为后续调用atoi做准备
	
	// 提取真实数据
	for (dataLen = atoi(lenStr), cpyIndex = 0; cpyIndex < dataLen; index++, cpyIndex++) {
		data[cpyIndex] = wifi_buf[index];
	}
	
	return dataLen;
}


/*
*********************************************************************************************************
*                                        BSP_ESP8266_Server_Write()
*
* Description : 模块作为服务器的写数据函数
*
* Argument(s) : data存储读取到的数据，id是通信设备的编号[0,4],len是要发送的长度最大为2048
*
* Return(s)   : none
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/
void BSP_ESP8266_Server_Write(uint8_t *data, uint16_t len, uint8_t id)
{
	char send_len[6];                            // 最大能接受2048,硬件限制 
	char send_cmd[26] = "AT+CIPSEND= ,";         // 逗号前的空格不能省略

	// 因为id范围是0-4
	send_cmd[11] = id+'0';
	
	// 将整数转换为字符串
	utoa(len, send_len); 
	// 拼接字符串
	strcat(send_cmd, send_len);
	
	// 发送命令
	WriteCmd(send_cmd);                                        
	ReadData(wifi_buf, 1000);                     // 等到wifi模块准备好 
	
	// 发送整数数据，不用回车加换行
	WriteData(data, len);
	ReadData(wifi_buf, 1000);                     // 等到wifi模块响应 
}






/*
*********************************************************************************************************
**                                         客户端模式相关函数
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        BSP_ESP8266_Client_Init()
*
* Description : 将wifi模块配置为TCP客户端模式，直接变为穿透模式，因为穿透模式即使已经掉线也能自动重新连接
*
* Return(s)   : none
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

void BSP_ESP8266_Client_Init(void) 
{
	WriteCmd("ATE0");                                                  // 不回显 
	ReadData(wifi_buf, 1000);                                          // 等待模块回应
	
	WriteCmd("AT+CWMODE_CUR=1");                                       // 设置为station模式不会存储到Flash 
	ReadData(wifi_buf, 1000);                                          // 等待模块回应
}




/*
*********************************************************************************************************
*                                        BSP_ESP8266_WIFIF_connect_AP()
*
* Description : 将wifi模块连接到wifi热点
*
* Argument    : ssid 热点名字， pwd热点密码（均为字符串形式）
*
* Return(s)   : 0表示连接失败， 1表示连接成功
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

uint8_t BSP_ESP8266_WIFI_connect(char *ssid, char *pwd)
{
	uint8_t status = 0x00;                             // 连接状态
	char connect_cmd[80] = "AT+CWJAP_CUR=\"";          // AT+CWJAP_CUR="

	// 拼接字符串成连接wifi形式 connect_cmd --> AT+CWJAP_CUR="ssid"," pwd"
	
    str_append(connect_cmd, ssid);                     // AT+CWJAP_CUR="ssid   
    str_append(connect_cmd, "\",\"");                  // AT+CWJAP_CUR="ssid","   
    str_append(connect_cmd, pwd);                      // AT+CWJAP_CUR="ssid","pwd 
    str_append(connect_cmd, "\"");                     // AT+CWJAP_CUR="ssid","pwd"
	
	// 确保不会回显
	
	WriteCmd("ATE0");                                  // 不回显 
	ReadData(wifi_buf, 1000);                          // 等待模块回应
	
	// 发送连接热点指令,等待返回
	
	WriteCmd(connect_cmd);
	ReadData(wifi_buf, 20000);                         // WIFI GOT IP或者返回错误码，失败等待时间很长
	
	// 判断是否连接成功
	
	wifi_buf[14] = '\0';                               // WIFI CONNECTED
	
	if (0 == strcmp(wifi_buf, "WIFI CONNECTED")) {
		status = 0x01;                                 // 表示连接成功
		ReadData(wifi_buf, 16000);                     // 连接成功和获取IP有一段时间
	} else
		status = 0x00;
	
	
	ReadData(wifi_buf, 3000);                          // 返回一个OK/FAIL
	
	return status;
}




/*
*********************************************************************************************************
*                                        BSP_ESP8266_WIFIF_connect_status()
*
* Description : 查询wifi模块连接热点的状态
*
* Argument    : none
*
* Return(s)   : 0表示连接失败， 1表示连接成功
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

uint8_t BSP_ESP8266_WIFI_connect_status(void)
{
	// 确保不会回显
	
	WriteCmd("ATE0");                                  // 不回显 
	ReadData(wifi_buf, 1000);                          // 等待模块回应
	
	// 发送发起读取状态指令
	
	WriteCmd("AT+CWJAP_CUR?");
	ReadData(wifi_buf, 1000);                          // 读取模块返回的状态
	
	wifi_buf[5] = '\0';                                // 使其成为一个字符串
	
	if (0 == strcmp(wifi_buf, "No AP"))
		return  0x00;
	
	return 0x01;
}



/*
*********************************************************************************************************
*                                        BSP_ESP8266_WIFIF_connect_server()
*
* Description : 连接到服务器，并且设置为穿透模式
*
* Argument    : none
*
* Return(s)   : 0表示连接失败， 1表示连接成功
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

uint8_t BSP_ESP8266_connect_server(void)
{
	
	// 确保不会回显
	
	WriteCmd("ATE0");                                  // 不回显 
	ReadData(wifi_buf, 1000);                          // 等待模块回应
	
	// 设置为单链接模式,穿透模式必须为单链接模式
	
	WriteCmd("AT+CIPMUX=0");         
	ReadData(wifi_buf, 1000);                          // 读取模块返回的状态

	
	// 发送连接到服务器的指令
	
	WriteCmd(CLIENT_CONFIG);
	ReadData(wifi_buf, 1000);                          // 读取模块返回的状态
	
	wifi_buf[7] = '\0';                                // 使其成为一个字符串
	
	if (0 == strcmp(wifi_buf, "CONNECT")) {
		WriteCmd("AT+CIPMODE=1");                      // 设置为透传
		ReadData(wifi_buf, 1000);                      // 会返回一个ok
		WriteCmd("AT+CIPSEND");                        // 允许发送数据
		ReadData(wifi_buf, 1000);                      // 等待返回>
		return  0x01;
	} else if (0 == strcmp(wifi_buf, "ALREADY"))       // 也表示连接成功
		return 0x01;
		
	return 0x00;
}







/*
*********************************************************************************************************
*                                        BSP_ESP8266_Client_Read()
*
* Description : 从服务器端读取数据，会阻塞由timeout觉定
*
* Argument(s) : data存储读取到的数据
*
* Return(s)   : n表示读取到的数据长度
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/
uint16_t BSP_ESP8266_Client_Read(uint8_t *data, OS_TICK timeout) 
{	
	uint16_t readLen, index;
	
	readLen = ReadData(wifi_buf, timeout);                         // 永久等待
	
	// 将数据拷贝到缓冲区
	
	for (index = 0; index < readLen; index++)
		data[index] = wifi_buf[index];

	return readLen;
}


/*
*********************************************************************************************************
*                                        BSP_ESP8266_Client_Write()
*
* Description : 模块向服务器的写数据
*
* Argument(s) : data存储读取到的数据,len是要发送的长度最大为2048
*
* Return(s)   : none
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/
void BSP_ESP8266_Client_Write(uint8_t *data, uint16_t len)
{
	// 发送整数数据，不用回车加换行，为穿透模式
	
	WriteData(data, len);
}




/*
*********************************************************************************************************
* 该函数将无符号32位整数转换为字符串
*
* Note(s)     : 请保证str足够容纳dat
*********************************************************************************************************
*/

void utoa(uint32_t dat, char *str)
{
    char    *tp;
    char    tmp_buf[12];
    uint8_t i;

    tp = tmp_buf;

    // 保证dat = 0也能被转化
    while (dat || tp == tmp_buf) {
        i   = dat % 10;
        dat = dat / 10;
        *tp++ = i + '0';
    }

    // 交换顺序
    while (tp > tmp_buf) {
        *str++ = *--tp;
    }

    // 字符串结束标志
    *str = 0;
}






/*
*********************************************************************************************************
*********************************************************************************************************
**                                         LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                        WriteCmd()
*
* Description : 发送AT指令，自动追加回车换行，即传入参数不需要回车换行结束
*
* Return(s)   : none
*
* Argument(s) : cmd  要执行的命令
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/
static  void  WriteCmd(char *cmd)
{
	BSP_UART_WrStr(WIFI_UART_PORT, cmd);
	BSP_UART_WrStr(WIFI_UART_PORT, "\r\n");
}


/*
*********************************************************************************************************
*                                        WriteData()
*
* Description : 直接发送数据，不对数据做任何处理
*
* Return(s)   : none
*
* Argument(s) : dat  要发送的数据
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/
static  void  WriteData(uint8_t *dat, uint16_t len)
{
	BSP_UART_WrData(WIFI_UART_PORT, dat, len);
}


/*
*********************************************************************************************************
*                                        ReadData()
*
* Description : 接收模块返回的信息，阻塞等待
*
* Return(s)   : 实际读取到的字节长度
*
* Argument(s) : data     模块返回的信息
*             
*             ：timeOut  设置最长的读取等待时间
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  uint16_t  ReadData(char *data, OS_TICK timeOut) 
{
	return BSP_UART_RdData(WIFI_UART_PORT, (uint8_t*)data, WIFI_BUFF_LEN, timeOut);
}


/*
*********************************************************************************************************
*                                        BSP_ESP8266_Rst()
*
* Description : 复位WIFI模块
*
* Return(s)   : none
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

static void  BSP_ESP8266_Rst(void)
{	
	//初始对应的串口
	WriteCmd("AT+RST");
	ReadData(wifi_buf, 1000);                                  // 等待模块回应
	init_delay(120);
}




/*
*********************************************************************************************************
*                                        str_append()
*
* Description : 将第二个字符串追加到第一个字符末尾
*
* Argument(s) : str_d为第一字符串，其必须能保证容纳第二个字符串， str_s为第二个字符串
*
* Return(s)   : none
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

static void str_append(char *str_d, char *str_s)
{
    strcpy(str_d+strlen(str_d), str_s);
}



