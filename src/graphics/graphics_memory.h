#pragma once
#include <deque>
#include <functional>

struct DeletionQueue {
	std::deque<std::function<void()>> _deletors;
	void PushFunction(std::function<void()>&& function);
	void Flush();
};