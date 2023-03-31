// client2.cpp - A client that communicates with a second client using triple RSA encrpytion/decryption
#include <arpa/inet.h>
#include <iostream>
#include <math.h>
#include <net/if.h>
#include <netinet/in.h>
#include <queue>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

using namespace std;

const char IP_ADDR[] = "127.0.0.1";
const int BUF_LEN = 256;
bool is_running;
int srcPort = 1155;
int destPort = 1153;
// Encrpytion/Decryption variables
double n;
double e;
double d;
double phi;

pthread_t tid[3];


struct sockaddr_in myaddr, clientAddr;
const int numMessages = 5;
const unsigned char messageList[numMessages][BUF_LEN] = {
    "You were lucky to have a room. We used to have to live in a corridor.",
    "Oh we used to dream of livin' in a corridor! Woulda' been a palace to us.",
    "We used to live in an old water tank on a rubbish tip.",
    "We got woken up every morning by having a load of rotting fish dumped all over us.",
    "Quit"};

queue<unsigned char *> messageQueue;

pthread_mutex_t lock_x;
void *display_func(void *arg);
void *send_func(void *arg);
void *recv_func(void *arg);

static void shutdownHandler(int sig)
{
    switch (sig)
    {
    case SIGINT:
        is_running = false;
        break;
    }
}

// Returns a^b mod c
unsigned char PowerMod(int a, int b, int c)
{
    int res = 1;
    for (int i = 0; i < b; ++i)
    {
        res = fmod(res * a, c);
    }
    return (unsigned char)res;
}

// Returns gcd of a and b
int gcd(int a, int h)
{
    int temp;
    while (1)
    {
        temp = a % h;
        if (temp == 0)
            return h;
        a = h;
        h = temp;
    }
}

// Code to demonstrate RSA algorithm
int main()
{
    // Two random prime numbers
    double p = 11;
    double q = 23;

    // First part of public key:
    n = p * q;

    // Finding other part of public key.
    // e stands for encrypt
    e = 2;
    phi = (p - 1) * (q - 1);
    while (e < phi)
    {
        // e must be co-prime to phi and
        // smaller than phi.
        if (gcd((int)e, (int)phi) == 1)
            break;
        else
            e++;
    }
    // Private key (d stands for decrypt)
    // choosing d such that it satisfies
    // d*e = 1 + k * totient
    int k = 2; // A constant value
    d = (1 + (k * phi)) / e;
    cout << "p:" << p << " q:" << q << " n:" << n << " phi:" << phi << " e:" << e << " d:" << d << endl;

    signal(SIGINT, shutdownHandler);

    // TODO: Complete the rest
       int fd, rc;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(-1);
    }
    memset(&myaddr, 0, sizeof(myaddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    // client 1
    myaddr.sin_family = AF_INET;
    rc = inet_pton(AF_INET, IP_ADDR, &myaddr.sin_addr);
    if (rc == -1)
    {
        perror("IP ADDRESS");
        exit(-1);
    }
    myaddr.sin_port = htons(srcPort);

    // client2
    clientAddr.sin_family = AF_INET;
    rc = inet_pton(AF_INET, IP_ADDR, &clientAddr.sin_addr);
    if (rc == -1)
    {
        perror("client 2 ip");

        exit(-1);
    }
    clientAddr.sin_port = htons(destPort);

    if ((bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr))) == -1)
    {
        perror("bind");
        exit(-1);
    }

    is_running = true;
    pthread_create(&tid[0], NULL, recv_func, &fd);
    pthread_create(&tid[1], NULL, send_func, &fd);
    pthread_create(&tid[2], NULL, display_func, NULL);

    sleep(1);

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);

}

void encrypt(const unsigned char msg[BUF_LEN], double eText[BUF_LEN])
{
    // so the theory here is , we convert each character in the string to its ascii value , then encrypt that char and put it in the double array
    // so if our array here of char would be ["abcd\0"], the double array would be [97,98,99,100,0], we will iterate through the char array until we reach 0 ascii value that is , null terminator

    int i = 0;
    int ascii_value = (int)msg[i];
    while (ascii_value != 0)
    {
        int ascii_encrypted = PowerMod(ascii_value, e, n);
        eText[i] = double(ascii_encrypted);
        i++;
        ascii_value = (int)msg[i];
    }
    cout << msg << endl;
    eText[i] = 0;
    cout << "Encryption is Done" << endl;
}
void decrypt(double eText[BUF_LEN], unsigned char msg[BUF_LEN])
{
    int i = 0;
    int ascii_doubleValue = (int)eText[i];
    while (ascii_doubleValue != 0)
    {
        msg[i] = (unsigned char)PowerMod(ascii_doubleValue, d, n);
        i++;
        ascii_doubleValue = (int)eText[i];
    }
    msg[i] = '\0';
    cout << "Decryption is done" << endl;
}
void *display_func(void *arg)
{
    unsigned char *msg;
    while (is_running)
    {
        pthread_mutex_lock(&lock_x);
        if (messageQueue.empty() == 0)
        {
            msg = messageQueue.front();
            if ((strcmp((char *)msg, "Quit")) == 0)
            {
                is_running = false;
            }

            cout << msg << endl;
        }
        pthread_mutex_unlock(&lock_x);
    }
    pthread_exit(0);
}
void *send_func(void *arg)
{
    int fd = *(int *)arg;
    // const unsigned char messages[5][256]=local_args->arg2;

    sleep(5);

    double encryptedText[5][BUF_LEN];
    memset(&encryptedText, 0, sizeof(encryptedText));

    for (int i = 0; i < numMessages; i++)
    {
        encrypt(messageList[i], encryptedText[i]);
    }
    for (int i = 0; i < numMessages; i++)
    {

        int numSent = sendto(fd, (double *)&encryptedText[i], BUF_LEN, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
#if defined(DEBUG1)
        cout << "Number of bytes sent are " << numSent << endl;
#endif

        sleep(1);
    }
}

void *recv_func(void *arg)
{
    int fd = *(int *)arg;
    struct sockaddr_in recvClient;
    socklen_t len = sizeof(recvClient);

    // we will be convertingt the recieved text(encrypted text) into  decrypted text
    double recvText[BUF_LEN];
    unsigned char msg[BUF_LEN];
    memset(&recvText, 0, BUF_LEN);
    memset(&msg, 0, BUF_LEN);

    while (is_running)
    {
        int numRecv = recvfrom(fd, &recvText, BUF_LEN, 0, (struct sockaddr *)&recvClient, &len);

        if (numRecv != 0)
        {
            decrypt(recvText, msg);

            pthread_mutex_lock(&lock_x);
            messageQueue.push(msg);
            pthread_mutex_unlock(&lock_x);
        }
        else
        {
            sleep(1);
        }

        // zeroing out the values again
        memset(&recvText, 0, BUF_LEN);
        memset(&msg, 0, BUF_LEN);
    }
}


// TODO: Complete the receive thread
