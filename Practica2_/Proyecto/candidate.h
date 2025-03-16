#ifndef CANDIDATE_H
#define CANDIDATE_H

#include <semaphore.h>

/**
 * @brief Waits for all votes to be written in the shared file.
 *        It polls the file every 1ms until (N_PROCS - 1) votes are found.
 * 
 * @param N_PROCS Number of voter processes in the system.
 */
void wait_for_votes(int N_PROCS);

/**
 * @brief The main candidate loop. 
 *        1) Waits for all voters to post a semaphore (sem2).
 *        2) Reads PIDs from LOG_FILE and sends SIGUSR2 to each voter.
 *        3) Calls wait_for_votes(N_PROCS) to collect votes.
 *        4) Removes FILE_CANDIDATO and VOTE_FILE, then waits 250ms.
 *        5) Posts sem4 to notify the parent that the voting cycle is done.
 *        6) Repeats in an infinite loop.
 * 
 * @param N_PROCS Number of voter processes in the system.
 */
void candidate(int N_PROCS);

#endif
