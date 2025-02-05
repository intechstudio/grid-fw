#ifndef PICO_POOL_H
#define PICO_POOL_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../grid_common/grid_protocol.h"

enum pico_bkt_state_t {
  PICO_BKT_STATE_EMPTY = 0,
  PICO_BKT_STATE_SPI_TX,
  PICO_BKT_STATE_UART_TX_NORTH,
  PICO_BKT_STATE_UART_TX_EAST,
  PICO_BKT_STATE_UART_TX_SOUTH,
  PICO_BKT_STATE_UART_TX_WEST,
  PICO_BKT_STATE_UART_RX_NORTH,
  PICO_BKT_STATE_UART_RX_EAST,
  PICO_BKT_STATE_UART_RX_SOUTH,
  PICO_BKT_STATE_UART_RX_WEST,
  PICO_BKT_STATE_UART_RX_FULL_NORTH,
  PICO_BKT_STATE_UART_RX_FULL_EAST,
  PICO_BKT_STATE_UART_RX_FULL_SOUTH,
  PICO_BKT_STATE_UART_RX_FULL_WEST,
  PICO_BKT_STATES,
};

typedef uint8_t pico_bkt_idx_t;

enum {
  PICO_BKT_SIZE = GRID_PARAMETER_SPI_TRANSACTION_length,
};

struct pico_bkt_t;

struct pico_bkt_t {

  // Current state
  enum pico_bkt_state_t state;

  // Previous bucket in cycle
  struct pico_bkt_t* prev;

  // Next bucket in cycle
  struct pico_bkt_t* next;

  // Read/write index
  uint32_t index;

  // Buffer contents
  uint8_t buf[PICO_BKT_SIZE];

  // Source port index (used in an external mechanism, could be removed)
  uint8_t port;
};

void pico_bkt_init(struct pico_bkt_t* bkt, pico_bkt_idx_t bucket, struct pico_bkt_t* prev, struct pico_bkt_t* next);
void pico_bkt_reset(struct pico_bkt_t* bkt);
uint8_t pico_bkt_next(struct pico_bkt_t* bkt);
void pico_bkt_push(struct pico_bkt_t* bkt, uint8_t rx);

enum {
  PICO_BKT_COUNT = 50,
};
enum {
  PICO_BKT_LIMIT = UINT8_MAX,
};
enum {
  PICO_BKT_IDX_NIL = PICO_BKT_LIMIT,
};

#define PICO_BKT_IDX_VALID(i) ((i) < PICO_BKT_IDX_NIL)

struct pico_pool_t {

  // First bucket in a cycle for each state
  struct pico_bkt_t* first[PICO_BKT_STATES];

  // Pool of buckets
  struct pico_bkt_t bkts[PICO_BKT_COUNT];
};

void pico_pool_init(struct pico_pool_t* pool);
struct pico_bkt_t* pico_pool_get_first(struct pico_pool_t* pool, enum pico_bkt_state_t state);
struct pico_bkt_t* pico_pool_change(struct pico_pool_t* pool, struct pico_bkt_t* bkt, enum pico_bkt_state_t state);

#endif /* PICO_POOL_H */
