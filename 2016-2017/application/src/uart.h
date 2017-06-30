#ifndef _UART_H_
#define _UART_H_

#if 0
#define HEARTBEAT         67 //通讯线路测试(巡检)

#define uart_SEND_DATA 2 // 2	发送数据	上位机向火灾报警控制器发送数据
#define uart_SEND_OK   3 // 3	确认	对控制命令和发送信息的确认回答
#define uart_REQUEST   4 // 4	请求	上位机查询火警报警控制器信息
#define uart_ANSWER    5 // 5	应答	返回查询的信息
#define uart_SEND_NO   6 // 6	否认	对控制命令和发送信息的否认回答

/////////////////////////////////////////////////////////////////////////////////
//类型标志定义表
//类型代码	说明	方向
//0	预留	
#define  DEVICE_STATE        1 	//	上传火灾自动报警系统设备状态	上行Device state
#define  PARTS_RUNNING_STATE 2	//	上传火灾自动报警系统部件运行状态	上行 Parts running state
#define  ANALOG_VALUE        3	//	上传火灾自动报警系统部件模拟量值	上行 Analog
#define  DEVICE_OPER_INFO    4	//	上传火灾自动报警系统设备操作信息	上行
#define  PASTS_OPER_INFO     5	//	上传火灾自动报警系统部件操作信息	上行
#define  DEVICE_SET          6	//	上传火灾自动报警系统设备配置情况	上行
#define  PASTS_SET           7	//	上传火灾自动报警系统部件配置情况	上行
#define  DVEICE_TIME         8	//	上传火灾自动报警系统设备时间	上行
#define  TEST                9	//	通信线路上行测试	上行
//10～60	预留	
#define READ_DEVICE_STATE        61 //61读火灾自动报警系统设备状态	下行
#define READ_PARTS_RUNNING_STATE 62//62	读火灾自动报警系统部件运行状态	下行
#define READ_ANALOG_VALUE        63//63	读火灾自动报警系统部件模拟量值	下行
#define READ_DEVICE_SET          64//64	读火灾自动报警系统设备配置情况	下行
#define READ_PASTS_SET           65//65	读火灾自动报警系统部件配置情况	下行
#define READ_DVEICE_TIME         66//66	读火灾自动报警系统设备时间	下行
#define DOWN_TEST                67//67	通信线路下行测试	下行
//68～127	预留	
//128～255	用户自定义	
#define WRITE_DEVICE_IFNO        128//128	写终端设备信息	下行
#define WRITE_LINKAGE_INFO       129//129	写联动公式	下行
#define WRITE_GAS_LINKAGE        130//130	写气体联动公式	下行
#define WRITE_WARN_LINKAGE       131//131	写预警联动公式	下行
#define WRITE_MULT_PANEL         132//132	写多线盘信息	下行
#define WRITE_BUS_PANEL          133//133	写总线盘信息	下行
#define WRITE_DISP_PANEL         134//134	写层显信息	下行
#define WRITE_BROADCAST          135//135	写广播分区信息	下行
#define WRITE_CONT_INFO          136//136	写控制器信息	下行
#define WRITE_LOOP_SET           137//137	写集中机控制器回路数设置	下行
//138~150	自定义预留	
#define READ_DEVICE_INFO         151//151	读终端设备信息	上行
#define READ_LINKAGE_INFO        152//152	读联动公式	上行
#define READ_GAS_LINKAGE         153//153	读气体联动公式	上行
#define READ_WARN_LINKAGE        154//154	读预警联动公式	上行
#define READ_MULT_PANEL          155//155	读多线盘信息	上行
#define READ_BUS_PANEL           156//156	读总线盘信息	上行
#define READ_DISP_PANEL          157//157	读层显信息	上行
#define READ_BROADCAST           158//158	读广播分区信息	上行
#define READ_CONT_INFO           159//159	读控制器信息	上行
#define READ_LOOP_SET            160//160	读集中机控制器回路数设置	上行

#define SET_CONT_OPER            182//182	设置疏散控制器操作(复位、消音、自检、月检、年检) //信息重传
///////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////火灾自动报警系统设备类型代码表//////////////////////////////////////////
#define FIRE_ALARM_CONT          1//火灾报警控制器/消防联动控制器
#define CRT_CONT                 2//消防控制室图形显示装置
#define GAS_ALARM_CONT           10//可燃气体报警控制器        
#define ELECTRICAL_FIRE_EQUIPMENT  11//电气火灾监控设备
#define FIRE_HYDRANT_SYSTEM      20//消火栓系统设备Fire hydrant system equipment
#define SPRINKLER_SYSTEM         21//自动喷水灭火系统设备、水喷雾灭火系统设备sprinkler system equipment
#define GAS_EXTI_CONT            22//气体灭火控制器Gas extinguishing controller
#define FOAM_EXTI_SYSTEM         23//泡沫灭火系统设备Foam fire extinguishing system equipment
#define POWDER_EXTI_SYSTEM       24//干粉灭火系统设备Powder extinguishing system equipment
#define SMOKE_CONTROL_SYSTEM     25//防烟排烟系统设备Smoke control exhaust system equipment
#define FIRE_DOORS_SHUT SYSTEM   26 //防火门及卷帘系统设备Fire doors and shutter systems equipment
#define FIRE_EMERGENCY_BROADCAST 27//消防应急广播Fire Emergency Broadcast
#define FIRE_TELEPHONE           28//消防电话Fire telephone
#define LIGHT_EVAC_SYSTEM        29//消防应急照明和疏散指示系统设备Fire emergency lighting and evacuation system equipment
#define FIRE_POWER               30//消防电源Fire Power
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////火灾自动报警系统设备状态、部件状态代码表//////////////////////////////////////////
#define CRT_FIRE                 1//火警（可燃气休、电气火灾报警）
#define CRT_LOW_ALARM            2//低限报警
#define CRT_HIGH_ALARM           3//高限报警      
#define CRT_OVERRANGE_ALARM      4//超量程报警Overrange alarm
#define CRT_RESIDUAL_CUR_ALARM   5//剩余电流报警Residual current alarm
#define CRT_TEMPERATURE_ALARM    6//温度报警 Temperature 
#define CRT_ELECTRIC_ALARM       7//电弧报警Electric 
#define CRT_SMOKE_ALARM          8//感烟探测器报警
#define CRT_HEAT_ALARM           9//感温探测器
#define CRT_MANUAL_BUTTON_ALARM  10//手动报警按钮报警Manual alarm button alarm
#define CRT_FLAME_ALARM          11//火焰探测器报警Flame detector alarm
#define CRT_WARN_ALARM           12//预警
#define CRT_FAULT                20//故障
#define CRT_COMM_FAULT           21//通讯故障     
#define CRT_MAIN_FAULT           22//主电故障
#define CRT_BATT_FAULT           23//备电故障
#define CRT_LOOP_FAULT           24//回路故障
#define CRT_PART_FAULT           25//部件故障
#define CRT_START_LINE_FAULT     26//启动线路故障circuit fault
#define CRT_SPRAY_LINE_FAULT     27//喷洒线路故障
#define CRT_RETURN_LINE_FAULT    28//反馈线故障     
#define CRT_SPRAY_RETURN_LINE_FAULT 29//喷洒反馈线路故障
#define CRT_LAMP_FAULT           30//灯具故障
#define CRT_SELF_CHECK           31//自检
#define CRT_SELF_CHECK_FAULT     32//自检失败
//#define CRT_COMM_FAULT           33//通信失败 同21通讯故障
#define CRT_FAULT_R              40//故障恢复
#define CRT_COMM_FAULT_R         41//通讯故障恢复
#define CRT_MAIN_FAULT_R         42//主电故障恢复
#define CRT_BATT_FAULT_R         43//备电故障恢复
#define CRT_LOOP_FAULT_R         44//回路故障恢复
#define CRT_PART_FAULT_R         45//部件故障恢复
#define CRT_START_LINE_FAULT_R   46//启动线路故障恢复
#define CRT_SPRAY_LINE_FAULT_R   47//喷洒线路故障恢复
#define CRT_RETURN_LINE_FAULT_R  48//反馈线路故障恢复
#define CRT_SPRAY_RETURN_LINE_FAULT_R 49//喷洒反馈线路故障恢复
#define CRT_LAMP_FAULT_R         50//灯具故障恢复
#define CRT_START                60//启动
#define CRT_AUTO_START           61//自动启动
#define CRT_MANUAL_START         62//手动启动
#define CRT_GAS_SPRAY            63//气体喷洒
#define CRT_EMERGENCY_START      64//现场急启emergency 
#define CRT_STOP                 70//停止
#define CRT_AUTO_STOP            71//自动停止
#define CRT_MANUAL_STOP          72//手动停止
#define CRT_EMERGENCY_STOP       73//现场急停
#define CRT_RETURN               80//反馈
#define CRT_SPRAY_RETURN         81//喷洒反馈
#define CRT_RETURN_R             82//反馈撤销
#define CRT_SHIELD               83//屏蔽
#define CRT_SHIELD_R             84//屏蔽撤销
#define CRT_WARDSHIP             85//监管
#define CRT_WARDSHIP_R           86//监管撤销
#define CRT_GUIDE                90//引导Guide
#define CRT_EMERGENCY            91//应急
#define CRT_MONTH_CHECK          92//月检
#define CRT_YEAR_CHECK           93//年检
#define CRT_CALL                 100//呼叫
#define CRT_TELEPHONE            101//通话 Conversation
#define CRT_OUTPUT_FAULT         128//输出线故障
#define CRT_OUTPUT_FAULT_R       129//输出线故障恢复
#define CRT_INPUT_FUALT          130//输入线故障
#define CRT_INPUT_FUALT_R        131//输入线故障恢复
#define CRT_BUS_SHORT            132//	总线短路
#define CRT_BUS_SHORT_R          133	//总线短路恢复
#define CRT_NEW_ADD              134	//新注册
#define CRT_SG_ALARM_FAULT       135	//声光警报器故障
#define CRT_SG_ALARM_FAULT_R     136	//声光警报器故障恢复
#define CRT_TRAN_ALARM_FAUTL     137	//火警传输设备故障
#define CRT_TRAN_ALARM_FAUTL_R   138	//火警传输设备故障恢复
#define CRT_DELAY_START          139	//延时启动
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//火灾自动报警系统设备操作、部件操作类型代码表
#define CRT_DEV_NONE            0//无操作
#define CRT_DEV_RESET           1//复位
#define CRT_DEV_SILENCER        2//消音
#define CRT_DEV_MANUAL_ALARM    3//手动报警    
#define CRT_DEV_SHIELD          4//屏蔽
#define CRT_DEV_SHIELD_CANCE    5//屏蔽解除
//#define CRT_    6//隔离 
//#define CRT_     7//隔离解除 
#define CRT_DEV_TEST            8//测试
#define CRT_DEV_CHECK           9//巡检
#define CRT_DEV_OK              10//确认
#define CRT_DEV_SELF_CHECK      11//自检
#define CRT_DEV_START           12//启动
#define CRT_DEV_DELAY_START     13//延时启动
//#define CRT_           14~127//预留
//#define CRT_           128~255//用户自定义
///////////////////////////////////////////////////////////////////////////////
#endif
int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop);
int dataRecv(int fd1, unsigned char* RxBuffer);
void send_data(int fd1, unsigned char* writemsg, int len);
int open232Port(char *rs232port, int baudrate);
int open485Port(char *rs485port, int baudrate);
void close232Port(int fd1);
void close485Port(int fd1);
int Set_485TX_Enable(int fd);  //set to hig
int Set_485RX_Enable(int fd);


#endif
