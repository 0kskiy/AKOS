#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>

using namespace std;
volatile sig_atomic_t last_bit = -1;
uint32_t received_value = 0;
pid_t sender_pid = -1;

void handle_sigusr1(int, siginfo_t* info, void*) {
    last_bit = 0;
    if (sender_pid > 0) kill(sender_pid, SIGUSR1);
}

void handle_sigusr2(int, siginfo_t* info, void*) {
    last_bit = 1;
    if (sender_pid > 0) kill(sender_pid, SIGUSR1);
}

void handle_sigint(int) {
}

int main() {
    pid_t mypid = getpid();
    cout << "RECEIVER PID: " << mypid << "\n";
    cout << "Enter sender PID: " << flush;
    cin >> sender_pid;
    struct sigaction sa{};
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handle_sigusr1;
    sigaction(SIGUSR1, &sa, nullptr);
    sa.sa_sigaction = handle_sigusr2;
    sigaction(SIGUSR2, &sa, nullptr);
    struct sigaction sa2{};
    sa2.sa_handler = handle_sigint;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    sigaction(SIGINT, &sa2, nullptr);
    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    sigaddset(&block, SIGUSR2);
    sigprocmask(SIG_BLOCK, &block, nullptr);
    cout << "Waiting for 32 bits...\n";
    for (int i = 0; i < 32; ++i) {
        sigset_t suspend_mask;
        sigemptyset(&suspend_mask);
        last_bit = -1;
        while (last_bit == -1) {
            sigsuspend(&suspend_mask);
        }
        if (last_bit == 1) {
            received_value |= (1u << i);
        }
    }
    int32_t final_value = (int32_t)received_value;
    cout << "Received integer: " << final_value << "\n";
    return 0;
}
