#ifndef VOTANTE_H
#define VOTANTE_H

/**
 * @brief Generates a random vote ('Y' or 'N') and appends it to VOTE_FILE.
 *        A semaphore is used to protect write operations on the file.
 */
void vote();

/**
 * @brief Main loop of the voter process. Waits for SIGUSR2, then calls vote().
 *        Uses sem2 to signal readiness to the candidate.
 */
void votante();

#endif
