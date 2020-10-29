/**
 * 测试长春人卫站1.2米望远镜时间锁存器, 时间锁存器为CCD相机提供时间基准
 * 测试功能:
 * - 在Linux下通过串口连接锁存器
 * - 尝试向锁存器发送曝光信号, 接收解析锁存器的反馈
 * - 尝试5次操作
 * @note
 * - Linux下串口名称格式为: ttyxxx
 */

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <string.h>
#include "GLog.h"
#include "SerialComm.h"

using namespace boost::placeholders;

GLog _gLog(stdout);

union uint32_u {
	uint32_t u32;
	uint8_t  u8[4];

public:
	uint32_u(uint32_t x) {
		u32 = x;
	}
};

union uint16_u {
	uint16_t u16;
	uint8_t  u8[2];

public:
	uint16_u(uint16_t x) {
		u16 = x;
	}

	uint16_u(uint8_t x1, uint8_t x2) {
		u8[0] = x1;
		u8[1] = x2;
	}
};

struct timelatch_trigger {
	uint8_t head[3];
	uint8_t begin[3];	// begin可替代帧间延迟, 即大于读出时间的最小整数
	uint8_t width[2];
	uint8_t count[2];
	uint8_t checksum;
	uint8_t tail[2];

public:
	timelatch_trigger() {
		memset(this, 0, sizeof(timelatch_trigger));
		head[0] = '$';
		head[1] = 'M';
		head[2] = 'C';
		set_count(1);
		tail[0] = 0x0D;
		tail[1] = 0x0A;
	}

	/*!
	 * @brief 设置触发时间
	 * @param x 相对当前时刻的触发时间, 量纲: 毫秒
	 */
	void set_begin(uint32_t x) {
		uint32_u u32(x);
		begin[0] = u32.u8[0];
		begin[1] = u32.u8[1];
		begin[2] = u32.u8[2];
	}

	/*!
	 * @brief 设置脉宽间隔
	 * @param x 脉宽间隔, 量纲: 毫秒
	 */
	void set_width(uint16_t x) {
		uint16_u u16(x);
		width[0] = u16.u8[0];
		width[1] = u16.u8[1];
	}

	void set_count(uint16_t x) {
		uint16_u u16(x);
		count[0] = u16.u8[0];
		count[1] = u16.u8[1];
	}

	void complete() {
		int i, n(sizeof(timelatch_trigger) - sizeof(checksum) - sizeof(tail));
		uint8_t *ptr = (uint8_t*) this;
		checksum = 0;
		for (i = 0; i < n; ++i) checksum += ptr[i];
		checksum &= 0x7F;
	}

	boost::shared_array<char> to_string(int &n) {
		boost::shared_array<char> output;
		n = sizeof(timelatch_trigger);
		output.reset(new char[n]);
		memcpy(output.get(), this, n);
		return output;
	}
};

struct timelatch_timemark {
	uint8_t head[3];
	uint8_t serialno;
	/* asc: 上升沿 */
	uint8_t ascyear[2];
	uint8_t ascmonth;
	uint8_t ascday;
	uint8_t aschour;
	uint8_t ascmin;
	uint8_t ascsecond;
	uint8_t ascmillisec[2];
	uint8_t ascmicrosec;	//< 10微秒
	/* asc: 上升沿 */
	/* w: 脉宽*/
	uint8_t wsecond[2];
	uint8_t wmillisec[2];
	uint8_t wmicrosec;
	/* w: 脉宽*/
	uint8_t checksum;
	uint8_t tail[2];

public:
	bool from_string(const char* rcvd, const int n) {
		if (n != sizeof(timelatch_timemark)) return false;
		memcpy(this, rcvd, n);
		return is_valid();
	}

	/*!
	 * @brief 检查数据是否有效
	 * @return
	 * 校验和是否一致
	 */
	bool is_valid() {
		uint8_t sum(0);
		uint8_t *ptr = (uint8_t*) this;
		int i, n(sizeof(timelatch_timemark) - sizeof(checksum) - sizeof(tail));
		for (i = 0; i < n; ++i) sum += ptr[i];
		sum &= 0x7F;
		return sum == checksum;
	}

	uint16_t asc_year() {
		uint16_u x(ascyear[0], ascyear[1]);
		return x.u16;
	}

	/*!
	 * @brief 计算秒位数
	 * @return
	 * 秒位数
	 */
	double asc_second() {
		uint16_u milsec(ascmillisec[0], ascmillisec[1]);
		double t = ascsecond + milsec.u16 * 0.001 + ascmicrosec * 1.0E-5;
		return t;
	}

	/*!
	 * @brief 计算脉宽
	 * @return
	 * 脉宽, 量纲: 秒
	 */
	double pulse_width() {
		uint16_u sec(wsecond[0], wsecond[1]);
		uint16_u milsec(wmillisec[0], wmillisec[1]);
		double t = sec.u16 + milsec.u16 * 0.001 + wmicrosec * 1.0E-5;
		return t;
	}
};

void on_read(SerialPtr port, const boost::system::error_code& ec) {
	if (!ec) {
		int to_read(22), has_read;
		boost::shared_array<char> buff;
		buff.reset(new char[to_read]);
		if ((has_read = port->Read(buff.get(), to_read)) == to_read) {
			timelatch_timemark mark;
			if (mark.from_string(buff.get(), has_read)) {
				_gLog.Write("Serial no = %2d, Time = %d-%02d-%02dT%02d:%02d:%8.5f, Pulse Width = %8.5f",
						mark.serialno,
						mark.asc_year(), mark.ascmonth, mark.ascday,
						mark.aschour, mark.ascmin, mark.asc_second(),
						mark.pulse_width());
			}
			else {
				_gLog.Write(LOG_FAULT, "wrong checksum. %s",
						buff.get());
			}
		}
		else {
			_gLog.Write(LOG_FAULT, "had_read = %d does not equal to to_read = %d. %s",
					has_read, to_read, buff.get());
		}
	}
	else {
		_gLog.Write(LOG_FAULT, "on_read() error: %s", ec.message().c_str());
		port->Close();
	}
}

void on_send(SerialPtr port, const boost::system::error_code& ec) {
	if (!ec) {
		_gLog.Write("Sent over");
	}
	else {
		_gLog.Write(LOG_FAULT, "on_send() error: %s", ec.message().c_str());
		port->Close();
	}
}

void thread_send(SerialPtr port, boost::asio::io_service* ios) {
	int n, t;
	int begin(1000), width(1000), count(1);
	boost::shared_array<char> output;
	timelatch_trigger trigger;

	trigger.set_begin(begin);
	trigger.set_width(width);
	trigger.set_count(count);
	t = (begin + width) * count;

	boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
	while (port->IsOpen()) {
		output = trigger.to_string(n);
		port->Write(output.get(), n);
		_gLog.Write("Sening: %s", output.get());

		boost::this_thread::sleep_for(boost::chrono::milliseconds(t));
	}
	ios->stop();
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf ("Usage:\n\ttimelatch port_name\n");
		return -1;
	}

	boost::asio::io_service ios;
	boost::asio::signal_set signals(ios, SIGINT, SIGTERM);  // interrupt signal
	signals.async_wait(boost::bind(&boost::asio::io_service::stop, &ios));

	SerialPtr serial = SerialComm::Create();
	const SerialComm::CBSlot& slot1 = boost::bind(&on_read, _1, _2);
	const SerialComm::CBSlot& slot2 = boost::bind(&on_send, _1, _2);
	serial->RegisterRead(slot1);
	serial->RegisterWrite(slot2);

	if (!serial->Open(argv[1], 115200))
		printf ("failed to open port<%s>\n", argv[1]);
	else {
		printf ("Press Ctrl+C to exit program\n");
		boost::thread thrd(boost::bind(&thread_send, serial, &ios));
		ios.run();
		serial->Close();
		thrd.join();
	}

	return 0;
}
