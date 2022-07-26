/*
Made by: Renārs Kokins

Uzdevums:
Uzrakstīt programmu (klients - serveris), kura pārsūta failu, izmantojot UDP protokolu (negarantē drošu un secīgu datu pārraidi).
Uzdevuma risinājums var būt kā 1 programma ar parametriem, kura nosaka vai programmu palaist kā serveri vai kā klientu, vai
arī 2 programmas (klients un serveris).

Jābūt/jāizmanto:
1. E-poll (https://man7.org/linux/man-pages/man7/epoll.7.html)
2. E-poll nepieciešams izmantot klienta un servera pusē
3. Pārsūtītam failam jāsaglabā oriģinālo nosaukumu
4. Klientam jāuzrāda sūtīšanas ātrumu
5. Serverim jāspēj apkalpot vairākus klientus vienlaicīgi
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

int send_end(int sockfd, struct sockaddr_in client, int len)
{
    int sent;
    char end[1] = {0};
    sent = sendto(sockfd, end, sizeof(end), 0, (const struct sockaddr *)&client, len);
    printf("Sent end message to client\n");
    return sent;
}

int send_file_list_response(int sockfd, struct sockaddr_in client, int len)
{
    DIR *d;
    int sent = 0;
    char null = '\0';
    struct dirent *dir;
    char buffer[BUFFER_SIZE];

    memset(buffer, 0, BUFFER_SIZE);
    d = opendir("./files");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_name[0] == '.')
                continue;
            strncat(buffer, dir->d_name, BUFFER_SIZE - sizeof(dir->d_name) - 1);
            buffer[strlen(buffer)] = '\n';
        }
        closedir(d);
    }
    sent = sendto(sockfd, buffer, BUFFER_SIZE, 0, (const struct sockaddr *)&client, len);
    printf("Bytes sent: %d\n", sent);
    sent = send_end(sockfd, client, len);
    return sent;
}

int write_file_to_buffer(char *buffer)
{
    char ch;
    char fullpath[BUFFER_SIZE];
    char *path = "./files/";
    FILE *fp;

    memset(fullpath, 0, BUFFER_SIZE);
    strncat(fullpath, path, strlen(path));
    strncat(fullpath, buffer + 1, BUFFER_SIZE - strlen(path));

    printf("File name: %s\n", fullpath);
    fp = fopen(fullpath, "r");
    if (fp == NULL)
        return -1;
    printf("File found!\n");
    memset(buffer, 0, BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        ch = fgetc(fp);
        buffer[i] = ch;
        if (ch == EOF)
            break;
    }
    fclose(fp);
    printf("File created (%s)!\n", fullpath);
    return 1;
}

int send_file_data(int sockfd, struct sockaddr_in client, int len, char *buffer)
{
    int sent, status;
    status = write_file_to_buffer(buffer);

    if (status == -1)
    {
        sent = send_end(sockfd, client, len);
        printf("File not found!\n");
        return 0;
    }

    sent = sendto(sockfd, buffer, BUFFER_SIZE, 0, (const struct sockaddr *)&client, len);
    printf("Bytes sent: %d\n", sent);
    sent = send_end(sockfd, client, len);
    return sent;
}

int main(int argc, char *argv[])
{
    bool end_flag = false;
    char buffer[BUFFER_SIZE];
    int sockfd, recieved, sent, len;
    struct sockaddr_in server, client;

    if (argc < 2)
    {
        printf("Usage: call program with 1 argument - a port for client to send data to.\n");
        return 1;
    }

    /* Create socket file descriptor */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return 2;
    }

    /* Create server and client address/port */
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1]));

    /* Bind socket with server address */
    if (bind(sockfd, (const struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Bind failed");
        return 3;
    }

    printf("Server started...\n");
    /* Main server loop */
    while (!end_flag)
    {
        /* Listen for requests */
        memset(buffer, 0, BUFFER_SIZE);
        len = sizeof(client);
        recieved = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client, &len);
        printf("Bytes recieved: %d\n", recieved);
        if (recieved < 0)
            printf("Recieve failed!\n");
        switch (buffer[0])
        {
        case 'l':
            sent = send_file_list_response(sockfd, client, len);
            if (sent < 0)
            {
                perror("Send file list failed");
                return 6;
            }
            break;
        case 'd':
            sent = send_file_data(sockfd, client, len, buffer);
            if (sent < 0)
            {
                perror("Send file data failed");
                return 7;
            }
            break;
        }
    }
    printf("Quitting program...\n");

    return 0;
}
