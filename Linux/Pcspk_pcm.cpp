// TODO: determine if using PWM multiplier is even necessary
//

#include <stdio.h>
#include <fstream>
#include <vector>
#include <sys/io.h>
#include <stdint.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>

static inline uint64_t rdtscp()
{
    uint64_t rax,rdx;
    uint32_t aux;
    asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (aux) : : );
    return (rdx << 32) + rax;
}

uint8_t out61;

const uint8_t SPKPORT = 0x61;
const uint8_t PITCONTROL = 0x43;
const uint8_t PITCHANNEL2 = 0x42;

uint32_t max_duty_cycles = 76000;
uint32_t FREQ = 44100;

void InitIO()
{
	ioperm(SPKPORT,1,1);
	ioperm(PITCONTROL,1,1);
	ioperm(PITCHANNEL2,1,1);

	outb(0xB0, PITCONTROL);
	outb(inb(0x61) | 0x03, 0x61);
}
void RestoreIO()
{
	outb((inb(0x61) & 0xFC), 0x61);
	
	ioperm(SPKPORT,1,0);
	ioperm(PITCONTROL,1,0);
	ioperm(PITCHANNEL2,1,0);
}
void TryRT()
{
	cpu_set_t  mask;
	CPU_ZERO(&mask);
	CPU_SET(1, &mask);
	if(sched_setaffinity(0, sizeof(mask), &mask)) printf("sched_setaffinity failed with errno=%d\n", errno);

	sched_param prio;
	prio.sched_priority = 32;
	if(sched_setscheduler(0, SCHED_FIFO, &prio)) printf("sched_setscheduler failed with errno=%d\n", errno);
}
		

int main(int argc, char* argv[])
{
        std::string fileName = "raw_8bit_44100.raw";

	if(argc <= 1) printf("Usage: %s [file name] [sample rate]\nWill play using default file name (%s) and sample rate (%u)\n", argv[0], fileName.c_str(), FREQ);
	if(argc > 1) fileName = argv[1];
	if(argc > 2) FREQ = atoi(argv[2]);
	
	InitIO();
	
	// Try to use real-time scheduling with CPU affinity
	TryRT();
	
	// Do simple rdtscp calibration - check how fast it is incrementing
	printf("Calibration will take 2 seconds.. ");
	fflush(stdout);
	uint64_t before = rdtscp();
	sleep(2);
	uint64_t after = rdtscp();
	max_duty_cycles = (after - before)/FREQ/2;
	printf("DONE.\nThere are %u cycles per sample.\n", max_duty_cycles);
				
	std::ifstream file(fileName, std::ios::binary | std::ios::ate);
	if(!file.is_open())
	{
		printf("Failed to open the file!\n");
		RestoreIO();
		return 0;
	}
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	
	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		printf("Playing at %u rate. PWM frequency is: %u", FREQ, FREQ);

		uint8_t high = (uint8_t)(out61 | 0x2);
		uint8_t low = (uint8_t)(out61 & 0xFD);		

		uint32_t outval;
		uint64_t cycles = rdtscp();
		
		max_duty_cycles = max_duty_cycles;
		for (int x = 0; x < size; ++x)
		{
			outval = (uint32_t)((uint8_t)buffer[x] * max_duty_cycles / 255);
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

	RestoreIO();
	return 0;
}
