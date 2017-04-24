// Copyright (c) 2017, Tencent Inc.
// All rights reserved.
//
// Author: rabbitliu <rabbitliu@tencent.com>
// Created: 04/17/17
// Description:

#ifndef SDK4_2_COS_CPP_SDK_INCLUDE_UTIL_THREADSAFE_OPENSSL_H
#define SDK4_2_COS_CPP_SDK_INCLUDE_UTIL_THREADSAFE_OPENSSL_H
#pragma once

// copy from https://curl.haxx.se/libcurl/c/opensslthreadlock.html

#include <openssl/crypto.h> //In addition to other ssl headers
#include <openssl/err.h>
#include <pthread.h>
#include <stdio.h>

#define MUTEX_TYPE       pthread_mutex_t
#define MUTEX_SETUP(x)   pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)    pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)  pthread_mutex_unlock(&(x))
#define THREAD_ID        pthread_self()

void handle_error(const char *file, int lineno, const char *msg)
{
  fprintf(stderr, "** %s:%d %s\n", file, lineno, msg);
  ERR_print_errors_fp(stderr);
  /* exit(-1); */
}

/* This array will store all of the mutexes available to OpenSSL. */
static MUTEX_TYPE *mutex_buf= NULL;

static void locking_function(int mode, int n, const char *file, int line)
{
  if(mode & CRYPTO_LOCK)
    MUTEX_LOCK(mutex_buf[n]);
  else
    MUTEX_UNLOCK(mutex_buf[n]);
}

static unsigned long id_function(void)
{
  return ((unsigned long)THREAD_ID);
}

int openssl_thread_setup(void)
{
  int i;

  mutex_buf = (MUTEX_TYPE*)malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
  if(!mutex_buf) {
    return 0;
  }
  for(i = 0;  i < CRYPTO_num_locks();  i++) {
    MUTEX_SETUP(mutex_buf[i]);
  }
  CRYPTO_set_id_callback(id_function);
  CRYPTO_set_locking_callback(locking_function);
  return 1;
}

int openssl_thread_cleanup(void)
{
  int i;

  if(!mutex_buf) {
    return 0;
  }
  CRYPTO_set_id_callback(NULL);
  CRYPTO_set_locking_callback(NULL);
  for(i = 0;  i < CRYPTO_num_locks();  i++) {
    MUTEX_CLEANUP(mutex_buf[i]);
  }
  free(mutex_buf);
  mutex_buf = NULL;
  return 1;
}

#endif // SDK4_2_COS_CPP_SDK_INCLUDE_UTIL_THREADSAFE_OPENSSL_H
