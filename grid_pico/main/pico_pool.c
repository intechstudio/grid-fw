#include "pico_pool.h"

void pico_bkt_init(struct pico_bkt_t* bkt, pico_bkt_idx_t bucket, struct pico_bkt_t* prev, struct pico_bkt_t* next) {

  bkt->state = PICO_BKT_STATE_EMPTY;
  bkt->prev = prev;
  bkt->next = next;
  bkt->index = 0;
}

void pico_bkt_reset(struct pico_bkt_t* bkt) { bkt->index = 0; }

uint8_t pico_bkt_next(struct pico_bkt_t* bkt) {

  uint32_t index = bkt->index;
  bkt->index = (bkt->index + 1) % PICO_BKT_SIZE;
  return bkt->buf[index];
}

void pico_bkt_push(struct pico_bkt_t* bkt, uint8_t rx) {

  bkt->buf[bkt->index] = rx;
  bkt->index = (bkt->index + 1) % PICO_BKT_SIZE;
}

void pico_pool_init(struct pico_pool_t* pool) {

  if (PICO_BKT_COUNT > PICO_BKT_LIMIT) {
    return;
  }

  for (int i = 0; i < PICO_BKT_STATES; ++i) {
    pool->first[i] = NULL;
  }

  pool->first[PICO_BKT_STATE_EMPTY] = &pool->bkts[0];

  for (int i = 0; i < PICO_BKT_COUNT; ++i) {
    uint8_t prev = (i - 1 + PICO_BKT_COUNT) % PICO_BKT_COUNT;
    uint8_t next = (i + 1 + PICO_BKT_COUNT) % PICO_BKT_COUNT;
    pico_bkt_init(&pool->bkts[i], i, &pool->bkts[prev], &pool->bkts[next]);
  }
}

struct pico_bkt_t* pico_pool_get_first(struct pico_pool_t* pool, enum pico_bkt_state_t state) {

  int valid = state >= 0 && state < PICO_BKT_STATES;
  return valid ? pool->first[state] : NULL;
}

struct pico_bkt_t* pico_pool_change(struct pico_pool_t* pool, struct pico_bkt_t* bkt, enum pico_bkt_state_t state) {

  int valid = state >= 0 && state < PICO_BKT_STATES;
  if (!valid) {
    return NULL;
  }

  // The bucket must be the first in the cycle of its type
  if (bkt != pool->first[bkt->state]) {
    return NULL;
  }

  // If the bucket is part of a cycle longer than one
  if (!(bkt->next == bkt && bkt->prev == bkt)) {

    // The first bucket of the old list becomes the second
    pool->first[bkt->state] = bkt->next;

    // Detach bucket from the cycle of its current type
    bkt->prev->next = bkt->next;
    bkt->next->prev = bkt->prev;
  }
  // If the bucket under change is in a list of one
  else {

    // Reset first-of-type bucket for the current type
    pool->first[bkt->state] = NULL;
  }

  // If a cycle of the new type already exists
  if (pool->first[state]) {

    struct pico_bkt_t* fst = pool->first[state];

    // Link the bucket into an existing cycle, before the first
    bkt->prev = fst->prev;
    bkt->next = fst;
    fst->prev->next = bkt;
    fst->prev = bkt;
  }
  // If no cycle of the new type currently exists
  else {

    // Set the bucket links to loop back onto the bucket
    bkt->prev = bkt;
    bkt->next = bkt;

    // Set this bucket as the first-of-type bucket for its type
    pool->first[state] = bkt;
  }

  bkt->state = state;

  return bkt;
}
