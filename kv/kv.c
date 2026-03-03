#include "kv.h"
#include "kv_internal.h"
#include "str/str.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

int kv_init(allocator_t allocator, kv_i self) {
  const char *sql =
      "CREATE TABLE IF NOT EXISTS kv (key TEXT PRIMARY KEY, value TEXT)";
  char *err_msg = 0;
  self = allocator.malloc(sizeof *self);
  self->allocator = allocator;

  int rc = sqlite3_open("db.db", &self->db);
  if (rc != SQLITE_OK) {

    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(self->db));
    goto fail;
  }
  rc = sqlite3_exec(self->db, sql, NULL, NULL, &err_msg);

  if (rc != SQLITE_OK) {

    fprintf(stderr, "Failed to select data\n");
    fprintf(stderr, "SQL error: %s\n", err_msg);

    sqlite3_free(err_msg);
    goto fail;
  }

  return 0;

fail:
  kv_deinit(self);
  return -1;
}

void kv_deinit(kv_i self) {
  sqlite3_close(self->db);
  self->allocator.free(self);
}

int callback(void *result, int argc, char **argv, char **azColName) {
  (void)(azColName);
  (void)(argc);

  kv_result_t*res = result;

  str_append(res->allocator, res->result, str_cstring_to_slice(argv[0], strlen(argv[0])));

  return 0;
}

int kv_get(kv_i self, str_slice_t key, str_t *result) {
  int rc;
  char *err_msg;
  char *query_cstring;
  str_t query;
  kv_result_t container;
  str_init(self->allocator, &query,
           to_slice("SELECT value FROM kv WHERE key = \""));
  str_append(self->allocator, &query, key);
  str_append(self->allocator, &query, to_slice("\";"));

  query_cstring = self->allocator.malloc(query.slice.len + 1);

  str_to_cstring(&query, query_cstring);

  container.allocator=self->allocator;
  container.result = result;

  rc = sqlite3_exec(self->db, query_cstring, callback, &container, &err_msg);

  self->allocator.free(query_cstring);

  str_deinit(self->allocator, &query);
  return rc;
}

int kv_set(kv_i self, str_slice_t key, str_slice_t val) {
  int rc;
  char *err_msg;
  char *query_cstring;
  str_t query;
  str_init(self->allocator, &query,
           to_slice("INSERT INTO kv(key, value) VALUES (\""));
  str_append(self->allocator, &query, key);
  str_append(self->allocator, &query, to_slice("\", \""));
  str_append(self->allocator, &query, val);
  str_append(
      self->allocator, &query,
      to_slice("\") ON CONFLICT (key) DO UPDATE SET value=excluded.value;"));

  query_cstring = self->allocator.malloc(query.slice.len + 1);

  str_to_cstring(&query, query_cstring);

  rc = sqlite3_exec(self->db, query_cstring, callback, NULL, &err_msg);

  self->allocator.free(query_cstring);

  str_deinit(self->allocator, &query);
  return rc;
}
