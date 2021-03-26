# CPipes
Program created for the Operating Systems course at my university. It's a multiprocess program which handles communication between processes via unnamed pipes.\
To use the program, firstly you have to compile it which can be done by using the following command in the terminal opened from directory consisting the `pipefork.c` file:
```
gcc pipefork.c -o pipefork -lrt
```
# 1. Usage 👩‍💻
## 1.1 Command line
To use the program execute the followin command (in terminal as for compiling):
```
./pipefork -t time -n number -r random -b size
```
Where all of the arguments are integers with followig ranges:
- `time` [50,500]
- `number` [3,30]
- `random` [0,100]
- `size` [1,`PIPE_BUF-8`]
## 1.2 Program logic
1. Main program creates a pipe and two child processes *m* and then each of them creates also its single pipe and one child process *c*
2. Each process *c* sends random data ['a'-'z'] to process *m* via pipe where the size of write is randomized between `size` and `PIPE_BUF-8`. The procedure is repeated `number` times and after each send of buffer, process *c* waits `time` miliseconds.
3. Process *m* reads that data to buffer and then with probability `random`% appends at its end the word "injected". Next it sends that buffer to the main process
4. The main process reads data and displays it to the **stdout**

 Program terminates when processes *m* and parent process detect closed pipes. Process *c* close its pipe when it either performed sending buffer `number` times or when it detects `SIGINT` signal sent by user. 
 
