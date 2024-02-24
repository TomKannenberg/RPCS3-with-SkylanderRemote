#include "stdafx.h"
#include "Skylander.h"
#include "Emu/Cell/lv2/sys_usbd.h"

#include "util/asm.hpp"

LOG_CHANNEL(skylander_log, "skylander");

sky_portal g_skyportal;

void skylander::save()
{
	if (!sky_file)
	{
		skylander_log.error("Tried to save skylander to file but no skylander is active!");
		return;
	}

	{
		sky_file.seek(0, fs::seek_set);
		sky_file.write(data.data(), 0x40 * 0x10);
	}
}

void sky_portal::activate()
{


	std::lock_guard lock(sky_mutex);
	if (activated)
	{
		// If the portal was already active no change is needed
		return;
	}

	// If not we need to advertise change to all the figures present on the portal
	for (auto& s : skylanders)
	{
		if (s.status & 1)
		{
			s.queued_status.push(3);
			s.queued_status.push(1);
		}
	}

	wchar_t buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::filesystem::path exePath(buffer);
	std::filesystem::path coresPath = exePath.parent_path() / "Skylanders" / "Cores";
	std::filesystem::path topsPath = exePath.parent_path() / "Skylanders" / "Swappers" / "Tops";
	std::filesystem::path botsPath = exePath.parent_path() / "Skylanders" / "Swappers" / "Bottoms";
	std::filesystem::path trapsPath = exePath.parent_path() / "Skylanders" / "Traps";
	coresPathStr = coresPath.string() + "/";
	topsPathStr = topsPath.string() + "/";
	botsPathStr = botsPath.string() + "/";
	trapsPathStr = trapsPath.string() + "/";

	std::thread tcpthread(&sky_portal::tcp_loop, this);
	tcpthread.detach();

	activated = true;
}

void sky_portal::tcp_loop() {

	WSADATA wsaData;
	SOCKET serverSocket, clientSocket;
	struct sockaddr_in serverAddr, clientAddr;
	int clientAddrSize = sizeof(clientAddr);
	char buffer[1024];

	std::string receivedData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "TCP SETUP FAIL STARTUP!" << std::endl;
		return;	
	}

	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (serverSocket == INVALID_SOCKET)
	{
		std::cout << "TCP SETUP FAIL INVALID SOCKET!" << std::endl;
		return;
		WSACleanup();
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(187);

	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cout << "TCP SETUP FAIL BINDING!" << std::endl;
		return;
		closesocket(serverSocket);
		WSACleanup();
	}

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "TCP SETUP FAIL LISTEN!" << std::endl;
		return;
		closesocket(serverSocket);
		WSACleanup();
	}

	while (true)
	{
		clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
		if (clientSocket == INVALID_SOCKET)
		{
			std::cout << "TCP SETUP FAIL MID RUN!" << std::endl;
			closesocket(serverSocket);
			WSACleanup();
			return;
		}

		while (true)
		{
			int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
			if (bytesReceived > 0)
			{
				buffer[bytesReceived] = '\0';
				receivedData = buffer;
				receivedData.pop_back();
				std::cout << "Received from client: " << receivedData << std::endl;
				char sslot = receivedData[0];
				u32 slot = atoi(&sslot);
				receivedData = receivedData.substr(1);
				std::string* skylanderToSend = &coresPathStr;

				std::cout << slot << " is the received numba!" << std::endl;

				if (slot > 0)
				{

					switch (slot)
					{
						case 1:
							skylanderToSend = &topsPathStr;
							std::cout << "TopLander" << std::endl;
							break;

						case 2:
							skylanderToSend = &botsPathStr;
							std::cout << "BottomLander" << std::endl;
							break;
						case 3:
							skylanderToSend = &trapsPathStr;
							std::cout << "Trap" << std::endl;
							break;

					}

					--slot;

				}

				load_skylander_app(*skylanderToSend + receivedData + ".sky", slot);
			}
			else if (bytesReceived == 0)
			{
				break;
			}
			else
			{
				break;
			}
		}

		closesocket(clientSocket);
	}
}
 
void sky_portal::load_skylander_app(std::string name, u32 slot) {

	

	std::cout << "Error1" << std::endl;
	std::cout << "open file! " << name << std::endl;

	inUse = true;

	fs::file sky_file(name, fs::read + fs::write + fs::lock);

	if (!sky_file)
	{
		std::cout << "Failed to open file! " << name << std::endl;
		return;
	}

	std::cout << "Error2" << std::endl;

	std::array<u8, 0x40 * 0x10> data;
	if (sky_file.read(data.data(), data.size()) != data.size())
	{

		std::cout << "Failed to copy file!" << std::endl;
		return;
	}

	std::cout << "Error3" << std::endl;

	if (auto slot_infos = sky_slots[slot])
	{
		std::cout << "Error4i" << std::endl;
		auto [cur_slot, id, var] = slot_infos.value();
		std::cout << "Error5i" << std::endl;
		inUse = false;
		g_skyportal.remove_skylander(cur_slot);
		inUse = true;
		std::cout << "Error6i" << std::endl;
		sky_slots[slot] = {};
		std::cout << "Error7i" << std::endl;
	}

	std::cout << "Error8" << std::endl;

	u16 sky_id = reinterpret_cast<le_t<u16>&>(data[0x10]);
	u16 sky_var = reinterpret_cast<le_t<u16>&>(data[0x1C]);

	std::cout << "Error9" << std::endl;

	inUse = false;
	u8 portal_slot = g_skyportal.load_skylander_slot(data.data(), std::move(sky_file), slot);

	first[portal_slot] = false;

	std::cout << slot << " IS THE SLOT" << std::endl;
	std::cout << portal_slot << " IS THE FINAL SLOT" << std::endl;

	sky_slots[slot] = std::tuple(slot, sky_id, sky_var);

	std::cout << "Succesfully loaded " << name << " !" << std::endl;
}

void sky_portal::deactivate()
{
	std::cout << "D!" << std::endl;
	std::lock_guard lock(sky_mutex);

	for (auto& s : skylanders)
	{
		// check if at the end of the updates there would be a figure on the portal
		if (!s.queued_status.empty())
		{
			s.status        = s.queued_status.back();
			s.queued_status = std::queue<u8>();
		}



		s.status &= 1;
	}

	activated = false;
}

void sky_portal::set_leds(u8 r, u8 g, u8 b)
{
	std::cout << "L!" << std::endl;
	std::lock_guard lock(sky_mutex);
	this->r = r;
	this->g = g;
	this->b = b;
}

void sky_portal::get_status(u8* reply_buf)
{
	std::lock_guard lock(sky_mutex);

	u16 status = 0;

	for (int i = 7; i >= 0; i--)
	{
		auto& s = skylanders[i];

		if (!s.queued_status.empty())
		{
			s.status = s.queued_status.front();
			s.queued_status.pop();
		}

		status <<= 2;
		status |= s.status;
	}

	std::memset(reply_buf, 0, 0x20);
	reply_buf[0] = 0x53;
	write_to_ptr<le_t<u16>>(reply_buf, 1, status);
	reply_buf[5] = interrupt_counter++;
	reply_buf[6] = 0x01;
}

void sky_portal::query_block(u8 sky_num, u8 block, u8* reply_buf)
{
	
	std::lock_guard lock(sky_mutex);

	const auto& thesky = skylanders[sky_num];

	reply_buf[0] = 'Q';
	reply_buf[2] = block;
	if (thesky.status & 1)
	{
		reply_buf[1] = (0x10 | sky_num);
		memcpy(reply_buf + 3, thesky.data.data() + (16 * block), 16);
	}
	else
	{
		reply_buf[1] = sky_num;
	}
}

void sky_portal::write_block(u8 sky_num, u8 block, const u8* to_write_buf, u8* reply_buf)
{
	std::lock_guard lock(sky_mutex);
	auto& thesky = skylanders[sky_num];
	reply_buf[0] = 'W';
	reply_buf[2] = block;
	if (thesky.status & 1)
	{
		reply_buf[1] = (0x10 | sky_num);
		memcpy(thesky.data.data() + (block * 16), to_write_buf, 16);
		thesky.save();
	}
	else
	{
		reply_buf[1] = sky_num;
	}
}

bool sky_portal::remove_skylander(u8 sky_num)
{
	if (first[sky_num])
	{
		return false;
	}

	std::cout << "R!" << std::endl;

	while (inUse)
	{
		Sleep(1);
	}

	std::cout << "R2!" << std::endl;

	if (false)
	{
		std::cout << "R3!" << std::endl;
		std::lock_guard lock(sky_mutex);
	}

	std::cout << "R3!" << std::endl;
	auto& thesky = skylanders[sky_num];

	if (thesky.status & 1)
	{
		std::cout << "R4!" << std::endl;
		thesky.status = 2;
		std::cout << "R5!" << std::endl;
		thesky.queued_status.push(2);
		std::cout << "R6!" << std::endl;
		thesky.queued_status.push(0);
		std::cout << "R7!" << std::endl;
		thesky.sky_file.close();
		std::cout << "R8!" << std::endl;
		return true;
	}

	std::cout << "R4!" << std::endl;

	return false;
}

u8 sky_portal::load_skylander(u8* buf, fs::file in_file)
{
	std::cout << "L!" << std::endl;
	if (false)
	{
		//std::lock_guard lock(sky_mutex);
	}

	const u32 sky_serial = read_from_ptr<le_t<u32>>(buf);
	u8 found_slot  = 0xFF;

	// mimics spot retaining on the portal
	for (u8 i = 0; i < 8; i++)
	{
		if ((skylanders[i].status & 1) == 0)
		{
			if (skylanders[i].last_id == sky_serial)
			{
				found_slot = i;
				break;
			}

			if (i < found_slot)
			{
				found_slot = i;
			}
		}
	}

	ensure(found_slot != 0xFF);

	skylander& thesky = skylanders[found_slot];
	memcpy(thesky.data.data(), buf, thesky.data.size());
	thesky.sky_file = std::move(in_file);
	thesky.status   = 3;
	thesky.queued_status.push(3);
	thesky.queued_status.push(1);
	thesky.last_id = sky_serial;

	return found_slot;
}

u8 sky_portal::load_skylander_slot(u8* buf, fs::file in_file, u8 slot_num)
{
	std::cout << "L!" << std::endl;
	while (inUse)
	{
		//std::lock_guard lock(sky_mutex);
		Sleep(1);
	}

	const u32 sky_serial = read_from_ptr<le_t<u32>>(buf);

	std::cout << "L1!" << std::endl;

	ensure(slot_num != 0xFF);

	std::cout << "L2!" << std::endl;

	skylander& thesky = skylanders[slot_num];
	std::cout << "L3!" << std::endl;
	memcpy(thesky.data.data(), buf, thesky.data.size());
	std::cout << "L4!" << std::endl;
	thesky.sky_file = std::move(in_file);
	std::cout << "L5!" << std::endl;
	thesky.status = 3;
	std::cout << "L6!" << std::endl;
	thesky.queued_status.push(3);
	std::cout << "L7!" << std::endl;
	thesky.queued_status.push(1);
	std::cout << "L8!" << std::endl;
	thesky.last_id = sky_serial;
	std::cout << "L9!" << std::endl;

	return slot_num;
}

usb_device_skylander::usb_device_skylander(const std::array<u8, 7>& location)
	: usb_device_emulated(location)
{
	std::cout << "U!" << std::endl;
	device        = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 0x40, 0x1430, 0x0150, 0x0100, 0x01, 0x02, 0x00, 0x01});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{0x0029, 0x01, 0x01, 0x00, 0x80, 0xFA}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID, UsbDeviceHID{0x0111, 0x00, 0x01, 0x22, 0x001d}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x03, 0x40, 0x01}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x02, 0x03, 0x40, 0x01}));
}

usb_device_skylander::~usb_device_skylander()
{
}

void usb_device_skylander::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake = true;

	// Control transfers are nearly instant
	switch (bmRequestType)
	{
	// HID Host 2 Device
	case 0x21:
		switch (bRequest)
		{
		case 0x09:
			transfer->expected_count  = buf_size;
			transfer->expected_result = HC_CC_NOERR;
			// 100 usec, control transfers are very fast
			transfer->expected_time = get_timestamp() + 100;

			std::array<u8, 32> q_result = {};

			switch (buf[0])
			{
			case 'A':
			{
				// Activate command
				ensure(buf_size == 2 || buf_size == 32);
				q_result = {0x41, buf[1], 0xFF, 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				    0x00, 0x00};
				q_queries.push(q_result);
				g_skyportal.activate();
				break;
			}
			case 'C':
			{
				// Set LEDs colour
				ensure(buf_size == 4 || buf_size == 32);
				g_skyportal.set_leds(buf[1], buf[2], buf[3]);
				break;
			}
			case 'J':
			{
				// Sync status from game?
				ensure(buf_size == 7);
				q_result[0] = 0x4A;
				q_queries.push(q_result);
				break;
			}
			case 'L':
			{
				// Trap Team Portal Side Lights
				ensure(buf_size == 5);
				// TODO Proper Light side structs
				break;
			}
			case 'M':
			{
				// Audio Firmware version
				// Return version of 0 to prevent attempts to
				// play audio on the portal
				ensure(buf_size == 2);
				q_result = {0x4D, buf[1], 0x00, 0x19};
				q_queries.push(q_result);
				break;
			}
			case 'Q':
			{
				// Queries a block
				ensure(buf_size == 3 || buf_size == 32);

				const u8 sky_num = buf[1] & 0xF;
				ensure(sky_num < 8);
				const u8 block = buf[2];
				ensure(block < 0x40);

				g_skyportal.query_block(sky_num, block, q_result.data());
				q_queries.push(q_result);
				break;
			}
			case 'R':
			{
				// Shutdowns the portal
				ensure(buf_size == 2 || buf_size == 32);
				q_result = {
				    0x52, 0x02, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
				q_queries.push(q_result);
				g_skyportal.deactivate();
				break;
			}
			case 'S':
			{
				// ?
				ensure(buf_size == 1 || buf_size == 32);
				break;
			}
			case 'V':
			{
				// ?
				ensure(buf_size == 4);
				break;
			}
			case 'W':
			{
				// Writes a block
				ensure(buf_size == 19 || buf_size == 32);

				const u8 sky_num = buf[1] & 0xF;
				ensure(sky_num < 8);
				const u8 block = buf[2];
				ensure(block < 0x40);

				g_skyportal.write_block(sky_num, block, &buf[3], q_result.data());
				q_queries.push(q_result);
				break;
			}
			default:
				skylander_log.error("Unhandled Query: buf_size=0x%02X, Type=0x%02X, bRequest=0x%02X, bmRequestType=0x%02X", buf_size, (buf_size > 0) ? buf[0] : -1, bRequest, bmRequestType);
				break;
			}
			break;
		}
		break;
	default:
		// Follow to default emulated handler
		usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
		break;
	}
}

void usb_device_skylander::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	ensure(buf_size == 0x20);

	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	
	if (endpoint == 0x02)
	{
		// Audio transfers are fairly quick(~1ms)
		transfer->expected_time = get_timestamp() + 1000;
		// The response is simply the request, echoed back
	}
	else
	{
		// Interrupt transfers are slow(~22ms)
		transfer->expected_time = get_timestamp() + 22000;
		if (!q_queries.empty())
		{
			memcpy(buf, q_queries.front().data(), 0x20);
			q_queries.pop();
		}
		else
		{
			g_skyportal.get_status(buf);
		}
	}
}
