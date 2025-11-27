#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>

using namespace std;
volatile sig_atomic_t ack_received = 0;
volatile sig_atomic_t timed_out = 0;

void ack_handler(int) {
    ack_received = 1;
}

void alarm_handler(int) {
    timed_out = 1;
}

int main() {
    pid_t mypid = getpid();
    cout << "SENDER PID: " << mypid << "\n";
    cout << "Enter receiver PID: " << flush;
    pid_t receiver;
    cin >> receiver;
    cout << "Enter integer to send: " << flush;
    int32_t value;
    cin >> value;
    struct sigaction sa{};
    sa.sa_handler = ack_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, nullptr);
    struct sigaction sa2{};
    sa2.sa_handler = alarm_handler;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    sigaction(SIGALRM, &sa2, nullptr);
    sigset_t block, oldmask;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    sigaddset(&block, SIGALRM);
    sigprocmask(SIG_BLOCK, &block, &oldmask);
    for (int i = 0; i < 32; ++i) {
        ack_received = 0;
        timed_out = 0;
        int bit = ((uint32_t)value >> i) & 1u;
        int signal_to_send = (bit == 0 ? SIGUSR1 : SIGUSR2);
        kill(receiver, signal_to_send);
        alarm(5);
        sigset_t suspend_mask;
        sigemptyset(&suspend_mask);
        while (!ack_received && !timed_out) {
            sigsuspend(&suspend_mask);
        }
        alarm(0);
        if (timed_out) {
            cerr << "Timeout waiting ACK\n";
            return 1;
        }
    }

    kill(receiver, SIGINT);
    cout << "Sender finished.\n";
    return 0;
}
