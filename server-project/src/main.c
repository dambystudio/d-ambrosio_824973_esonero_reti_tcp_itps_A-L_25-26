/*
 * main.c
 *
 * TCP Server - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a TCP server
 * portable across Windows, Linux and macOS.
 */

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
#include <time.h>
#include "protocol.h"

// === FUNZIONI ===

float get_temperature(void) {
    return ((rand() % 501) / 10.0) - 10.0;
}

float get_humidity(void) {
    return ((rand() % 801) / 10.0) + 20.0;
}

float get_wind(void) {
    return (rand() % 1001) / 10.0;
}

float get_pressure(void) {
    return ((rand() % 1001) / 10.0) + 950.0;
}

void errorhandler(char *errorMessage) {
	printf("%s", errorMessage);
}

void clearwinsock() {
	#if defined WIN32
		WSACleanup();
	#endif
}

const char *supported_cities[] = {
    "bari", "roma", "milano", "napoli", "torino",
    "palermo", "genova", "bologna", "firenze", "venezia"
};

const int num_cities = 10;

int is_valid_type(char type) {
    return (type == TYPE_TEMPERATURE || 
            type == TYPE_HUMIDITY || 
            type == TYPE_WIND || 
            type == TYPE_PRESSURE);
}

int is_valid_city(const char *city) {
    for (int i = 0; i < num_cities; i++) {
        #if defined WIN32
            if (_stricmp(city, supported_cities[i]) == 0) {
                return 1;
            }
        #else
            if (strcasecmp(city, supported_cities[i]) == 0) {
                return 1;
            }
        #endif
    }
    return 0;
}

float get_weather_value(char type) {
    switch(type) {
        case TYPE_TEMPERATURE: 
			return get_temperature();
        case TYPE_HUMIDITY:    
			return get_humidity();
        case TYPE_WIND:        
			return get_wind();
        case TYPE_PRESSURE:    
			return get_pressure();
        default:               
			return 0.0;
    }
}

int comunicazione(int clientSocket, struct sockaddr_in client_addr){
	weather_request_t request;
    weather_response_t response;
	int bytes_rcvd;

    bytes_rcvd = recv(clientSocket, (char*)&request, sizeof(request), 0); 
    if (bytes_rcvd <= 0) {
        errorhandler("Ricezione richiesta fallita.\n");
        closesocket(clientSocket);  
        return -1;  
    }


    printf("Richiesta '%c %s' dal client ip %s\n", request.type, request.city, inet_ntoa(client_addr.sin_addr)); 

	if (!is_valid_type(request.type)) {
        response.status = STATUS_INVALID_REQUEST;
        response.type = '\0';
        response.value = 0.0;
    }

	else if (!is_valid_city(request.city)) {
        response.status = STATUS_CITY_NOT_FOUND;
        response.type = request.type; 
        response.value = 0.0;
    }

    else {
        response.status = STATUS_SUCCESS;
        response.type = request.type;
        response.value = get_weather_value(request.type);
    }

	if (send(clientSocket, (char*)&response, sizeof(response), 0) < 0) {
        errorhandler("Invio risposta fallito.\n");
        closesocket(clientSocket);
        return -1;
    }

	closesocket(clientSocket);
    printf("Connessione chiusa.\n");
    
    return 0;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    // ✅ AGGIUNGI QUESTO: Parsing argomenti per la porta
    int server_port = SERVER_PORT;  // Porta di default
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            server_port = atoi(argv[i + 1]);
            if (server_port < 1024 || server_port > 65535) {
                printf("Errore: porta non valida (usa 1024-65535)\n");
                return -1;
            }
            printf("Utilizzo porta: %d\n", server_port);
            i++;
        }
    }

#if defined WIN32
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (result != 0) {
        printf("Errore nella funziona WSAStartup()\n");
        return 0;
    }
#endif

    int socketServer;
    socketServer = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketServer < 0) {
        errorhandler("Creazione del socket fallita.\n");
        clearwinsock();
        return -1;
    }
    printf("Creazione socket eseguita.\n");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);  // ✅ USA server_port invece di SERVER_PORT
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketServer, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        errorhandler("Binding fallito.\n");
        closesocket(socketServer);
        clearwinsock();
        return -1;
    }
    printf("Binding completato sulla porta %d.\n", server_port);  // ✅ Stampa la porta

    if (listen(socketServer, QUEUE_SIZE) < 0) {
        errorhandler("Ascolto fallito.\n");
        closesocket(socketServer);
        clearwinsock();
        return -1;
    }
    printf("In attesa di client...\n");

    struct sockaddr_in cad;
    int clientSocket;
    int clientLen;
    
    while(1) {
        printf("\nAspettando connessione client...\n");
        clientLen = sizeof(cad);
        
        if ((clientSocket = accept(socketServer, (struct sockaddr*)&cad, &clientLen)) < 0) {
            errorhandler("Accettazione fallita.\n");
            continue;
        }

        printf("Client connesso: %s\n", inet_ntoa(cad.sin_addr));
        
        comunicazione(clientSocket, cad);
    }

    closesocket(socketServer);
    clearwinsock();
    return 0;
}