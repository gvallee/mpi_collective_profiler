/*************************************************************************
 * Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
 *
 * See LICENSE.txt for license information
 ************************************************************************/


#ifndef MPI_COLLECTIVE_PROFILER_BUFFCONTENT_H
#define MPI_COLLECTIVE_PROFILER_BUFFCONTENT_H

#include <inttypes.h>
#include <stdbool.h>

#include "mpi.h"

#define COLLECTIVE_PROFILER_MAX_CALL_CHECK_BUFF_CONTENT_ENVVAR "COLLECTIVE_PROFILER_MAX_CALL_CHECK_BUFF_CONTENT"
#define COLLECTIVE_PROFILER_CHECK_SEND_BUFF_ENVVAR "COLLECTIVE_PROFILER_CHECK_SEND_BUFF"

// buffcontent_logger is the central structure to track and profile backtrace in
// the context of MPI collective. We track in a unique manner each trace but for each
// trace, multiple contexts can be tracked. A context is the tuple communictor id/rank/call.
typedef struct buffcontent_logger
{
    char *collective_name;
    uint64_t id;
    int world_rank;
    FILE *fd;
    char *filename;
    uint64_t comm_id;
    MPI_Comm comm;
    struct buffcontent_logger *next;
    struct buffcontent_logger *prev;
} buffcontent_logger_t;

static inline void _display_config(int dt_num_intergers, int dt_num_addresses, int dt_num_datatypes, int dt_combiner) {
    fprintf(stderr, "-> Num datatypes: %d\n", dt_num_datatypes);
    switch (dt_combiner) {
        case MPI_COMBINER_NAMED: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_NAMED\n");
            break;
        }
        case MPI_COMBINER_DUP: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_DUP\n");
            break;
        }
        case MPI_COMBINER_CONTIGUOUS: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_CONTIGUOUS\n");
            break;
        }
        case MPI_COMBINER_VECTOR: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_VECTOR\n");
            break;
        }
        case MPI_COMBINER_HVECTOR: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_HVECTOR\n");
            break;
        }
        case MPI_COMBINER_INDEXED: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_INDEXED\n");
            break;
        }
        case MPI_COMBINER_HINDEXED: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_HINDEXED\n");
            break;
        }
        case MPI_COMBINER_INDEXED_BLOCK: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_INDEXED_BLOCK\n");
            break;
        }
        case MPI_COMBINER_STRUCT: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_STRUCT\n");
            break;
        }
        case MPI_COMBINER_SUBARRAY: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_SUBARRAY\n");
            break;
        }
        case MPI_COMBINER_DARRAY: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_DARRAY\n");
            break;
        }
        case MPI_COMBINER_F90_REAL: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_F90_REAL\n");
            break;
        }
        case MPI_COMBINER_F90_COMPLEX: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_F90_COMPLEX\n");
            break;
        }
        case MPI_COMBINER_F90_INTEGER: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_F90_INTEGER\n");
            break;
        }
        case MPI_COMBINER_RESIZED: {
            fprintf(stderr, "-> Combiner: MPI_COMBINER_RESIZED\n");
            break;
        }
        default:


        
            fprintf(stderr, "-> Combiner: unknown\n");
    }
}

#define DT_CHECK(dt) do {                                                                                          \
    int dt_num_intergers;                                                                                          \
    int dt_num_addresses;                                                                                          \
    int dt_num_datatypes;                                                                                          \
    int dt_combiner;                                                                                               \
    PMPI_Type_get_envelope(dt, &dt_num_intergers, &dt_num_addresses, &dt_num_datatypes, &dt_combiner);             \
    if (dt_num_datatypes > 1 || !(dt_combiner == MPI_COMBINER_CONTIGUOUS || dt_combiner == MPI_COMBINER_NAMED)) {  \
        fprintf(stderr, "Unsupported datatype configuration\n");                                                   \
        _display_config(dt_num_intergers, dt_num_addresses, dt_num_datatypes, dt_combiner);                        \
        MPI_Abort(MPI_COMM_WORLD, 1);                                                                              \
    }                                                                                                              \
} while(0)

#define GET_BUFFCONTENT_LOGGER(_collective_name, _comm, _world_rank, _comm_rank, _buffcontent_logger) do {        \
    int _rc;                                                                                                      \
    uint32_t _comm_id;                                                                                            \
    _rc = lookup_comm(_comm, &_comm_id);                                                                          \
    if (_rc)                                                                                                      \
    {                                                                                                             \
        _rc = add_comm(_comm, _world_rank, _comm_rank, &_comm_id);                                                \
        if (_rc)                                                                                                  \
        {                                                                                                         \
            fprintf(stderr, "add_comm() failed: %d\n", _rc);                                                      \
            return 1;                                                                                             \
        }                                                                                                         \
    }                                                                                                             \
    _rc = lookup_buffcontent_logger(_collective_name, _comm, &_buffcontent_logger);                               \
    if (_rc)                                                                                                      \
    {                                                                                                             \
        fprintf(stderr, "lookup_buffcontent_logger() failed: %d\n", _rc);                                         \
        return 1;                                                                                                 \
    }                                                                                                             \
    if (_buffcontent_logger == NULL)                                                                              \
    {                                                                                                             \
        _rc = init_buffcontent_logger(_collective_name, _world_rank, _comm, _comm_id, "w", &_buffcontent_logger); \
        if (_rc)                                                                                                  \
        {                                                                                                         \
            fprintf(stderr, "init_buffcontent_logger() failed: %d\n", _rc);                                       \
            return 1;                                                                                             \
        }                                                                                                         \
    }                                                                                                             \
    assert(_buffcontent_logger);                                                                                  \
    assert(_buffcontent_logger->fd);                                                                              \
} while(0)

int store_call_data(char *collective_name, MPI_Comm comm, int comm_rank, int world_rank, uint64_t n_call, void* buf, int counts[], int displs[], MPI_Datatype dt);
int read_and_compare_call_data(char *collective_name, MPI_Comm comm, int comm_rank, int world_rank, uint64_t n_call, void *buf, int counts[], int displs[], MPI_Datatype dt, bool check);
int release_buffcontent_loggers();

#endif // MPI_COLLECTIVE_PROFILER_BUFFCONTENT_H
