#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>         
#include <sys/stat.h>        
#include <sys/mman.h>        
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MSG_SIZE 256

void proceso_usuario(int read_fd, int write_fd, const char* nombre, sem_t* sem_read, sem_t* sem_write, int proceso) {
    char mensaje[MSG_SIZE];
    char buffer[MSG_SIZE + 50]; // Buffer para incluir el nombre del usuario
    int it = 0;
    while (1) {
        sem_wait(sem_write);
        

	printf("------- PROCESANDO USUARIO: %i -------\n\n", proceso);
	if(it != 0 || proceso != 1) {
        	ssize_t elpep = read(read_fd, buffer, sizeof(buffer));
		buffer[elpep] = '\0';

        	printf("Mensaje recibido -> %s\n", buffer);

	}  

        // Leer mensaje del usuario
        printf("%s: ", nombre);
        fgets(mensaje, MSG_SIZE, stdin);

        // Agregar nombre al mensaje
        snprintf(buffer, sizeof(buffer), "%s: %s", nombre, mensaje);

        // Enviar mensaje
        write(write_fd, buffer, strlen(buffer) + 1);
	
	it++;

	printf("-----------FIN DE PROCESO DE USUARIO: %i----------\n\n", proceso);
        sem_post(sem_read);
    }
}

int main() {
    int pipe1[2], pipe2[2];
    pid_t pid1, pid2;

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        exit(1);
    }

    const char *sem_name1 = "/elpepe_sem1";
    const char *sem_name2 = "/elpepe_sem2";
    sem_t *sem1, *sem2;

    int shm_fd1 = shm_open("/elpepe_shm1", O_CREAT | O_RDWR, 0666);
    int shm_fd2 = shm_open("/elpepe_shm2", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd1, sizeof(sem_t));
    ftruncate(shm_fd2, sizeof(sem_t));
    sem1 = (sem_t *)mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd1, 0);
    sem2 = (sem_t *)mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd2, 0);

    sem_init(sem1, 1, 0); // Semáforo para proceso 1
    sem_init(sem2, 1, 1); // Semáforo para proceso 2

    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(1);
    }

    if (pid1 == 0) {
        close(pipe1[1]);
        close(pipe2[0]);
        proceso_usuario(pipe1[0], pipe2[1], "Usuario1", sem1, sem2, 1);
        exit(0);
    }

    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(1);
    }

    if (pid2 == 0) {
        close(pipe1[0]);
        close(pipe2[1]);
        proceso_usuario(pipe2[0], pipe1[1], "Usuario2", sem2, sem1, 2);
        exit(0);
    }

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    wait(NULL);
    wait(NULL);

    sem_destroy(sem1);
    sem_destroy(sem2);
    munmap(sem1, sizeof(sem_t));
    munmap(sem2, sizeof(sem_t));
    shm_unlink("/elpepe_shm1");
    shm_unlink("/elpepe_shm2");
    sem_unlink(sem_name1);
    sem_unlink(sem_name2);

    return 0;
}

