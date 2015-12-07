#ifndef WN_DB_H
#define WN_DB_H

#include <cstdio>
#include <cstring>
#include <db.h>

//! Custom hash function.
inline u_int32_t 
hash_string(DB* dbp, const void* s, u_int32_t len);

//! Init access to the Berkeley database. The handle dbp can then
//! be used for open, close, and lookup operations.
void db_init(DB*& dbp, bool allow_dup=false);

//! Use a slightly more efficient hash function.
//! Beware: once a DB is created, you can't change your choice
//! of hash function.
void db_use_custom_hash(DB*& dbp);

//! Set cache size of DB.
void
set_db_cache_size(DB*& dbp, unsigned int cache_size);

//! Compute 'optimal' DB params from page size, expected number of DB
//! entries, and expected average key and data size.
void
set_db_params(DB*& dbp, unsigned int page_size, 
              unsigned int nb_min_keys, unsigned int nb_keys, 
				  float avg_key_size, float avg_data_size);

//! Open/create Berkeley database. File name must be 
//! specified in db_name, and mode (read_only or read/write)
//! with the flag read_only.
void db_open(DB*& dbp, const char *db_name, bool read_only);

//! Close Berkeley database file.
int db_close(DB*& dbp);

//! Lookup the entry pointed to by key_str.
char* db_lookup(DB*& dbp, char* key_str);
int* db_lookup_s2i(DB*& dbp, char* key_str);

//! Insert (key_str,value_str) pair into a database opened
//! in read/write mode.
void db_insert(DB*& dbp, char* key_str, char* value_str);
void db_insert(DB*& dbp, char* key_str, int value);

#endif
