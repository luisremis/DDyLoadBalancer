
#include "common.h"

#define NUM_THREADS 1

void *task1(void *);

bool bootstrap()
{
    struct comm_packet msg;
    read(connFd, &msg, sizeof(struct comm_packet) );

    if (msg.opcode != 's')
    {
        cout << "ERROR No received S in bootstrap" << endl;
    }

    bool ret ;
    ret = receiveJobs(false);

    if (ret)
    {
        cout << "Boostrap Done! " << endl;

        //cout << "jobs received: " << counter << endl;

        //Just for control, this part can be eliminated
        
        for (int i = 0; i < VECTOR_SIZE / 2; ++i)
        {
            if (mp_array[i] != 1.111111f)
            {
                cout << "problem in index: "<< i << endl;
                ret = false;
                break;
            }
        }
        
    }
    else
    {
        cout << "ERROR Problem in boostrap" << endl;
    }

    return ret;
}

bool finalTransfer()
{
    transferJobs(0.0f, true);
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cerr << "Syntam : ./server <port> " << endl;
        return 0;
    }

    check_throtling();

    unsigned int size;
    bool flag;
    mp_array = (double*) malloc( VECTOR_SIZE * sizeof(double) ) ;
    io_array = (double*) malloc( JOB_SIZE * sizeof(double));


    if ( !init_server_connection(atoi(argv[1])) ) 
    {
        cout << "Problems with connection client" << endl;
        close(connFd);
        exit(0);
    }

    flag = bootstrap();
    while ( !flag )
    {
        cout << "ERROR IN BOOSTRAP... terminating " << endl;
        exit(0);
        char t = 'n';
        write(connFd, &t, sizeof(char));
        flag = bootstrap();
    }
    char t = 'k';
    write(connFd, &t, sizeof(char));

    cout << "Bootstrap SUCCESS " << endl;
    cout << "----------------------------------------------------" << endl;

    if (argc > 2)
    {
        if ( ! connect_to_server(argv[2], atoi(argv[3]), &guiFd) )
        {
            cout << "Problems with connection GUI" << endl;
            close(connFd);
            exit(0);
        }
        else{
            cout << "Connection established at: " << argv[2] << ":" << atoi(argv[3]) << endl;
        }
    }

    work();

    finalTransfer();
    
    cout << "Server COMPLETED! " << endl;

    close(connFd);

}
