#ifndef _APPEND_H_
#define _APPEND_H_

#include "embedded.h"

// Couldn't figure out how to use monetdb_result struct for insertion. 
// So for now creating my own structures.
typedef struct {
    char* name;
    monetdb_types type;
    size_t count;
    size_t size;
    void* data;
} col_data;

typedef struct {
    size_t ncols;
    size_t nrows;
    col_data* cols;
} tab_data;

embedded_export char* append(void* conn, char* table, tab_data* data);

// Helper functons
embedded_export tab_data* createData(size_t nrows, size_t ncols);
embedded_export void setColumnData(col_data* col, char* col_name, monetdb_types type, size_t size, void* data);
embedded_export void cleanup(tab_data* data);
embedded_export unsigned char monetdbBatTypesMap(monetdb_types typ);

#endif
