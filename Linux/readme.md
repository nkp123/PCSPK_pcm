# PCSPK_pcm

Linux version. Should compile with plain G++ command:
```
g++ Pcspk_pcm.cpp -o pcspk
```

Requires *root* to run.

Sounds okay when not much is going on in the background. To improve the sound you can for example:
- Increase process priority without setting real-time scheduling (ex. set nice value to -20).
- Set CPU affinity to only one core. Some cores will behave better than others. In my case CPU1 yields good results.
- Use real-time scheduling (probably SCHED\_FIFO is the best) and set high priority (ex. sched\_priority to 32).
- Disable CPU frequency scaling.
- Isolate specified core from the scheduler: https://wiki.linuxfoundation.org/realtime/documentation/howto/tools/cpu-partitioning/isolcpus . This *should* yield good results.
- Terminate GUI.
- Switch to single-user mode, with ```init 1```.

The code attempts to set SCHED\_FIFO real-time scheduling with sched\_priority value of 32. It also sets affinity to CPU1.
