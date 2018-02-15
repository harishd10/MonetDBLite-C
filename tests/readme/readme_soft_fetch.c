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
	err = monetdb_startup("C:/Users/harishd/Desktop/Projects/Urbane/code/data/nyc/db", 0, 1);
	if (err != 0)
		error(err)

	conn = monetdb_connect();
	if (conn == NULL)
		error("Connection failed")

	printf("successfully connected\n");


	err = monetdb_query(conn, "SELECT pickup_loc, fare FROM \"Taxi\" WHERE pickup_time BETWEEN 1231768000 AND 1372668000", 1, &result, NULL, NULL);
	if (err != 0)
		error(err)

	printf("result has %d rows\n", result->nrows);
	monetdb_column col0, col1;
	monetdb_result_fetch_soft(result,0, &col0);
	monetdb_result_fetch_soft(result,1, &col1);
	printf("Column names: %s, %s \n", col0.name, col1.name);
	printf("result has %d, %d rows\n", col0.count, col1.count);
	if(col0.data != NULL) {
		float *fdata = (float *)col0.data;
		printf("First point (soft): (%f, %f) \n", fdata[0], fdata[1]);
	}
	if(col1.data != NULL) {
		int *fdata = (int *)col1.data;
		printf("First fare (soft): %d \n", fdata[0]);
	}


	monetdb_column *hcol0 = monetdb_result_fetch(result,0);
	monetdb_column *hcol1 = monetdb_result_fetch(result,1);
	printf("*************** Hard fetch ********************\n");
	char *name0 = monetdb_column_name(result,0);
	char *name1 = monetdb_column_name(result,1);
	printf("Column names: %s, %s \n", name0, name1);
	printf("result has %d, %d rows\n", hcol0->count, hcol1->count);
	{
		float *fdata = (float *)hcol0->data;
		printf("First point (hard): (%f, %f) \n", fdata[0], fdata[1]);
	}
	{
		int *fdata = (int *)hcol1->data;
		printf("First fare (hard): %d \n", fdata[0]);
	}


	monetdb_cleanup_result(conn, result);

	monetdb_disconnect(conn);
	monetdb_shutdown();
	return 0;
}
