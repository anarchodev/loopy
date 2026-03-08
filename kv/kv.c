#include "kv.h"
#include "kv_internal.h"
#include "str/str.h"
#include <sqlite3.h>
#include <stdio.h>

kv_i kv_new(allocator_t allocator) {
  kv_i self;
  const char *sql =
      "CREATE TABLE IF NOT EXISTS kv (key TEXT PRIMARY KEY, value TEXT)";
  char *err_msg = 0;
  self = allocator.malloc(sizeof *self);
  self->allocator = allocator;
  self->get_stmt = NULL;
  self->set_stmt = NULL;
  self->remove_stmt = NULL;

  int rc = sqlite3_open("db.db", &self->db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(self->db));
    goto fail;
  }

  rc = sqlite3_exec(self->db, sql, NULL, NULL, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to create table\n");
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    goto fail;
  }

  sqlite3_prepare_v2(self->db,
      "SELECT value FROM kv WHERE key = ?", -1, &self->get_stmt, NULL);
  sqlite3_prepare_v2(self->db,
      "INSERT INTO kv(key, value) VALUES (?, ?) "
      "ON CONFLICT (key) DO UPDATE SET value=excluded.value", -1, &self->set_stmt, NULL);
  sqlite3_prepare_v2(self->db,
      "DELETE FROM kv WHERE key = ?", -1, &self->remove_stmt, NULL);

  return self;

fail:
  kv_delete(self);
  return NULL;
}

void kv_delete(kv_i self) {
  sqlite3_finalize(self->get_stmt);
  sqlite3_finalize(self->set_stmt);
  sqlite3_finalize(self->remove_stmt);
  sqlite3_close(self->db);
  self->allocator.free(self);
}

int kv_get(kv_i self, str_slice_t key, str_t *result) {
  int rc;
  sqlite3_bind_text(self->get_stmt, 1, key.ptr, (int)key.len, SQLITE_STATIC);
  rc = sqlite3_step(self->get_stmt);
  if (rc == SQLITE_ROW) {
    const char *val = (const char *)sqlite3_column_text(self->get_stmt, 0);
    int len = sqlite3_column_bytes(self->get_stmt, 0);
    str_append(self->allocator, result, str_cstring_to_slice(val, len));
    rc = SQLITE_OK;
  } else if (rc == SQLITE_DONE) {
    rc = SQLITE_OK;
  }
  sqlite3_reset(self->get_stmt);
  sqlite3_clear_bindings(self->get_stmt);
  return rc;
}

int kv_set(kv_i self, str_slice_t key, str_slice_t val) {
  int rc;
  sqlite3_bind_text(self->set_stmt, 1, key.ptr, (int)key.len, SQLITE_STATIC);
  sqlite3_bind_text(self->set_stmt, 2, val.ptr, (int)val.len, SQLITE_STATIC);
  rc = sqlite3_step(self->set_stmt);
  if (rc == SQLITE_DONE) rc = SQLITE_OK;
  sqlite3_reset(self->set_stmt);
  sqlite3_clear_bindings(self->set_stmt);
  return rc;
}

int kv_remove(kv_i self, str_slice_t key) {
  int rc;
  sqlite3_bind_text(self->remove_stmt, 1, key.ptr, (int)key.len, SQLITE_STATIC);
  rc = sqlite3_step(self->remove_stmt);
  if (rc == SQLITE_DONE) rc = SQLITE_OK;
  sqlite3_reset(self->remove_stmt);
  sqlite3_clear_bindings(self->remove_stmt);
  return rc;
}
