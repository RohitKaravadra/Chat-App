#pragma once
#include "client.h"
#include <thread>
#include "GUI.h"

enum PanelFlags
{
	plain = ImGuiWindowFlags_NoTitleBar |
	ImGuiWindowFlags_NoScrollbar |
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove,

	noMoveResize = ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoCollapse,
};

class ClientInterface
{
	const int width = 1280;
	const int height = 720;

	const char* host = "127.0.0.1"; // Server IP address
	unsigned int port = 65432;

	Client client;
	GUIWindow* canvas;

	std::string inputMsg;

	int selectedUser = 0;
	std::vector<std::string> user;
	std::vector<std::vector<std::string>> chat;
public:

	void create()
	{
		canvas = new GUIWindow(width, height, "Client");

		updateUsers();
		updateChat();

		if (InitWinSock())
			if (client.create());
	}

	void updateUsers()
	{
		for (int i = 0; i < 10; i++)
			user.emplace_back("User " + std::to_string(i));
	}

	void updateChat()
	{
		for (int i = 0; i < 10; i++)
			chat.emplace_back(std::vector<std::string>());
	}

	void OnSend()
	{
		if (strlen(inputMsg.c_str()) > 0)
		{
			// The button was pressed, process the input string
			if (client.send(inputMsg, selectedUser))
				chat[selectedUser].emplace_back("You : " + inputMsg);
			inputMsg.clear();
		}
	}

	void LogInPanel()
	{
		ImVec2 size = ImVec2(300, 300);
		static char name[20];

		ImGui::SetNextWindowFocus();
		if (ImGui::Begin("Log In", NULL, PanelFlags::noMoveResize))
		{
			ImGui::SetWindowSize(size);
			ImGui::SetWindowPos(ImVec2((width - size.x) / 2, (height - size.y) / 2));

			// Create an input text field
			ImGui::InputText(" - Name ", name, 20, ImGuiInputTextFlags_CharsNoBlank);

			// Create a button
			if (ImGui::Button("Log In", ImVec2(100, 50)) && std::string(name).size() > 3)
			{
				client.name = std::string(name);
				if (client.connect(host, port))
					name[0] = '\0';
			}

			ImGui::End();
		}
	}

	void UpdateChatPanel()
	{
		if (ImGui::Begin("Chat", NULL, PanelFlags::noMoveResize))
		{
			ImGui::SetWindowSize(ImVec2(width * 2 / 3, height - 50));
			ImGui::SetWindowPos(ImVec2(width * 1 / 3, 0));

			for (int i = 0; i < chat.size(); i++)
			{
				if (selectedUser == i)
					for (int j = 0; j < chat[i].size(); j++)
						ImGui::TextWrapped(chat[i][j].c_str());
			}

			ImGui::End();
		}
	}

	void UpdateUserPanel()
	{
		if (ImGui::Begin("Users", NULL, PanelFlags::noMoveResize))
		{
			ImGui::SetWindowSize(ImVec2(width * 1 / 3, height));
			ImGui::SetWindowPos(ImVec2(0, 0));

			for (int i = 0; i < user.size(); i++)
				if (ImGui::Selectable(user[i].c_str(), selectedUser == i))
					selectedUser = i;

			ImGui::End();
		}
	}


	void UpdateInputPanel()
	{
		int h = 50;

		if (ImGui::Begin("Input", NULL, PanelFlags::plain))
		{
			ImGui::SetWindowSize(ImVec2(width * 2 / 3, h));
			ImGui::SetWindowPos(ImVec2(width * 1 / 3, height - h));

			// Create an input text field
			if (ImGui::InputText(" ", &inputMsg, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_ElideLeft))
				OnSend();

			ImGui::SameLine();

			// Create a button
			if (ImGui::Button("Send", ImVec2(100, h - 10)))
				OnSend();

			ImGui::End();
		}
	}

	void UpdatePanels()
	{
		if (ImGui::Begin("Input", NULL, PanelFlags::plain))
		{
			ImGui::SetWindowSize(ImVec2(width, height));
			ImGui::SetWindowPos(ImVec2(0, 0));

			if (client.isConnected())
			{
				UpdateUserPanel();
				UpdateChatPanel();
				UpdateInputPanel();
			}
			else
				LogInPanel();

			ImGui::End();
		}
	}

	/// <summary>
	/// Update all panels of client interface
	/// </summary>
	/// <returns> true if client is still running false if client closed</returns>
	bool update()
	{
		canvas->checkInputs();

		// check for window closed
		if (canvas->quit())
		{
			if (client.isConnected()) // check for client connection
				client.disconnect(); // disconnect client from server

			return false;
		}

		canvas->startFrame(); // start imGui frame
		UpdatePanels(); // update all panels
		canvas->endFrame(); // end and render imGui frame

		return true;
	}

	// destructor
	~ClientInterface()
	{
		client.destroy();
		CleanWinSock();

	}
};