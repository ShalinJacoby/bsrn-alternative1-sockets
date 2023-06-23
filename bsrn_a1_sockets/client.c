#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <time.h>


int run = 1; // Speichert den Status einer Endlosschleife
pid_t childproc_conv; //pid_t gibt PID der Prozesse zurück
pid_t childproc_stat;
pid_t childproc_log;
pid_t childproc_report;

// Signal-Handler-Funktion für "SIGINT", die die Endlosschleife beendet:
void sigint_handler(int sig)
{
    printf("Signal SIGINT erhalten\n");
    run = 0;

}
void cleanup()
{
    // Kindprozesse beenden
    kill(childproc_conv, SIGTERM);// SIGTERM: Signal um  Prozess anzuzeigen, dass er beendet werden soll
    kill(childproc_stat, SIGTERM);
    kill(childproc_log, SIGTERM);
    kill(childproc_report, SIGTERM);
}


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);

    while(run) {

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    portno = 8080;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Fehler beim Öffnen des Sockets");

    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "Fehler, Host existiert nicht\n");
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Fehler beim Verbinden");

    // Hier ist der Prozess Conv!
    int childproc_conv = fork();
    if (childproc_conv == 0)
    {
        printf("Conv: liest Messwerte von A/D-Konvertern ein und sendet sie an den Server\n");
        printf("Conv: PID: %i\n", getpid());

        int zufallszahlen[10];
        for (int i = 0; i < 10; i++)
        {
            zufallszahlen[i] = rand() % 100;
        }

        n = write(sockfd, zufallszahlen, sizeof(int) * 10);
        if (n < 0)
            error("Fehler beim Schreiben des Sockets");

        exit(0);
    }

    // Hier ist der Prozess Log!
    int childproc_log = fork();
    if (childproc_log == 0)
    {
        printf("Log: liest Messwerte von Conv aus und schreibt sie in eine lokale Datei\n");
        printf("Log: PID: %i\n", getpid());

        int zufallszahlen[10];

        n = read(sockfd, zufallszahlen, sizeof(int) * 10);
        if (n < 0)
            error("Fehler beim Lesen des Sockets");

        FILE *lokal_file;
        lokal_file = fopen("log", "w");
        if (lokal_file == 0)
        {
            printf("Fehler beim Öffnen der Datei\n");
            exit(1);
        }

        for (int i = 0; i < 10; i++)
        {
            fprintf(lokal_file, "%d\n", zufallszahlen[i]);
        }
        fclose(lokal_file);

        exit(0);
    }

    // Hier ist der Prozess Stat!
    int childproc_stat = fork();
    if (childproc_stat == 0)
    {
        printf("Stat: liest Messwerte von Conv aus und berechnet statistische Daten\n");
        printf("Stat: PID: %i\n", getpid());

        int zufallszahlen[10];
        float summe = 0;
        float mittelwert;

        n = read(sockfd, zufallszahlen, sizeof(int) * 10);
        if (n < 0)
            error("Fehler beim Lesen des Sockets");

        for (int i = 0; i < 10; i++)
        {
            summe += zufallszahlen[i];
        }
        mittelwert = summe / 10;

        n = write(sockfd, &summe, sizeof(float));
        if (n < 0)
            error("Fehler beim Schreiben des Sockets");

        n = write(sockfd, &mittelwert, sizeof(float));
        if (n < 0)
            error("Fehler beim Schreiben des Sockets");

        exit(0);
    }

    // Hier ist der Prozess Report!
    int childproc_report = fork();
    if (childproc_report == 0)
    {
        printf("Report: greift auf Ergebnisse von Stat zu\n");
        printf("Report: PID: %i\n", getpid());

        float summe, mittelwert;

        n = read(sockfd, &summe, sizeof(float));
        if (n < 0)
            error("Fehler beim Lesen des Sockets");
        n = read(sockfd, &mittelwert, sizeof(float));
        if (n < 0)
            error("Fehler beim Lesen des Sockets");

        printf("\n");
        printf("Summe der Zufallszahlen: %.2f\n", summe);
        printf("Mittelwert der Zufallszahlen: %.2f\n", mittelwert);

        exit(0);
    }

    // Warten auf Kindprozesse
  //  wait(NULL);
  //  wait(NULL);
  //  wait(NULL);
  //  wait(NULL);

    close(sockfd);
    }
    cleanup();
    return 0;
}
