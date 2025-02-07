#pragma once

#include <iostream>
#include <mutex>
#include <vector>


template<typename T>
class MsgQueue {

	struct MsgNode {
		MsgNode* next = nullptr;
		T info;
		MsgNode(T _data) :info(_data) {};
	};

	MsgNode* head;
	MsgNode* tail;

	std::mutex mtx;
public:
	MsgQueue() {
		head = tail = nullptr;
	}

	void enqueue(T _data) {

		std::lock_guard<std::mutex> lock(mtx);

		MsgNode* n = new MsgNode(_data);
		if (tail == nullptr)
			head = tail = n;
		else {
			tail->next = n;
			tail = n;
		}
	}

	bool dequeue(T& i) {
		if (head == nullptr) return false;

		std::lock_guard<std::mutex> lock(mtx);

		MsgNode* n = head;
		i = n->info;
		if (head == tail) // check for final element
			head = tail = nullptr;
		else
			head = head->next;
		delete n;

		return true;
	}

	bool isNull() {
		return head == nullptr;
	}

	bool dequeueAll(std::vector<T>& _all)
	{
		if (head == nullptr) return false;

		mtx.lock();

		MsgNode* newHead = head;
		head = tail = nullptr;

		mtx.unlock();

		while (newHead != nullptr)
		{
			T info = newHead->info;
			_all.emplace_back(info);

			MsgNode* node = newHead;
			newHead = newHead->next;
			delete node;
		}

		return true;
	}
};
