/*
 * ngtcp2
 *
 * Copyright (c) 2020 ngtcp2 contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "client_base.h"

#include <cassert>
#include <array>
#include <iostream>
#include <fstream>

#include "debug.h"
#include "template.h"
#include "util.h"

using namespace ngtcp2;

extern Config config;

static ngtcp2_conn *get_conn(ngtcp2_crypto_conn_ref *conn_ref) {
  auto c = static_cast<ClientBase *>(conn_ref->user_data);
  return c->conn();
}

ClientBase::ClientBase()
    : conn_ref_{get_conn, this}, qlog_(nullptr), conn_(nullptr) {
  ngtcp2_connection_close_error_default(&last_error_);
}

ClientBase::~ClientBase() {
  if (conn_) {
    ngtcp2_conn_del(conn_);
  }

  if (qlog_) {
    fclose(qlog_);
  }
}

int ClientBase::write_transport_params(const char *path,
                                       const ngtcp2_transport_params *params) {
  auto f = std::ofstream(path);
  if (!f) {
    return -1;
  }

  f << "initial_max_streams_bidi=" << params->initial_max_streams_bidi << '\n'
    << "initial_max_streams_uni=" << params->initial_max_streams_uni << '\n'
    << "initial_max_stream_data_bidi_local="
    << params->initial_max_stream_data_bidi_local << '\n'
    << "initial_max_stream_data_bidi_remote="
    << params->initial_max_stream_data_bidi_remote << '\n'
    << "initial_max_stream_data_uni=" << params->initial_max_stream_data_uni
    << '\n'
    << "initial_max_data=" << params->initial_max_data << '\n'
    << "active_connection_id_limit=" << params->active_connection_id_limit
    << '\n'
    << "max_datagram_frame_size=" << params->max_datagram_frame_size << '\n';

  f.close();
  if (!f) {
    return -1;
  }

  return 0;
}

int ClientBase::read_transport_params(const char *path,
                                      ngtcp2_transport_params *params) {
  auto f = std::ifstream(path);
  if (!f) {
    return -1;
  }

  for (std::string line; std::getline(f, line);) {
    if (util::istarts_with_l(line, "initial_max_streams_bidi=")) {
      if (auto n = util::parse_uint(line.c_str() +
                                    str_size("initial_max_streams_bidi="));
          !n) {
        return -1;
      } else {
        params->initial_max_streams_bidi = *n;
      }
      continue;
    }

    if (util::istarts_with_l(line, "initial_max_streams_uni=")) {
      if (auto n = util::parse_uint(line.c_str() +
                                    str_size("initial_max_streams_uni="));
          !n) {
        return -1;
      } else {
        params->initial_max_streams_uni = *n;
      }
      continue;
    }

    if (util::istarts_with_l(line, "initial_max_stream_data_bidi_local=")) {
      if (auto n = util::parse_uint(
              line.c_str() + str_size("initial_max_stream_data_bidi_local="));
          !n) {
        return -1;
      } else {
        params->initial_max_stream_data_bidi_local = *n;
      }
      continue;
    }

    if (util::istarts_with_l(line, "initial_max_stream_data_bidi_remote=")) {
      if (auto n = util::parse_uint(
              line.c_str() + str_size("initial_max_stream_data_bidi_remote="));
          !n) {
        return -1;
      } else {
        params->initial_max_stream_data_bidi_remote = *n;
      }
      continue;
    }

    if (util::istarts_with_l(line, "initial_max_stream_data_uni=")) {
      if (auto n = util::parse_uint(line.c_str() +
                                    str_size("initial_max_stream_data_uni="));
          !n) {
        return -1;
      } else {
        params->initial_max_stream_data_uni = *n;
      }
      continue;
    }

    if (util::istarts_with_l(line, "initial_max_data=")) {
      if (auto n =
              util::parse_uint(line.c_str() + str_size("initial_max_data="));
          !n) {
        return -1;
      } else {
        params->initial_max_data = *n;
      }
      continue;
    }

    if (util::istarts_with_l(line, "active_connection_id_limit=")) {
      if (auto n = util::parse_uint(line.c_str() +
                                    str_size("active_connection_id_limit="));
          !n) {
        return -1;
      } else {
        params->active_connection_id_limit = *n;
      }
      continue;
    }

    if (util::istarts_with_l(line, "max_datagram_frame_size=")) {
      if (auto n = util::parse_uint(line.c_str() +
                                    str_size("max_datagram_frame_size="));
          !n) {
        return -1;
      } else {
        params->max_datagram_frame_size = *n;
      }
      continue;
    }
  }

  return 0;
}

int ClientBase::on_rx_key(ngtcp2_crypto_level level, const uint8_t *secret,
                          size_t secretlen) {
  std::array<uint8_t, 64> key, iv, hp_key;

  if (ngtcp2_crypto_derive_and_install_rx_key(conn_, key.data(), iv.data(),
                                              hp_key.data(), level, secret,
                                              secretlen) != 0) {
    return -1;
  }

  auto crypto_ctx = level == NGTCP2_CRYPTO_LEVEL_EARLY
                        ? ngtcp2_conn_get_early_crypto_ctx(conn_)
                        : ngtcp2_conn_get_crypto_ctx(conn_);
  auto aead = &crypto_ctx->aead;
  auto keylen = ngtcp2_crypto_aead_keylen(aead);
  auto ivlen = ngtcp2_crypto_packet_protection_ivlen(aead);

  const char *title = nullptr;
  switch (level) {
  case NGTCP2_CRYPTO_LEVEL_EARLY:
    assert(0);
  case NGTCP2_CRYPTO_LEVEL_HANDSHAKE:
    title = "handshake_traffic";
    break;
  case NGTCP2_CRYPTO_LEVEL_APPLICATION:
    title = "application_traffic";
    break;
  default:
    assert(0);
  }

  if (!config.quiet && config.show_secret) {
    std::cerr << title << " rx secret" << std::endl;
    debug::print_secrets(secret, secretlen, key.data(), keylen, iv.data(),
                         ivlen, hp_key.data(), keylen);
  }

  return 0;
}

int ClientBase::on_tx_key(ngtcp2_crypto_level level, const uint8_t *secret,
                          size_t secretlen) {
  std::array<uint8_t, 64> key, iv, hp_key;

  if (ngtcp2_crypto_derive_and_install_tx_key(conn_, key.data(), iv.data(),
                                              hp_key.data(), level, secret,
                                              secretlen) != 0) {
    return -1;
  }

  auto crypto_ctx = level == NGTCP2_CRYPTO_LEVEL_EARLY
                        ? ngtcp2_conn_get_early_crypto_ctx(conn_)
                        : ngtcp2_conn_get_crypto_ctx(conn_);
  auto aead = &crypto_ctx->aead;
  auto keylen = ngtcp2_crypto_aead_keylen(aead);
  auto ivlen = ngtcp2_crypto_packet_protection_ivlen(aead);
  auto title = debug::secret_title(level);

  if (!config.quiet && config.show_secret) {
    std::cerr << title << " tx secret" << std::endl;
    debug::print_secrets(secret, secretlen, key.data(), keylen, iv.data(),
                         ivlen, hp_key.data(), keylen);
  }

  return 0;
}

ngtcp2_conn *ClientBase::conn() const { return conn_; }

void qlog_write_cb(void *user_data, uint32_t flags, const void *data,
                   size_t datalen) {
  auto c = static_cast<ClientBase *>(user_data);
  c->write_qlog(data, datalen);
}

void ClientBase::write_qlog(const void *data, size_t datalen) {
  assert(qlog_);
  fwrite(data, 1, datalen, qlog_);
}

ngtcp2_crypto_conn_ref *ClientBase::conn_ref() { return &conn_ref_; }
