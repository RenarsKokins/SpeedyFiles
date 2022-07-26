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
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define INPUT_SIZE 127
#define BUFFER_SIZE 1024

void print_actions()
{
    printf("----\n");
    printf("Press 'd' to download file\n");
    printf("Press 'l' to get a list of files on server\n");
    printf("Press 'q' to quit\n");
    printf("----\n");
    printf("Your action: ");
}

int send_download_request(int sockfd, struct sockaddr_in server, char *file_name)
{
    int sent = 0;
    char buffer[INPUT_SIZE + 1];
    memset(buffer, 0, sizeof(buffer));

    /* Specify what we want (so server can understand us) */
    buffer[0] = 'd';
    strncat(buffer, file_name, INPUT_SIZE);
    buffer[strlen(buffer) - 1] = '\0';

    /* Send action type and file name to fetch from server */
    sent = sendto(sockfd, buffer, sizeof(buffer), 0, (const struct sockaddr *)&server, sizeof(server));
    printf("Bytes sent: %d\n", sent);
    return sent;
}

int send_file_list_request(int sockfd, struct sockaddr_in server)
{
    int sent = 0;
    char buffer[1];

    /* Specify what we want (so server can understand us) */
    buffer[0] = 'l';

    /* Send list action to fetch file names from server */
    sent = sendto(sockfd, buffer, sizeof(buffer), 0, (const struct sockaddr *)&server, sizeof(server));
    printf("Bytes sent: %d\n", sent);
    return sent;
}

void print_file_list(char *buffer)
{
    char current = buffer[0];
    int counter = 0;

    while (current != '\0')
    {
        printf("%c", current);
        counter++;
        current = buffer[counter];
    }
}

int get_file_list(int sockfd, struct sockaddr_in *server)
{
    int len;
    int recieved = 0;
    char buffer[BUFFER_SIZE];

    printf("\n\n\n\n---- File list on server ----\n");
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        recieved = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server, &len);
        if (recieved == 1)
            break;
        else if (recieved == -1)
        {
            perror("Failed to recieve");
            exit(EXIT_FAILURE);
        }
        print_file_list(buffer);
    }
    printf("-----------------------------\n");
    return recieved;
}

int write_to_file(char *buffer, char *file_name)
{
    char ch;
    FILE *fp;
    char *path = "./downloads/";
    char fullpath[BUFFER_SIZE];

    memset(fullpath, 0, BUFFER_SIZE);
    strncat(fullpath, path, strlen(path));
    strncat(fullpath, file_name, BUFFER_SIZE - strlen(path));

    printf("FILE NAME: %s\n", fullpath);
    fp = fopen(fullpath, "w");
    if (fp == NULL)
        return -1;
    printf("File created!\n");
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        ch = buffer[i];
        if (ch == EOF)
            break;
        fputc(ch, fp);
    }
    fclose(fp);
    return 1;
}

int download_file(int sockfd, struct sockaddr_in *server, char *file_name)
{
    FILE *fp;
    int len, status;
    int recieved = 0;
    bool file_found = 0;
    unsigned long size = 0;
    char buffer[BUFFER_SIZE];

    printf("Waiting for data from server...\n");
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        recieved = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server, &len);
        printf("Recieved %d bytes of data...\n", recieved);

        if (recieved == 1)
            break;

        status = write_to_file(buffer, file_name);
        if (status == -1)
            printf("File write failed!");
        else if (recieved == -1)
        {
            perror("Failed to recieve");
            exit(EXIT_FAILURE);
        }
        file_found = 1;
    }
    if (file_found == 0)
        printf("File not found on server!\n");
    return recieved;
}

int main(int argc, char *argv[])
{
    int sockfd;
    char input[INPUT_SIZE];
    bool end_flag = false;
    struct sockaddr_in server;

    if (argc < 3)
    {
        printf("Usage: call program with 2 arguments - IPv4 address (xx.xx.xx.xx) and a server port to connect to.\n");
        return 1;
    }

    /* Create socket file descriptor */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return 2;
    }

    /* Create server address/port */
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    if (inet_aton(argv[1], &server.sin_addr) != 1)
    {
        perror("Invalid IP address");
        return 3;
    }
    server.sin_port = htons(atoi(argv[2]));

    /* Main user interaction loop */
    while (!end_flag)
    {
        print_actions();
        fgets(input, INPUT_SIZE, stdin);
        switch (input[0])
        {
        case 'q':
            end_flag = true;
            break;
        case 'l':
            printf("Getting a list of files...\n");
            /* Send a file list request so server knows what we want */

            if (send_file_list_request(sockfd, server) < 0)
            {
                perror("Problem sending file list request");
                return 4;
            }

            /* Get ready to recieve file list and print it */
            if (get_file_list(sockfd, &server) < 0)
            {
                perror("Problem getting file list");
                return 5;
            }
            break;
        case 'd':
            printf("Enter file name (press enter to cancel): ");
            memset(input, 0, INPUT_SIZE);
            fgets(input, INPUT_SIZE, stdin);

            /* Don't send a request to server if user does not want to */
            if (input[0] == '\n')
                break;

            /* Send a download request so server knows what we want */
            if (send_download_request(sockfd, server, input) < 0)
            {
                perror("Problem sending download request");
                return 4;
            }

            /* Get ready to recieve file data and save it */
            if (download_file(sockfd, &server, input) < 0)
            {
                perror("Problem downloading file");
                return 5;
            }
            break;
        default:
            printf("Incorrect mode selected...\n");
            break;
        }
    }
    printf("Quitting program...\n");

    return 0;
}
