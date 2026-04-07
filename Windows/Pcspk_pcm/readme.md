# PCSPK_pcm

Windows version. Should compile with any stock Visual Studio 2010+ version.

As Windows is often busy with background tasks and its scheduler is a mess it is *really* hard to get high quality output from it.
You can try setting real-time process priority and CPU affinity to only one core (preferably not CPU0 as it is often busy serving interrupts) to make the sound a bit better.
