
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <strings.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <list>
#include <fcntl.h>
#include <pthread.h>
#include <zlib.h>
#include "ChronoCpu.h"

using namespace std;

#define VECTOR_SIZE (1024*1024*16)
#define JOB_SIZE    (1024*8) // 1024 jobs
#define JOBS_NUMBER (VECTOR_SIZE / JOB_SIZE) 
#define ITERATION   1000
#define FREQ_CHECK  100
#define TRANSFER_THRESHOLD 10

#define NUMBER_THREADS 1
pthread_mutex_t mutex;
bool stop = false;

double* mp_array;
double* io_array;

int connFd = 0;
int guiFd = 0;

std::list <struct jobs_packet > queue;
std::list <struct jobs_packet > queueready;

ChronoCpu chrono("chono");

double latency = 0.0;
double throtling = 0.1;
double utilization = 0;

bool flag_other_done = false;

//Packet used for communication
struct comm_packet {
    char opcode;
    int latency_ms;
};

struct jobs_packet{

    unsigned int init;
    unsigned int end;
    long unsigned int size;
};

struct gui_packet
{
    char init;
    unsigned int total;
    unsigned int completed;
    unsigned int throtling;
};

struct pthread_param
{
    std::list <struct jobs_packet >* queue;
    std::list <struct jobs_packet >* queueready;
    double* throtling;
    double* latency;
    double* utilization;
    bool* stop;
};

void checkresult(){

    int counter=0;

    cout << "Result: " << mp_array[0] << endl;

    for (int i = 0; i < VECTOR_SIZE; ++i)
    {
        if (mp_array[i] != mp_array[0])
        {
            counter++;
        }
    }
    cout << "Result Verified:" << counter << endl;
}

void makeBlocking(int fd)
{
    int opts;
    opts = fcntl(fd,F_GETFL);
    if (opts < 0) {
        cout << "fcntl(F_GETFL)" << endl;
        exit(0);
    }
    opts = (opts & ~O_NONBLOCK);
    if ( fcntl(fd,F_SETFL,opts) < 0) {
        cout << "fcntl(F_SETFL)" << endl;
        exit(0);
    }
}

void makeNonBlocking(int fd)
{
    int opts;
    opts = fcntl(fd,F_GETFL);
    if (opts < 0) {
        cout << "fcntl(F_GETFL)" << endl;
        exit(0);
    }
    opts = (opts | O_NONBLOCK);
    if ( fcntl(fd,F_SETFL,opts) < 0) {
        cout << "fcntl(F_SETFL)" << endl;
        exit(0);
    }
}

bool receiveJobs(bool isFinal)
{
    struct jobs_packet packet;
    bool error = false;
    int counter = 0;
    int ret;

    cout << "Waiting for read in Transfering" << endl;

    char ack;
    char init;

    while ( true )
    {
        ret = 0;
        error = false;
        init = 'x';

        read(connFd, &init, sizeof(init));

        if (init != 'i')
        {
            //cout << "INIT no received " << endl;
            continue;
        }

        ret = read(connFd, &packet,  sizeof(struct jobs_packet) );
        ack = 'r';

        if (ret < 1)
        {
            cout << "problems reading1: " << ret << endl;
            error = true;
            ack = 'n';
            write(connFd, &ack, sizeof(char));
            continue;
        }

        //cout << counter << " : " << packet.init << "  " << packet.end << endl;

        if ( packet.init == packet.end )
            break;

        //ret = read(connFd, io_array, JOB_SIZE * sizeof(double) );
        ret = read(connFd, io_array, packet.size );

        if (ret < 1)
        {
            cout << "problems reading2: " << ret << endl;
            error = true;
            ack = 'n';
            write(connFd, &ack, sizeof(char));
            continue;
        }

        if ( !error )
        {
            //cout << "Recevied ok... sending ack... " << endl;
            if ( !isFinal )queue.push_back(packet);
#ifdef COMPRESSION
            long unsigned int uncompress_size = JOB_SIZE * sizeof(double);
            uncompress OF( ( (Bytef *)&mp_array[packet.init], &uncompress_size, 
                             (Bytef *)io_array, packet.size ) ); 
#else
            memcpy(&mp_array[packet.init], io_array, JOB_SIZE * sizeof(double) );
#endif

            write(connFd, &ack, sizeof(char));
        }

        counter ++;
    }

    cout << "Transfering Done! " << endl;
    cout << "Jobs received: " << counter << endl;

    //return !error;
    return true;
}

bool sendJob(struct jobs_packet packet)
{
    int ret;
    char ack;
    char init = 'i';
    long unsigned int buf_size = JOB_SIZE * sizeof(double);
    //Write init
    ret = write(connFd, &init, sizeof(char) );
    if ( ret != sizeof(char)  )
    {
        cout << "ERROR Cannot send init: " << ret << endl;
        return false;
    }

#ifdef COMPRESSION
    ret = compress2 OF( ((Bytef* )io_array, &buf_size, 
                  (Bytef* )&mp_array[packet.init], JOB_SIZE * sizeof(double), 6));
    //cout << "Compress Size: " << buf_size  << " " << ret << endl;
    //cout << "Sending: " << packet.init << "  " << packet.end << endl;
#endif

    packet.size = buf_size;
    size_t size = sizeof(struct jobs_packet);
    ret = write(connFd, &packet, size );
    
    if ( ret != size )
    {
        cout << "error sending packet: " << ret << endl;
        return false;
    }

#ifdef COMPRESSION
    ret = write(connFd, io_array, buf_size ) ;
#else
    size = JOB_SIZE * sizeof(double);
    ret = write(connFd, &mp_array[packet.init], size );
#endif

    if ( ret != buf_size )
    {
        cout << "error sending data: " << ret << endl;
        return false;
    }

    read (connFd, &ack, sizeof(ack));

    if (ack != 'r')
    {
        cout << "ERROR no ack received: " << ack << endl;
        return false;
    }

    return true;

}

bool transferJobs(int other_latency_ms, bool isFinal)
{
    size_t size;
    int ret;
    bool error = false;
    int counter = 0;
    int jobs_to_send;

    struct jobs_packet packet;
    struct comm_packet msg;

    cout << "Latency received: " << other_latency_ms << endl;

    if (queue.size() < TRANSFER_THRESHOLD && !isFinal)
    {
        cout << "Not enough work to send. Wating for completion" << endl;
        flag_other_done = true;
        return true;
    }

    if (other_latency_ms == 0.0f && latency == 0.0f)
    {
        cout << "Other Latency or Latenct == 0" << endl;
        jobs_to_send = queue.size() / 2;
    }
    else
    {
        double my_latency = latency / queue.size();
        double factor = other_latency_ms / my_latency;

        if (factor > 1) // I am faster
        {
            jobs_to_send = ((double)queue.size()) * 0.333f ; 
        }
        else // The other is faster
        {
            jobs_to_send = ((double)queue.size()) * 0.666f ;
        }
    }

    if ( isFinal )
    {
        jobs_to_send = queueready.size();
        queue = queueready;
        cout << "Performing Final send... " << endl;
    }

    cout << "Jobs to be sended: " << jobs_to_send << endl;

    msg.opcode = 's';
    write(connFd, &msg, sizeof(struct comm_packet) );

    makeBlocking(connFd);

    for (int i = 0; i < jobs_to_send ; ++i)
    {

        bool ret = sendJob(queue.front());

        if ( ret )
        {
            //cout << "Received ack" << endl;
            queue.pop_front();
            counter ++;
        }
        else
        {
            cout << "ERROR sending: " << endl;
            queue.push_back(queue.front());
            queue.pop_front();
            continue;
        }

    }

    /* signaling end */
    char init = 'i';
    ret = write(connFd, &init, sizeof(char) );
    if ( ret != sizeof(char)  )
    {
        cout << "ERROR Cannot send init last packet: " << ret << endl;
        error = true;
    }
    packet.init = 0;
    packet.end  = 0;
    write(connFd, &packet, sizeof(struct jobs_packet));

    cout << "Transfered ok: " << counter <<  endl;

    return !error;
}

void check_throtling()
{
    ifstream file;
    file.open("throtling.txt");

    double aux;
    file >> aux;

    if (aux != throtling)
    {
        throtling =  aux;
        cout << "Updating Throling to: " << throtling << endl;
    }
    file.close();
}

void send_gui()
{
    if (guiFd != 0)
    {
        struct gui_packet packet;
        packet.init = 'i';
        packet.total = JOBS_NUMBER;
        packet.completed = queueready.size();
        packet.throtling = (unsigned int)(throtling*100);
        write(guiFd, &packet, sizeof(struct gui_packet));
    }
}

void check_balance()
{
    struct comm_packet msg;
    int ret;

    makeNonBlocking(connFd);
    cout << "Processing Latency: " << latency / queueready.size() << endl;
    cout << "Worker CPU Utilizatio: " << utilization*100 << "%" << endl;
 
/* CODE USED TO ANALIZE THRESHOLD */
    float msecs_send = JOB_SIZE * 4 * 8 * 2 / 10e6 * 1000; 
    //cout << "msecs_send: "<< msecs_send << endl;
    cout << "THRESHOLD: " << ( msecs_send + 52 ) / (latency / queueready.size() )<< endl;


    ret = read(connFd, &msg, sizeof(struct comm_packet) );

    if (ret < 1)
    {
        cout << "No Message received yet..." << endl;
    }
    else
    {
        if (msg.opcode == 'd')
        {
            cout << "Other Done: " << endl;
            transferJobs(msg.latency_ms, false);
        }
        else
        {
            cout << "ERROR opcode received: " << msg.opcode << endl;
        }
    }
}

void* workThread(void* ptr)
{
    ChronoCpu chrono_job("job_time");
    struct pthread_param* p = (struct pthread_param*)ptr;
    struct jobs_packet job;

    while( !(*(p->stop)) )
    {
        pthread_mutex_lock(&mutex);
        if ( ! (p->queue)->empty() )
        {
            job = (p->queue)->front();
            (p->queue)->pop_front();
            pthread_mutex_unlock(&mutex);
        }
        else
        {
            pthread_mutex_unlock(&mutex);
            continue;
        }
        
        chrono_job.tic();
        //cout << "Processing thread: " << pthread_self() << endl;
        for (int i = job.init; i <= job.end; ++i)
        {
            for (int j = 0; j < ITERATION; ++j)
            {
                mp_array[i] +=1.1111; 
            }
        }
        chrono_job.tac();

        pthread_mutex_lock(&mutex);
        *(p->latency) += chrono_job.getElapsedStats().lastTime_ms;
        (p->queueready)->push_back(job);
        pthread_mutex_unlock(&mutex);

        float exe_time_ms = chrono_job.getElapsedStats().lastTime_ms;
        int time_sleep_ms = exe_time_ms / *(p->throtling);
        time_sleep_ms -= exe_time_ms;
        //cout << "Time: " << exe_time_ms << " Sleep: " << time_sleep_ms << endl;
        if (time_sleep_ms >= 1)
        {
            usleep( time_sleep_ms * 1000 );
            *(p->utilization) = (float)exe_time_ms / ((float)time_sleep_ms + exe_time_ms);
        }
        
    }
   
}

void work()
{
    struct jobs_packet job;
    struct comm_packet msg;
    int counter = 0; 

    struct pthread_param params;
    params.queue = &queue;
    params.queueready = &queueready;
    params.throtling = &throtling;
    params.latency = &latency;
    params.stop = &stop;
    params.utilization = &utilization;

    pthread_t threadA[NUMBER_THREADS];

    for (int i = 0; i < NUMBER_THREADS; ++i)
    {
        pthread_create(&threadA[i], NULL, workThread, (void*)&params );
    }
    
    while ( !queue.empty() ){

        usleep( 500000 ); // Sleep 500ms

        pthread_mutex_lock(&mutex);
        check_balance();
        check_throtling();
        send_gui();
        cout << queueready.size() << endl;

        if (queue.empty())
        {
            msg.opcode = 'd';
            msg.latency_ms = (int) (latency / (double)queueready.size());
            write(connFd, &msg, sizeof(struct comm_packet) );

            if (flag_other_done)
            {
                cout << "Both done, transfering back..." << endl;
                //transferBack();
                return;
            }

            //Blocking read
            makeBlocking(connFd);
            int ret = read (connFd, &msg, sizeof(struct comm_packet) ) ;
            
            if (ret < 1)
            {
                cout << "ERROR Reading not working: " << ret << endl;
            }
                
            if (msg.opcode == 's')
            {
                if ( !receiveJobs(false) )
                {
                    cout << "ERROR Problem receiving" << endl;
                    exit(0);
                }
                else
                {
                    cout << "More jobs recieved done!" << endl;
                }
            }
            else if (msg.opcode == 'd')
            {
                cout << "Other system Done received" << endl;
            }
            else
            {
                cout << "ERROR invalid opcode, Received: " << msg.opcode << endl;
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    stop = true;

    for(int i = 0; i < NUMBER_THREADS; i++)
    {
        pthread_join(threadA[i], NULL);
    }
    cout << "All threads finish" << endl;


}

bool init_server_connection(int port) 
{
    socklen_t len; //store size of the address
    struct sockaddr_in svrAdd, clntAdd;
    int listenFd;
        
    if((port > 65535) || (port < 2000))
    {
        cerr << "Please enter a port number between 2000 - 65535" << endl;
        return false;
    }
    
    //create socket
    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(listenFd < 0)
    {
        cerr << "Cannot open socket" << endl;
        return false;
    }
    
    bzero((char*) &svrAdd, sizeof(svrAdd));
    
    svrAdd.sin_family = AF_INET;
    svrAdd.sin_addr.s_addr = INADDR_ANY;
    svrAdd.sin_port = htons(port);
    
    //bind socket
    if(bind(listenFd, (struct sockaddr *)&svrAdd, sizeof(svrAdd)) < 0)
    {
        cerr << "Cannot bind" << endl;
        return false;
    }
    
    listen(listenFd, 5);
    
    len = sizeof(clntAdd);
    
    int noThread = 0;

    cout << "Listening" << endl;

    //this is where client connects. svr will hang in this mode until client conn
    connFd = accept(listenFd, (struct sockaddr *)&clntAdd, &len);

    if (connFd < 0)
    {
        cerr << "Cannot accept connection" << endl;
        return false;
    }
    else
    {
        cout << "Connection successful: " << endl;
    }

    return true;
}

bool connect_to_server(char* add, int port, int* connectionFd){

    struct sockaddr_in svrAdd;
    struct hostent *server;
    
    if((port > 65535) || (port < 2000))
    {
        cout<<"Please enter port number between 2000 - 65535"<<endl;
        return false;
    }       
    
    //create client skt
    *connectionFd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(*connectionFd < 0)
    {
        cout << "Cannot open socket" << endl;
        return false;
    }
    else{
        cout << "Socket opened" << endl;
    }
    
    server = gethostbyname(add);
    
    if(server == NULL)
    {
        cout << "Host does not exist" << endl;
        return false;
    }
    
    bzero((char *) &svrAdd, sizeof(svrAdd));
    svrAdd.sin_family = AF_INET;
    
    bcopy((char *) server -> h_addr, (char *) &svrAdd.sin_addr.s_addr, server -> h_length);
    
    svrAdd.sin_port = htons(port);
    
    int checker = connect(*connectionFd,(struct sockaddr *) &svrAdd, sizeof(svrAdd));
    
    if (checker < 0)
    {
        cout << "Cannot connect!" << endl;
        return false;
    }
    else{
        cout << "Connection Successful" << endl;
    }

    return true;
}
