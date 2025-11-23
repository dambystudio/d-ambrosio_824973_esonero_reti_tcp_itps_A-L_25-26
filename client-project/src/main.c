#if defined WIN32
#include <winsock.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>  // ✅ AGGIUNGI QUESTO!
#include "protocol.h"

// ❌ RIMUOVI QUESTA RIGA:
// #define NO_ERROR 0

void errorhandler(char *errorMessage) {
    printf("%s", errorMessage);
}

void clearwinsock() {
#if defined WIN32
    WSACleanup();
#endif
}

void print_usage(void) {
    printf("Usage: client-project [-s server] [-p port] -r \"type city\"\n");
    printf("  -s server : Server IP (default: 127.0.0.1)\n");
    printf("  -p port   : Server port (default: 56700)\n");
    printf("  -r request: Weather request \"t|h|w|p city\" (REQUIRED)\n");
    printf("\nExample: client-project -r \"t bari\"\n");
}

void print_result(char *server_ip, weather_response_t response, char *city) {
    printf("Ricevuto risultato dal server ip %s. ", server_ip);

    if (response.status == STATUS_SUCCESS) {
        city[0] = toupper((unsigned char)city[0]);  // ✅ Ora funziona con ctype.h
        
        switch(response.type) {
            case TYPE_TEMPERATURE:
                printf("%s: Temperatura = %.1f*C\n", city, response.value);
                break;
            case TYPE_HUMIDITY:
                printf("%s: Umidita' = %.1f%%\n", city, response.value);
                break;
            case TYPE_WIND:
                printf("%s: Vento = %.1f km/h\n", city, response.value);
                break;
            case TYPE_PRESSURE:
                printf("%s: Pressione = %.1f hPa\n", city, response.value);
                break;
        }
    }
    else if (response.status == STATUS_CITY_NOT_FOUND) {
        printf("Citta' non disponibile\n");
    }
    else if (response.status == STATUS_INVALID_REQUEST) {
        printf("Richiesta non valida\n");
    }
}

int main(int argc, char *argv[]) {

    char server_ip[16] = "127.0.0.1";
    int server_port = SERVER_PORT;  // ✅ Cambia nome variabile
    char request_string[128] = "";
    int has_request = 0;

    // PARSING ARGOMENTI
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            strcpy(server_ip, argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            server_port = atoi(argv[i + 1]);  // ✅ Usa server_port
            i++;
        }
        else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            strcpy(request_string, argv[i + 1]);
            has_request = 1;
            i++;
        }
    }

    if (!has_request) {
        printf("Errore: argomento -r obbligatorio\n");
        print_usage();
        return -1;
    }

#if defined WIN32
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (result != 0) {  // ✅ Usa 0 invece di NO_ERROR
        printf("Error at WSAStartup()\n");
        return 0;
    }
#endif

    // CREA SOCKET
    int my_socket;
    my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (my_socket < 0) {
        errorhandler("Socket creation failed.\n");
        clearwinsock();
        return -1;
    }

    // CONFIGURA INDIRIZZO SERVER
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);  // ✅ USA server_port QUI!
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // CONNETTI AL SERVER
    if (connect(my_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        errorhandler("Connessione al server fallita.\n");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    // PARSE DELLA RICHIESTA
    weather_request_t request;
    memset(&request, 0, sizeof(request));
    
    request.type = request_string[0];
    
    if (strlen(request_string) > 2) {
        strcpy(request.city, &request_string[2]);
    } else {
        printf("Errore: formato richiesta non valido\n");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }
    
    // Converti città in lowercase
    for (int i = 0; request.city[i]; i++) {
        request.city[i] = tolower((unsigned char)request.city[i]);  // ✅ Ora funziona
    }

    // INVIA RICHIESTA
    if (send(my_socket, (char*)&request, sizeof(request), 0) < 0) {
        errorhandler("Invio richiesta fallito.\n");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    // RICEVI RISPOSTA
    weather_response_t response;
    int bytes_rcvd = recv(my_socket, (char*)&response, sizeof(response), 0);
    if (bytes_rcvd <= 0) {
        errorhandler("Ricezione risposta fallita.\n");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    // STAMPA RISULTATO
    print_result(server_ip, response, request.city);

    // CHIUDI E PULISCI
    closesocket(my_socket);
    clearwinsock();
    
    return 0;
}