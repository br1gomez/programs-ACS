#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFFER_SIZE 4096

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    // Ejecutar cliente con IP del server y mismo puerto
    if (argc < 3) {
       fprintf(stderr,"Se necesita IP, PUERTO\n");
       exit(0);
    }

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];

    portno = atoi(argv[2]);
    
    // Socket TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error abriendo socket");

    //Obtener direccion del server, por ip o nombre (localhost)
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"Error al encontrar el host\n");
        exit(0);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    // Conectando al server
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("Error al conectar");

    printf("**** CONECTADO AL SERVER SSH ****\n");
    printf("Probando ejecución: ls -l, pwd, ps, whoami, cat, uname -a. \n\n");
    printf("Escribe 'exit' o 'salir' para desconectar.\n");

    while(1) {
        printf("ServerSSH> "); // Para simular un prompt
        memset(buffer, 0, BUFFER_SIZE);
        
        // Entrada de usuario
        fgets(buffer, 255, stdin);

        // Quitando salto de linea de entrada de usuario
        buffer[strcspn(buffer, "\n")] = 0;

        // Enviando comando al server
        n = send(sockfd, buffer, strlen(buffer), 0);
        if (n < 0) error("Error escribiendo al socket");

        //  Para salir del bucle
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "salir") == 0) {
            break;
        }

        // Recibiendo respuesta del server
        memset(buffer, 0, BUFFER_SIZE);
        n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (n < 0) error("Error leyendo del socket");
        
        // Respuesta
        printf("%s\n", buffer);
    }

    close(sockfd);
    return 0;
}