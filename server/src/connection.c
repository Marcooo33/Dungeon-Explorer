#include <stdio.h>      
#include <stdlib.h>  
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>  

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket fallito");
        exit(EXIT_FAILURE);
    }

    // Evita l'errore "Address already in use" al riavvio del server
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Accetta connessioni da ogni IP
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind fallita");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen fallita");
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("In attesa di connessioni sulla porta 8080...\n");
        
        int client_socket = accept(server_fd, NULL, NULL);
        if (client_socket < 0) {
            perror("Accept fallita");
            exit(EXIT_FAILURE);
        }

        printf("Client Godot connesso con successo!\n");
    }

    // Qui andrebbe la logica per il matchmaking o la fork()

    return 0;
}