/*** Credits for this test script: Somnath Jena (18CS30047) and Archisman Pathak (18CS30050) ***/
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
using namespace std;

#define PB2_SET_CAPACITY  _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT_RIGHT  _IOW(0x10, 0x32, int32_t*)
#define PB2_INSERT_LEFT   _IOW(0x10, 0x33, int32_t*)
#define PB2_GET_INFO      _IOR(0x10, 0x34, int32_t*)
#define PB2_POP_LEFT  _IOR(0x10, 0x35, int32_t*)
#define PB2_POP_RIGHT _IOR(0x10, 0x36, int32_t*)

 
#define int int32_t
#define PROCNAME "/proc/cs60038_a2_18CS10069"
#define MAX_CAPACITY 100
#define NUM_OPERATIONS 10005
#define NUM_FORKS 10

struct obj_info {
    int32_t deque_size;         //current number of elements in deque
    int32_t capacity;           //maximum capacity of the deque
};

int main(){
    for(int i=0;i<NUM_FORKS;i++)
    {
        fork();
    }
 
    deque<int> dq;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1,INT_MAX); 
    std::uniform_int_distribution<std::mt19937::result_type> ops(0,5);
    std::uniform_int_distribution<std::mt19937::result_type> caps(1,MAX_CAPACITY);
 
    vector<int> input;
    vector<int> cap;
    vector<int> expected_output;
    vector<obj_info> expected_info;
    vector<char> opers;
    //a=capacity, b=writeleft c=writeright d=popleft e=popright f=getinfo
    int init=0;
    cap.push_back(caps(rng));
    opers.push_back('a');
    init= 1;
    for(int i=1;i<NUM_OPERATIONS;i++)
    {
        int operation = ops(rng);
        if(operation == 0)
        {
            //set capacity
            cap.push_back(caps(rng));
            opers.push_back('a');
            if(init)
                dq.clear();
            else
                init=1;
        }
        else if(operation==1 && dq.size()<cap.back())
        {
            // insert left
            int val  =  dist(rng);
            dq.push_front(val);
            input.push_back(val);
            opers.push_back('b');
        }
        else if(operation==2 && dq.size()<cap.back())
        {
            // insert right
            int val  =  dist(rng);
            dq.push_back(val);
            input.push_back(val);
            opers.push_back('c');
        }
        else if(operation == 3)
        {
            //pop left
            if(dq.empty()) continue;
            int val;
            val = dq.front();
            dq.pop_front();
            expected_output.push_back(val);
            opers.push_back('d');
        }
        else if(operation == 4)
        {
            //pop right
            if(dq.empty()) continue;
            int val;
            val = dq.back();
            dq.pop_back();
            expected_output.push_back(val);
            opers.push_back('e');
        }
        else if(operation == 5)
        {
            //get info
            obj_info info = {(int32_t)dq.size(),(int32_t)cap.back()};
            expected_info.push_back(info);
            opers.push_back('f');
        }
    }
    pid_t curr_pid = getpid();
    char procname[] = PROCNAME;
 
    int fd = open(procname, O_RDWR);
    if(fd<0)
    {
        printf("PID %d : Error in opening PROC FILE\n", curr_pid);
        return 0;
    }
 
    printf("PID %d : PROC FILE successfully opened!\n", curr_pid);
    vector<int> output(expected_output.size());
    vector<obj_info> infos(expected_info.size());
    int wct = 0, rct = 0;int ret_val; int cct=0; int oct=0;
    for(int i=0;i<opers.size();i++)
    {
        // printf("PID %d : At Iteration %d ...!\n", curr_pid, i);
        if(opers[i]=='a')
        {
            ret_val = ioctl(fd, PB2_SET_CAPACITY, &cap[cct++]);
            if(ret_val < 0)
            {
                printf("PID %d : Failed to (re)initialize with capacity%d!\n", curr_pid, cap[cct-1]);
                close(fd);
                return 0;
            }
        }
        else if(opers[i]=='b')
        {
            // printf("PID %d : Writing value : %d!\n", curr_pid, input[wct]);
            ret_val = ioctl(fd, PB2_INSERT_LEFT, &input[wct++]);
            if(ret_val < 0)
            {
                printf("PID %d : Failed to write value %d!\n", curr_pid, input[wct-1]);
                close(fd);
                return 0;
            }
        }
        else if(opers[i]=='c')
        {
            // printf("PID %d : Writing value : %d!\n", curr_pid, input[wct]);
            ret_val = ioctl(fd, PB2_INSERT_RIGHT, &input[wct++]);
            if(ret_val < 0)
            {
                printf("PID %d : Failed to write value %d!\n", curr_pid, input[wct-1]);
                close(fd);
                return 0;
            }
        }
        else if(opers[i]=='d')
        {
            ret_val = ioctl(fd, PB2_POP_LEFT, &output[rct++]);
            if(ret_val < 0)
            {
                printf("d PID %d : Failed to Read value!\n", curr_pid);
                close(fd);
                return 0;
            }
            // printf("PID %d : Read value %d from file\n", curr_pid, output[rct-1]);
        }
        else if(opers[i]=='e')
        {
            ret_val = ioctl(fd, PB2_POP_RIGHT, &output[rct++]);
            if(ret_val < 0)
            {
                printf("e PID %d : Failed to Read value!\n", curr_pid);
                close(fd);
                return 0;
            }
            // printf("PID %d : Read value %d from file\n", curr_pid, output[rct-1]);
        }
        else if(opers[i]=='f')
        {
            struct obj_info curr_info;
            ret_val = ioctl(fd, PB2_GET_INFO, &curr_info);
            if(ret_val < 0)
            {
                printf("f PID %d : Failed to Read value!\n", curr_pid);
                close(fd);
                return 0;
            }
            infos[oct].deque_size = curr_info.deque_size;
            infos[oct].capacity   = curr_info.capacity;
            oct++;
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
    for(int i=0;i<(int)infos.size();i++)
    {
        if(infos[i].deque_size!=expected_info[i].deque_size || infos[i].capacity!=expected_info[i].capacity)
        {
            printf("PID %d : The output didnot match! size=%d cap=%d exp_size=%d exp_cap=%d\n", curr_pid,
            infos[i].deque_size,infos[i].capacity,expected_info[i].deque_size,expected_info[i].capacity);
            close(fd);
            return 0;
        }
    }
    printf("PID %d : All Outputs Matched!\n", curr_pid);
    close(fd);
    return 0;
}
