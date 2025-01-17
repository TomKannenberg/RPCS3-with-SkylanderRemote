#pragma once

#include "Emu/Io/usb_device.h"
#include "Utilities/mutex.h"
#include "skyslots.h"
#include <queue>
#include <thread>
#include <filesystem>

struct skylander
{
	fs::file sky_file;
	u8 status = 0;
	std::queue<u8> queued_status;
	std::array<u8, 0x40 * 0x10> data{};
	u32 last_id = 0;
	void save();
};

class sky_portal
{
public:
	void activate();
	void load_skylander_app(std::string name, u32 slot);
	void tcp_loop();
	void deactivate();
	void set_leds(u8 r, u8 g, u8 b);

	void get_status(u8* buf);
	void query_block(u8 sky_num, u8 block, u8* reply_buf);
	void write_block(u8 sky_num, u8 block, const u8* to_write_buf, u8* reply_buf);

	bool remove_skylander(u8 sky_num);
	u8 load_skylander(u8* buf, fs::file in_file);
	u8 load_skylander_slot(u8* buf, fs::file in_file, u8 slot_num);

protected:
	shared_mutex sky_mutex;
	std::thread tcpthread;
	std::string coresPathStr;
	std::string topsPathStr;
	std::string botsPathStr;
	std::string trapsPathStr;
	bool accessAllowed = true;

	bool activated       = true;
	bool first[8] = { 1 };

	bool inUse = false;
	u8 interrupt_counter = 0;
	u8 r = 0, g = 0, b = 0;



	skylander skylanders[8];
};

extern sky_portal g_skyportal;

class usb_device_skylander : public usb_device_emulated
{
public:
	usb_device_skylander(const std::array<u8, 7>& location);
	~usb_device_skylander();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;

protected:
	std::queue<std::array<u8, 32>> q_queries;
};
