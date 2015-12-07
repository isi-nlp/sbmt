#ifndef WN_DB_H
#define WN_DB_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <db.h>
#include "db_access.h"
#include "defs.h"

/* Database parameters: */

#define BUFFERSIZE 8192
char KEYBUFFER[BUFFERSIZE];
char DATABUFFER[BUFFERSIZE];

//! A function to hash rule strings (seems to work a bit better than the
//! default hash function in this case; since the increase of performance
//! is really minor, this function is currently not used). 
inline u_int32_t 
hash_string(DB* dbp, const void* s, u_int32_t len) {
  u_int32_t h = 0;
  for(int i=(int)(len/4)-1 ;i>=0 ; --i) {
	 h ^= *((int *)(s)+i);   // XOR h with next 4 bytes
	 h = (h<<7)|(h>>(32-7)); // apply a rotate left of 7 bits on h
  }
  return h;
} 

void
db_init(DB*& dbp, bool recno) {
   // Create a DB handle:
   int ret;
	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		fprintf(stderr, "db_create: %s\n", db_strerror(ret));
		exit(1);
	}
	if(recno)
	  // Allow duplicates:
	  if ((ret = dbp->set_flags(dbp, DB_RECNUM)) != 0) {
		  fprintf(stderr, "db_create: %s\n", db_strerror(ret));
		  exit(1);
	  }
	/* Turn on additional error output. */ 
	dbp->set_errfile(dbp, stderr); 
}

void
db_use_custom_hash(DB*& dbp) {
  int ret=0;
  if ((ret = dbp->set_h_hash(dbp, &hash_string)) != 0) {
    fprintf(stderr, "set_h_hash: %s\n", db_strerror(ret));
	 exit(1);
  }
}

void
set_db_cache_size(DB*& dbp, unsigned int cache_size) {
  if(!DB_DEFAULT_PARAMS) {
	 dbp->set_cachesize(dbp,0,cache_size,0);
  }
}

void
set_db_params(DB*& dbp, unsigned int page_size, unsigned int nb_min_keys, 
				  unsigned int nb_keys, float avg_key_size, float avg_data_size) {
  if(!DB_DEFAULT_PARAMS) {
    // BD_HASH optimization:
	 if(DB_ACCESS == DB_HASH) {
		int ffactor = ((int)((page_size-32)/(avg_key_size+avg_data_size+8)));
		dbp->set_h_ffactor(dbp,ffactor);
		dbp->set_h_nelem(dbp,nb_keys);
	 } else 
	 // DB_BTREE optimization:
	 if(DB_ACCESS == DB_BTREE) {
		dbp->set_bt_minkey(dbp,nb_min_keys);
	 }
	 dbp->set_pagesize(dbp,page_size);
  }
}

void 
db_open(DB*& dbp, const char *db_name, bool read_only) {
	
	int ret=0;
   int mode = (read_only) ? 
     ( DB_RDONLY | DB_DIRTY_READ ) : DB_CREATE; 	
	
	if ((ret = dbp->open(dbp,
#if	DB_VERSION_MAJOR	==  4 && DB_VERSION_MINOR > 1
		NULL,
#endif
		db_name, NULL, DB_ACCESS, mode, 0664)) != 0) {
		dbp->err(dbp, ret, "DB->open");
		exit(1);
	}
}

int
db_close(DB*& dbp) {
	return dbp->close(dbp, 0);
}

char*
db_lookup(DB*& dbp, char* key_str) {
   DBT key, data;
	int ret;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = key_str; 
	key.size = strlen(key_str)+1;

	if ((ret = dbp->get(dbp, NULL, &key, &data, 0)) == 0)
		return (char *)data.data;
	return NULL;
}

int* 
db_lookup_s2i(DB*& dbp, char* key_str) {
   DBT key, data;
	int ret;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = key_str; 
	key.size = strlen(key_str) + 1;

	if ((ret = dbp->get(dbp, NULL, &key, &data, 0)) == 0)
		return (int*)data.data;
	return NULL;
}

void
db_insert(DB*& dbp, char* key_str, char* value_str) {
   DBT key, data;
	int ret;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = key_str;
	key.size = strlen(key_str)+1;
	data.data = value_str;
	data.size = strlen(value_str)+1;

	if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) == 0)
	  return;
   dbp->err(dbp, ret, "DB->put");
}

void
db_insert(DB*& dbp, char* key_str, int value) {
   DBT key, data;
	int ret;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = key_str;
	key.size = strlen(key_str)+1;
	data.data = &value;
	data.size = sizeof(int);

	if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) == 0)
	  return;
   dbp->err(dbp, ret, "DB->put");
}
#endif
