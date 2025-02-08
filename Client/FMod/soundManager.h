#pragma once
#include <unordered_map>#
#include <fmod.hpp>
#include <fmod_errors.h>
#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>
#include <conio.h>
#include <vector>

class SoundManager
{
	FMOD::System* system;
	std::unordered_map<std::string, FMOD::Sound*> sounds;

public:

	SoundManager()
	{
		FMOD::System_Create(&system);
		system->init(512, FMOD_INIT_NORMAL, NULL);
	}

	bool load(std::string filename)
	{
		if (sounds.find(filename) == sounds.end())
		{
			FMOD::Sound* sound = NULL;
			system->createSound(filename.c_str(), FMOD_DEFAULT, NULL, &sound);
			sounds.insert({ filename, sound });
			return true;
		}
		return false;
	}

	bool play(std::string filename)
	{
		if (sounds.find(filename) == sounds.end())
			return false;

		system->playSound(sounds[filename], NULL, false, NULL);
		return true;
	}

	~SoundManager()
	{
		for (auto i = sounds.begin(); i != sounds.end(); i++)
			i->second->release();

		system->close();
		system->release();
	}
};