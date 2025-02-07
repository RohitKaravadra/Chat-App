#pragma once
#include "client.h"
#include <thread>
#include "GUI.h"
#include <algorithm>

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
	const int width = 700;
	const int height = 500;

	const char* host = "127.0.0.1"; // Server IP address
	unsigned int port = 65432;

	Client client;
	GUIWindow* canvas;

	std::string inputMsg;

	int selectedUser = 0;
	bool scrollEnd = false;

public:

	void create()
	{
		canvas = new GUIWindow(width, height, "Client");

		if (InitWinSock())
			if (client.create());
	}

	void OnSend()
	{
		if (strlen(inputMsg.c_str()) > 0)
		{
			// The button was pressed, process the input string
			if (client.sendMessage(inputMsg, selectedUser))
				inputMsg.clear();
		}
	}

	void LogInPanel()
	{
		ImVec2 size = ImVec2(300, 200);
		static char username[20];

		ImGui::SetNextWindowFocus();
		if (ImGui::Begin("Log In", NULL, PanelFlags::noMoveResize))
		{
			ImGui::SetWindowSize(size);
			ImGui::SetWindowPos(ImVec2((width - size.x) / 2, (height - size.y) / 2));

			ImGui::SetCursorPos(ImVec2(50, 50));
			ImGui::Text("Enter Your Username");

			// Create an input text field
			ImGui::SetCursorPos(ImVec2(50, 80));
			if (ImGui::InputText("##Name ", username, 20, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue))
			{
				client.username = std::string(username);
				if (client.connect(host, port))
					username[0] = '\0';
			}
			else
			{
				ImGui::SetCursorPos(ImVec2(100, 125));

				// Create a button
				if (ImGui::Button("Log In", ImVec2(100, 50)) && std::string(username).size() > 3)
				{
					client.username = std::string(username);
					if (client.connect(host, port))
						username[0] = '\0';
				}
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

			selectedUser = std::clamp(selectedUser, 0, client.getTotalUsers());
			ImGui::TextWrapped(client.getChat(selectedUser).c_str());

			if (scrollEnd)
			{
				scrollEnd = false;
				ImGui::SetScrollHereY(1.0f);
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

			ImGui::TextColored(ImVec4(1, 1, 0, 1), client.username.c_str());
			int total = client.getTotalUsers();
			for (int i = 0; i < total; i++)
			{
				// update usernames
				if (ImGui::Selectable(client.getUsername(i).c_str(), selectedUser == i, 0))
				{
					selectedUser = i;
					scrollEnd = true;
				}

				// update new message count if any
				int newMsgs = client.getNewMsgCount(i);
				if (newMsgs != 0)
				{
					ImGui::SameLine();
					std::string notifi = "*(" + std::to_string(newMsgs) + ")";
					ImGui::Text(notifi.c_str());
				}
			}

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
			{
				OnSend();
				ImGui::SetKeyboardFocusHere(-1);
				scrollEnd = true;
			}

			ImGui::SameLine();

			// Create a button
			if (ImGui::Button("Send", ImVec2(100, h - 10)))
			{
				OnSend();
				scrollEnd = true;
			}

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

	// Update all panels of client interface
	// return true if client is still running false if client closed
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