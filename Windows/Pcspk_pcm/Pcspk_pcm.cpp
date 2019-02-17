// PCSPK - PC Speaker PCM player

#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <intrin.h>

inline uint64_t rdtsc(){
    return __rdtsc();
}

typedef void	(__stdcall *lpOut32)(short, short);
typedef short	(__stdcall *lpInp32)(short);
typedef BOOL	(__stdcall *lpIsInpOutDriverOpen)(void);
typedef BOOL	(__stdcall *lpIsXP64Bit)(void);

//Some global function pointers (messy but fine for an example), taken from InpOut library example
lpOut32 gfpOut32;
lpInp32 gfpInp32;
lpIsInpOutDriverOpen gfpIsInpOutDriverOpen;
lpIsXP64Bit gfpIsXP64Bit;

uint8_t old61;
uint8_t out61;

const short SPKPORT = 0x61;
const short PITCONTROL = 0x43;
const short PITCHANNEL2 = 0x42;

uint32_t max_duty_cycles = 76000;
uint32_t FREQ = 44100;
uint32_t PWMmul = 1;

void InitIO()
{
		gfpOut32(PITCONTROL, 0xB0);
		gfpOut32(SPKPORT, gfpInp32(SPKPORT) | 0x03);
}
void RestoreIO()
{
		gfpOut32(SPKPORT, (gfpInp32(SPKPORT) & 0xFC));
}
		

int main(int argc, char* argv[])
{
	    std::string fileName = "raw_8bit_44100.raw";
		if(argc <= 1) printf("Usage: %s [file name] [sample rate] [pwm multiplier (>=1)]\nWill play using default file name (%s) and sample rate (%u)\n", argv[0], fileName.c_str(), FREQ);
		if(argc > 1) fileName = argv[1];
		if(argc > 2) FREQ = atoi(argv[2]);
		if(argc > 3) PWMmul = atoi(argv[3]);
		//Dynamically load the DLL at runtime (not linked at compile time)
		HINSTANCE hInpOutDll ;
		hInpOutDll = LoadLibrary ( L"InpOut32.DLL" ) ;	//The 32bit DLL. If we are building x64 C++ 
														//applicaiton then use InpOutx64.dll
		if ( hInpOutDll != NULL )
		{
			gfpOut32 = (lpOut32)GetProcAddress(hInpOutDll, "Out32");
			gfpInp32 = (lpInp32)GetProcAddress(hInpOutDll, "Inp32");
			gfpIsInpOutDriverOpen = (lpIsInpOutDriverOpen)GetProcAddress(hInpOutDll, "IsInpOutDriverOpen");
			gfpIsXP64Bit = (lpIsXP64Bit)GetProcAddress(hInpOutDll, "IsXP64Bit");

			if (gfpIsInpOutDriverOpen())
			{
				// Do simple rdtsc calibration - check how fast it is incrementing
				printf("Calibration will take 2 seconds.. ");
				uint64_t before = rdtsc();
				Sleep(2000);
				uint64_t after = rdtsc();
				max_duty_cycles = (after - before)/FREQ/2/PWMmul;
				printf("DONE.\nThere are %u cycles per sample.\n", max_duty_cycles);

				InitIO();

				std::ifstream file(fileName.c_str(), std::ios::binary | std::ios::ate);
				if(!file.is_open())
				{
					printf("Failed to open the file!\n");
					RestoreIO();
					FreeLibrary ( hInpOutDll ) ;
					return 0;
				}
				std::streamsize size = file.tellg();
				file.seekg(0, std::ios::beg);

				std::vector<char> buffer(size);
				uint8_t* buf = (uint8_t*)buffer.data();
				uint8_t* end = buf+size;

				if (file.read(buffer.data(), size))
				{
					printf("Playing at %u rate. PWM frequency is: %u", FREQ, FREQ*PWMmul);

					gfpOut32(PITCHANNEL2, (uint8_t)0x01); // LSB ------> this will near instantenously put OUT2 to low state, it is then 
					gfpOut32(PITCHANNEL2, (uint8_t)0x00); // MSB -/  \-> just a matter of connecting and disconnecting the speaker from OUT2.

					uint32_t outval;
					uint64_t cycles = rdtsc();
					uint32_t rep = 0; // used for PWM frequency multiplier

					while(buf<end) // one sample, it will take ~max_duty_cycles counted by rdtsc()
					{
						outval = (uint32_t)(*buf * max_duty_cycles / 255);
							
						gfpOut32(0x61, gfpInp32(0x61) | 0x03); // setting high -> connect speaker
						cycles += outval;
						while(cycles > rdtsc());

						gfpOut32(0x61, gfpInp32(0x61) & 0xFD); // setting low -> disconnect speaker
						cycles += (max_duty_cycles-outval);
						while(cycles > rdtsc());
						
						if(PWMmul == 1) // PWM multiplier logic
						{
							++buf;
						}
						else
						{
							++rep;
							if(rep >= PWMmul)
							{
								rep = 0;
								++buf;
							}
						} 
						
					}
						
				}

				RestoreIO();
			}
			else
			{
				printf("Unable to open InpOut32 Driver!\n");
			}

			//All done
			FreeLibrary ( hInpOutDll ) ;
			return 0;
		}
		else
		{
			printf("Unable to load InpOut32 DLL!\n");
			return -1;
		}
	
	return -2;
}
