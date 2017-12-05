# Bounded-Buffer-Synchronization-with-Multiple-Producers
This program tries to solve a slightly modified synchronization problem. A consumer thread tries to read input from many producer threads via buffers. All the buffers have to be synchronized to prevent a race condition. To achieve this condition variables and mutexes are used from the pthread library. 
