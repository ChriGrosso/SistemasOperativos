#ifndef CANDIDATE_H
#define CANDIDATE_H

#include <semaphore.h>

/**
 * @brief Espera a que todos los votos se escriban en el archivo compartido.
 *        Consulta el archivo cada 1ms hasta que se encuentren (N_PROCS - 1) votos.
 * 
 * @param N_PROCS Número de procesos votantes en el sistema.
 */
void wait_for_votes(int N_PROCS);

/**
 * @brief Bucle principal del candidato.
 *        1) Espera a que todos los votantes publiquen un semáforo (sem2).
 *        2) Lee los PIDs de LOG_FILE y envía SIGUSR2 a cada votante.
 *        3) Llama a wait_for_votes(N_PROCS) para recolectar los votos.
 *        4) Elimina FILE_CANDIDATO y VOTE_FILE, luego espera 250ms.
 *        5) Publica sem4 para notificar al padre que el ciclo de votación ha terminado.
 *        6) Repite en un bucle infinito.
 * 
 * @param N_PROCS Número de procesos votantes en el sistema.
 */
void candidate(int N_PROCS);

#endif
