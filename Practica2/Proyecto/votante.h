#ifndef VOTANTE_H
#define VOTANTE_H

/**
 * @brief Genera un voto aleatorio ('Y' o 'N') y lo añade a VOTE_FILE.
 *        Se utiliza un semáforo para proteger las operaciones de escritura en el archivo.
 */
void vote();

/**
 * @brief Bucle principal del proceso votante. Espera SIGUSR2, luego llama a vote().
 *        Utiliza sem2 para señalar la preparación al candidato.
 */
void votante();

#endif
