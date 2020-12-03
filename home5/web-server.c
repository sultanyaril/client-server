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

char *get_word(int client_socket) {
    char *word = NULL;
    char ch;
    int size = 0;
    for(read(client_socket, &ch, 1); ch != '\0'; read(client_socket, &ch, 1)) {
        size++;
        word = realloc(word, sizeof(char) * (size + 1));
        word[size - 1] = ch;
    }
    word[size] = '\0';
    return word;
}

int telnet(char *word, int client_socket) {
    int fd = open(word, O_RDONLY, 0);
    if (fd > -1) {
        char cont1[] = "HTTP/1.1 200\ncontent-type: ";
        write(client_socket, cont1, strlen(cont1));
        int content_type;
        int type_ind = 0;
        for (; word[type_ind] != '.'; type_ind++);
        write(client_socket, &(word[type_ind]), strlen(&(word[type_ind])));
        printf("%s/text\n", &(word[type_ind + 1]));
        
        struct stat stats;
        if (stat(word, &stats) != 0)
            perror("stat error");
        char cont2[] = "/text\ncontent-size: ";
        write(client_socket, cont2, strlen(cont2));
        char str_size[15];
        sprintf(str_size, "%ld", stats.st_size);
        write(client_socket, str_size, strlen(str_size));
        write(client_socket, "\n", 2);
        char *buff = malloc(sizeof(char) * stats.st_size);
        read(fd, buff, stats.st_size);
        write(client_socket, buff, stats.st_size);
        free(buff);
        write(client_socket, "\n", 1);
    } else {
        char msg[] = "HTTP/1.1 404\n";
        write(client_socket, msg, strlen(msg));
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
    char *cont = NULL;
    while(1) {
        free(word);
        char ch;
        int size;

        cont = get_word(client_socket);
        if (!strcmp(cont, "quit"))
            break;
        if (strcmp(cont, "GET"))
            perror("Incorrect query 1");

        free(cont);

        word = get_word(client_socket);

        cont = get_word(client_socket);
        if (strcmp(cont, "HTTP/1.1"))
            perror("Incorrect query 2");
        free(cont);

        cont = get_word(client_socket);
        if (strcmp(cont, "Host:"))
            perror("Incorrect query 3");
        free(cont);

        cont = get_word(client_socket);
        if (strcmp(cont, "mymath.info"))
            perror("Incorrect query 4");
        free(cont);

        printf("%s\n", word);
        telnet(word, client_socket);
    }
    free(cont);
    close(client_socket);
    return OK;
}
