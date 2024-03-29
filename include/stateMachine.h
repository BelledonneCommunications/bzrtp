/*
 * Copyright (c) 2014-2019 Belledonne Communications SARL.
 *
 * This file is part of bzrtp.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "typedef.h"

/* types definition for event and state function */
/* the INIT event type is used to run some state for the firt time : create packet and send it */
#define BZRTP_EVENT_INIT            0
#define BZRTP_EVENT_MESSAGE     	1
#define BZRTP_EVENT_TIMER           2
#define BZRTP_EVENT_GOCLEAR         3
#define BZRTP_EVENT_ACCEPT_GOCLEAR  4
#define BZRTP_EVENT_BACKTOSECURE    5

/* error code definition */
#define BZRTP_ERROR_UNSUPPORTEDZRTPVERSION		0xe001
#define BZRTP_ERROR_UNMATCHINGPACKETREPETITION	0xe002
#define BZRTP_ERROR_CACHEMISMATCH				0xe004

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief The event type, used as a parameter for the state function
 */
typedef struct bzrtpEvent_struct {
	uint8_t eventType; /**< Event can be a message or a timer's end */
	uint8_t *bzrtpPacketString; /**< a pointer to the zrtp packet string, NULL in case of timer event */
	uint16_t bzrtpPacketStringLength; /**< the length of packet string in bytes */
	bzrtpPacket_t *bzrtpPacket; /**< a pointer to the zrtp packet structure created by the processMessage function */
	bzrtpContext_t *zrtpContext; /**< the current ZRTP context */
	bzrtpChannelContext_t *zrtpChannelContext; /**< the current ZRTP channel hosting this state machine context */
} bzrtpEvent_t;

/**
 * @brief the state function pointer definition
 */
typedef int (*bzrtpStateMachine_t)(bzrtpEvent_t);


/* state functions prototypes, split in categories corresponding to the differents protocol phases: discovery, key agreement, confirmation */
/**
 * @brief This is the initial state
 * On first call, we will create the Hello message and start sending it until we receive an helloACK or a hello message from peer
 *
 * Arrives from :
 * 	- This is the initial state
 * Goes to:
 * 	- state_discovery_waitingForHello upon HelloACK reception
 * 	- state_discovery_waitingForHelloAck upon Hello reception
 * Send :
 * 	- Hello until timer's end or transition
 */
int state_discovery_init(bzrtpEvent_t event);


/**
 * @brief Arrives in this state coming from init upon reception on Hello ACK, we are now waiting for the Hello packet from peer
 *
 * Arrives from :
 *	- state_discovery_init upon HelloACK reception
 * Goes to:
 * 	- state_keyAgreement_sendingCommit upon Hello reception
 * Send :
 * 	- HelloACK on Hello reception
 *
 */
int state_discovery_waitingForHello(bzrtpEvent_t event);


/**
 * @brief We are now waiting for the HelloACK packet from peer or a Commit packet
 *
 * Arrives from :
 * 	- state_discovery_init upon Hello reception
 * Goes to:
 * 	- state_keyAgreement_sendingCommit upon HelloACK reception
 * 	- state_keyAgreement_responderSendingDHPart1 upon Commit reception in DHM mode
 * 	- state_confirmation_responderSendingConfirm1 upon Commit reception in non DHM mode
 * Send :
 * 	- Hello until timer's end or transition
 * 	- HelloACK on Hello reception
 *
 */
int state_discovery_waitingForHelloAck(bzrtpEvent_t event);


/**
 * @brief For any kind of key agreement (DHM, Mult, PreShared), we keep sending commit.
 *
 * Arrives from :
 * 	- state_discovery_waitingForHello upon Hello received
 * 	- state_discovery_waitingForHelloAck upon HelloACK received
 * Goes to:
 * 	- state_keyAgreement_initiatorSendingDHPart2 upon DHPart1 reception in DHM mode
 * 	- state_confirmation_initiatorSendingConfirm2 upon Confirm1 reception in non DHM mode
 * 	- state_keyAgreement_responderSendingDHPart1 upon Commit reception in DHM mode and commit contention gives us the responder role
 * 	- state_confirmation_responderSendingConfirm1 upon Commit reception in non DHM mode and commit contention gives us the responder role
 * Send :
 * 	- Commit until timer's end or transition
 * 	- HelloACK on Hello reception
 *
 */
int state_keyAgreement_sendingCommit(bzrtpEvent_t event);

/**
 * @brief For DHM mode only, responder send DHPart1 packet
 *
 * Arrives from:
 *  - state_discovery_waitingForHelloAck upon Commit reception in DHM mode
 *  - state_keyAgreement_sendingCommit upon Commit reception in DHM mode and commit contention gives us the responder role
 * Goes to:
 * 	- state_confirmation_responderSendingConfirm1 upon DHPart2 reception
 * Send :
 * 	- DHPart1 on Commit reception
 *
 */
int state_keyAgreement_responderSendingDHPart1(bzrtpEvent_t event);

/**
 * @brief For DHM mode only, initiator send DHPart2 packet
 *
 * Arrives from:
 *  - state_keyAgreement_sendingCommit upon DHPart1 reception
 * Goes to:
 * 	- state_confirmation_initiatorSendingConfirm2 upon reception of Confirm1
 * Send :
 * 	- DHPart2 until timer's end or transition
 *
 */
int state_keyAgreement_initiatorSendingDHPart2(bzrtpEvent_t event);


/**
 * @brief Responder send the confirm1 message
 *
 * Arrives from:
 * - state_keyAgreement_responderSendingDHPart1 upon DHPart2 reception
 * - state_keyAgreement_sendingCommit upon Commit reception in non DHM mode and commit contention gives us the responder role
 * - state_discovery_waitingForHelloAck upon Commit reception in non DHM mode
 * Goes to:
 * - state_secure on Confirm2 reception
 * Send :
 * - Confirm1 on Commit or DHPart2 reception
 *
 */
int state_confirmation_responderSendingConfirm1(bzrtpEvent_t event);


/**
 * @brief Initiator send the confirm2 message
 *
 * Arrives from:
 * - state_keyAgreement_initiatorSendingDHPart2 upon confirm1 reception
 * - state_keyAgreement_sendingCommit upon Confirm1 reception in non DHM mode
 * Goes to:
 * - state_secure on Conf2ACK reception or first SRTP message
 * Send :
 * - Confirm2 until timer's end or transition
 *
 */
int state_confirmation_initiatorSendingConfirm2(bzrtpEvent_t event);

/**
 * @brief We are in secure state
 *
 * Arrives from:
 * 	- state_confirmation_responderSendingConfirm1 on Confirm2 reception
 * 	- state_confirmation_initiatorSendingConfirm2 on conf2ACK or first SRTP message
 * Goes to:
 * 	- state_sending_GoClear when user pressed a button to indicate that he wants to change the encryption mode
 * 	- state_clear on GoClear reception
 * Send :
 * 	- Conf2ACK on Confirm2 reception
 *	- ClearACK on GoClear reception (+ destroy all key materials)
 *
 */
int state_secure(bzrtpEvent_t event);

/**
 * @brief GoClear initiator send a GoClear message
 *
 * Arrives from:
 * 	- state_secure when user pressed a button to indicate that he wants to change the encryption mode
 * Goes to:
 * 	- state_clear on ClearACK reception
 * Send :
 * 	- GoClear message (+ destroy all key materials on ClearACK reception)
 *
 */
int state_sending_GoClear(bzrtpEvent_t event);

/**
 * @brief We are in clear state
 *
 * Arrives from:
 * 	- state_sending_GoClear on ClearACK reception
 *	- state_secure on GoClear reception
 *	- state_clear on manuel confirmation of the responder (of the GoClear)
 * Goes to:
 * 	- state_keyAgreement_sendingCommit when user pressed a button to indicate that he wants to back to secure mode
 * 	- state_confirmation_responderSendingConfirm1 on commit reception
 *
 */
int state_clear(bzrtpEvent_t event);

/**
 * @brief Compute the new rs1 and update the cached secrets according to rfc section 4.6.1
 *
 * param[in]		zrtpContext			The context we are operation on
 * param[in/out]	zrtpChannelContext	The channel context we are operation on(contains s0)
 *
 * return 0 on success, error code otherwise
 */
int bzrtp_updateCachedSecrets(bzrtpContext_t *zrtpContext, bzrtpChannelContext_t *zrtpChannelContext);

#ifdef __cplusplus
}
#endif

#endif /* STATEMACHINE_H */
