#pragma once

#include <iostream>
#include <mutex>
#include <vector>

struct MsgNode {
	MsgNode* next = nullptr;
	std::string data;
	MsgNode(std::string _data) :data(_data) {};
};

class MsgQueue {
	MsgNode* head;
	MsgNode* tail;
	std::mutex mtx;
public:
	MsgQueue() {
		head = tail = nullptr;
	}

	void enqueue(std::string _data) {

		std::lock_guard<std::mutex> lock(mtx);

		MsgNode* n = new MsgNode(_data);
		if (tail == nullptr)
			head = tail = n;
		else {
			tail->next = n;
			tail = n;
		}
	}

	void display() {
		std::lock_guard<std::mutex> lock(mtx);

		std::cout << "H->";
		for (MsgNode* nptr = head; nptr != nullptr; nptr = nptr->next)
			std::cout << nptr->data << '\t';
		std::cout << "<-T" << std::endl;
	}

	bool dequeue(std::string& i) {
		if (head == nullptr) return false;

		std::lock_guard<std::mutex> lock(mtx);

		MsgNode* n = head;
		i = n->data;
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

	bool dequeueAll(std::vector<std::string>& _all)
	{
		if (head == nullptr) return false;

		mtx.lock();

		MsgNode* newHead = head;
		head = tail = nullptr;

		mtx.unlock();

		while (newHead != nullptr)
		{
			std::string data = newHead->data;
			_all.emplace_back(data);

			MsgNode* node = newHead;
			newHead = newHead->next;
			delete node;
		}

		return true;
	}
};
