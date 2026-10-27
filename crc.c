// crc server
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define PORT 8080

int receive_all(int sock, void *data, int size)
{
    int received = 0;
    while (received < size)
    {
        int n = recv(sock, (char *)data + received,
                     size - received, 0);
        if (n <= 0) return 0;
        received += n;
    }
    return 1;
}

int main()
{
    int server, client, length;
    struct sockaddr_in addr;
    char frame[8192];
    char temp[8192];
    char generator[] = "10011";

    server = socket(AF_INET, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);

    printf("CRC Server waiting...\n");

    client = accept(server, NULL, NULL);

    if (client < 0 ||
        !receive_all(client, &length, sizeof(length)) ||
        length <= 4 || length >= (int)sizeof(frame))
    {
        printf("Connection or frame error\n");
        close(server);
        return 1;
    }

    if (!receive_all(client, frame, length))
    {
        printf("Frame reception failed\n");
        close(client);
        close(server);
        return 1;
    }

    frame[length] = '\0';
    printf("Received frame: %s\n", frame);

    /* Introduce a bit error */
    frame[0] = (frame[0] == '0') ? '1' : '0';
    printf("Frame after introducing error: %s\n", frame);

    strcpy(temp, frame);

    /* Modulo-2 division */
    for (int i = 0; i <= length - 5; i++)
    {
        if (temp[i] == '1')
        {
            for (int j = 0; j < 5; j++)
                temp[i + j] =
                    (temp[i + j] == generator[j]) ? '0' : '1';
        }
    }

    printf("CRC remainder: %s\n", temp + length - 4);

    if (strspn(temp + length - 4, "0") == 4)
        printf("Frame is ERROR-FREE\n");
    else
        printf("ERROR DETECTED in received frame\n");

    close(client);
    close(server);
    return 0;
}


//crc client
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define PORT 8080

int send_all(int sock, void *data, int size)
{
    int sent = 0;
    while (sent < size)
    {
        int n = send(sock, (char *)data + sent,
                     size - sent, 0);
        if (n <= 0) return 0;
        sent += n;
    }
    return 1;
}

int main()
{
    int sock;
    struct sockaddr_in server;
    char data[100], bits[8192], temp[8192];
    char generator[] = "10011";
    int length = 0;

    printf("Enter data: ");
    scanf("%99s", data);

    /* Convert each character to 8-bit binary */
    for (int i = 0; data[i] != '\0'; i++)
    {
        for (int b = 7; b >= 0; b--)
            bits[length++] =
                ((data[i] >> b) & 1) ? '1' : '0';
    }

    /* Append four zeros */
    for (int i = 0; i < 4; i++)
        bits[length++] = '0';

    bits[length] = '\0';
    strcpy(temp, bits);

    /* Calculate CRC remainder */
    for (int i = 0; i <= length - 5; i++)
    {
        if (temp[i] == '1')
        {
            for (int j = 0; j < 5; j++)
                temp[i + j] =
                    (temp[i + j] == generator[j]) ? '0' : '1';
        }
    }

    /* Append the four-bit remainder */
    for (int i = 0; i < 4; i++)
        bits[length - 4 + i] = temp[length - 4 + i];

    printf("CRC remainder: %s\n", temp + length - 4);
    printf("Transmitted frame: %s\n", bits);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server,
                sizeof(server)) < 0)
    {
        printf("Connection failed\n");
        close(sock);
        return 1;
    }

    send_all(sock, &length, sizeof(length));
    send_all(sock, bits, length);

    printf("CRC frame sent to server\n");

    close(sock);
    return 0;
}