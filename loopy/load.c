#include "internal.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "kv/kv.h"
#include "str/str.h"

static int ends_with_mjs(const char *name) {
  size_t len = strlen(name);
  return len > 4 && strcmp(name + len - 4, ".mjs") == 0;
}

static void load_dir(allocator_t allocator, kv_i kv,
                     const char *base, const char *rel);

static void load_file(allocator_t allocator, kv_i kv,
                      const char *full_path, const char *rel_key) {
  char key_raw[4096] = {0};
  char bc_key_raw[4096] = {0};
  str_fixed_t key, bc_key;
  FILE *f;
  long size;
  char *buf;
  size_t nread;

  key = str_fixed_init(str_cstring_to_slice(key_raw, 0), sizeof key_raw - 1);
  str_fixed_append(&key, to_slice("_js.f."));
  str_fixed_append(&key, str_cstring_to_slice(rel_key, strlen(rel_key)));

  f = fopen(full_path, "rb");
  if (!f) return;
  if (fseek(f, 0, SEEK_END) < 0) { fclose(f); return; }
  size = ftell(f);
  if (size < 0) { fclose(f); return; }
  rewind(f);

  buf = allocator.malloc((size_t)size);
  if (!buf) { fclose(f); return; }

  nread = fread(buf, 1, (size_t)size, f);
  fclose(f);

  if ((long)nread != size) { allocator.free(buf); return; }

  kv_set(kv, str_fixed_get_slice(&key),
             str_cstring_to_slice(buf, (unsigned long)size));
  allocator.free(buf);

  bc_key = str_fixed_init(str_cstring_to_slice(bc_key_raw, 0), sizeof bc_key_raw - 1);
  str_fixed_append(&bc_key, to_slice("_js.b."));
  str_fixed_append(&bc_key, str_cstring_to_slice(rel_key, strlen(rel_key)));
  kv_remove(kv, str_fixed_get_slice(&bc_key));
}

static void load_dir(allocator_t allocator, kv_i kv,
                     const char *base, const char *rel) {
  char full_raw[4096] = {0};
  char child_full_raw[4096] = {0};
  char child_rel_raw[4096] = {0};
  str_fixed_t full_path, child_full, child_rel;
  size_t rel_len = strlen(rel);
  DIR *dir;
  struct dirent *entry;

  full_path = str_fixed_init(str_cstring_to_slice(full_raw, 0), sizeof full_raw - 1);
  str_fixed_append(&full_path, str_cstring_to_slice(base, strlen(base)));
  if (rel_len > 0) {
    str_fixed_append(&full_path, to_slice("/"));
    str_fixed_append(&full_path, str_cstring_to_slice(rel, rel_len));
  }
  full_raw[str_fixed_len(&full_path)] = '\0';

  dir = opendir(full_raw);
  if (!dir) return;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.') continue;

    child_full = str_fixed_init(str_cstring_to_slice(child_full_raw, 0), sizeof child_full_raw - 1);
    str_fixed_append(&child_full, str_cstring_to_slice(base, strlen(base)));
    str_fixed_append(&child_full, to_slice("/"));
    if (rel_len > 0) {
      str_fixed_append(&child_full, str_cstring_to_slice(rel, rel_len));
      str_fixed_append(&child_full, to_slice("/"));
    }
    str_fixed_append(&child_full, str_cstring_to_slice(entry->d_name, strlen(entry->d_name)));
    child_full_raw[str_fixed_len(&child_full)] = '\0';

    child_rel = str_fixed_init(str_cstring_to_slice(child_rel_raw, 0), sizeof child_rel_raw - 1);
    if (rel_len > 0) {
      str_fixed_append(&child_rel, str_cstring_to_slice(rel, rel_len));
      str_fixed_append(&child_rel, to_slice("/"));
    }
    str_fixed_append(&child_rel, str_cstring_to_slice(entry->d_name, strlen(entry->d_name)));
    child_rel_raw[str_fixed_len(&child_rel)] = '\0';

    {
      struct stat st;
      if (stat(child_full_raw, &st) == 0 && S_ISDIR(st.st_mode))
        load_dir(allocator, kv, base, child_rel_raw);
      else if (ends_with_mjs(entry->d_name))
        load_file(allocator, kv, child_full_raw, child_rel_raw);
    }
  }

  closedir(dir);
}

int loopy_load_mjs(allocator_t allocator, kv_i kv, const char *dir_path) {
  load_dir(allocator, kv, dir_path, "");
  return 0;
}
