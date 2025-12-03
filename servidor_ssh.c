#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 4096 // Buffer para la salida de los datos

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    // Pasar el puerto como argumento por linea de comandos
    if (argc < 2) {
        fprintf(stderr, "Se necesita especificar el puerto como argumento\n");
        exit(1);
    }

    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    int opt = 1;

    // Crear Socket TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error en el socket");

    // SO_REUSEADDR para reiniciar server rapido.
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Estructura del server
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Bind
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("Error en bind");

    // Listen
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    printf("SERVER esperando conexiones en puerto: %d\n", portno);
    
    // Accept 
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) error("Error en accept");
    
    printf("Cliente conectado.\n");

    // While para recibir comandos del cliente
    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        
        // Recibir comando del cliente
        n = recv(newsockfd, buffer, 255, 0); 
        if (n <= 0) break; // Error o conexión cerrada

        // Se elimina el salto de linea, esto es para popen
        buffer[strcspn(buffer, "\n")] = 0;

        printf("Comando leído: %s\n", buffer);

        // Si el exit o salir se cierra la conexión
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "salir") == 0) {
            printf("****Desconectando cliente****\n");
            break;
        }

        //  USO DE POPEN PARA LA EJECUCION DE COMANDOS
        //popen abre un nuevo proceso y ejecuta el comando, da un puntero de archivo para leer la salida
        FILE *fp = popen(buffer, "r");
        if (fp == NULL) {
            char *msg = "Error al ejecutar comando.\n";
            send(newsockfd, msg, strlen(msg), 0);
            continue;
        }

        // Se lee la salida del comando y se guarda 
        char output[BUFFER_SIZE] = {0};
        char line[1024];
        
        // Se lee linea por linea la salida del comando
        while (fgets(line, sizeof(line), fp) != NULL) {
            // Para no exceder el buffer
            if (strlen(output) + strlen(line) < BUFFER_SIZE - 1) {
                strcat(output, line);
            }
        }

        pclose(fp); // Se cierra el proceso

        // En caso de que no haya salida se genera una confirmación (mkdir)
        if (strlen(output) == 0) {
            strcpy(output, "Comando ejecutado correctamente.\n");
        }

        // Enviar la salida de regreso al cliente
        send(newsockfd, output, strlen(output), 0);
    }

    close(newsockfd);
    close(sockfd);
    return 0;
}