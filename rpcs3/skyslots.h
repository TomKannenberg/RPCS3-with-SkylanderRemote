#pragma once
#include <optional>
#include "util/types.hpp"
#include <iostream>
#include <Windows.h>

constexpr auto UI_SKY_NUM = 8;
static std::optional<std::tuple<u8, u16, u16>> sky_slots[UI_SKY_NUM] = {};

#include <iostream>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>

// Define the type alias for u8
using u8 = uint8_t;

// Define a thread-safe queue wrapper
template <typename T>
class ThreadSafeQueue
{
private:
	std::queue<T> queue_;
	bool accessed = false;

public:

	ThreadSafeQueue() {}

	void access() {
		int i = 0;
		while (accessed)
		{
			Sleep(1);

			if (++i > 10000) {
				std::cout << "Took longer than 10 seconds" << std::endl;
				accessed = 1;
				return;
			}
		}

		accessed = 1;
	}

	// Back operation
	const T& back()
	{

		std::cout << "Back" << std::endl;
		access();
		T returnv = queue_.back();
		accessed = false;

		return returnv;
	}



	// Front operation
	const T& front()
	{
		std::cout << "Front" << std::endl;
		access();
		T returnv = queue_.front();
		accessed = false;
		return returnv;
	}


	// Push operation
	void push(const T& item)
	{
		std::cout << "Push" << std::endl;
		access();
		queue_.push(item);
		accessed = 0;
	}

	// Pop operation
	bool pop(T& item)
	{
		std::cout << "Pop" << std::endl;
		access();
		if (queue_.empty())
		{
			accessed = 0;
			return false;
		}
		item = queue_.front();
		queue_.pop();
		accessed = 0;
		return true;
	}

	// Pop operation (overloaded)
	void pop()
	{
		std::cout << "Pop" << std::endl;
		access();
		if (!queue_.empty())
		{
			queue_.pop();
		}
		accessed = 0;
	}

	// Check if the queue is empty
	bool empty()
	{
		std::cout << "Empty" << std::endl;
		access();
		bool returnv = queue_.empty();
		accessed = 0;
		return returnv;
	}

	// Get the size of the queue
	size_t size() const
	{
		std::cout << "Size" << std::endl;
		access();
		size_t returnv = queue_.size();
		accessed = 0;
		return returnv;
	}

	void operator=(const std::queue<T>& other)
	{
		std::cout << "operator=" << std::endl;
		access();
		queue_ = other;
		accessed = 0;
	}
};