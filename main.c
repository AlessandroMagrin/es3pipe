#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main() {
    int pipe1[2]; // Pipe 1: A -> B
    int pipe2[2]; // Pipe 2: B -> C
    pid_t pidB, pidC;

    //Creazione delle due pipe prima dei fork
    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        perror("Errore nella creazione delle pipe");
        exit(EXIT_FAILURE);
    }

    //Generazione del primo figlio (Processo B)
    pidB = fork();
    if (pidB < 0) {
        perror("Errore fork B");
        exit(EXIT_FAILURE);
    }

    if (pidB == 0) {

        // B legge da pipe1 e scrive su pipe2. Chiudiamo i canali non usati:
        close(pipe1[1]); // Non scrive su pipe1
        close(pipe2[0]); // Non legge da pipe2

        int num;

        // Legge i numeri inviati da A finché la pipe rimane aperta
        while (read(pipe1[0], &num, sizeof(num)) > 0) {
            // Scarta i numeri minori di 50
            if (num >= 50) {
                int quadrato = num * num;
                // Invia il quadrato al processo C tramite pipe2
                write(pipe2[1], &quadrato, sizeof(quadrato));
            }
        }

        // Chiude i descrittori rimasti aperti per segnalare la fine dei dati
        close(pipe1[0]);
        close(pipe2[1]);

        exit(EXIT_SUCCESS);
    }

    //Generazione del secondo figlio (Processo C)
    pidC = fork();
    if (pidC < 0) {
        perror("Errore fork C");
        exit(EXIT_FAILURE);
    }

    if (pidC == 0) {
        // C legge solo da pipe2. Chiudiamo tutto il resto:
        close(pipe1[0]);
        close(pipe1[1]); // C non usa pipe1
        close(pipe2[1]); // Non scrive su pipe2

        int num;
        int somma = 0;
        int conteggio = 0;

        printf("Processo C:Inizio ricezione dei numeri elaborati:\n");

        // Legge da pipe2 finché B invia dati e mantiene aperta la scrittura
        while (read(pipe2[0], &num, sizeof(num)) > 0) {
            printf("Processo C Ricevuto: %d\n", num);
            somma += num;
            conteggio++;
        }

        close(pipe2[0]); // Chiude il canale di lettura

        // Calcola e stampa la media aritmetica
        if (conteggio > 0) {
            double media = (double)somma / conteggio;
            printf("Processo C:Totale numeri ricevuti: %d\n", conteggio);
            printf("Processo C:Media aritmetica: %.2f\n", media);
        } else {
            printf("Processo C:essun numero ricevuto (nessun valore era >= 50).\n");
        }

        exit(EXIT_SUCCESS);
    }


    // A scrive solo su pipe1. Chiude tutti i canali non usati:
    close(pipe1[0]); // Non legge da pipe1
    close(pipe2[0]);
    close(pipe2[1]); // A non usa pipe2

    // Inizializza il generatore di numeri casuali
    srand(time(NULL));

    printf("Processo A:Generazione e invio di 20 numeri casuali (1-100):\n");
    for (int i = 0; i < 20; i++) {
        int n = (rand() % 100) + 1;
        printf("Processo A:Invio numero %d: %d\n", i + 1, n);
        write(pipe1[1], &n, sizeof(n));
    }

    // Finito l'invio, chiude il lato di scrittura di pipe1 per inviare l'EOF a B
    close(pipe1[1]);

    // Il padre attende la terminazione di entrambi i figli
    wait(NULL);
    wait(NULL);

    printf("Processo A:Tutti i processi figli hanno terminato. Chiusura completata.\n");

    return 0;
}
