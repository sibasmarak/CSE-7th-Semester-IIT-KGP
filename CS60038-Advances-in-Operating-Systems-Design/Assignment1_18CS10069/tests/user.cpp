/*** Credits for this test script: Archisman Pathak (18CS30050) ***/
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
using namespace std;
 
#define int int32_t
#define PROCNAME "/proc/partb_1_18CS10069"
#define MAX_CAPACITY 100
#define NUM_OPERATIONS 10005
#define NUM_FORKS 6
 
int main(){
    for(int i=0;i<NUM_FORKS;i++)
    {
        fork();
    }
 
    deque<int> dq;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1,INT_MAX); 
    std::uniform_int_distribution<std::mt19937::result_type> dist2(0,1);
 
    vector<int> input;
    vector<int> expected_output;
    vector<char> opers;
    for(int i=0;i<NUM_OPERATIONS;i++)
    {
        int operation = dist2(rng);
        if(operation == 0)
        {
            // read
            if(dq.empty()) continue; 
            int val = dq.front();
            dq.pop_front();
            expected_output.push_back(val);
            opers.push_back('r');
        }
        else if(dq.size()<MAX_CAPACITY){
            // write
            int val  =  dist(rng);
            (val&1)?dq.push_front(val):dq.push_back(val);
            input.push_back(val);
            opers.push_back('w');
        }
    }
    vector<int> output(expected_output.size());
    pid_t curr_pid = getpid();
    char procname[] = PROCNAME;
 
    int fd = open(procname, O_RDWR);
    if(fd<0)
    {
        printf("PID %d : Error in opening PROC FILE\n", curr_pid);
        return 0;
    }
 
    printf("PID %d : PROC FILE successfully opened!\n", curr_pid);
    char buffer[1];
    buffer[0] = MAX_CAPACITY;
    int ret_val = write(fd, buffer, 1);
    if(ret_val < 0)
    {
        printf("PID %d : Could Not write capacity to proc file!\n", curr_pid);
        close(fd);
        return 0;
    }
 
    printf("PID %d : PROC FILE Initialized with capacity %d!\n", curr_pid, MAX_CAPACITY);
    int wct = 0, rct = 0;
    for(int i=0;i<opers.size();i++)
    {
        // printf("PID %d : At Iteration %d ...!\n", curr_pid, i);
        if(opers[i]=='w')
        {
            // printf("PID %d : Writing value : %d!\n", curr_pid, input[wct]);
            ret_val = write(fd, &input[wct++], sizeof(int32_t));
            if(ret_val < 0)
            {
                printf("PID %d : Failed to write value %d!\n", curr_pid, input[wct-1]);
                close(fd);
                return 0;
            }
        }
        else if(opers[i]=='r')
        {
            ret_val = read(fd, &output[rct++], sizeof(int32_t)); /* NOTE: Change this size to 3 and check if your code still works */
            if(ret_val < 0)
            {
                printf("PID %d : Failed to Read value!\n", curr_pid);
                close(fd);
                return 0;
            }
            // printf("PID %d : Read value %d from file\n", curr_pid, output[rct-1]);
        }
    }
    for(int i=0;i<(int)output.size();i++)
    {
        if(output[i]!=expected_output[i]) {
            printf("PID %d : The output didnot match! \n\t At i=%d => Expected : %d \t Found : %d\n", curr_pid, i, expected_output[i], output[i]);
            close(fd);
            return 0;
        }
    }
    printf("PID %d : All Outputs Matched!\n", curr_pid);
    close(fd);
    return 0;
}