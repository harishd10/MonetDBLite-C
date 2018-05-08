#include "append.h"
#include "embedded.h"
#include "monetdb_config.h"
#include "gdk.h"

unsigned char monetdbBatTypesMap(monetdb_types typ) {
    switch(typ) {
    case monetdb_int8_t:
        return TYPE_bte;

    case monetdb_int16_t:
        return TYPE_sht;

    case monetdb_int32_t:
        return TYPE_int;

    case monetdb_int64_t:
        return TYPE_lng;

    case monetdb_float:
        return TYPE_flt;

    case monetdb_double:
        return TYPE_dbl;

    case monetdb_str:
        return TYPE_str;

    case monetdb_date:
    case monetdb_size_t:
    case monetdb_time:
    case monetdb_timestamp:
    case monetdb_blob:
        // I don't know what to do for these types
        // so not supporting them for now
        break;
    }
    return TYPE_any;
}

char* append(void* conn, char* table, tab_data* data) {
    char* err = 0;
    size_t c;
    append_data *ad = NULL;

    ad = malloc(data->ncols * sizeof(append_data));
    for (c = 0; c < data->ncols; c++) {
        col_data* rcol = &(data->cols[c]);
        unsigned char colType = monetdbBatTypesMap(rcol->type);
        if(colType == TYPE_any) {
            return GDKstrdup("column type not supported");
        }

        BAT* bcol = COLnew(0, colType, rcol->count, TRANSIENT);
        BATsettrivprop(bcol);

        if(colType == TYPE_str) {
            for (size_t j = 0; j < rcol->count; j++) {
                char* str_to_append = ((char **) rcol->data)[j];
                if (BUNappend(bcol, str_to_append, FALSE) != GDK_SUCCEED) {
                    return "string append error";
                }
            }
        } else {
            memcpy(bcol->theap.base, (char *)(rcol->data), rcol->size);
        }
        BATsetcount(bcol, rcol->count);

        bcol->tnil = 0;
        bcol->tnonil = 0;
        bcol->tkey = 0;
        bcol->tsorted = 0;
        bcol->trevsorted = 0;
        bcol->tdense = 0;
        BATassertProps(bcol);

        BBPkeepref(bcol->batCacheid);

        ad[c].colname = rcol->name;
        ad[c].batid = bcol->batCacheid;
    }
    err = monetdb_append(conn, "sys", table, ad, data->ncols);
    if (err != NULL) {
        return err;
    }
    cleanup(data);
    free(ad);
    return NULL;
}

tab_data* createData(size_t nrows, size_t ncols) {
    size_t c;
    tab_data* data = malloc(sizeof(tab_data));
    data->nrows = nrows;
    data->ncols = ncols;
    data->cols = malloc(ncols * sizeof(col_data));
    for(c = 0; c < ncols; c ++) {
        data->cols[c].count = nrows;
    }
    return data;
}

void setColumnData(col_data* col, char* col_name, monetdb_types type, size_t size, void* data) {
    col->name = col_name;
    col->type = type;
    col->data = data;
    col->size = size;
}

void cleanup(tab_data* data) {
    free(data->cols);
    free(data);
}
