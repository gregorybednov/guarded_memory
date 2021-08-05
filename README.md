# guarded_memory
Consept of thread-guarded memory (readers-writers problem)

Solve of readers-writers problem in POSIX system with some example code. (but almost ready to release as library).

Tested in Debian 10 (x64, VM) and TinyCoreLinux (x86, real machine). To compile with GCC
1) download the repo
2) go to repo directory in terminal
3) write in terminal:
```gcc -o ./test1 -lpthread main.c```
