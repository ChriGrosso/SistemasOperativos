#ifndef ECANDIDATOS_H
#define ECANDIDATOS_H

/**
 * @brief Función principal que espera SIGUSR1 para iniciar la selección de candidatos.
 *        Si FILE_CANDIDATO tiene -1, el proceso actual se convierte en candidato.
 *        De lo contrario, se convierte en un proceso votante.
 * 
 * @param N_PROCS Número de procesos en el sistema.
 */
void elige_candidato(int N_PROCS);

#endif
