/*
 *
 * authfd.h
 *
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 *
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 *
 * Created: Wed Mar 29 01:17:41 1995 ylo
 *
 * Functions to interface with the SSH_AUTHENTICATION_FD socket.
 *
 */

/* RCSID("$OpenBSD: src/usr.bin/ssh/authfd.h,v 1.8 2000/06/20 01:39:38 markus Exp $"); */

#ifndef AUTHFD_H
#define AUTHFD_H

#include "buffer.h"

/* Messages for the authentication agent connection. */
#define SSH_AGENTC_REQUEST_RSA_IDENTITIES	1
#define SSH_AGENT_RSA_IDENTITIES_ANSWER		2
#define SSH_AGENTC_RSA_CHALLENGE		3
#define SSH_AGENT_RSA_RESPONSE			4
#define SSH_AGENT_FAILURE			5
#define SSH_AGENT_SUCCESS			6
#define SSH_AGENTC_ADD_RSA_IDENTITY		7
#define SSH_AGENTC_REMOVE_RSA_IDENTITY		8
#define SSH_AGENTC_REMOVE_ALL_RSA_IDENTITIES	9

typedef struct {
	int     fd;
	Buffer  packet;
	Buffer  identities;
	int     howmany;
}       AuthenticationConnection;
/* Returns the number of the authentication fd, or -1 if there is none. */
int     ssh_get_authentication_socket();

/*
 * This should be called for any descriptor returned by
 * ssh_get_authentication_socket().  Depending on the way the descriptor was
 * obtained, this may close the descriptor.
 */
void    ssh_close_authentication_socket(int authfd);

/*
 * Opens and connects a private socket for communication with the
 * authentication agent.  Returns NULL if an error occurred and the
 * connection could not be opened.  The connection should be closed by the
 * caller by calling ssh_close_authentication_connection().
 */
AuthenticationConnection *ssh_get_authentication_connection();

/*
 * Closes the connection to the authentication agent and frees any associated
 * memory.
 */
void    ssh_close_authentication_connection(AuthenticationConnection * ac);

/*
 * Returns the first authentication identity held by the agent. Returns true
 * if an identity is available, 0 otherwise. The caller must initialize the
 * integers before the call, and free the comment after a successful call
 * (before calling ssh_get_next_identity).
 */
int
ssh_get_first_identity(AuthenticationConnection * connection,
    BIGNUM * e, BIGNUM * n, char **comment);

/*
 * Returns the next authentication identity for the agent.  Other functions
 * can be called between this and ssh_get_first_identity or two calls of this
 * function.  This returns 0 if there are no more identities.  The caller
 * must free comment after a successful return.
 */
int
ssh_get_next_identity(AuthenticationConnection * connection,
    BIGNUM * e, BIGNUM * n, char **comment);

/* Requests the agent to decrypt the given challenge.  Returns true if
   the agent claims it was able to decrypt it. */
int
ssh_decrypt_challenge(AuthenticationConnection * auth,
    BIGNUM * e, BIGNUM * n, BIGNUM * challenge,
    unsigned char session_id[16],
    unsigned int response_type,
    unsigned char response[16]);

/*
 * Adds an identity to the authentication server.  This call is not meant to
 * be used by normal applications.  This returns true if the identity was
 * successfully added.
 */
int
ssh_add_identity(AuthenticationConnection * connection, RSA * key,
    const char *comment);

/*
 * Removes the identity from the authentication server.  This call is not
 * meant to be used by normal applications.  This returns true if the
 * identity was successfully added.
 */
int     ssh_remove_identity(AuthenticationConnection * connection, RSA * key);

/*
 * Removes all identities from the authentication agent.  This call is not
 * meant to be used by normal applications.  This returns true if the
 * operation was successful.
 */
int     ssh_remove_all_identities(AuthenticationConnection * connection);

/* Closes the connection to the authentication agent. */
void    ssh_close_authentication(AuthenticationConnection * connection);

#endif				/* AUTHFD_H */
