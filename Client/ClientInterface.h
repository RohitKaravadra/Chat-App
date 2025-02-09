#pragma once
#include <thread>
#include <algorithm>
#include "client.h"
#include "ImGui/GUI.h"
#include "FMod/soundManager.h"

const char* ping1Sound = "Sounds/ping1.wav";
const char* ping2Sound = "Sounds/ping2.wav";

// ImGui window flags for creating panels
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

// holds client GUI
class ClientInterface
{
	const int width = 700;		// width for main window
	const int height = 500;		// height for main window

	const char* host = "127.0.0.1"; // Server IP address
	unsigned int port = 65432;		// server port

	Client client;				// client object
	GUIWindow* canvas;			// ImGui window wrapper object

	SoundManager soundManager;	// FMOD sound manager object

	std::string inputMsg;		// input field message 

	int selectedUser = 0;		// current selected user to chat with
	bool scrollEnd = false;		// to check if to scroll window to latest data

public:

	// create client user interface
	void create()
	{
		canvas = new GUIWindow(width, height, "Client");

		soundManager.load(ping1Sound);
		soundManager.load(ping2Sound);

		if (InitWinSock())
			if (client.create());
	}

	// send message to selected user
	void OnSend()
	{
		if (strlen(inputMsg.c_str()) > 0)
		{
			// The button was pressed, process the input string
			if (client.sendMessage(inputMsg, selectedUser))
				inputMsg.clear();
		}
	}

	// draws login panel on window and keeps focus
	void LogInPanel()
	{
		ImVec2 size = ImVec2(300, 200);			// size of this window
		static char username[20];				// username input field

		ImGui::SetNextWindowFocus();			// keep focus on this window
		if (ImGui::Begin("Log In", NULL, PanelFlags::noMoveResize))
		{
			ImGui::SetWindowSize(size);
			ImGui::SetWindowPos(ImVec2((width - size.x) / 2, (height - size.y) / 2));

			// input name text
			ImGui::SetCursorPos(ImVec2(50, 50));
			ImGui::Text("Enter Your Username");
			ImGui::Text("Name must be > 3 characters");

			// Create an input text field
			ImGui::SetCursorPos(ImVec2(50, 100));
			// pressing enter accepts username
			if (ImGui::InputText("##Name ", username, 20, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue))
			{
				client.username = std::string(username);
				if (client.connect(host, port))
					username[0] = '\0';
			}
			else
			{
				// if enter not pressed check for send button press
				ImGui::SetCursorPos(ImVec2(100, 135));

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

	// draws chat panel and populates current users chat data
	void UpdateChatPanel()
	{
		if (ImGui::Begin("Chat", NULL, PanelFlags::noMoveResize))
		{
			ImGui::SetWindowSize(ImVec2(width * 2 / 3, height - 50));	// 2/3 of windows size
			ImGui::SetWindowPos(ImVec2(width * 1 / 3, 0));				// position on right side of parent window

			selectedUser = std::clamp(selectedUser, 0, client.getTotalUsers());	// get selected users username
			ImGui::TextWrapped(client.getChat(selectedUser).c_str());			// get and set selected users chat

			if (scrollEnd)		// check for scroll to bottom
			{
				scrollEnd = false;
				ImGui::SetScrollHereY(1.0f);
			}

			ImGui::End();
		}
	}

	// draw and populate connected users on user panel
	void UpdateUserPanel()
	{
		if (ImGui::Begin("Users", NULL, PanelFlags::noMoveResize))
		{
			ImGui::SetWindowSize(ImVec2(width * 1 / 3, height));
			ImGui::SetWindowPos(ImVec2(0, 0));

			// set text color to yellow for this client username
			ImGui::TextColored(ImVec4(1, 1, 0, 1), client.username.c_str());

			// populate all users as selectable
			int total = client.getTotalUsers();
			for (int i = 0; i < total; i++)
			{
				// update usernames
				if (ImGui::Selectable(client.getUsername(i).c_str(), selectedUser == i, 0))	// if new user selected, update current user and set scroll to bottom true
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

	// draw and populate input panel data
	void UpdateInputPanel()
	{
		int h = 50;		// height for input panel

		if (ImGui::Begin("Input", NULL, PanelFlags::plain))
		{
			ImGui::SetWindowSize(ImVec2(width * 2 / 3, h));
			ImGui::SetWindowPos(ImVec2(width * 1 / 3, height - h));	// position input panel to bottom of parent

			// Create an input text field
			// pressing enter accepts input and sends message to selected user
			if (ImGui::InputText(" ", &inputMsg, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_ElideLeft))
			{
				OnSend();
				ImGui::SetKeyboardFocusHere(-1);	// keep focus to input field if enter is pressed
				scrollEnd = true;					// scroll to bottom when sending message
			}

			ImGui::SameLine();

			// Create a button
			if (ImGui::Button("Send", ImVec2(100, h - 10)))
			{
				OnSend();
				scrollEnd = true;					// scroll to bottom when sending message
			}

			ImGui::End();
		}
	}

	// check and play notification sounds
	void playNotificationSound()
	{
		int chat = client.newMsgCheck();
		if (chat == -1)
			return;

		if (chat == 0)						// 0 for public message
			soundManager.play(ping1Sound);
		else								// anything else for private message
			soundManager.play(ping2Sound);
	}

	// draw and populate all panels
	void UpdatePanels()
	{
		if (ImGui::Begin("Input", NULL, PanelFlags::plain))
		{
			ImGui::SetWindowSize(ImVec2(width, height));
			ImGui::SetWindowPos(ImVec2(0, 0));

			if (client.isConnected())		// if draw and populate user, chat and input panel
			{
				UpdateUserPanel();
				UpdateChatPanel();
				UpdateInputPanel();
			}	
			else							// if not connected draw login panel
				LogInPanel();

			playNotificationSound();		// check for notification if any

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