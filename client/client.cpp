
#include "common.h"

void init_vector(){

    mp_array = (double*) malloc( VECTOR_SIZE * sizeof(double) ) ;

    for (int i = 0; i < VECTOR_SIZE; ++i)
    {
        mp_array[i] = 1.111111f;
    }
    cout << "Vector inicialized" << endl;
}

void chekresult(){

    int counter=0;

    cout << "Result: " << mp_array[0] << endl;

    for (int i = 0; i < VECTOR_SIZE; ++i)
    {
        if (mp_array[i] != mp_array[0])
        {
            counter;
        }
    }
    cout << "Result Verified:" << counter << endl;
}


void createWorkqueue()
{
    unsigned int init = 0;

    for (int i = 0; i < JOBS_NUMBER; ++i)
    {
        struct jobs_packet job;
        job.init = init;
        job.end  = init + JOB_SIZE - 1;
        init +=JOB_SIZE;
        queue.push_back(job);
    }

    cout << "Work queue ready" << endl;
}

bool finalReceive()
{
    struct jobs_packet packet;
    bool error = false;
    int counter = 0;
    int ret;

    struct comm_packet msg;

    makeBlocking(connFd);
    read(connFd, &msg, sizeof(struct comm_packet) );

    if (msg.opcode != 's')
    {
        cout << "ERROR No received S in final receive..." << endl;
    }

    receiveJobs(true);
}

int main (int argc, char* argv[])
{
    char a;
    bool flag;

    io_array = (double*) malloc(JOB_SIZE * sizeof(double));

    if(argc < 3)
    {
        cout<<"Syntax : ./client <host name> <port> <gui name> <port>"<<endl;
        return 0;
    }

    check_throtling();

    init_vector();
    createWorkqueue();

    if ( ! connect_to_server(argv[1], atoi(argv[2]), &connFd) )
    {
        cout << "Problems with connection server" << endl;
        close(connFd);
        exit(0);
    }
    else{
        cout << "Connection established at: " << argv[1] << ":" << atoi(argv[2]) << endl;
    }

    flag = false;
    while ( !flag )
    {
        
        flag = transferJobs(0.0f, false);
        makeBlocking(connFd);
        read(connFd, &a, sizeof(char));

        if (a != 'k')
        {
            flag = false;
            cout << "ERROR BOORSTRAP SERVER... terminating" << endl;
            close(connFd);
            exit(0);
        }
    }

    cout << "Bootstrap SUCCESS " << endl;
    cout << "----------------------------------------------------" << endl;

    if (argc > 3)
    {

        if ( ! connect_to_server(argv[3], atoi(argv[4]), &guiFd) )
        {
            cout << "Problems with connection GUI" << endl;
            close(connFd);
            exit(0);
        }
        else{
            cout << "Connection established at: " << argv[3] << ":" << atoi(argv[4]) << endl;
        }
    }


    work();

    finalReceive();

    cout << "Client COMPLETED! " << endl;

    chekresult();

    close(connFd);

}