#include "embedded.h"
#include "append.h"

#include <stdio.h>
#include <stdlib.h>

#define error(msg) {fprintf(stderr, "Failure: %s\n", msg); return -1;}

int main(int argc, char** argv) {
	char* err = 0;
	void* conn = 0;
	monetdb_result* result = 0;
	size_t r, c;

	// first argument is a string for the db directory or NULL for in-memory mode
	if (argc > 1) {

		err = monetdb_startup(argv[1], 0, 1);
		if (err != 0)
			error(err)

		conn = monetdb_connect();
		if (conn == NULL)
			error("Connection failed")

		printf("successfully connected\n");

		// if there is a second  argument, then it specifies the size of the data set to generate
		if (argc > 2) {
			err = monetdb_query(conn, "CREATE TABLE test (gridid integer, latlon bigint)", 1, NULL, NULL, NULL);
			if (err != 0)
				error(err)

			size_t n = atol(argv[2]);

			int* gridid = malloc(n * sizeof(int));
			float* latlon = malloc(2 * n * sizeof(float));

			for (size_t i = 0; i < n; i++) {
				gridid[i] = i % 100;

				latlon[i*2 + 1] = (float)rand()/(float)(RAND_MAX/360);
				latlon[i*2] = (float)rand()/(float)(RAND_MAX/180);
			}

	        tab_data *data = createData(n,2);
	        setColumnData(&(data->cols[0]),"gridid",monetdb_int32_t,n * sizeof(int32_t), (void *)gridid);
	        setColumnData(&(data->cols[1]),"latlon",monetdb_int64_t,n * sizeof(int64_t), (void *)latlon);

			if (append(conn, "test", data) != NULL) {
				error("append");
			}

			printf("inserted\n");

			// Testing: selecting all, and manually filtering the gridid's
			err = monetdb_query(conn, "SELECT gridid FROM test", 1, &result, NULL, NULL);
			if (err != 0)
				error(err)

			fprintf(stdout, "Query 0 result with %d cols and %d rows\n", (int) result->ncols, (int) result->nrows);
			monetdb_column_int32_t * id_col = (monetdb_column_int32_t *) monetdb_result_fetch(result, 0);
			size_t ct = 0;
			for(r = 0;r < result->nrows; r ++) {
				if(id_col->data[r] == 42 || id_col->data[r] == 84) {
					ct ++;
				}	
			}
			printf("Manual query using Query 0: %d\n\n", (int) ct);
			monetdb_cleanup_result(conn, result);

			// Testing: using where clause to filter the gridid's
			err = monetdb_query(conn, "SELECT gridid FROM test WHERE gridid in (42, 84)", 1, &result, NULL, NULL);
			if (err != 0)
				error(err)

			fprintf(stdout, "Query 1 with where clause: result with %d cols and %d rows\n", (int) result->ncols, (int) result->nrows);
			id_col = (monetdb_column_int32_t *) monetdb_result_fetch(result, 0);
			printf("%d\n\n", id_col->data[0]);
			monetdb_cleanup_result(conn, result);

			// Testing: getting the latlons using where clause to filter the gridid's
			err = monetdb_query(conn, "SELECT latlon FROM test WHERE gridid in (42, 84)", 1, &result, NULL, NULL);
			if (err != 0)
				error(err)

			fprintf(stdout, "Query 2 getting latlon with where clause: result with %d cols and %d rows\n", (int) result->ncols, (int) result->nrows);

			for (c = 0; c < result->ncols; c++) {
				monetdb_column* rcol = monetdb_result_fetch(result, c);
				switch (rcol->type) {
				case monetdb_float: {
					monetdb_column_float * col = (monetdb_column_float *) rcol;

					printf("%f\n", col->data[0]);
					break;
				}

				case monetdb_double: {
							monetdb_column_double * col = (monetdb_column_double *) rcol;

							printf("%f\n", col->data[0]);
							break;
						}

				case monetdb_int32_t: {
					monetdb_column_int32_t * col = (monetdb_column_int32_t *) rcol;

					printf("%d\n", col->data[0]);
					break;
				}

				// Assuming that the two floats are encoded as a single int64_t (bigint)
				case monetdb_int64_t: {
					monetdb_column_int64_t * col = (monetdb_column_int64_t *) rcol;
					float* fcol = (float*) col->data; 
					printf("%f, %f\n\n", fcol[0], fcol[1]);
					break;
				}

				default: {
					printf("UNKNOWN");
				}
				}

				if (c + 1 < result->ncols) {
					printf(", ");
				}
			}
			monetdb_cleanup_result(conn, result);
		}

		monetdb_disconnect(conn);
		monetdb_shutdown();
	}
	return 0;
}
