#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_SETSOCKETOPT,
    ERR_BIND,
    ERR_LISTEN
};

int init_socket(int port) {
    // open socket, return socket descriptor
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Fail: open socket");
        _exit(ERR_SOCKET);
    }

    // set socket option
    int socket_option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
        &socket_option, sizeof(socket_option));
    if (server_socket < 0) {
        perror("Fail: set socket options");
        _exit(ERR_SETSOCKETOPT);
    }

    // set socket address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *) &server_address,
        sizeof(server_address)) < 0) {
        perror("Fail: bind socket address");
        _exit(ERR_BIND);
    }

    // listen mode start
    if (listen(server_socket, 5) < 0) {
        perror("Fail: bind socket address");
        _exit(ERR_LISTEN);
    }
    return server_socket;
}

int telnet(char *word) {
    int fd = open(word, O_WRONLY, 0);
    if (fd > -1) {
        printf("HTTP/1.1 200\n");
        int content_type;
        int type_ind = 0;
        for (; word[type_ind] != '.'; type_ind++);
        printf("content-type: %s/text\n", &(word[type_ind + 1]));
        printf("content-length:");
        if (fork() == 0) {
            dup2(fd, 0);
            execlp("wc", "wc", "-c", NULL);
            close(fd);
            _exit(1);
        } else {
            wait(NULL);
        }

        puts("\n");
        execlp("head", "head", word, NULL);
    } else {
        printf("HTTP/1.1 404\n");
    }
    close(fd);
    puts("\n");
    return 1;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        puts("Incorrect args.");
        puts("./server <port> <client_numb>");
        puts("Example:");
        puts("./server 5000");
        return ERR_INCORRECT_ARGS;
    }
    int port = atoi(argv[1]);
    int server_socket = init_socket(port);
    puts("Wait for connection");
    struct sockaddr_in client_address;
    socklen_t size;
    int client_socket = accept(server_socket,
                            (struct sockaddr *) &client_address,
                            &size);
    printf("connected: %s %d\n", inet_ntoa(client_address.sin_addr),
                            ntohs(client_address.sin_port));
    char *word = NULL;
    while(1) {
        free(word);
        int trash[12];
        word = NULL;
        char ch;
        read(client_socket, &trash , 4);
        read(client_socket, &ch, 1);
        for(int j = 1; ch != 0; j++) {
            word = realloc(word, sizeof(char) * j);
            word[j - 1] = ch;
            read(client_socket, &ch, 1);
        }
        read(client_socket,  &trash, 9);
        read(client_socket, &trash , 6);
        read(client_socket, &trash , 12);
        printf("%s\n", word);
        telnet(word);
    }
    close(client_socket);
    return OK;
}
