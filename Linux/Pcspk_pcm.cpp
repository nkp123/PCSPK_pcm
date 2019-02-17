// TODO: tidy up and make it on par with Windows version
//

#include <stdio.h>
#include <fstream>
#include <vector>
#include <sys/io.h>
#include <stdint.h>
#include <sched.h>
#include <errno.h>

static inline uint64_t rdtscp_( uint32_t & aux )
{
    uint64_t rax,rdx;
    asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (aux) : : );
    return (rdx << 32) + rax;
}

static inline uint64_t rdtscp()
{
    uint64_t rax,rdx;
    uint32_t aux;
    asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (aux) : : );
    return (rdx << 32) + rax;
}

uint8_t old61;
uint8_t out61;
uint8_t count_values[256];

const uint8_t SPKPORT = 0x61;
const uint8_t PITCONTROL = 0x43;
const uint8_t PITCHANNEL2 = 0x42;
//const int max_duty_cycles = 8000;
//const int max_duty_cycles = 5500;
uint32_t max_duty_cycles = 76000;
const int experimental = 2;

void InitIO()
{
	ioperm(SPKPORT,1,1);
	ioperm(PITCONTROL,1,1);
	ioperm(PITCHANNEL2,1,1);
	//gfpOut32(0x90, PITCONTROL);
	////gfpOut32(0x92, PITCONTROL); //0b10010010 // new? 10010000
	if(experimental == 2)
	{
		outb(0xB0, PITCONTROL);
		
		outb(inb(0x61) | 0x03, 0x61);
	}
	else if(experimental == 1)
	{
		outb(0xB0, PITCONTROL);
		
		outb(inb(0x61) | 0x03, 0x61);
		//gfpOut32(0x61, (gfpInp32(0x61) & 0xFD));
	}
	else
	{
		old61 = inb(SPKPORT);
		out61 = old61 & 0xFE;
		outb(SPKPORT, old61);
	}
    
	////out61 = old61 | 3;
	////gfpOut32(out61, SPKPORT);

	
}
void RestoreIO()
{
	
    //gfpOut32((short)old61, SPKPORT);
	if(experimental == 2)
	{
		outb((inb(0x61) & 0xFC), 0x61);
	}
	else if(experimental == 1)
	{
		outb((inb(0x61) & 0xFC), 0x61);
	}
	else
	{
		outb(old61, SPKPORT);
	}
	////gfpOut32(0xB0, PITCONTROL);
	ioperm(SPKPORT,1,0);
	ioperm(PITCONTROL,1,0);
	ioperm(PITCHANNEL2,1,0);
}
		

int main(int argc, char* argv[])
{
	printf("1\n");
	InitIO();
	
	std::ifstream file("outraw_8bit_44100.raw", std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	printf("2\n");
	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		printf("OOOOOOOK");

		uint8_t high = (uint8_t)(out61 | 0x2);
		uint8_t low = (uint8_t)(out61 & 0xFD);
		uint8_t bef1 = 0;
		uint8_t bef2 = 0;
		uint8_t aft1 = 0;
		uint8_t aft2 = 0;
		

		cpu_set_t  mask;
		CPU_ZERO(&mask);
		CPU_SET(0, &mask);
		//CPU_SET(1, &mask);
		if(sched_setaffinity(0, sizeof(mask), &mask)) printf("\nsched_setaffinity failed with errno=%d\n", errno);

		sched_param prio;
		prio.sched_priority = 99;
		//if(sched_setscheduler(0, SCHED_FIFO, &prio)) printf("\nsched_setscheduler failed with errno=%d\n", errno);

		




		if(experimental == 2)
		{ 
			

			uint32_t outval;
			uint64_t cycles = rdtscp();
			
			max_duty_cycles = max_duty_cycles;
			for (int x = 0; x < size; ++x)
			{

			   // Console.WriteLine(audio[x]);


				
				// 16BIT CODE - something is wrong
				/*uint8_t LSB = buffer[x];
				uint8_t MSB = buffer[++x];
				++x;

				int16_t val = ((uint16_t)LSB | ((uint16_t)MSB << 8));
				uint16_t uval = (uint16_t)((int32_t)val + 65535/2);
				
				outval = (uint32_t)((uint32_t)uval * (uint32_t)max_duty_cycles / 65535); */
				 
				
				outval = (uint32_t)((uint8_t)buffer[x] * max_duty_cycles / 255);
				//if(!(x%44100)) printf("%u %u\n", (uint8_t)buffer[x], outval);
				if(!(x%44100))
				{
					outb(0xB0, PITCONTROL);
					outb((uint8_t)0x01, PITCHANNEL2); // LSB
					outb((uint8_t)0x00, PITCHANNEL2); // MSB
				}
				outb(inb(0x61) | 0x03, 0x61);

				cycles += outval;
				while(cycles > rdtscp());

				outb(inb(0x61) & 0xFD, 0x61);

				cycles += (max_duty_cycles-outval);
				while(cycles > rdtscp());
				

				

			}
		}
		else if(experimental == 1)
		{
			for (int x = 0; x < size; ++x)
			{
			   // Console.WriteLine(audio[x]);
				outb(inb(SPKPORT) & 0xFD, SPKPORT);
				if(!(x%20000)) printf("%u %u\n", (uint8_t)buffer[x], inb(SPKPORT));
			

			
				outb(PITCHANNEL2, (uint8_t)buffer[x] >> 2); // LSB
				//gfpOut32(PITCHANNEL2, (uint8_t)127); // LSB
				outb(PITCHANNEL2, (uint8_t)0x00);			// MSB
				//gfpOut32(PITCHANNEL2, 0xFF); // LSB
				//gfpOut32(PITCHANNEL2, 0xFF);			// MSB
				
				bef1 = inb(PITCHANNEL2);
				bef2 = inb(PITCHANNEL2);
				//for(int x = 0; x < 18000; ++x) __asm { nop };
				uint64_t cycles = rdtscp() + 105000;

				while(cycles > rdtscp());

				aft1 = inb(PITCHANNEL2);
				aft2 = inb(PITCHANNEL2);

				

				if(!(x%20000))
				{   printf("-TIM-\n");
					printf("%llu\n", cycles);
					printf("--\n");
					printf("%u\n", (uint8_t)bef1 + (bef2 << 8));
					printf("W\n");
					printf("%u\n", (uint8_t)aft1 + (aft2 << 8));
					printf("--\n");
				}
				//if(!(x%44100)) printf("%u\n", gfpInp32(PITCHANNEL2));
			}
		}

		else
		{
			for (int x = 0; x < size; ++x)
			{
			   // Console.WriteLine(audio[x]);
				if(!(x%44100)) printf("%u %u\n", (uint8_t)buffer[x], inb(SPKPORT));
				uint8_t val = buffer[x];
				short outval = (short)(val * max_duty_cycles / 255);

				outb(SPKPORT, high);
				for (short z = 0; z < outval; z++) asm("nop");
				outb(SPKPORT, low); 
				for (short z = outval; z < max_duty_cycles; z++) asm("nop");
			}
		}
		
	}


	//StopBeep();
	RestoreIO();

	
	return -2;
}
