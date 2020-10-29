/*
 * @file SerialComm.h 声明文件, 基于boost::asio实现串口异步操作
 * @version 0.1
 * @date 2017-10-10
 * - 打开串口
 * - 设置串口参数
 * - 关闭串口
 * - 注册回调函数
 * - 读出数据
 * - 写入数据
 * - 异常处理
 */

#ifndef SERIALCOMM_H_
#define SERIALCOMM_H_

#include <string>
#include <boost/signals2.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/smart_ptr/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "AsioIOServiceKeep.h"

using std::string;
using boost::system::error_code;

class SerialComm : public boost::enable_shared_from_this<SerialComm> {
public:
	SerialComm();
	virtual ~SerialComm();

public:
	/* 数据类型 */
	typedef boost::shared_ptr<SerialComm> Pointer;
	/*!
	 * @brief 声明SerialComm回调函数类型
	 * @param _1 对象指针
	 * @param _2 错误代码. 0: 正确
	 */
	typedef boost::signals2::signal<void (Pointer, const boost::system::error_code&)> CallbackFunc;
	// 基于boost::signals2声明插槽类型
	typedef CallbackFunc::slot_type CBSlot;

protected:
	/* 数据类型 */
	typedef boost::unique_lock<boost::mutex> mutex_lock;	//< 互斥锁
	typedef boost::circular_buffer<char> crcbuff;	//< 循环缓冲区
	typedef boost::shared_array<char> carray;		//< 字符型数组

protected:
	/* 成员变量 */
	AsioIOServiceKeep keep_;		//< 提供io_service对象
	boost::asio::serial_port port_;	//< 串口
	CallbackFunc  cbrcv_;	//< receive回调函数
	CallbackFunc  cbsnd_;	//< send回调函数

	carray bufrcv_;		//< 单条接收缓冲区
	crcbuff crcrcv_;		//< 循环接收缓冲区
	crcbuff crcsnd_;		//< 循环发送缓冲区
	boost::mutex mtxrcv_;	//< 接收互斥锁
	boost::mutex mtxsnd_;	//< 发送互斥锁

public:
	/* 接口 */
	static Pointer Create();
	/*!
	 * @brief 尝试打开串口
	 * @param portname  串口名称
	 * @param baud_rate 波特率
	 * @return
	 * 串口打开结果
	 */
	bool Open(const string& portname, const int baud_rate = 9600);
	/*!
	 * @brief 尝试关闭串口
	 */
	void Close();
	/*!
	 * @brief 串口打开标志
	 * @return
	 * 串口是否已经打开
	 */
	bool IsOpen();
	/*!
	 * @brief 查找已接收信息中第一次出现flag的位置
	 * @param flag 标识字符串
	 * @param len  标识字符串长度
	 * @param from 从from开始查找标识符
	 * @return
	 * 标识串第一次出现位置. 若flag不存在则返回-1
	 */
	int Lookup(const char* flag, const int len, const int from = 0);
	/*!
	 * @brief 向串口发送信息
	 * @param buff 待发送信息
	 * @param len  待发送信息长度
	 * @return
	 * 已发送信息长度
	 */
	int Write(const char* buff, const int len);
	/*!
	 * @brief 从串口读出信息
	 * @param buff 读出信息缓存区
	 * @param len  期望读出信息长度
	 * @param from 从from开始读出信息
	 * @return
	 * 实际读出信息长度
	 */
	int Read(char* buff, const int len, const int from = 0);
	/*!
	 * @brief 注册read_some回调函数, 处理收到的网络信息
	 * @param slot 函数插槽
	 */
	void RegisterRead(const CBSlot& slot);
	/*!
	 * @brief 注册write_some回调函数, 处理网络信息发送结果
	 * @param slot 函数插槽
	 */
	void RegisterWrite(const CBSlot& slot);

protected:
	/* 功能 */
	/*!
	 * @brief 异步通信中, 处理串口数据读出
	 * @param ec 错误代码
	 * @param n  已读出数据长度, 量纲: 字节
	 */
	void handle_read(const error_code& ec, int n);
	/*!
	 * @brief 异步通信中, 处理串口数据写入
	 * @param ec 错误代码
	 * @param n  已写入数据长度, 量纲: 字节
	 */
	void handle_write(const error_code& ec, int n);
	/*!
	 * @brief 尝试接收串口信息
	 */
	void start_read();
	/*!
	 * @brief 尝试发送缓冲区数据
	 */
	void start_write();
};
typedef SerialComm::Pointer SerialPtr;

#endif /* SERIALCOMM_H_ */
