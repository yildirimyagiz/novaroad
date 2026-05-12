/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_attest.c — Proof Attestation & Cryptographic Sealing
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Proof receipt'lerin tahrif-edilemez sertifikaları.
 * Her kanıtlanmış kernel execution'ı kriptografik bir
 * attestation record ile mühürlenir.
 */

#include "nova_errors.h"
#include "nova_obligation.h"
#include "nova_policy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * ATTESTATION RECORD
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  uint64_t record_id;
  const char *kernel_name;
  uint64_t obligation_set_hash;
  uint64_t verdict_hash;
  int64_t timestamp_ns;
  uint64_t chain_hash; /* Hash of this + previous record */
  const char *policy_mode;
  size_t proved_count;
  size_t total_count;
} AttestationRecord;

typedef struct {
  AttestationRecord *records;
  size_t count;
  size_t capacity;
  uint64_t chain_head; /* Current chain hash */
  uint64_t next_id;
} AttestationChain;

/* ═══════════════════════════════════════════════════════════════════════════
 * FNV-1a HASH
 * ═══════════════════════════════════════════════════════════════════════════
 */

static uint64_t fnv1a_mix(uint64_t h, uint64_t val) {
  h ^= val;
  h *= 0x100000001b3ULL;
  return h;
}

static uint64_t fnv1a_str(uint64_t h, const char *s) {
  if (!s)
    return h;
  while (*s) {
    h ^= (uint64_t)*s++;
    h *= 0x100000001b3ULL;
  }
  return h;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CHAIN
 * ═══════════════════════════════════════════════════════════════════════════
 */

static AttestationChain g_attest_chain = {0};

void nova_attest_init(void) {
  if (g_attest_chain.records)
    return; /* Already initialized */

  g_attest_chain.capacity = 256;
  g_attest_chain.records =
      calloc(g_attest_chain.capacity, sizeof(AttestationRecord));
  g_attest_chain.count = 0;
  g_attest_chain.chain_head = 0xcbf29ce484222325ULL;
  g_attest_chain.next_id = 1;
}

void nova_attest_shutdown(void) {
  free(g_attest_chain.records);
  memset(&g_attest_chain, 0, sizeof(g_attest_chain));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * RECORD AN ATTESTATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

int nova_attest_record(const ObligationSet *obligations,
                         const VerdictReport *verdict,
                         const char *policy_mode) {
  if (!obligations || !verdict)
    return -1;

  nova_attest_init();
  AttestationChain *c = &g_attest_chain;

  /* Grow if needed */
  if (c->count >= c->capacity) {
    size_t new_cap = c->capacity * 2;
    AttestationRecord *new_rec =
        realloc(c->records, new_cap * sizeof(AttestationRecord));
    if (!new_rec)
      return -1;
    c->records = new_rec;
    c->capacity = new_cap;
  }

  AttestationRecord *rec = &c->records[c->count];
  memset(rec, 0, sizeof(AttestationRecord));

  rec->record_id = c->next_id++;
  rec->kernel_name = obligations->kernel_name;
  rec->obligation_set_hash = obligations->set_hash;
  rec->verdict_hash = verdict->verdict_hash;
  rec->policy_mode = policy_mode;
  rec->proved_count = verdict->proved_obligations;
  rec->total_count = verdict->total_obligations;

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  rec->timestamp_ns = ts.tv_sec * 1000000000LL + ts.tv_nsec;

  /* Chain hash: H(prev_chain, record_data) */
  uint64_t h = c->chain_head;
  h = fnv1a_mix(h, rec->record_id);
  h = fnv1a_str(h, rec->kernel_name);
  h = fnv1a_mix(h, rec->obligation_set_hash);
  h = fnv1a_mix(h, rec->verdict_hash);
  h = fnv1a_mix(h, (uint64_t)rec->timestamp_ns);
  rec->chain_hash = h;

  c->chain_head = h;
  c->count++;

  return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CHAIN INTEGRITY VERIFICATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

bool nova_attest_verify_chain(void) {
  AttestationChain *c = &g_attest_chain;
  if (!c->records || c->count == 0)
    return true;

  uint64_t running_hash = 0xcbf29ce484222325ULL;

  for (size_t i = 0; i < c->count; i++) {
    AttestationRecord *rec = &c->records[i];

    uint64_t h = running_hash;
    h = fnv1a_mix(h, rec->record_id);
    h = fnv1a_str(h, rec->kernel_name);
    h = fnv1a_mix(h, rec->obligation_set_hash);
    h = fnv1a_mix(h, rec->verdict_hash);
    h = fnv1a_mix(h, (uint64_t)rec->timestamp_ns);

    if (h != rec->chain_hash) {
      fprintf(stderr,
              "❌ [Attest] Chain integrity violation at record %zu!\n"
              "   Expected: 0x%016llx\n"
              "   Got:      0x%016llx\n",
              i, (unsigned long long)rec->chain_hash, (unsigned long long)h);
      return false;
    }

    running_hash = h;
  }

  fprintf(
      stderr,
      "✅ [Attest] Chain integrity verified (%zu records, head=0x%016llx)\n",
      c->count, (unsigned long long)running_hash);
  return true;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * EXPORT CHAIN
 * ═══════════════════════════════════════════════════════════════════════════
 */

int nova_attest_export_json(const char *path) {
  AttestationChain *c = &g_attest_chain;
  if (!path || !c->records)
    return -1;

  FILE *f = fopen(path, "w");
  if (!f)
    return -1;

  fprintf(f, "{\n  \"chain_head\": \"0x%016llx\",\n",
          (unsigned long long)c->chain_head);
  fprintf(f, "  \"record_count\": %zu,\n", c->count);
  fprintf(f, "  \"records\": [\n");

  for (size_t i = 0; i < c->count; i++) {
    AttestationRecord *r = &c->records[i];
    fprintf(
        f,
        "    {\n"
        "      \"id\": %llu,\n"
        "      \"kernel\": \"%s\",\n"
        "      \"obligation_hash\": \"0x%016llx\",\n"
        "      \"verdict_hash\": \"0x%016llx\",\n"
        "      \"chain_hash\": \"0x%016llx\",\n"
        "      \"policy\": \"%s\",\n"
        "      \"proved\": %zu,\n"
        "      \"total\": %zu,\n"
        "      \"timestamp_ns\": %lld\n"
        "    }%s\n",
        (unsigned long long)r->record_id, r->kernel_name ? r->kernel_name : "",
        (unsigned long long)r->obligation_set_hash,
        (unsigned long long)r->verdict_hash, (unsigned long long)r->chain_hash,
        r->policy_mode ? r->policy_mode : "", r->proved_count, r->total_count,
        (long long)r->timestamp_ns, (i < c->count - 1) ? "," : "");
  }

  fprintf(f, "  ]\n}\n");
  fclose(f);
  return 0;
}

void nova_attest_print_chain(void) {
  AttestationChain *c = &g_attest_chain;

  fprintf(stderr,
          "\n═══════════════════════════════════════════════════════\n"
          "  ATTESTATION CHAIN (%zu records)\n"
          "  Head: 0x%016llx\n"
          "═══════════════════════════════════════════════════════\n",
          c->count, (unsigned long long)c->chain_head);

  for (size_t i = 0; i < c->count; i++) {
    AttestationRecord *r = &c->records[i];
    fprintf(stderr, "  [%llu] %s — proved %zu/%zu — chain 0x%08llx\n",
            (unsigned long long)r->record_id,
            r->kernel_name ? r->kernel_name : "?", r->proved_count,
            r->total_count, (unsigned long long)(r->chain_hash & 0xFFFFFFFF));
  }

  fprintf(stderr,
          "═══════════════════════════════════════════════════════\n\n");
}
