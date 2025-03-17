#ifndef ECANDIDATOS_H
#define ECANDIDATOS_H

/**
 * @brief Main function that waits for SIGUSR1 to start the candidate selection.
 *        If FILE_CANDIDATO has -1, the current process becomes candidate.
 *        Otherwise, it becomes a voter process.
 * 
 * @param N_PROCS Number of processes in the system.
 */
void elige_candidato(int N_PROCS);

#endif
