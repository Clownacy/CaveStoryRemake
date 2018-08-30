#include "org.h"
#include "sound.h"
#include "render.h"
#include "input.h"
#include "filesystem.h"
#include "mathUtils.h"

#include <vector>
#include <memory>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <SDL.h>

#include <string>

WAVE orgWaves[8];
DRUM orgDrums[8];

std::vector<char *> musicList;

MUSICINFO org;
Uint32 currentOrg = 0;
Uint32 prevOrg = 0;
Uint32 prevOrgPos = 0;

void organyaAllocNote(unsigned short alloc)
{
	for (int j = 0; j < 16; j++) {
		org.tdata[j].wave_no = 0;
		org.tdata[j].note_list = NULL; //I want to make the constructor
		org.tdata[j].note_p = (NOTELIST*)calloc(alloc, sizeof(NOTELIST)); //new NOTELIST[alloc];

		if (org.tdata[j].note_p == NULL)
			return;

		for (int i = 0; i < alloc; i++) {
			(org.tdata[j].note_p + i)->from = NULL;
			(org.tdata[j].note_p + i)->to = NULL;
			(org.tdata[j].note_p + i)->length = 0;
			(org.tdata[j].note_p + i)->pan = 0xFF;
			(org.tdata[j].note_p + i)->volume = 0xFF;
			(org.tdata[j].note_p + i)->y = 0xFF;
		}
	}
}

void organyaReleaseNote()
{
	for (int i = 0; i < 16; i++) {
		if (org.tdata[i].note_p != NULL)
			free(org.tdata[i].note_p);//delete org.tdata[i].note_p;
	}

	for (int i = 0; i < 8; i++)
	{
		if (orgDrums[i].wave != NULL)
			free(orgDrums[i].wave);
	}
}

//sound function things
SDL_AudioDeviceID orgSoundDev = 0;
SDL_AudioSpec orgSoundSpec;
SDL_AudioSpec orgWant;
const int orgSampleRate = 44100;

__int16 octfreq[12] = { 1, 2, 4, 8, 16, 32, 64, 128, 0, 0, 0, 0 };
__int16 notefreq[12] = { 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494 };

/*typedef struct {
	short wave_size;
	short oct_par;
	short oct_size;
}OCTWAVE;

OCTWAVE oct_wave[8] = {
	{ 256,  1, 4 },//0 Oct
	{ 256,  2, 8 },//1 Oct
	{ 128,  4, 12 },//2 Oct
	{ 128,  8, 16 },//3 Oct
	{ 64, 16, 20 },//4 Oct
	{ 32, 32, 24 },
	{ 16, 64, 28 },
	{ 8,128, 32 },
};

double freq_tbl[12] = { 261.62556530060, 277.18263097687, 293.66476791741, 311.12698372208, 329.62755691287, 349.22823143300, 369.99442271163, 391.99543598175, 415.30469757995, 440.00000000000, 466.16376151809, 493.88330125612 };
*/

void mixOrg(uint8_t *stream, int len)
{
	for (int i = 0; i < len; i++)
	{
		//Update
		int samplesPerBeat = (orgSampleRate / 1000) * org.wait;
		if (++org.samples > samplesPerBeat)
		{
			organyaPlayStep();
			org.samples = 0;
		}

		//Play waves
		for (int wave = 0; wave < 8; wave++)
		{
			int waveSamples = (int)(((long double)(octfreq[orgWaves[wave].key / 12] * (signed int)notefreq[orgWaves[wave].key % 12])
				* 32.0
				+ (long double)org.tdata[wave].freq
				- 1000.0)
				/ 44100.0
				* 4096.0);
			
			if (orgWaves[wave].playing)
			{
				orgWaves[wave].pos = (orgWaves[wave].pos + waveSamples) & 0xFFFFF;
				unsigned int s_offset_1 = orgWaves[wave].pos >> 12;

				int sample1 = orgWaves[wave].wave[s_offset_1];
				int sample2 = orgWaves[wave].wave[(s_offset_1 + 1) % 0x100];//(unsigned __int8)(((unsigned int)((s_offset_1 + 1) >> 31) >> 24) + s_offset_1 + 1) - ((unsigned int)((s_offset_1 + 1) >> 31) >> 24)];
				
				int val = (int)(sample1 + (sample2 - sample1) * ((double)(orgWaves[wave].pos & 0xFFF) / 4096.0));

				stream[2 * i] += (uint8_t)((long double)val * orgWaves[wave].volume * orgWaves[wave].volume_l / 2.0);
				stream[2 * i + 1] += (uint8_t)((long double)val * orgWaves[wave].volume * orgWaves[wave].volume_r / 2.0);
			}
		}

		//Play Drums
		for (int wave = 0; wave < 8; wave++)
		{
			unsigned int waveSamples = (unsigned int)((long double)(800 * orgDrums[wave].key + 100) / (double)orgSampleRate * 4096.0);

			if (orgDrums[wave].playing)
			{
				orgDrums[wave].pos += waveSamples;

				if ((orgDrums[wave].pos >> 12) >= orgDrums[wave].length)
				{
					orgDrums[wave].playing = false;
				}
				else
				{
					size_t s_offset_1 = orgDrums[wave].pos >> 12;

					int sample1 = orgDrums[wave].wave[s_offset_1] - 0x80;
					int sample2 = 0;//orgDrums[wave].wave[(s_offset_1 + 1) % 0x100] - 0x80; //(unsigned __int8)(((unsigned int)((s_offset_1 + 1) >> 31) >> 24) + s_offset_1 + 1) - ((unsigned int)((s_offset_1 + 1) >> 31) >> 24)];

					if ((orgDrums[wave].pos >> 12) < orgDrums[wave].length - 1)
						sample2 = orgDrums[wave].wave[s_offset_1 + 1] - 0x80;

					int val = (int)(sample1 + (sample2 - sample1) * ((double)(orgDrums[wave].pos & 0xFFF) / 4096.0));

					stream[2 * i] += (uint8_t)((long double)val * orgDrums[wave].volume * orgDrums[wave].volume_l / 2.0);
					stream[2 * i + 1] += (uint8_t)((long double)val * orgDrums[wave].volume * orgDrums[wave].volume_r / 2.0);
				}
			}
		}
	}
}

void orgCallback(void *userdata, uint8_t *stream, int len)
{
	memset(stream, 0, len);
	mixOrg(stream, len / 2);
}

// Load musicList from musicList.txt
void loadMusicList(const char *path)
{
	uint32_t c = 0;
	char *temp = nullptr;
	char *current;
	char *buf = nullptr;
	loadFile(path, reinterpret_cast<uint8_t**>(&buf));
	if (buf == nullptr)
		doError();
	current = buf;
	for (c = 0; buf[c] != 0; c++)
		if (buf[c] == '\n')
		{
			temp = static_cast<char*>(calloc(1, &buf[c] - current));
			if (temp == nullptr)
				doCustomError("Could not allocate temp memory");

			strncpy(temp, current, (&buf[c] - current) - 1);
			musicList.push_back(temp);
			current = &buf[c + 1];
		}
	temp = static_cast<char*>(calloc(1, &buf[c] - current));
	if (temp == nullptr)
		doCustomError("Could not allocate temp memory");

	strcpy(temp, current);
	for (c = 0; temp[c] != 0; c++)
		if (temp[c] == -3)
		{
			temp[c] = 0;
			break;
		}
	musicList.push_back(temp);
	free(buf);
}

void initOrganya()
{
	//Load music list
	loadMusicList("data/Org/musicList.txt");

	//Create sound device
	orgWant.channels = 2;
	orgWant.freq = orgSampleRate;
	orgWant.format = 0x8008;//AUDIO_S8;
	orgWant.samples = 1024;
	orgWant.callback = orgCallback;
	orgWant.userdata = nullptr;

	orgSoundDev = SDL_OpenAudioDevice(
		nullptr,
		0,
		&orgWant,
		&orgSoundSpec,
		0);

	if (orgSoundDev == 0)
		doError();

	SDL_PauseAudioDevice(orgSoundDev, 0);
}

///////////////////////////
////==ORGANYA  MELODY==////
///////////////////////////

long play_p; //Current playback position
NOTELIST *play_np[16]; //Currently ready to play notes
long now_leng[16] = { NULL }; //Length of notes during playback

//Change frequency
void changeNoteFrequency(int key, char track, long a)
{
	if (key != 0xFF)
	{
		orgWaves[track].key = key;
	}
}

unsigned char old_key[16] = { 255 }; //Sound during playback
unsigned char key_on[16] = { 0 }; //Key switch
unsigned char key_twin[16] = { 0 }; //Key used now

const long double orgVolumeMin = 0.04;
short pan_tbl[13] = { 0,43,86,129,172,215,256,297,340,383,426,469,512 };

void changeNotePan(unsigned char pan, char track)
{
	if (old_key[track] != 0xFF)
	{
		orgWaves[track].volume_l = 1.0f;
		orgWaves[track].volume_r = 1.0f;

		int pan_val = pan_tbl[pan];

		orgWaves[track].volume_l = (512.0f - (long double)pan_val) / 256.0f;
		orgWaves[track].volume_r = (long double)pan_val / 256.0f;

		if (orgWaves[track].volume_l > 1.0f) orgWaves[track].volume_l = 1.0f;
		if (orgWaves[track].volume_r > 1.0f) orgWaves[track].volume_r = 1.0f;
	}
}

void changeNoteVolume(long volume, char track)
{
	if (old_key[track] != 0xFF)
	{
		orgWaves[track].volume = orgVolumeMin + ((long double)volume / 255.0f * (1.0 - orgVolumeMin));
	}
}

//Play note
void playOrganyaNote(int key, int mode, char track, long freq)
{
	switch (mode)
	{
	case -1:
		if (old_key[track] == 0xFF) //Play
		{
			changeNoteFrequency(key, track, freq); //Set the frequency
			orgWaves[track].playing = true;
			old_key[track] = key;
			key_on[track] = 1;
		}
		else if (key_on[track] == 1 && old_key[track] == key) //Same note
		{
			orgWaves[track].pos = 0; //Reset
			key_twin[track]++;
			if (key_twin[track] == 2)
				key_twin[track] = 0;
			orgWaves[track].playing = true;
		}
		else //Different note
		{
			orgWaves[track].pos = 0; //Reset
			key_twin[track]++;
			if (key_twin[track] == 2)
				key_twin[track] = 0;
			changeNoteFrequency(key, track, freq); //Set the frequency
			orgWaves[track].playing = true;
			old_key[track] = key;
		}
		break;

	case 0: //Stop
		orgWaves[track].playing = false;
		orgWaves[track].pos = 0;
		break;

	case 2: //Stop playing
		if (old_key[track] != 255)
		{
			orgWaves[track].playing = false;
			old_key[track] = 255;
		}
		break;

	default:
		break;
	}
}

///////////////////////////
////===ORGANYA DRUMS===////
///////////////////////////
void changeDrumFrequency(int key, char track)
{
	orgDrums[track].key = key;
}

void changeDrumPan(unsigned char pan, char track)
{
	orgDrums[track].volume_l = 1.0f;
	orgDrums[track].volume_r = 1.0f;

	int pan_val = pan_tbl[pan];

	orgDrums[track].volume_l = (512.0f - (long double)pan_val) / 256.0f;
	orgDrums[track].volume_r = (long double)pan_val / 256.0f;

	if (orgDrums[track].volume_l > 1.0f) orgDrums[track].volume_l = 1.0f;
	if (orgDrums[track].volume_r > 1.0f) orgDrums[track].volume_r = 1.0f;
}

void changeDrumVolume(long volume, char track)
{
	orgDrums[track].volume = orgVolumeMin + ((long double)volume / 255.0f * (1.0 - orgVolumeMin));
}

void playOrganyaDrum(int key, int mode, char track)
{
	switch (mode) {
	case 0: // Stop
		orgDrums[track].playing = false;
		orgDrums[track].pos = 0;
		break;

	case 1: // Play
		changeDrumFrequency(key, track); //Set the frequency
		orgDrums[track].pos = 0;
		orgDrums[track].playing = true;
		break;

	case 2: // Stop playing
		break;

	case -1:
		break;
	}
}

//Playing functions
void organyaSetPlayPosition(long x)
{
	for (int i = 0; i < 16; i++) {
		play_np[i] = org.tdata[i].note_list;
		while (play_np[i] != NULL && play_np[i]->x < x)
			play_np[i] = play_np[i]->to; //Set notes to watch
	}

	play_p = x;
}

void organyaPlayStep(void)
{
	//char str[10];
	//char oldstr[10];
	char end_cnt = 16;

	//Melody playback
	for (int i = 0; i < 8; i++)
	{
		if (play_np[i] != NULL && play_p == play_np[i]->x) //The sound came.
		{
			if (play_np[i]->y != 0xFF) {
				playOrganyaNote(play_np[i]->y, -1, i, org.tdata[i].freq);
				now_leng[i] = play_np[i]->length;
			}

			if (play_np[i]->pan != 0xFF)
				changeNotePan(play_np[i]->pan, i);
			if (play_np[i]->volume != 0xFF)
				changeNoteVolume(play_np[i]->volume, i);

			play_np[i] = play_np[i]->to; //Point to the next note
		}

		if (now_leng[i] == 0)
			playOrganyaNote(NULL, 2, i, org.tdata[i].freq);

		if (now_leng[i] > 0)now_leng[i]--;
	}

	//Drum playback
	for (int i = 8; i < 16; i++)
	{
		if (play_np[i] != NULL && play_p == play_np[i]->x) //The sound came.
		{
			if (play_np[i]->y != 0xFF)
				playOrganyaDrum(play_np[i]->y, 1, i - 8);

			if (play_np[i]->pan != 0xFF)
				changeDrumPan(play_np[i]->pan, i - 8);
			if (play_np[i]->volume != 0xFF)
				changeDrumVolume(play_np[i]->volume, i - 8);

			play_np[i] = play_np[i]->to; //Point to the next note
		}
	}

	play_p++;
	if (play_p >= org.end_x) {
		play_p = org.repeat_x; //Because it is ++
		organyaSetPlayPosition(play_p);
	}
}

//Load function
void loadOrganya(const char *name)
{
	SDL_PauseAudioDevice(orgSoundDev, -1);

	NOTELIST *np;

	//Init some things
	organyaReleaseNote();
	organyaAllocNote(10000);

	memset(old_key, 0xFF, sizeof(old_key));
	memset(key_on, 0x00, sizeof(key_on));
	memset(key_twin, 0x00, sizeof(key_twin));

	//Load
	SDL_RWops *fp = SDL_RWFromFile(name, "rb");

	if (!fp)
		doCustomError("File cannot be accessed");

	//Password check
	char passCheck[6];
	bool pass = false;

	SDL_RWread(fp, &passCheck[0], sizeof(char), 6);
	
	if (!memcmp(passCheck, "Org-01", 6) || !memcmp(passCheck, "Org-02", 6))
	{
		//Reading song information
		org.wait = SDL_ReadLE16(fp);
		org.line = SDL_ReadU8(fp);
		org.dot = SDL_ReadU8(fp);
		org.repeat_x = SDL_ReadLE32(fp);
		org.end_x = SDL_ReadLE32(fp);

		for (int i = 0; i < 16; i++) {
			org.tdata[i].freq = SDL_ReadLE16(fp);
			org.tdata[i].wave_no = SDL_ReadU8(fp);
			org.tdata[i].pipi = SDL_ReadU8(fp);
			org.tdata[i].note_num = SDL_ReadLE16(fp);

			if (!memcmp(passCheck, "Org-01", 6))
				org.tdata[i].pipi = 0;
		}

		//Load notes
		for (int i = 0; i < 16; i++)
		{
			//Check if there are no notes to load
			if (org.tdata[i].note_num == 0)
			{
				org.tdata[i].note_list = NULL;
				continue;
			}

			//Load the notes
			np = org.tdata[i].note_p;
			org.tdata[i].note_list = org.tdata[i].note_p;
			np->from = NULL;
			np->to = (np + 1);
			np++;

			for (int j = 1; j < org.tdata[i].note_num; j++) {
				np->from = (np - 1);
				np->to = (np + 1);
				np++;
			}

			//The last note to is NULL
			np--;
			np->to = NULL;

			//Substitute content
			np = org.tdata[i].note_p; //X coordinate
			for (int j = 0; j < org.tdata[i].note_num; j++) {
				np->x = SDL_ReadLE32(fp);
				np++;
			}

			np = org.tdata[i].note_p; //Y coordinate
			for (int j = 0; j < org.tdata[i].note_num; j++) {
				np->y = SDL_ReadU8(fp);
				np++;
			}

			np = org.tdata[i].note_p; //Length
			for (int j = 0; j < org.tdata[i].note_num; j++) {
				np->length = SDL_ReadU8(fp);
				np++;
			}

			np = org.tdata[i].note_p; //Volume
			for (int j = 0; j < org.tdata[i].note_num; j++) {
				np->volume = SDL_ReadU8(fp);
				np++;
			}

			np = org.tdata[i].note_p; //Panning
			for (int j = 0; j < org.tdata[i].note_num; j++) {
				np->pan = SDL_ReadU8(fp);
				np++;
			}
		}

		SDL_RWclose(fp);

		//Load waves
		SDL_RWops *rawWave100 = SDL_RWFromFile("data/Wave100.dat", "rb");

		if (!rawWave100)
			doError();

		memset(orgWaves, 0, sizeof(orgWaves));
		
		for (int wave = 0; wave < 8; wave++)
		{
			SDL_RWseek(rawWave100, 0x100 * org.tdata[wave].wave_no, 0);
			for (int sample = 0; sample < 0x100; sample++)
				orgWaves[wave].wave[sample] = SDL_ReadU8(rawWave100);
		}

		SDL_RWclose(rawWave100);

		//Load drums
		memset(orgDrums, 0, sizeof(orgDrums));

		for (int wave = 0; wave < 8; wave++)
		{
			char *drumPath = nullptr;

			switch (org.tdata[wave + 8].wave_no)
			{
			case 0:
				drumPath = (char*)"data/Sound/96.wav";
				break;
			case 2:
				drumPath = (char*)"data/Sound/97.wav";
				break;
			case 4:
				drumPath = (char*)"data/Sound/9A.wav";
				break;
			case 5:
				drumPath = (char*)"data/Sound/98.wav";
				break;
			case 6:
				drumPath = (char*)"data/Sound/99.wav";
				break;
			case 9:
				drumPath = (char*)"data/Sound/9B.wav";
				break;
			default:
				break;
			}

			if (drumPath)
			{
				if (fileExists(drumPath))
				{
					uint8_t *pBuf = nullptr;
					SDL_LoadWAV(drumPath, &orgSoundSpec, &pBuf, (uint32_t*)&orgDrums[wave].length);

					if (pBuf == nullptr)
						doError();

					orgDrums[wave].wave = (uint8_t*)malloc(orgDrums[wave].length);
					for (size_t b = 0; b < orgDrums[wave].length; b++)
						orgDrums[wave].wave[b] = pBuf[b];
				}
				else
				{
					char error[0x100];
					sprintf(error, "Failed to load drum at: %s", drumPath);
					doCustomError(error);
				}
			}
		}

		//Make sure position is at start
		organyaSetPlayPosition(0);
	}
	else
	{
		doCustomError("File given either isn't a .org or isn't a valid version.");
	}

	SDL_PauseAudioDevice(orgSoundDev, 0);
}

//Other functions
const char *orgFolder = "data/Org/";

void changeOrg(const uint32_t num)
{
	char path[64];
	if (num == currentOrg)
		return;
	prevOrg = currentOrg;
	prevOrgPos = play_p;
	currentOrg = num;
	strcpy(path, orgFolder);
	strcat(path, musicList[num]);
	loadOrganya(path);
}

void resumeOrg()
{
	char path[64];
	Uint32 temp = 0;
	temp = currentOrg;
	currentOrg = prevOrg;
	prevOrg = temp;
	strcpy(path, orgFolder);
	strcat(path, musicList[currentOrg]);
	temp = play_p;
	loadOrganya(path);
	organyaSetPlayPosition(prevOrgPos);
	prevOrgPos = temp;
}
